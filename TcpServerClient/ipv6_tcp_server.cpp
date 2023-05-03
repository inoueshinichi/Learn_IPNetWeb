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

struct sockaddr client_info;// クライアント用ソケット情報
struct sockaddr_in *p_client; // クライアント接続用インターフェース
struct hostent *host;
socklen_t socket_length;
unsigned short port_of_self = 12345;
int listen_queue_size = 5;
int ret;

int passive_socket;   // Accept用ソケット
int socket_to_client; // クライアントに接続するソケット
char buf[BUFSIZE];

int main(int argc, char** argv)
{
    try
    {
        /* 1.ソケットの作成 */
        if ((passive_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }
        std::printf("[Done] Step1. create socket\n");

        /* 2.接続受付用構造体の準備 */
        socket_length = sizeof(client_info);
        p_client = (struct sockaddr_in *)&client_info; // `sockeaddr`構造体を`sockaddr_in*`構造体ポインタを通して使用する.
        p_client->sin_family = AF_INET;
        p_client->sin_port = htons(port_of_self); // @warning 初期値はサーバーのポート番号. クライアントとの接続後(Accept後)は, クラアンとのポート番号が格納される.
        p_client->sin_addr.s_addr = INADDR_ANY; // 全てのINetインターフェースで受け付ける

        /* 3.待受を行うIPアドレスとポート番号を指定 */
        if ((ret = bind(passive_socket, (struct sockaddr *)p_client, socket_length)) != 0)
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
                std::printf("Other socket listen this port. port of self is %s\n", port_of_self);
            }

            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("listen");
        }
        std::printf("[Done] Step3. listen socket and accepting client ...\n");

        /* 5.TCPクライアントからの接続要求を受ける */
        if ((socket_to_client = accept(passive_socket, (struct sockaddr *)p_client, &socket_length)) == -1)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("accept");
        }
        std::printf("[Done] Step4. accept client\n");

        // 接続に成功したら, `passive_socket`でははく, `server_socket`を使用する.

        /* 6.クライアントのIPv4アドレスを文字列に変換 */
        char client_address[INET_ADDRSTRLEN]; // ipv6はINET6_ADDRSTRLEN
        inet_ntop(AF_INET, &(p_client->sin_addr), client_address, sizeof(client_address));
        unsigned short port_of_client = ntohs(p_client->sin_port);
        std::printf("connection from : client %s, port=%u\n", client_address, port_of_client);

        /* 7.クライアントからのメッセージを受信 */
        std::memset(buf, 0, sizeof(buf));
        int n = read(socket_to_client, buf, sizeof(buf));
        std::printf("read n=%d, message : %s\n", n , buf);

        /* 8.サーバーからクライアントへ送信 */
        std::memset(buf, 0, sizeof(buf));
        std::snprintf(buf, sizeof(buf), "message from IPv4 server");
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