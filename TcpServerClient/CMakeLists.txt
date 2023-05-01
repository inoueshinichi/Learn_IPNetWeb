cmake_minimum_required(VERSION 3.14.6)

include(../is_ip_net_web_test_case.cmake)

# Simple Client Server Model
make_ip_net_web("" "" simple_tcp_client.cpp)
make_ip_net_web("" "" simple_tcp_server.cpp)