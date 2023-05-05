/**
 * @file dual_tcp_server.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief IPv4ソケットとIPv6ソケットの両刀待ち
 * @version 0.1
 * @date 2023-05-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <test_utils.hpp>

// tcp
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> // getaddrinfo, getnameinfo
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
// #include <sys/epoll.h> // No MacOS  http://linuxjm.osdn.jp/html/LDP_man-pages/man7/epoll.7.html

#if defined(__linux__)

#elif defined(__MACH__)

#else
// Windows
#endif

struct addrinfo hints, *response_list, *response;
constexpr int only_ipv6_flag = 1;
char *port_of_self = "54321";
constexpr int max_listen_size = 5;

// 使わない
char addr_name_ipv4[INET_ADDRSTRLEN];
char addr_name_ipv6[INET6_ADDRSTRLEN];
struct sockaddr_in *address_ipv4;
struct sockaddr_in6 *address_ipv6;
struct sockaddr_storage *p_client_bin;

#define BUFSIZE 1500


// ソケットマップ
using socket_t = int;
std::map<socket_t, std::pair<struct sockaddr_storage, socklen_t>> map_tcp_sockets;

void socket_address_error(int socket, struct sockaddr *address)
{
    char host_name[NI_MAXHOST];    // address
    char service_name[NI_MAXSERV]; // port
    if (getnameinfo(
            address, sizeof(struct sockaddr),
            host_name, sizeof(host_name),
            service_name, sizeof(service_name),
            NI_NUMERICHOST | NI_NUMERICSERV) != 0)
    {
        std::printf("[Error] %s\n", gai_strerror(errno));
        throw std::runtime_error("getnameinfo");
    }

    std::printf("[Error] socket: %d, host(address): %s, service(port): %s\n",
                socket, host_name, service_name);
}

struct HostInfo
{
    std::string mHostName;
    std::string mServiceName;
    std::string mNumericHostName;
    std::string mNumericServiceName;
};

HostInfo get_host_info(struct sockaddr *address)
{
    char host_name[NI_MAXHOST];    // address
    char service_name[NI_MAXSERV]; // port

    if (getnameinfo(
            address, sizeof(struct sockaddr),
            host_name, sizeof(host_name),
            service_name, sizeof(service_name),
            NI_NUMERICHOST | NI_NUMERICSERV) != 0)
    {
        std::printf("[Error] %s\n", gai_strerror(errno));
        throw std::runtime_error("getnameinfo");
    }

    HostInfo host_info;
    host_info.mNumericHostName = host_name;
    host_info.mNumericServiceName = service_name;

    if (getnameinfo(
            address, sizeof(struct sockaddr),
            host_name, sizeof(host_name),
            service_name, sizeof(service_name), 
            NI_NAMEREQD) != 0)
    {
        std::printf("[Error] %s\n", gai_strerror(errno));
        throw std::runtime_error("getnameinfo");
    }

    host_info.mHostName = host_name;
    host_info.mServiceName = service_name;

    return host_info;
}



int main(int argc, char **argv)
{
    try
    {
        /* 1.名前解決(FQDN -> IP) */
        hints.ai_family = PF_UNSPEC;     // IPv4/IPv6両刀待ち
        hints.ai_flags = AI_PASSIVE;     // 自動設定; IPv4: IN_ADDR_ANY, IPv6: IN6_ADDR_ANY_INIT
        hints.ai_socktype = SOCK_STREAM; // TCP
        int error = getaddrinfo(NULL, port_of_self, &hints, &response_list);
        if (error != 0)
        {
            std::printf("[Error] %s\n", gai_strerror(error));
            throw std::runtime_error("getaddrinfo");
        }
        std::printf("[Done] Step1. resolve FQDN.\n");

        /* 2.ソケットの作成 */
        for (response = response_list; 
             response != NULL; 
             response = response->ai_next)
        {
            int sock = socket(response->ai_addr->sa_family,
                              response->ai_socktype,
                              response->ai_protocol);

            if (sock < 0)
            {
                std::printf("[Error] %s\n", strerror(errno));
                socket_address_error(sock, response->ai_addr);
                throw std::runtime_error("socket");
            }

            if (response->ai_family == AF_INET6)
            {
                if (setsockopt(sock,
                               IPPROTO_IPV6,
                               IPV6_V6ONLY,
                               (void *)&only_ipv6_flag,
                               sizeof(only_ipv6_flag)) != 0)
                {
                    std::printf("[Error] %s\n", strerror(errno));
                    socket_address_error(sock, response->ai_addr);
                    throw std::runtime_error("setsockopt IPV6_V6ONLY");
                }
            }

            struct sockaddr_storage ss;
            std::memcpy(&ss, response->ai_addr, sizeof(struct sockaddr_storage)); // copy binary address info
            map_tcp_sockets[sock] = std::make_pair(ss, response->ai_addrlen);     // register
            std::printf("Make socket %d, %s\n", sock, response->ai_addr->sa_family == AF_INET6 ? "IPv6" : "IPv4");
        }
        freeaddrinfo(response_list); // response_listの解放
        std::printf("[Done] Step2. make passive sockets.\n");

        /* 3.bind処理 (IPアドレスとポート番号をソケットに紐付ける) */
        for (const auto &kv : map_tcp_sockets)
        {
            int passive_socket = kv.first;
            struct sockaddr *address = (struct sockaddr *)&(kv.second.first);

            // @warning 型サイズが異なる!
            std::printf("sizeof(struct sockaddr_in): %d\n", sizeof(struct sockaddr_in));
            std::printf("sizeof(struct sockaddr_in6): %d\n", sizeof(struct sockaddr_in6));
            std::printf("sizeof(struct sockaddr): %d\n", sizeof(struct sockaddr));
            std::printf("sizeof(struct sockaddr_storage): %d\n", sizeof(struct sockaddr_storage));
            std::printf("sizeof(kv.second): %d\n", sizeof(kv.second.first));

            if (bind(/*passive_socket*/ kv.first, address, kv.second.second) != 0)
            {
                std::printf("[Error] %s\n", strerror(errno));
                socket_address_error(passive_socket, address);
                throw std::runtime_error("bind");
            }
        }
        std::printf("[Done] Step3. bind sockets\n");

        /* 4.listen処理 (クライアントからの接続状態を待てる状態にする) */
        for (const auto &kv : map_tcp_sockets)
        {

            struct sockaddr *address = (struct sockaddr *)&(kv.second.first);
            int ret = listen(/*passive_socket*/ kv.first, max_listen_size);
            if (ret != 0)
            {
                std::printf("[Error] %s\n", strerror(errno));
                socket_address_error(/*passive_socket*/ kv.first, address);
                if (ret == EADDRINUSE)
                {
                    std::printf("Other socket listen this port. port of self is %u\n", port_of_self);
                }
            }
        }
        std::printf("[Done] Step4. listen sockets and accepting client ...\n");

        /* 5.accept処理 (クライアントからの接続要求を受ける) */
        // I/Oの多重化 https://blog.shibayu36.org/entry/20120101/1325418188
        // poll http://linuxjm.osdn.jp/html/LDP_man-pages/man2/poll.2.html
        // select http://linuxjm.osdn.jp/html/LDP_man-pages/man2/select.2.html
        // pollによるイベントループのやり方 https://www.geekpage.jp/programming/linux-network/book/08/8-18.php
        int num_targets = map_tcp_sockets.size();
        std::vector<struct pollfd> targets;
        targets.reserve(num_targets);
        targets.resize(num_targets);
        for (int i = 0; i < num_targets; ++i)
        { 
            std::memset(&targets[i], 0, sizeof(struct pollfd)); 
        }
        int index = 0;
        int nready = 0;
        
        // ポーリング設定(ソケットを巡回して状態をチェック)
        for (const auto &kv : map_tcp_sockets)
        {
            targets[index].fd = kv.first;             /* socket */
            targets[index].events = POLLIN | POLLERR; /* 読み出し可 | エラー */
            index++;
        }

        // acceptできるソケットを巡回する
        int timeout_count = 0;
        while (true)
        {
            // タイムアウトまでBlocking 1000[ms]
            nready = poll(targets.data(), num_targets, 300); // 300[ms] 

            if (nready == 0)
            {
                // タイムアウト
                if (timeout_count < 20)
                {
                    std::printf("*");
                    std::fflush(stdout);
                    timeout_count++;
                }
                else
                {
                    std::printf("\n");
                    timeout_count = 0;
                }
                
                continue;
            }

            if (nready == -1)
            {
                if (errno == EINTR)
                {
                    // 要求されたイベントのどれかが起こる前にシグナルが発生した
                    std::printf("[Error] EINTR -> continue: %s\n");
                    continue;
                }
                else
                {
                    std::printf("[Error] poll: %s\n", strerror(errno));
                    break; // whileを抜ける
                }
            }

            for (index = 0; index < num_targets; ++index)
            {
                if (targets[index].revents & POLLIN)
                {
                    break;
                }
            }
            // indexが処理すべきソケット

            socket_t passive_socket = targets[index].fd;
            socket_t socket_to_client;
            struct sockaddr *address = (struct sockaddr *)&(map_tcp_sockets[passive_socket].first);
            // socklen_t addlen = map_tcp_sockets[passive_socket].second; /* length of address (IPv4, IPv4 mapped IPv6, IPv6) */

            // ホスト情報
            HostInfo server_host_info = get_host_info(address);

            /* クライアント情報を受け取る(accept) */
            struct sockaddr client_info;
            socklen_t addlen = sizeof(client_info);
            socket_to_client = accept(passive_socket, &client_info, &addlen);
            if (socket_to_client == -1)
            {
                std::printf("[Error] %s\n", strerror(errno));
                socket_address_error(passive_socket, address);
                throw std::runtime_error("accept");
            }
            std::printf("[Done] Step5. accept client; passive_socket %d -> socket_to_client %d\n", 
                        passive_socket, socket_to_client);

            sleep(250); // 250[ms]

            /* 6.socket_to_clientと通信 */
            HostInfo client_host_info = get_host_info(&client_info);
            std::printf("Connection from : client %s, port=%s\n", 
                        client_host_info.mNumericHostName.c_str(), 
                        client_host_info.mNumericServiceName.c_str());

            // クライアントから受信
            char buf[BUFSIZE];
            std::memset(buf, 0, sizeof(buf));
            int n = read(socket_to_client, buf, sizeof(buf));
            std::printf("read n=%d, message : %s\n", n, buf);

            // クライアントに送信
            std:memset(buf, 0, sizeof(buf));
            std::snprintf(buf, sizeof(buf), "message from server %s, port=%s\n",
                          server_host_info.mNumericHostName.c_str(),
                          server_host_info.mNumericServiceName.c_str());
            n = write(socket_to_client, buf, strnlen(buf, sizeof(buf)));

            // クローズ
            close(socket_to_client);
        }

        // クローズ
        for (auto& kv : map_tcp_sockets)
        {
            close(/* passive_socket */kv.first);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}
