# アドレスファミリ独立なソケットプログラミング

### 参考
+ http://www.02.246.ne.jp/~torutk/ipv6/socket.html
+ https://www.iajapan.org/ipv6/summit/KYOTO2013/pdf/Hiromi_Kyoto.pdf
+ getaddrinfo https://linuxjm.osdn.jp/html/LDP_man-pages/man3/getaddrinfo.3.html
+ getnameinfo https://linuxjm.osdn.jp/html/LDP_man-pages/man3/getnameinfo.3.html
+ socket https://linuxjm.osdn.jp/html/LDP_man-pages/man2/socket.2.html
+ bind https://linuxjm.osdn.jp/html/LDP_man-pages/man2/bind.2.html
+ connect https://linuxjm.osdn.jp/html/LDP_man-pages/man2/connect.2.html
+ accept https://linuxjm.osdn.jp/html/LDP_man-pages/man2/accept.2.html
+ poll http://linuxjm.osdn.jp/html/LDP_man-pages/man2/poll.2.html
+ close https://linuxjm.osdn.jp/html/LDP_man-pages/man2/close.2.html
+ inet_pton https://linuxjm.osdn.jp/html/LDP_man-pages/man3/inet_pton.3.html
+ inet_ntop https://linuxjm.osdn.jp/html/LDP_man-pages/man3/inet_ntop.3.html


### プログラミングの流れ
1. ホスト名とサービス名から名前解決(ホスト名->IPアドレス, サービス名->Port番号)を行う.
2. アドレスのバイナリ情報は, `struct sockaddr_storage`に格納する. 
3. アドレス情報を受け渡しする際は, 特定のアドレスファミリ(IPv4/IPv6)に依存しない型`struct sockaddr *`を使う.
4. `ss_family`または`sa_family`を`AF_INET`,`AF_INET6`で判別する.
5. `struct sockaddr_storage`or`struct sockaddr`を, 本来の型(struct sockaddr_in, struct sockaddr_in6)にキャストして処理.


### ソケットのためのアドレスを保持する構造体
+ 各構造体で構造体のサイズが異なるので注意.
+ 各構造体のポインタ型のキャストで, 構造体内の利用できるメモリ領域を切り替える.
| 構造体 | 構造体サイズ sizeof(*) | 用途 |
| :-- | :-- | :-- |
| struct sockaddr | 16 | バイナリアドレス汎用 IPv4/IPv6 |
| struct sockaddr_storage | 128 | IPv4/IPv6のどちらかわからないパターンで使用する |
| struct sockaddr_in | 16 | IPv4 |
| struct sockaddr_in6 | 28 | IPv6 |

#### アドレス構造体の関係図(クラス図)
```
addrinfo <--- sockaddr <--+ socaddr_storage
 |   ↑                    |--- sockaddr_in 
 |---|                    |--- sockaddr_in6
```


### `struct sockaddr`の定義
```
// [Linux] sockaddr
struct sockaddr
{
	sa_family_t sa_family; // アドレスファミリ
	char sa_data[14]; // 14バイト以内のプロトコルアドレス
};

// [MacOS] sockaddr
struct sockaddr
{
	__uint8_t sa_len; /* total length */
	sa_family_t sa_family; /* address family */
#if __has_ptrcheck
	char sa_data[__counted_by(sa_len - 2)];
#else
	char sa_data[14];  /* addr value */
#endif
}; 
```


### `struct sockaddr_storage`の定義
```
// [Linux] sockaddr_storage
struct sockaddr_storage
{
	sa_family_t ss_family;
	char _sa_pad1[_SS_PAD1SIZE];
	sockaddr_maxalign_t _ss_align;
	char _ss_pad2[_SS_PAD2SIZE];
};

// [MacOS] sockaddr_storage
struct sockaddr_storage 
{
	__uint8_t        ss_len;         /* address length */
	sa_family_t     ss_family;      /* [XSI] address family */
	char                 __ss_pad1[_SS_PAD1SIZE];
	__int64_t         __ss_align;     /* force structure storage alignment */
	char                 __ss_pad2[_SS_PAD2SIZE];
};
```


