/**
 * @file ipv4_udp_reciever.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-05
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
struct sockaddr* p_sender; // インターフェース
socklen_t socket_length;
unsigned short port_of_self = 54321;
int ret;

int passive_socket; // 受信用ソケット
char buf[BUFSIZE];
char addr_name_ipv4[INET_ADDRSTRLEN];

int main(int argc, char **argv)
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
        sender_info.sin_addr.s_addr = INADDR_ANY; // 全てのINetインターフェースで受け付ける
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
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}