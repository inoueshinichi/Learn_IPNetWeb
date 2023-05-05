cmake_minimum_required(VERSION 3.14.6)

include(../../is_ip_net_web_test_case.cmake)

# Simple Client Server Model
make_ip_net_web("" "" ipv4_tcp_client.cpp)
make_ip_net_web("" "" ipv4_tcp_server.cpp)
make_ip_net_web("" "" ipv6_tcp_client.cpp)
make_ip_net_web("" "" ipv6_tcp_server.cpp)
make_ip_net_web("" "" dual_tcp_server.cpp)