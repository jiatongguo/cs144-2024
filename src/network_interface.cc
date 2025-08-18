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
  // Your code here.
  (void)frame;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;
}
