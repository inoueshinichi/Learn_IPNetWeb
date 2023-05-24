/**
 * @file ipv6_udp_multicast_reciever.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief IPv6 `ff0e::9999:9999`というマルチキャストグループにJOINする
 * @version 0.1
 * @date 2023-05-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <test_utils.hpp>

// udp
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_pton, inet_ntop
#include <netdb.h>
#include <net/if.h> // if_nametoindex

#if defined(__linux__)

#elif defined(__MACH__)

#else
// Windows
#endif

#define BUFSIZE 2048

struct sockaddr_in6 sender_info; // IPv6アドレス情報
struct sockaddr *p_sender;      // インターフェース
socklen_t socket_length;
unsigned short port_of_self = 54321;
int ret;

int passive_socket; // 受信用ソケット
char buf[BUFSIZE];
char addr_name_ipv6[INET6_ADDRSTRLEN]; // 送信元IPアドレス

/**
 * @brief group_req構造体
 *
 * /* Multicast group request.
 * struct group_rec
 * {
 *    // Interface index
 *    uint32_t gr_interface;
 *    // Group address
 *    struct sockaddr_storage gr_group; // IPv4とIPv6の両方に対応.
 * }
 */
struct group_req group_request;
char multicast_addr_name_ipv6[INET6_ADDRSTRLEN] = "ff0e::9999:9999"; // Multicast IPアドレス
struct addrinfo self_addr_hists, *addr_response;

int main(int argc, char** argv)
{
    try
    {
        /* 1.ソケットの作成 */
        if ((passive_socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }
        std::printf("[Done] Step1. create socket\n");

        /* 2.接続受付用構造体の準備 */
        std::memset(&sender_info, 0, sizeof(sender_info));
        sender_info.sin6_family = AF_INET6;
        sender_info.sin6_port = htons(port_of_self);
        sender_info.sin6_addr = in6addr_any;
        socket_length = sizeof(sender_info);
        p_sender = (struct sockaddr *)&sender_info;

        /* 3.待受を行うIPアドレスとポート番号を指定 */
        if ((ret = bind(passive_socket,
                        p_sender,
                        socket_length)) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("bind");
        }
        std::printf("[Done] Step2. bind socket\n");

        /* 5.マルチキャストグループ(ff0e::9999:9999)に参加 */
        std::memset(&self_addr_hists, 0, sizeof(self_addr_hists));
        self_addr_hists.ai_family = AF_INET6; // IPv6のみ取得
        self_addr_hists.ai_socktype = SOCK_DGRAM;
        if ((ret = getaddrinfo(multicast_addr_name_ipv6, NULL, &self_addr_hists, &addr_response)) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            std::printf("getaddrinfo : %s\n", gai_strerror(ret));
            throw std::runtime_error("getaddrinfo");
        }
        std::memset(&group_request, 0, sizeof(group_request));
        group_request.gr_interface = 0; //if_nametoindex("lo0"); // 任意のインターフェースを指定 (lo0, en0, etc.) 要は, NICデバイス
        std::memcpy(&group_request.gr_group, addr_response->ai_addr, addr_response->ai_addrlen);

        char status_addr_name_ipv6[INET6_ADDRSTRLEN]; // 受信マルチキャストアドレスに対応したグローバルIPアドレス?
        inet_ntop(AF_INET6,
                  &(addr_response->ai_addr),
                  status_addr_name_ipv6,
                  sizeof(status_addr_name_ipv6));

        std::printf("[Status] Multicast IP : %s <-> IP : %s, addr_response->ai_addrlen : %u\n",
                    multicast_addr_name_ipv6, status_addr_name_ipv6, addr_response->ai_addrlen);
        freeaddrinfo(addr_response);

        if ((ret = setsockopt(passive_socket,
                              IPPROTO_IPV6,
                              MCAST_JOIN_GROUP,
                              (char *)&group_request,
                              sizeof(group_request))) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("setsockopt");
        }

        /* 6.受信 */
        std::memset(buf, 0, sizeof(buf));
        socket_length = sizeof(sender_info); // IPv6サイズ
        int n = recvfrom(passive_socket,
                         buf,
                         sizeof(buf) - 1,
                         0,
                         p_sender, // 送信元情報が入る
                         &socket_length);

        /* 送信元のIPアドレスとポート番号を表示 */
        inet_ntop(AF_INET6,
                  &(sender_info.sin6_addr),
                  addr_name_ipv6,
                  sizeof(addr_name_ipv6));
        std::printf("UDP packet from : %s, port=%d\n", addr_name_ipv6, ntohs(sender_info.sin6_port));

        // 標準出力にそのまま出力
        std::printf("%s\n", buf);

        /* 5. ソケットを閉じる */
        close(passive_socket);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}