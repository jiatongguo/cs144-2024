#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  router_table_.emplace_back(route_{route_prefix, prefix_length, next_hop, interface_num});
}

bool Router::prefix_match(uint32_t prefix1, uint32_t prefix2, uint8_t prefix_length)
{
  if (prefix_length == 0) return true;
  return (prefix1 >> (32 - prefix_length)) == (prefix2 >> (32 - prefix_length)); 
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for (const auto& interface_ : _interfaces) // 遍历网络接口
  {
    auto& dgram_queue {interface_->datagrams_received()};
    while(!dgram_queue.empty())  // 遍历消息队列
    {
      auto dgram = dgram_queue.front();
      dgram_queue.pop();

      std::optional<uint8_t> max_len;
      std::optional<Address> next_hop;
      size_t interface_num {};
      
      // 最长前缀匹配
      for (const auto& r : router_table_)
      {
        uint8_t len { r.prefix_length};
        uint32_t dst = dgram.header.dst;

        if ( !max_len.has_value() || (len > max_len && prefix_match(r.route_prefix, dst, len)))
        {
            max_len = len;
            interface_num = r.interface_num;
            next_hop = r.next_hop;
        }
      }
      
      if (!max_len.has_value())
      {
        continue;
      }

      if (dgram.header.ttl <= 1) // 处理ttl
      {
        continue;
      }
      else
      {
        --dgram.header.ttl;
      }
      
      const Address& next_h = next_hop.has_value() ? next_hop.value() : Address::from_ipv4_numeric(dgram.header.dst);
      dgram.header.cksum = 0;
      dgram.header.compute_checksum();
      _interfaces[interface_num]->send_datagram(dgram, next_h);
    }
  }
}
