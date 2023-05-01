/**
 * @file simple_tcp_server.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-01
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
#include <netdb.h>      // gethostbyname

#if defined(__linux__)

#elif defined(__MACH__)

#else
// Windows
#endif

#define BUFSIZE 1500

int main(int argc, char** argv)
{
    try
    {
        struct sockaddr client_0_info; // ソケット情報
        struct sockaddr_in *p_client;  // ネットワーク用インターフェース
        struct hostent* host;
        socklen_t socket_length;
        unsigned short port = 54123;
        int listen_queue_size = 5;
        int ret;

        int sock_passive_server;
        int sock_server;
        char buf[BUFSIZE];

        /* 1.ソケットの作成 */
        if ((sock_passive_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }

        /* 2.接続先指定用構造体の準備 */
        p_client = (struct sockaddr_in*)&client_0_info;
        p_client->sin_family = AF_INET;
        p_client->sin_port = htons(port);
        p_client->sin_addr.s_addr = INADDR_ANY; // 全てのINetインターフェースで受け付ける

        /* 3.待受を行うIPアドレスとポート番号を指定 */
        if ((ret = bind(sock_passive_server, (struct sockaddr *)p_client, sizeof(sizeof(struct sockaddr)))) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("bind");
        }

        /* 4.TCPクライアントから接続要求を待てる状態にする */
        if ((ret = listen(sock_passive_server, listen_queue_size)) != 0)
        {
            if (ret == EADDRINUSE)
            {
                std::printf("Other socket listen this port. port is %s\n", port);
            }

            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("listen");
        }

        /* 5.TCPクライアントからの接続要求を受ける */
        socklen_t len = sizeof(client_0_info);
        if ((sock_server = accept(sock_passive_server, (struct sockaddr *)p_client, &len)) == -1)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("accept");
        }

        /* 6.クライアントのIPv4アドレスを文字列に変換 */
        char client_address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(p_client->sin_addr), client_address, sizeof(client_address));
        std::printf("Connection from : %s, port=%d\n", client_address, ntohs(p_client->sin_port));

        /* 7.クライアントからのメッセージを受信 */
        std::memset(buf, 0, sizeof(buf));
        int n = read(sock_server, buf, sizeof(buf));
        std::printf("n=%d\nmessage : %s\n", n , buf);

        /* 8.サーバーからクライアントへ送信 */
        std::memset(buf, 0, sizeof(buf));
        std::snprintf(buf, sizeof(buf), "message from IPv4 server");
        n = write(sock_server, buf, strnlen(buf, sizeof(buf)));

        /* 9.ソケットを閉じる */
        close(sock_server);

        /* 10.Listenしているソケットを閉じる */
        close(sock_passive_server);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}