#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

ARPMessage NetworkInterface::make_arp(const uint16_t& opcode, const EthernetAddress& target_ethernet_address, const uint32_t& target_ip_address)
{
  ARPMessage arp_msg {};
  arp_msg.opcode = opcode;
  arp_msg.sender_ethernet_address = ethernet_address_;
  arp_msg.sender_ip_address = ip_address_.ipv4_numeric();
  arp_msg.target_ethernet_address = target_ethernet_address;
  arp_msg.target_ip_address = target_ip_address;

  return arp_msg;
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const auto addr_ipv4 = next_hop.ipv4_numeric();

  auto it1 = arp_cache_.find(addr_ipv4);
  if (it1 != arp_cache_.end()) // 已arp, 直接发送
  {
    const EthernetAddress& dst { it1->second.first };
    const EthernetFrame& frame { {dst, ethernet_address_, EthernetHeader::TYPE_IPv4}, {serialize(dgram)} };
    transmit(frame);
  }
  else
  { // 未arp，若5s之内没发arp包，发arp包
    auto it = arp_expire_time.find(addr_ipv4);
    if (it == arp_expire_time.end() || it->second < timer_elapsed ) // arp抑制（5s)
    {
      const ARPMessage& arp_msg {make_arp(ARPMessage::OPCODE_REQUEST, {}, addr_ipv4) };
      const EthernetFrame& frame {{ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(arp_msg) }; // 广播
      transmit(frame); 

      arp_expire_time[addr_ipv4] = timer_elapsed + 5000; // 插入或更新过期时间
    }
    
    // 判断该ip在等待队列中是否有消息
    auto it2 = dgram_waiting_queue_.find(addr_ipv4);
    if (it2 == dgram_waiting_queue_.end()) 
    {
      dgram_waiting_queue_.insert({addr_ipv4, {dgram}}); 
    }
    else
    {
      it2->second.emplace_back(dgram);
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if (frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST)
  {
    return;
  }

  if (frame.header.type == EthernetHeader::TYPE_IPv4)
  {
    InternetDatagram dgram{};
    if (parse(dgram, frame.payload))
    {
      datagrams_received_.emplace(std::move(dgram));
    }
  }

  else if (frame.header.type == EthernetHeader::TYPE_ARP)
  {
    ARPMessage arp_msg{};
    if (!parse(arp_msg, frame.payload)) // 解析
    {
      return;
    }

    arp_cache_.insert({arp_msg.sender_ip_address, {arp_msg.sender_ethernet_address, timer_elapsed + 30000}}); // arp缓存
    
    if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == ip_address_.ipv4_numeric()) // 收到arp request， 回复
    {
      const auto& dst = arp_msg.sender_ethernet_address;
      ARPMessage arp_reply { make_arp(ARPMessage::OPCODE_REPLY, arp_msg.sender_ethernet_address, arp_msg.sender_ip_address) };
      const EthernetFrame& frame_reply {dst, ethernet_address_, EthernetHeader::TYPE_ARP, serialize(arp_reply)};
      transmit(frame_reply);
    }
    
    // 立即发送等待队列中的数据报
    auto it = dgram_waiting_queue_.find(arp_msg.sender_ip_address);
    if (it != dgram_waiting_queue_.end())
    {
      const EthernetAddress& dst = arp_msg.sender_ethernet_address;
      for (auto& dgram : it->second)
      {
        const EthernetFrame& frame2 { {dst, ethernet_address_, EthernetHeader::TYPE_IPv4}, serialize(dgram)};
        transmit(frame2);
      }
      dgram_waiting_queue_.erase(it);
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  timer_elapsed += ms_since_last_tick;

  auto it = arp_cache_.begin();
  while (it != arp_cache_.end())
  {
    if (timer_elapsed > it->second.second )
    {
      it = arp_cache_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}
