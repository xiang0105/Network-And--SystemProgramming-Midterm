# Loopback Packet Capture Evidence

此目錄保存本專案在 loopback `127.0.0.1:9734` 上的最小封包證據。擷取 filter 僅包含 `tcp port 9734`，不包含其他網路流量。

## Evidence file

- 檔案：`loopback-9734.pcap`
- 擷取時間：2026-11-08 01:28:54（Asia/Taipei）
- 封包數：22
- SHA-256：`f7f5d5cda30088e85e50de741b8d30f35986044540d7d087ec110f84b087e307`
- Endpoint：client `127.0.0.1:46982` → server `127.0.0.1:9734`
- Client 操作：取得 system information、取得 connection stats、QUIT

## Capture and read commands

```sh
sudo tcpdump -U -i lo -nn -s 0 \
  -w evidence/packet-capture/loopback-9734.pcap \
  'tcp port 9734'

tcpdump -nn -tttt -r evidence/packet-capture/loopback-9734.pcap
tcpdump -nn -X -r evidence/packet-capture/loopback-9734.pcap
```

## TCP analysis

1. `01:28:54.011165` client 傳送 `SYN`。
2. `01:28:54.011188` server 回覆 `SYN, ACK`。
3. `01:28:54.011202` client 回覆 `ACK`，TCP three-way handshake 完成。
4. Client 傳送 `QUIT` 後，在 `01:28:54.099830` 傳送 `FIN, ACK`；server 於 `01:28:54.099863` 回覆 `FIN, ACK`；client 最後 ACK，連線正常關閉。

## Application framing analysis

以下欄位皆可在 `tcpdump -X` 的 TCP payload 中直接核對：

| 方向 | Header hex | 解碼 |
| --- | --- | --- |
| Server → Client | `4142474d 01 81 0000 0000001b` | `WELCOME`，payload 27 bytes |
| Client → Server | `4142474d 01 03 0000 00000000` | `GET_SYSTEM_INFO`，payload 0 bytes |
| Server → Client | `4142474d 01 86 0000 000000a4` | `SYSTEM_INFO`，payload 164 bytes |
| Client → Server | `4142474d 01 04 0000 00000000` | `GET_STATS`，payload 0 bytes |
| Server → Client | `4142474d 01 87 0000 0000002c` | `STATS`，payload 44 bytes |
| Client → Server | `4142474d 01 05 0000 00000000` | `QUIT`，payload 0 bytes |

`4142474d` 即 ASCII `ABGM`；接著是 version `01`、message type、reserved `0000`，最後四 bytes 是 network byte order 的 payload length。

值得注意的是 `WELCOME`、`SYSTEM_INFO` 與 `STATS` 的 12-byte header 和文字 payload 分別出現在不同 TCP segments。例如 `WELCOME` header 的 TCP payload 長度是 12，下一個 segment 才是 27-byte 文字。這是實際執行期證據，證明 TCP 不保留 application message boundary，因此本專案必須使用完整讀寫迴圈與明確 frame length，不能假設一次 `recv()` 會取得完整 message。

## Privacy check

擷取內容只有 loopback 位址、臨時來源 port、協定 header 與規定的唯讀 system information；未包含密碼、token、使用者名稱、私人路徑、環境變數或其他網路介面流量。
