/**
 * @file ipv6_tcp_client.cpp
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
#include <arpa/inet.h>  // inet_pton(OK IPv6), inet_ntop(OK IPv6), inet_addr(NG IPv6)
#include <netdb.h>      // gethostbyname

// inet_pton : テキスト形式からバイナリ形式に変更
// inet_ntop : バイナリ形式からテキスト形式に変更

#if defined(__linux__)

#elif defined(__MACH__)

#else
// Windows
#endif

#define BUFSIZE 1500

/**
 * @brief IPv4を脱却してIPv6に移行する方法
 * https://www.iajapan.org/ipv6/summit/KYOTO2013/pdf/Hiromi_Kyoto.pdf
 */

struct sockaddr_in6 server_info; // IPv6アドレス情報
struct sockaddr *p_server; // インターフェース
socklen_t socket_length;
unsigned short port_of_server = 23456;

int socket_to_server; // サーバに接続するソケット
char buf[BUFSIZE];

int main(int argc, char **argv)
{
    try
    {
        /* 1.ソケットの作成 */
        if ((socket_to_server = socket(AF_INET6, SOCK_STREAM, 0)) < 0) /* IPv6 */
        {
            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("socket");
        }
        std::printf("[Done] Step1. create socket\n");

        /* 2.接続先指定用構造体の準備 */
        server_info.sin6_family = AF_INET6; /* IPv6 */
        server_info.sin6_port = htons(port_of_server); /* IPv6 */
        socket_length = sizeof(server_info);
        p_server = (struct sockaddr *)&server_info;

        /* 3.宛先(server)の確定 */
        // server_info.sin6_addr = in6addr_loopback;
        char *server_name = "::1"; // loopback /* IPv6 */
        if (inet_pton(AF_INET6, server_name, &(server_info.sin6_addr)) == INADDR_NONE) /* IPv6 */
        {
            // IPアドレスではない
            // IPv6では, `gethostbyname`は使えない.
            // https://www.geekpage.jp/programming/winsock/getaddrinfo.php
            // https://www.ibm.com/docs/ja/zos/2.2.0?topic=applications-application-awareness-whether-system-is-ipv6-enabled
            // host = gethostbyname(server_name); // IPv4 名前解決

            std::printf("[Error] %s\n", strerror(errno));
            throw std::runtime_error("IP Address");
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
        std::snprintf(buf, sizeof(buf), "message from IPv6 client");
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