### `struct sockaddr_in`の定義
```
// [Linux] sockaddr_in
struct sockaddr_in
{
	__SOCKADDR_COMMON(sin_);
	in_port_t sin_port; /* Port number. */
	struct in_addr sin_addr; /* Internet address. */
	
	/* Pad to size of `struct sockaddr`. */
	unsigned char sin_zero[sizeof (struct sockaddr) - 
	                         __SOCKADDR_COMMON_SIZE -
	                             sizeof (in_port_t) -
	                         sizeof (struct in_addr)];
};

// [Linux] in_addr
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;
struct in_addr
{
	in_addr_t s_addr;
};

// [MacOS] sockaddr_in
struct sockaddr_in
{
	__uint8_t sin_len;
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	char sin_zero[8];
};

// [MacOS] in_addr
struct in_addr
{
	in_addr_t s_addr;
};
```

### `struct sockaddr_in6`の定義
```
// [Linux] sockaddr_in6
struct sockaddr_in6
{
	sa_family_t sin6_family; /* AF_INET6 */
	in_port_t sin6_port; /* port number */
	uint32_t sin6_flowinfo; /* IPv6 flow information */
	struct in6_addr sin6_addr; /* IPv6 address */
	uint32_t sin6_scope_id; /* Scope ID */
};

// [Linux] in6_addr
struct in6_addr
{
	unsigned char s6_addr[16]; /* IPv6 address */
};

// [MacOS] sockaddr_in6
struct sockaddr_in6
{
	__uint8_t sin6_len; /* length of this struct(sa_family_t) */
	sa_family_t sin6_family; /* AF_INET6 */
	in_port_t sin6_port; /* Transport layer port */
	__uint32_t sin6_flowinfo; /* IP6 flow information */
	struct in6_addr sin6_addr; /* IP6 address */
	__uint32_t sin6_scope_id; /* scope zero index */
};

// [MacOS] in6_addr
typedef struct in6_addr
{
	union
	{
		__uint8_t    __u6_addr8[16];
		__uint16_t  __u6_addr16[8];
		__uint32_t  __u6_addr32[4];
	} __u6_addr; /* 128-bit IP6 address */
} in6_addr_t;
```


### アドレス情報を扱うAPI
| API(ポインタ型) | 用途 |
| :-- | :-- |
| struct sockaddr * | バイナリアドレス情報を受け渡す汎用API |
| struct sockaddr_in * | IPv4アドレス用API |
| struct sockaddr_in6 * | IPv6アドレス用API | 


### アドレスファミリ(IPv4とIPv6)の判別方法
+ `struct sockaddr_storage`の`ss_family`で判断
+ `struct sockaddr`の`sa_family`で判断
+ IPv6 : `AF_INET6`
+ IPv4 : `AF_NET`
```
// ss_familyで判断
struct sockaddr_storage ss;
if (ss.ss_family == AF_INET)
{
	// IPv4
	struct sockaddr_in* p_ipv4addr = (struct sockaddr_in*)&ss;
}
if (ss.ss_family == AF_INET6)
{
	// IPv6
	struct sockaddr_in6* p_ipv6addr = (struct sockaddr_in6*)&ss;
}

// sa_familyで判断
struct sockaddr addr;
if (addr.sa_family == AF_INET)
{
	// IPv4
	struct sockaddr_in* p_ipv4addr = (struct sockaddr_in*)&addr;
}
if (addr.sa_family == AF_INET6)
{
	// IPv6
	struct sockaddr_in6* p_ipv6addr = (struct sockaddr_in6*)&addr;
}
```

### `struct addrinfo`の定義 
```
// 名前解決時に取得できるアドレス情報
struct addrinfo
{
	int ai_flags; // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
	int ai_family; // AF_[XX]
	int ai_socktype; // SOCK_[XX]
	int ai_protocol; // IPv4とIPv6に応じて`0`または`IPPROTO_[XX]`
	size_t ai_addrlen; // ai_addr長
	char* ai_canonname; // ノード名の正規名
	struct sockaddr* ai_addr; // バイナリアドレス
	struct addrinfo* ai_next; // バイナリアドレスの次の構造体 (リスト構造になっている)
};
```

