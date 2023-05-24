/**
 * @file ipv4_udp_multicast_reciever.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief IPv4 `239.192.100.100`というマルチキャストグループにJOINする
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

#if defined(__linux__)

#elif defined(__MACH__)

#else
// Windows
#endif

#define BUFSIZE 2048

struct sockaddr_in sender_info; // IPv4アドレス情報
struct sockaddr *p_sender;      // インターフェース
socklen_t socket_length;
unsigned short port_of_self = 54321;
int ret;

int passive_socket; // 受信用ソケット
char buf[BUFSIZE];
char addr_name_ipv4[INET_ADDRSTRLEN]; // 送信元IPアドレス

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
char multicast_addr_name_ipv4[INET_ADDRSTRLEN] = "239.192.100.100"; // Multicast IPアドレス
struct addrinfo self_addr_hists, *addr_response;

int main(int argc, char** argv)
{
    try
    {
        /* 1.ソケットの作成 */
        if ((passive_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }
        std::printf("[Done] Step1. create socket\n");

        /* 2.接続受付用構造体の準備 */
        std::memset(&sender_info, 0, sizeof(sender_info));
        sender_info.sin_family = AF_INET;
        sender_info.sin_port = htons(port_of_self);
        sender_info.sin_addr.s_addr = INADDR_ANY;
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

        /* 5.マルチキャストグループ(239.192.100.100)に参加 */
        // struct sockaddr_storage multicast_reciever_info; // マルチキャスト受信用IPv4アドレス情報
        // std::memset(&multicast_reciever_info, 0, sizeof(multicast_reciever_info));
        // struct sockaddr_in* p_multicast_ipv4_reciever = (struct sockaddr_in*)&multicast_reciever_info; // IPv4インターフェース
        // p_multicast_ipv4_reciever->sin_family = AF_INET;
        // p_multicast_ipv4_reciever->sin_len = sizeof(struct sockaddr_in);
        // // マルチキャスト用IPアドレス`239.192.100.100`を登録
        // inet_ntop(AF_INET, &(p_multicast_ipv4_reciever->sin_addr), multicast_addr_name_ipv4, sizeof(multicast_addr_name_ipv4));
        // struct sockaddr *p_multicast_reciever = (struct sockaddr *)&multicast_reciever_info;

        // std::memset(&group_request, 0, sizeof(group_request)); // 初期化
        // group_request.gr_interface = 0; // 任意のインターフェースを指定 (lo0, en0, etc.) 要は, NICデバイス
        // std::memcpy(&group_request.gr_group, p_multicast_reciever, sizeof(struct sockaddr_in)); // アドレス情報をgroup_req.gr_group変数にコピー

        std::memset(&self_addr_hists, 0, sizeof(self_addr_hists));
        self_addr_hists.ai_family = AF_INET; // IPv4のみ取得
        self_addr_hists.ai_socktype = SOCK_DGRAM;
        if ((ret = getaddrinfo(/*multicast_addr_name_ipv4*/"239.192.100.100", NULL, &self_addr_hists, &addr_response)) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            std::printf("getaddrinfo : %s\n", gai_strerror(ret));
            throw std::runtime_error("getaddrinfo");
        }
        std::memset(&group_request, 0, sizeof(group_request));
        group_request.gr_interface = 0; // 任意のインターフェースを指定 (lo0, en0, etc.) 要は, NICデバイス
        std::memcpy(&group_request.gr_group, addr_response->ai_addr, addr_response->ai_addrlen);

        char status_addr_name_ipv4[INET_ADDRSTRLEN]; // 受信マルチキャストアドレスに対応したグローバルIPアドレス?
        inet_ntop(AF_INET,
                  &(addr_response->ai_addr),
                  status_addr_name_ipv4,
                  sizeof(status_addr_name_ipv4));
                  
        std::printf("[Status] Multicast IP : %s <-> IP : %s, addr_response->ai_addrlen : %u\n", 
                    multicast_addr_name_ipv4, status_addr_name_ipv4, addr_response->ai_addrlen);
        freeaddrinfo(addr_response);

        if ((ret = setsockopt(passive_socket,
                              IPPROTO_IP,
                              MCAST_JOIN_GROUP,
                              (char *)&group_request,
                              sizeof(group_request))) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("setsockopt");
        }

        /* 6.受信 */
        std::memset(buf, 0, sizeof(buf));
        socket_length = sizeof(sender_info); // IPv4サイズ
        int n = recvfrom(passive_socket,
                         buf,
                         sizeof(buf) - 1,
                         0,
                         p_sender, // 送信元情報が入る
                         &socket_length);

        /* 送信元のIPアドレスとポート番号を表示 */
        inet_ntop(AF_INET,
                  &(sender_info.sin_addr),
                  addr_name_ipv4,
                  sizeof(addr_name_ipv4));
        std::printf("UDP packet from : %s, port=%d\n", addr_name_ipv4, ntohs(sender_info.sin_port));

        // 標準出力にそのまま出力
        // write(fileno(stdout), buf, n);
        std::printf("%s\n", buf);

        /* 5. ソケットを閉じる */
        close(passive_socket);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}