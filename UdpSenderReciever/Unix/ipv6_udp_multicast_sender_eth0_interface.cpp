/**
 * @file ipv6_udp_multicast_sender_eth0_interface.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief eth0インターフェースを通じてマルチキャストで送信するSender
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

struct sockaddr_in6 reciever_info; // IPv6アドレス情報
struct sockaddr *p_reciever;      // インターフェース
socklen_t socket_length;
unsigned short port_of_reciever = 54321;
int ret;

int socket_to_reciever; // 受信側に接続するソケット
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
char multicast_addr_name_ipv6[INET6_ADDRSTRLEN];
// struct sockaddr_in specified_interface; /* 出力に使用するネットワークインターフェース指定用(IPv4) */
unsigned int specified_interface_index;

int main(int argc, char** argv)
{
    try
    {
        /* 1.ソケットの作成 */
        if ((socket_to_reciever = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }
        std::printf("[Done] Step1. create socket\n");

        /* 2.接続先指定用構造体の準備 */
        reciever_info.sin6_family = AF_INET6;
        reciever_info.sin6_port = htons(port_of_reciever);
        socket_length = sizeof(reciever_info);
        p_reciever = (struct sockaddr *)&reciever_info;

        /* 3.宛先(reciever)の確定 */
        char *multicast_reciever_name_ipv6 = "ff0e::9999:9999";
        if (inet_pton(AF_INET6,
                      multicast_reciever_name_ipv6,
                      &(reciever_info.sin6_addr)) == INADDR_NONE)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("Resolve IP Address");
        }
        std::printf("[Done] Step2. configure destination (reciever): `%s`; port=%u\n",
                    multicast_reciever_name_ipv6, port_of_reciever);

        /* 4.マルチキャスト出力インターフェースとしてループバック(eth0)を指定 */
        // IPv6ではif_nametoindex()関数を用いてインターフェースを指定する.
        specified_interface_index = if_nametoindex/*mac*/("en0"); // /*linux*/("eth0");
        if ((ret = setsockopt(socket_to_reciever,
                              IPPROTO_IPV6,
                              IPV6_MULTICAST_IF,
                              (char *)&specified_interface_index,
                              sizeof(specified_interface_index))) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("setsockopt");
        }

        /* 4.受信側に送信 */
        char msg[] = "HELLO IPv6 with MCAST";
        int n = sendto(socket_to_reciever,
                       msg,
                       sizeof(msg),
                       0,
                       p_reciever, // 受信側情報を受取る
                       socket_length);

        if (n < 1)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("sendto");
        }

        /* 5.ソケットを閉じる */
        close(socket_to_reciever);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}
