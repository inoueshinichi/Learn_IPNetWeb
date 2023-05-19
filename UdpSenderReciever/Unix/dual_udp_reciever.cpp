/**
 * @file dual_udp_reciever.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief Ipv4 & IPv6 両刀待ち
 * @version 0.1
 * @date 2023-05-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <test_utils.hpp>

// udp
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> // getaddrinfo, getnameinfo
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>

#if defined(__linux__)

#elif defined(__MACH__)

#else
// Windows
#endif

#define BUFSIZE 2048

// 使う
struct addrinfo hints, *response_list, *response;
constexpr int only_ipv6_flag = 1;
char* port_of_self = "54321";
char buf[BUFSIZE];

// 使わない
char addr_name_ipv4[INET_ADDRSTRLEN];
char addr_name_ipv6[INET6_ADDRSTRLEN];
struct sockaddr_in *address_ipv4;
struct sockaddr_in6 *address_ipv6;
struct sockaddr_storage *p_client_bin;

// ソケットマップ
using socket_t = int;
std::map<socket_t, 
         std::pair<struct sockaddr_storage, socklen_t>
        > map_udp_sockets;

// ホスト情報
struct HostInfo
{
    std::string mHostName;
    std::string mServiceName;
    std::string mNumericHostName;
    std::string mNumericServiceName;
};

// 送信元ホスト情報のマップ
using MapIPSenderHostInfo = std::map<std::string, HostInfo>;
std::map<socket_t, MapIPSenderHostInfo> map_sender_hosts_ipv6;
std::map<socket_t, MapIPSenderHostInfo> map_sender_hosts_ipv4;

// アドレス解決時のエラーを表示
void socket_address_error(socket_t socket, struct sockaddr* address)
{
    char host_name[NI_MAXHOST]; // address
    char service_name[NI_MAXSERV]; // port
    if (getnameinfo(
        address, sizeof(struct sockaddr),
        host_name, sizeof(host_name),
        service_name, sizeof(service_name),
        NI_NUMERICHOST | NI_NUMERICSERV
    ) != 0)
    {
        std::printf("[Error] %s\n", gai_strerror(errno));
        throw std::runtime_error("getnameinfo");
    }

    std::printf("[Error] socket: %d, host(address): %s, service(port): %s\n",
                socket, host_name, service_name);
}


// ホスト情報を得る
HostInfo get_host_info(struct sockaddr* address)
{
    char host_name[NI_MAXHOST]; // address
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


int main(int argc, char** argv)
{
    try
    {
        /* 1.名前解決(FQDN -> IP) */
        hints.ai_family = PF_UNSPEC;     // IPv4/IPv6両刀待ち
        hints.ai_flags = AI_PASSIVE;     // 自動設定; IPv4: IN_ADDR_ANY, IPv6: IN6_ADDR_ANY_INIT
        hints.ai_socktype = SOCK_DGRAM; // UDP
        int error = getaddrinfo(NULL, port_of_self, &hints, &response_list);
        if (error != 0)
        {
            std::printf("[Error] %s\n", gai_strerror(error));
            throw std::runtime_error("getaddrinfo");
        }
        std::printf("[Done] Step1. resolve FQDN.\n");

        /* 2.ソケットの作成 */
        for (response = response_list;
             response != nullptr;
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
            map_udp_sockets[sock] = std::make_pair(ss, response->ai_addrlen);     // register
            std::printf("Make socket %d, %s\n", sock, response->ai_addr->sa_family == AF_INET6 ? "IPv6" : "IPv4");
        }
        freeaddrinfo(response_list); // response_listの解放
        std::printf("[Done] Step2. make passive sockets.\n");

        /* 3.bind処理 (IPアドレスとポート番号をソケットに紐付ける) */
        for (const auto &kv : map_udp_sockets)
        {
            int passive_socket = kv.first;
            socklen_t socket_length = kv.second.second;
            struct sockaddr *address = (struct sockaddr *)&(kv.second.first);
            // @warning 型サイズが異なる!
            std::printf("sizeof(struct sockaddr_in): %d\n", sizeof(struct sockaddr_in));
            std::printf("sizeof(struct sockaddr_in6): %d\n", sizeof(struct sockaddr_in6));
            std::printf("sizeof(struct sockaddr): %d\n", sizeof(struct sockaddr));
            std::printf("sizeof(struct sockaddr_storage): %d\n", sizeof(struct sockaddr_storage));
            std::printf("----------------------------------------------\n");
            std::printf("sizeof(kv.second): %d\n", sizeof(kv.second.first));

            if (bind(passive_socket, address, socket_length) != 0)
            {
                std::printf("[Error] %s\n", strerror(errno));
                socket_address_error(passive_socket, address);
                throw std::runtime_error("bind");
            }
        }
        std::printf("[Done] Step3. bind sockets\n");

        /* 6.I/Oの多重化 */
        int num_targets = map_udp_sockets.size();
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
        for (const auto &kv : map_udp_sockets)
        {
            targets[index].fd = kv.first;             // socket
            targets[index].events = POLLIN | POLLERR; /* 読み出し可 | エラー */
            index++;
        }
        int timeout_count = 0;
        const int timeout_ms = 500;
        const int shutdown_count = 20;
        while (true)
        {
            // タイムアウトまでBlocking [ms]
            nready = poll(targets.data(), num_targets, timeout_ms);

            if (nready == 0)
            {
                // タイムアウト
                if (timeout_count < shutdown_count)
                {
                    std::printf("*%d", timeout_count + 1);

                    int digit = 0; // 桁数
                    int tmp_count = timeout_count + 1;
                    while (tmp_count != 0)
                    {
                        tmp_count /= 10;
                        digit++;
                    }
                    // カウント(tmp_count)の桁数分だけカーソルを戻す
                    for (int i = 0; i < digit; ++i)
                    {
                        std::printf("\b"); // カーソルを左に一つ戻す
                    }
                    
                    std::fflush(stdout); // https://daeudaeu.com/fflush/#fflush-3
                    timeout_count++;
                }
                else if (timeout_count == shutdown_count)
                {
                    std::printf("\n");
                    break; // whileループを抜ける
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

            // 受信チェック
            for (index = 0; index < num_targets; ++index)
            {
                if (targets[index].revents & POLLIN)
                {
                    break;
                }
            }
            // indexが処理すべきソケット
            timeout_count = 0;

            socket_t passive_socket = targets[index].fd;
            struct sockaddr_storage ss = map_udp_sockets[passive_socket].first;
            struct sockaddr *address = (struct sockaddr *)&ss;
            socklen_t socket_length = map_udp_sockets[passive_socket].second; /* length of address (IPv4, IPv4 mapped IPv6, IPv6) */

            /* 7.senderからの受信 */
            std::memset(buf, 0, sizeof(buf));
            int n = recvfrom(passive_socket,
                             buf,
                             sizeof(buf) - 1,
                             0,
                             address, // 複数の送信元ホストからの情報が流れ込む
                             &socket_length);

            // ホスト情報
            HostInfo sender_host_info = get_host_info(address);

            std::printf("Connection from : sender %s, port=%s\n",
                        sender_host_info.mNumericHostName.c_str(),
                        sender_host_info.mNumericServiceName.c_str());

            // 標準出力にそのまま出力
            std::printf("%s\n", buf);

            // 送信元ホスト情報を登録
            if (address->sa_family == AF_INET6)
            {
                // IPv6
                auto &map_ipv6_host = map_sender_hosts_ipv6[passive_socket];

                auto iter = map_ipv6_host.find(sender_host_info.mNumericHostName);
                if (iter == map_ipv6_host.end())
                {
                    map_ipv6_host[sender_host_info.mNumericHostName] = sender_host_info; // register
                }
            }
            else
            {
                // IPv4
                auto &map_ipv4_host = map_sender_hosts_ipv4[passive_socket];

                auto iter = map_ipv4_host.find(sender_host_info.mNumericHostName);
                if (iter == map_ipv4_host.end())
                {
                    map_ipv4_host[sender_host_info.mNumericHostName] = sender_host_info; // register
                }
            }
        } // while

        // クローズ
        for (auto &kv : map_udp_sockets)
        {
            close(/* passive_socket */ kv.first);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}