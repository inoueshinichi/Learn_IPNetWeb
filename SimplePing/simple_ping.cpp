/**
 * @file simple_ping.cpp
 * @author Shinichi Inoue (inoue.shinichi.1800@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-04-30
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <test_utils.hpp>

// ping
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>

#define BUFSIZE 1500
#define ECHO_HDR_SIZE 8

/* チェックサム作成 */
static int CalcChecksum(u_short *ptr, int nbytes)
{
    long sum;
    u_short oddbyte;
    u_short answer;

    /* 対象となるパケットに対して,
    16ビット毎の1の補数和をとり,
    更にそれの1の補数をとる*/

    // パケット(先頭ptr, 長さnbytes)のメモリを16bit毎に和を取る.
    sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++; // 16bit
        nbytes -= 2;   // 2byte = 16bit
    }
    // このタイミングで, nbytes == 0 or 1

    // nbytesは奇数バイト
    if (nbytes == 1)
    {
        oddbyte = 0;
        *((u_char *)&oddbyte) = *(u_char *)ptr; // 先頭ポインタptrの先頭8bit
        sum += oddbyte;
    }

    // 1の補数を取る
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    answer = ~sum;

    return answer;
}

/* ping送信 */
static int SendPing(int soc,
                    char *name,
                    int len,
                    unsigned short sqc,
                    struct timeval *sendtime)
{
    struct hostent *host;
    struct sockaddr_in *sinp;
    struct sockaddr sa;

#if defined(__linux__)
    struct icmphdr *icp;
#elif defined(__MACH__)
    struct icmp *icp;
#else
    // Windows
#endif
    unsigned char *ptr;
    int psize;
    int n;
    char sbuff[BUFSIZE];

    sinp = (struct sockaddr_in *)&sa;
    sinp->sin_family = AF_INET;

    /* 宛先の確定 */
    if ((sinp->sin_addr.s_addr = inet_addr(name)) == INADDR_NONE)
    {
        /* IPアドレスではない */
        host = gethostbyname(name);
        if (host == NULL)
        {
            // std::printf("%s\n", strerror(errno));
            std::printf("No IP Address : %s\n", name);
            return -100;
        }
        sinp->sin_family = host->h_addrtype;
        std::memcpy(&(sinp->sin_addr), host->h_addr, host->h_length);
    }

    /* 送信時間 */
    gettimeofday(sendtime, NULL);

    /* 送信データ作成 */
    std::memset(sbuff, 0, BUFSIZE);
#if defined(__linux__)

#elif defined(__MACH__)
    icp = (struct icmp *)sbuff;
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_hun.ih_idseq.icd_id = htons((unsigned short)getpid()); // Intel リトルエンディアン形式 -> INet ビッグエンディアン形式
    icp->icmp_hun.ih_idseq.icd_seq = htons(sqc); // シーケンス番号
    ptr = (unsigned char *)&sbuff[ECHO_HDR_SIZE];
    psize = len - ECHO_HDR_SIZE; // 全体 : len, Echo Header : ECHO_HDR_SIZE
    for (; psize; psize--)       // 残りバイトにパディング
    {
        *ptr++ = (unsigned char)0xA5; // 仮データ
    }
    ptr = (unsigned char *)&sbuff[ECHO_HDR_SIZE]; // Echo Headerの末尾(残りバイトの先頭)
    std::memcpy(ptr, sendtime, sizeof(struct timeval));
    icp->icmp_cksum = CalcChecksum((u_short *)icp, len);
#else

#endif

    /* 送信 */
    n = sendto(soc, sbuff, len, 0, &sa, sizeof(struct sockaddr));
    std::printf("send %d bytes\n", n);
    if (n == len)
    {
        return 0;
    }
    else
    {
        return -1000;
    }
}

/* 受信パケットの確認 */
static int CheckPacket(char *rbuff,
                       int nbytes,
                       int len,
                       struct sockaddr_in *from,
                       unsigned short sqc,
                       int *ttl, /* time to live */
                       struct timeval *sendtime,
                       struct timeval *recvtime,
                       double *diff)
{
#if defined(__linux__)

#elif defined(__MACH__)
    struct ip *iph;
    struct icmp *icp;

#else
    // Windows
#endif

    unsigned char *ptr;

    /* RTTを計算(ms) */
    *diff = (double)(recvtime->tv_sec - sendtime->tv_sec) +
            (double)(recvtime->tv_usec - sendtime->tv_usec) / 1000000.0;

    /* 受信バッファにはIPヘッダも含まれている */
#if defined(__linux__)

#elif defined(__MACH__)
    iph = (struct ip *)rbuff;
    *ttl = iph->ip_ttl;
#else
    // Windows
#endif

    /* ICMPヘッダ */
#if defined(__linux__)

#elif defined(__MACH__)
    icp = (struct icmp *)(rbuff + iph->ip_hl * 4);
#else
    // Windows
#endif

    /* 内容の確認 */
#if defined(__linux__)

#elif defined(__MACH__)
    if (ntohs(icp->icmp_hun.ih_idseq.icd_id) != (unsigned short)getpid())
    {
        std::printf("%s\n", strerror(errno));
        return 1; // プロセスID エラー
    }
    if (nbytes < len + iph->ip_hl * 4)
    {
        return -3000; // IPヘッダ エラー
    }
    if (icp->icmp_type != ICMP_ECHOREPLY)
    {
        return -3010; // ICMPタイプ エラー
    }
    if (ntohs(icp->icmp_hun.ih_idseq.icd_seq) != sqc)
    {
        return -3030; // シーケンス番号 エラー
    }
#else
    // Windows
#endif

    ptr = (unsigned char *)(rbuff + iph->ip_hl * 4 + ECHO_HDR_SIZE); // ICMPデータの先頭ポインタ
    std::memcpy(sendtime, ptr, sizeof(struct timeval));              // 送信時刻を取得
    ptr += sizeof(struct timeval);
    int rest_datasize = nbytes - iph->ip_hl * 4 - ECHO_HDR_SIZE - sizeof(struct timeval);
    for (int i = rest_datasize; i > 0; i--)
    {
        // すべて0xA5の詰め物
        if (*ptr++ != 0xA5)
        {
            return -3040; // データ内容 エラー
        }
    }

    std::printf(
        "%d bytes from %s : icmp_seq=%d ttl=%d time=%.2f ms\n",
        nbytes - iph->ip_hl * 4,
        inet_ntoa(from->sin_addr),
        sqc,
        *ttl,
        *diff * 1000.0);

    return 0;
}

