cmake_minimum_required(VERSION 3.14.6)

include(../../is_ip_net_web_test_case.cmake)

# Simple Client Server Model
make_ip_net_web("" "" ipv4_udp_reciever.cpp)
make_ip_net_web("" "" ipv4_udp_sender.cpp)
make_ip_net_web("" "" ipv6_udp_reciever.cpp)
make_ip_net_web("" "" ipv6_udp_sender.cpp)
make_ip_net_web("" "" dual_udp_reciever.cpp)

# UDP Multicast
make_ip_net_web("" "" ipv4_udp_multicast_reciever.cpp)
make_ip_net_web("" "" ipv6_udp_multicast_reciever.cpp)
make_ip_net_web("" "" ipv4_udp_multicast_sender_lo_interface.cpp)
make_ip_net_web("" "" ipv6_udp_multicast_sender_eth0_interface.cpp)

