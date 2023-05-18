/**
 * @file ipv4_udp_sender.cpp
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

#define BUFSIZE 1500

struct sockaddr_in reciever_info; // IPv4アドレス情報
struct sockaddr* p_reciever; // インターフェース
socklen_t socket_length;
unsigned short port_of_reciever = 54321;

int socket_to_reciever; // 受信側に接続するソケット
char buf[BUFSIZE];

int main(int argc, char** argv)
{
    try
    {
        /* 1.ソケットの作成 */
        if ((socket_to_reciever = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }
        std::printf("[Done] Step1. create socket\n");

        /* 2.接続先指定用構造体の準備 */
        reciever_info.sin_family = AF_INET;
        reciever_info.sin_port = htons(port_of_reciever);
        socket_length = sizeof(reciever_info);
        p_reciever = (struct sockaddr *)&reciever_info;

        /* 3.宛先(reciever)の確定 */
        char* reciever_name = "127.0.0.1";
        if (inet_pton(AF_INET, reciever_name, &(reciever_info.sin_addr)) == INADDR_NONE)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("Resolve IP Address");
        }
        std::printf("[Done] Step2. configure destination (reciever): `%s`; port=%u\n", reciever_name, port_of_reciever);

        /* 4.受信側に送信 */
        char msg[] = "HELLO IPv4";
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
    return 0;
}