### 名前解決 ホスト名(FQDN) -> IPアドレス, サービス名(Service) -> Port番号.
```
// 名前解決関数
int getaddrinfo(
	const char* hostname,
	const char* servname,
	const struct addrinfo* hints;
	struct addrinfo** res
);

// アドレス情報の解放関数
void freeaddrinfo(struct addrinfo* res);

// エラー時の文字列取得関数
const char* gai_strerror(int errcode); // errnoを代入する.

```
+ hostnameかservnameどちらかはNULLでないことが必要.
+ hintsの条件にマッチしたaddrinfoのリストがresにセットされる.
+ DNS検索を避けるなら, `hints->ai_flags`にAI_NUMERICHOSTを指定する.
+ hostnameにNULLを指定すると, ループバックアドレス(`127.0.0.1`, `::1`)となる.
+ hostnameにNULLを指定し, 且つAI_PASSIVEを指定すると, ワイルドカードアドレスとなる.
+ スレッドセーフな関数.
+ 第4引数の`struct addrinfo** res`は, 内部でメモリ確保されるので, 使い終わった後は, `freeaddrinfo`でメモリを解放する.


#### アドレス構造体の関係図(クラス図)
```
addrinfo   <--- sockaddr <----------+ socaddr_storage
 |   ↑                              |---- sockaddr_in 
 |---|                              |---- sockaddr_in6
 ```

#### getaddrinfoのリスト構造
```
response_list-> addrinfo[0] ----------> addrinfo[1] ----------> NULL    
		        |                       |
		        | sockaddr              | sockaddr
		        | (sockaddr_in6)        | (sockaddr_in)
		        ----------------        ----------------
		        | IPv6 Address |        | IPv4 Address |
		        | Port Number  |        | Port Number  |
		        | etc...       |        | etc...       |
		        ----------------        ----------------
```	


### アドレス解決 IPアドレス -> ホスト名, ポート番号 -> サービス名
```
// アドレス解決関数
int getnameinfo(
	const struct sockaddr* addr,
	socklen_t addrlen,
	char* host,
	socklen_t hostlen,
	char* serv,
	socklen_t srvlen,
	int flag
);

// 各種メモリ領域
char addr_name_ipv4[INET_ADDRSTRLEN];
char addr_name_ipv6[INET6_ADDRSTRLEN];
char host_name[NI_MAXHOST];
char service_name[NI_MAXSERV];
```


### 複数ソケットを処理するサーバーの流れ
1. getaddrinfo関数で, 自身のプロトコル・アドレスをaddrinfo構造体のリストで受け取る
+ hintsパラメータでAI_PASSIVEを指定すると, IN_ADDR_ANYとIN6ADDR_ANY_INITが得られる.
2. リストで得られたプロトコル・アドレス個別に下記を実施
+ socket, bind, listen
3. 得られた複数のファイルディスクリプタを`struct pollfd`構造体(`fd_set`)に保存
4. `fd_set`に対するイベントループ処理の実施
+ select/pollで接続の待機
+ select/pollを抜けてきたfdに対してaccept
+ 必要に応じてacceptで得られたクライアントソケットを別スレッド/別プロセスに渡して処理を委譲する
+ 読み書き処理
+ クローズ

### 推奨API (非アドレスファミリ依存)
| 名前 | 機能 |
| :-- | :-- |
| struct sockaddr_storage | 汎用バイナリアドレス用ソケット構造体 |
| getaddrinfo | 名前解決(FQDN -> IP Address)用関数 |
| getnameinfo | アドレス解決(IP Address -> FQDN)用関数 |
| struct addinfo | 1インスタンスで1アドレスを持ち, リスト構造の1要素 |
| struct sockaddr | IPv4やIPv6等各種アドレス情報を汎化した構造体. 実体は, sockaddr_in6, sockaddr_in, どのアドレスが入るかわからない場合は, sockaddr_storageで定義する |
| int inet_pton(int af, const char *src, void *dst); | IPv4/IPv6 アドレスをテキスト形式からバイナリ形式に変換する   |
| const char *inet_ntop(int af, const void *src, char *dst, socklen_t size); | IPv4/IPv6 アドレスをバイナリ形式からテキスト形式に変換する |


