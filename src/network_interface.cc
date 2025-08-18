#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

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
  if (it1 != arp_cache_.end()) // 已arp
  {
    EthernetFrame frame {};
    frame.header.dst = it1->second.first; // 目的地址
    frame.header.src = ethernet_address_; // 源地址
    frame.header.type = EthernetHeader::TYPE_IPv4; // 类型
    frame.payload = serialize(std::move(dgram)); // 负载

    transmit(frame);
  }
  else
  {
    ARPMessage apr_msg {};
    apr_msg.opcode = 1; // request
    apr_msg.sender_ethernet_address = ethernet_address_;
    apr_msg.sender_ip_address = addr_ipv4;
    apr_msg.target_ethernet_address = {};
    apr_msg.target_ip_address = addr_ipv4;

    EthernetFrame frame {};
    frame.header.dst = ETHERNET_BROADCAST; // 目的地址, 广播
    frame.header.src = ethernet_address_; // 源地址
    frame.header.type = EthernetHeader::TYPE_ARP; // arp类型

    frame.payload = serialize(std::move(apr_msg)); // 负载

    transmit(frame);

    auto it2 = dgram_waiting_queue_.find(addr_ipv4);
    if (it2 == dgram_waiting_queue_.end()) // 不在数据报队列中
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
    if (parse(arp_msg, frame.payload))
    {
      arp_cache_.insert({arp_msg.sender_ip_address, {arp_msg.sender_ethernet_address, timer_elapsed}}); //
      if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST)
      {
        ARPMessage arp_reply {};
        arp_reply.opcode = ARPMessage::OPCODE_REPLY; // reply
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = arp_msg.sender_ethernet_address;
        arp_reply.target_ip_address = arp_msg.sender_ip_address;

        EthernetFrame frame_reply {};
        frame_reply.header.dst = arp_msg.sender_ethernet_address;
        frame_reply.header.src = ethernet_address_;
        frame_reply.header.type = EthernetHeader::TYPE_ARP;

        frame_reply.payload = serialize(std::move(arp_reply));

        transmit(frame_reply);
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  timer_elapsed += ms_since_last_tick;
  auto it = arp_cache_.begin();
  while (it != arp_cache_.end())
  {
    if (it->second.second > timer_elapsed + 3000)
    {
      it = arp_cache_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}
