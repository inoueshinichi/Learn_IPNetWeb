# Learn_IPNetWeb
How to use and understand Web IP Internet


### How to connect by TCP
| 手順 | 受信側(サーバー) | 送信側(クライアント) |
| :-- | :-- | :-- |
| 1 | ソケットの作成 socket() | ソケットの作成 socket() |
| 2 | 接続待ちのための<br>IPアドレスと<br>ポート番号を設定 | 接続相手の<br>IPアドレス<br>とポート番号を設定 |
| 3 | ソケットに名前をつける bind() | - |
| 4 | 接続を待つ listen() | - |
| 5 | 接続を受け付ける accept() | - |
| 6 | 通信を行う write() -> read() |  通信を行う read() <- write() |
| 7 | ソケットを閉じる close() | ソケットを閉じる close() |


### How to connect by UDP
| 手順 | 受信側(サーバー) | 送信側(クライアント) |
| :-- | :-- | :-- |
| 1 | ソケットの作成 socket() | ソケットの作成 scoket() |
| 2 | 接続待ちのための<br>IPアドレスと<br>ポート番号を設定 | 接続相手の<br>IPアドレス<br>とポート番号を設定 |
| 3 | ソケットに名前をつける bind() | - |
| 4 | 受信する recv() | 送信する sendto() |
| 5 | ソケットを閉じる close() | ソケットを閉じる close() |




