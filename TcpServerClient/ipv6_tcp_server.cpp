/**
 * @file ipv6_tcp_server.cpp
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

struct sockaddr_in6 client_info; // IPv6アドレス情報
struct sockaddr *p_client; // インターフェース
socklen_t socket_length;
unsigned short port_of_self = 54321;
int listen_queue_size = 5;
int ret;
int only_ipv6_flag = 1;

int passive_socket;   // Accept用ソケット
int socket_to_client; // クライアントに接続するソケット
char buf[BUFSIZE];

int main(int argc, char** argv)
{
    try
    {
        /* 1.ソケットの作成 */
        if ((passive_socket = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }
        std::printf("[Done] Step1. create socket\n");

        if ((ret = setsockopt(passive_socket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&only_ipv6_flag, sizeof(only_ipv6_flag))) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("setsockopt IPV6_V6ONLY");
        }

        /* 2.接続受付用構造体の準備 */
        client_info.sin6_family = AF_INET6;
        client_info.sin6_port = htons(port_of_self); // @warning 初期値はサーバーのポート番号. クライアントとの接続後(Accept後)は, クラアンとのポート番号が格納される.
        client_info.sin6_addr = in6addr_any; // in6addr_any "::" , in6addr_loopback "::1"
        socket_length = sizeof(client_info);

        /* 3.待受を行うIPアドレスとポート番号を指定 */
        if ((ret = bind(passive_socket, p_client, socket_length)) != 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("bind");
        }
        std::printf("[Done] Step2. bind socket\n");

        /* 4.TCPクライアントから接続要求を待てる状態にする */
        if ((ret = listen(passive_socket, listen_queue_size)) != 0)
        {
            if (ret == EADDRINUSE)
            {
                std::printf("Other socket listen this port. port of self is %u\n", port_of_self);
            }

            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("listen");
        }
        std::printf("[Done] Step3. listen socket and accepting client ...\n");

        /* 5.TCPクライアントからの接続要求を受ける */
        if ((socket_to_client = accept(passive_socket, p_client, &socket_length)) == -1)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("accept");
        }
        std::printf("[Done] Step4. accept client\n");

        // 接続に成功したら, `passive_socket`でははく, `server_to_socket`を使用する.

        /* 6.クライアントのIPv6アドレスを文字列に変換 */
        char client_address[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(client_info.sin6_addr), client_address, sizeof(client_address));
        unsigned short port_of_client = ntohs(client_info.sin6_port);
        std::printf("connection from : client %s, port=%u\n", client_address, port_of_client);

        /* 7.クライアントからのメッセージを受信 */
        std::memset(buf, 0, sizeof(buf));
        int n = read(socket_to_client, buf, sizeof(buf));
        std::printf("read n=%d, message : %s\n", n , buf);

        /* 8.サーバーからクライアントへ送信 */
        std::memset(buf, 0, sizeof(buf));
        std::snprintf(buf, sizeof(buf), "message from IPv6 server");
        n = write(socket_to_client, buf, strnlen(buf, sizeof(buf)));

        /* 9.ソケットを閉じる */
        close(socket_to_client);

        /* 10.Listenしているソケットを閉じる */
        close(passive_socket);
    }
    catch(const std::exception& e)
    {
        close(socket_to_client);
        close(passive_socket);
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}