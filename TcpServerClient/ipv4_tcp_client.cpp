/**
 * @file simple_tcp_client.cpp
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

struct sockaddr server_info;  // ソケット情報
struct sockaddr_in *p_server; // ネットワーク用インターフェース
struct hostent *host;
socklen_t socket_length;
unsigned short port_of_server = 54321;

int socket_to_server; // サーバに接続するソケット
char buf[BUFSIZE];

int main(int argc, char **argv)
{
    try
    {
        /* 1.ソケットの作成 */
        if ((socket_to_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }
        std::printf("[Done] Step1. create socket\n");

        /* 2.接続先指定用構造体の準備 */
        p_server = (struct sockaddr_in *)&server_info; // `sockeaddr`構造体を`sockaddr_in*`構造体ポインタを通して使用する.
        p_server->sin_family = AF_INET;
        p_server->sin_port = htons(port_of_server);

        /* 3.宛先(server)の確定 */
        char *server_name = "127.0.0.1";
        if ((p_server->sin_addr.s_addr = inet_addr(server_name)) == INADDR_NONE)
        {
            // IPアドレスではない
            host = gethostbyname(server_name);
            if (host == NULL)
            {
                std::printf("No IP Address : %s\n", server_name);
                throw std::runtime_error("address");
            }
            p_server->sin_family = host->h_addrtype;
            std::memcpy(&(p_server->sin_addr), host->h_addr, host->h_length);
        }
        std::printf("[Done] Step2. configure destination (server): `%s`; port=%u\n", server_name, port_of_server);

        /* 4.サーバーに接続 */
        socket_length = sizeof(server_info);
        if (connect(socket_to_server, (struct sockaddr *)p_server, socket_length) != 0)
        {
            // 成功 0, 失敗 0以外
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("connect");
        }
        std::printf("[Done] Step3. connect to server `%s`; port=%u\n", server_name, port_of_server);

        /* 5.サーバーに送信 */
        std::snprintf(buf, sizeof(buf), "message from IPv4 client");
        int n = write(socket_to_server, buf, strnlen(buf, sizeof(buf)));

        /* 6.サーバーから受信 */
        std::memset(buf, 0, sizeof(buf));
        n = read(socket_to_server, buf, sizeof(buf));

        std::printf("read n=%d, %s\n", n, buf);

        /* 7.ソケットを閉じる */
        close(socket_to_server);
    }
    catch (const std::exception &e)
    {
        close(socket_to_server);
        std::cerr << e.what() << '\n';
    }
}