static int RecvPing(int soc, int len, unsigned short sqc, timeval *sendtime, int timeout_sec)
{
    struct pollfd targets[1];
    double diff;
    int nready;
    int ret;
    int nbytes;
    int ttl;
    struct sockaddr_in from;
    socklen_t fromlen;
    struct timeval recvtime;
    char rbuff[BUFSIZE];

    std::memset(rbuff, 0, BUFSIZE);

    // Socketをポーリング(一定周期で監視してイベントが来たときに通知)
    while (true)
    {
        // ポーリング設定
        targets[0].fd = soc;
        targets[0].events = POLLIN | POLLERR;
        nready = poll(targets, 1, timeout_sec * 1000); // 監視チェック

        std::printf("poll\n");

        // 結果
        if (nready == 0)
        {
            // タイムアウト
            std::printf("%s\n", strerror(errno));
            return -2000;
        }
        if (nready == -1)
        {
            // エラー
            if (errno == EINTR)
            {
                // インターネット起因
                std::printf("continue\n");
                continue; // 再度受信バッファの確認
            }
            else
            {
                std::printf("%s\n", strerror(errno));
                return -2010;
            }
        }

        /* 受信 */
        fromlen = sizeof(from);
        nbytes = recvfrom(soc, rbuff, sizeof(rbuff), 0, (struct sockaddr *)&from, &fromlen);

        /* 受信時刻 */
        gettimeofday(&recvtime, NULL);

        /* 受信パケットの確認 */
        ret = CheckPacket(rbuff,
                          nbytes,
                          len,
                          &from,
                          sqc,
                          &ttl,
                          sendtime,
                          &recvtime,
                          &diff);

        switch (ret)
        {
        case /* constant-expression */ 0:
        {
            /* 自プロセスREPLYを正常に受信 */
            return (int(diff * 1000.0));
        }

        case /* constant-expression */ 1:
        {
            /* 他プロセスREPLYだった */
            if (diff > (timeout_sec * 1000))
            {
                // タイムアウト
                return -2000;
            }
            break;
        }

        default:
            /* 自プロセスREPLYだが内容が異常 */
            ;
        }

        std::printf("loop\n");
    } // while

#if defined(__linux__)

#elif defined(__MACH__)

#else
    // Windows
#endif
}

/* ping送受信 */
int PingCheck(char *name, int len, int times, int timeout_sec)
{
    int soc;
    struct timeval sendtime;
    int ret;
    int total = 0, total_no = 0;

    /* ソケット作成 */
    if ((soc = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        std::printf("%s\n", strerror(errno));
        return -300;
    }

    for (int i = 0; i < times; ++i)
    {
        /* Echo Requestの送信 */
        ret = SendPing(soc, name, len, (unsigned short)(i + 1), &sendtime);
        if (ret == 0)
        {
            /* Echo Replyを受信 */
            ret = RecvPing(soc, len, (unsigned short)(i + 1), &sendtime, timeout_sec);
            if (ret >= 0)
            {
                total += ret;
                total_no++;
            }
        }

        /* スリープ */
#if defined(__linux__)

#elif defined(__MACH__)
        sleep(1);
#else
        // Windows
#endif
    }

    /* ソケットを閉じる */
#if defined(__linux__)

#elif defined(__MACH__)
    close(soc);
#else
    // Windows
#endif

    if (total_no > 0)
    {
        return total / total_no;
    }
    else
    {
        return -1;
    }
}

int main(int argc, char **argv)
{
    try
    {
        char ip_address[] = "127.0.0.1";

        std::cout << "root uid : 0. Given is uid: " << getuid() << std::endl;

        /**
         * @warning macOSの場合, root権限にしないと, pingでRAWソケットを作成できない.
         */

        /* ping送受信 */
        int ret = PingCheck(ip_address,
                            64, // 64バイトのICMPパケット
                            5,  // 5回送受信を繰り返し
                            1); // 待ち時間は1秒

        if (ret < 0)
        {
            std::printf("[Error]: %d\n", ret);
            // return EXIT_FAILURE;
        }
        else
        {
            std::printf("[Success]: %d\n", ret);
            // return EXIT_SUCCESS;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
};