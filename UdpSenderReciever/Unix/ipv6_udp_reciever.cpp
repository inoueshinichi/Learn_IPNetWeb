/**
 * @file ipv6_udp_reciever.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <test_utils.hpp>

// tcp
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // socket_in
#include <arpa/inet.h>  // inet_pton
#include <netdb.h>

#if defined(__linux__)

#elif defined(__MACH__)

#else
// Windows
#endif

#define BUFSIZE 2048

struct sockaddr_in6 sender_info; // IPv6アドレス情報
struct sockaddr* p_sender; // インターフェース
socklen_t socket_length;
unsigned short port_of_self = 54321;
int ret;
int only_ipv6_flag = 1;

int passive_socket; // 受信ソケット
char buf[BUFSIZE];
char addr_name_ipv6[INET6_ADDRSTRLEN];

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

        if ((ret = setsockopt(passive_socket,
                              IPPROTO_IPV6,
                              IPV6_V6ONLY,
                              (void *)&only_ipv6_flag,
                              sizeof(only_ipv6_flag))) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("setsockopt IPV6_V6ONLY");
        }

        /* 2.接続受付用構造体の準備 */
        std::memset(&sender_info, 0, sizeof(sender_info));
        sender_info.sin6_family = AF_INET6;
        sender_info.sin6_port = htons(port_of_self); // @warning 初期値はサーバーのポート番号. クライアントとの接続後(Accept後)は, クラアンとのポート番号が格納される.
        sender_info.sin6_addr = in6addr_any;         // in6addr_any "::" , in6addr_loopback "::1"
        socket_length = sizeof(sender_info);
        p_sender = (struct sockaddr *)&sender_info;
        
        /* 3.待受を行うIPアドレスとポート番号を指定 */
        if ((ret = bind(passive_socket, p_sender, socket_length)) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("bind");
        }
        std::printf("[Done] Step2. bind socket\n");

        /* 4.受信 */
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