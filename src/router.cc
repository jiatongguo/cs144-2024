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

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for (const auto& interface_ : _interfaces) // 遍历网络接口
  {
    auto& dgram_queue {interface_->datagrams_received()};
    while(!dgram_queue.empty())  // 遍历消息队列
    {
      auto dgram = dgram_queue.front();

      uint8_t max_len {0};
      std::optional<size_t> interface_num;
      std::optional<Address> next_hop;
      
      // 最长前缀匹配
      for (auto r : router_table_)
      {
        uint8_t len { r.prefix_length};
        uint32_t msk {0xFFFFFFFFu << (32 - len)};
        uint32_t dst = dgram.header.dst;

        if ((dst & msk) == (r.route_prefix & msk))
        {
          if (len > max_len)
          {
            max_len = len;
            interface_num = r.interface_num;
            next_hop = r.next_hop;
          }
        }
      }

      if (--dgram.header.ttl > 0 && interface_num.has_value())
      {
          _interfaces[interface_num.value()]->send_datagram(dgram,
                      next_hop.has_value()? next_hop.value() : Address::from_ipv4_numeric(dgram.header.dst));
      }

      dgram_queue.pop();
    }
  }
}
