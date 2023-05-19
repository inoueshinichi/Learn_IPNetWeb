/**
 * @file udp_ipv4_from_lan_a_to_wan_b.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <test_utils.hpp>

// udp
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> // getaddrinfo, getnameinfo
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>