## 旧APIの使用をやめる(非推奨) プロトコル依存(IPv4依存)関数・構造体・マクロ

### インターネット操作ルーチン
+ inet_aton, inet_addr, inet_network, inet_ntoa, inet_makeaddr, inet_lnaof, inet_netof.
+ https://linuxjm.osdn.jp/html/LDP_man-pages/man3/inet_addr.3.html 
| 名前 | 機能 |
| :-- | :-- |
| in_addr_t inet_addr(const char *cp); |  |
| int inet_aton(const char *cp, struct in_addr *inp); |  |
| char *inet_ntoa(struct in_addr in); |  |
| in_addr_t inet_network(const char *cp); |  |
| struct in_addr inet_makeaddr(in_addr_t net, in_addr_t host); |  |
| in_addr_t inet_lnaof(struct in_addr in); |  |
| in_addr_t inet_netof(struct in_addr in); |  |

### ネットワーク上のホストのエントリーを取得する
+ gethostbyname, gethostbyaddr, sethostent, gethostent, endhostent, 
+ h_errno, herror, hstrerror, 
+ gethostbyaddr_r, gethostbyname2, gethostbyname2_r, gethostbyname_r, gethostent_r  
+ https://linuxjm.osdn.jp/html/LDP_man-pages/man3/gethostbyname.3.html
| 名前 | 機能 |
| :-- | :-- |
| struct hostent *gethostbyname(const char *name); | |
| struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type); | |
| void sethostent(int stayopen); | |
| void endhostent(void); | |
| void herror(const char *s); | |
| const char *hstrerror(int err); | |
| struct hostent *gethostent(void); | /* System V/POSIX 拡張 */ |
| struct hostent *gethostbyname2(const char *name, int af); | /* GNU 拡張 */ |
| int gethostent_r(struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop); | /* GNU 拡張 */ |
| int gethostbyaddr_r(const void *addr, socklen_t len, int type, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop); | /* GNU 拡張 */ |
| int gethostbyname_r(const char *name, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop); | /* GNU 拡張 */ |
| int gethostbyname2_r(const char *name, int af, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop); | /* GNU 拡張 */ |
+ struct hostent構造体
```
struct hostent { 
    char  *h_name;            /* official name of host */ 
    char **h_aliases;         /* alias list */ 
    int    h_addrtype;        /* host address type */ 
    int    h_length;          /* length of address */ 
    char **h_addr_list;       /* list of addresses */ } #define h_addr h_addr_list[0] /* 過去との互換性のため */
```
+ h_name ホストの正式名 (official name)。
+ h_aliases ホストの別名の配列。配列はヌルポインターで終端される。
+ h_addrtype アドレスのタイプ。現在はすべて AF_INET または AF_INET6 である。
+ h_length バイト単位で表したアドレスの長さ。
+ h_addr_list ホストのネットワークアドレスへのポインターの配列。 配列はヌルポインターで終端される。 ネットワークアドレスはネットワークバイトオーダ形式である。
+ h_addr h_addr_list の最初のアドレス。過去との互換性を保つためのものである。


### サービスのエントリーを取得する 
+ getservent, getservbyname, getservbyport, setservent, endservent
+ https://linuxjm.osdn.jp/html/LDP_man-pages/man3/getservent.3.html
| 名前 | 機能 |
| :-- | :-- |
| struct servent *getservent(void); | |
| struct servent *getservbyname(const char *name, const char *proto); | |
| struct servent *getservbyport(int port, const char *proto); | | 
| void setservent(int stayopen); | |
| void endservent(void); |
+ struct servent構造体
```
struct servent { 
    char  *s_name;       /* official service name */ 
    char **s_aliases;    /* alias list */ 
    int    s_port;       /* port number */ 
    char  *s_proto;      /* protocol to use */ }
```
+ s_name サービスの正式名 (official name)。
+ s_aliases サービスの別名のリスト。 リストはヌルで終端される。
+ s_port サービスのポート番号。ネットワークバイトオーダで指定される。
+ s_proto このサービスと共に用いるプロトコルの名前。




