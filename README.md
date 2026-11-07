# 幾A幾B Client-Server Game

期中專題：以 C 實作「幾A幾B」client-server 小遊戲。

目前確定採用獨立單人模式：每個 client 各自連線並挑戰由 server 產生的答案；server 對每個 connection 使用獨立的 child process 保存該局狀態並處理猜測。

## 基準開發環境

- Windows Subsystem for Linux 2（WSL 2.6.3）
- 預設 WSL distribution 名稱：`Ubuntu-24.04`；實際系統：Ubuntu 24.04.5 LTS、x86_64
- Linux kernel：`6.6.87.2-microsoft-standard-WSL2`
- 專案路徑：`/mnt/d/Data/Project/Network-And--SystemProgramming-midterm`
- 已驗證工具：GCC 13.3、GNU Make 4.3、CMake 3.28.3、Git 2.43、GDB、`ldd`、`nc`、tcpdump 4.99.4、valgrind 3.22、socat 1.8
- 目前 WSL 網路模式為 NAT；本機開發與 loopback 測試先沿用 NAT，跨主機測試前再設定 mirrored networking 或等效的安全轉送方式。
- 舊的 `Ubuntu-22.04` registration 已依使用者確認移除；`Ubuntu-24.04` 的完整建置、CTest 與整合測試均通過後，遷移備份亦已依使用者先前指示刪除。

## 建置系統

- 正式建置系統採用 CMake，不另外維護手寫 Makefile。
- 採用 out-of-source build，所有建置產物放在 `build/`，不得混入原始碼目錄或提交 Git。
- CMake targets 至少包含動態共用函式庫 `game_common`、`server`、`client` 與自動測試。
- 支援 Debug／Release 組態；測試統一透過 CTest 執行。
- 預定的基本操作為：

  ```sh
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build build
  ctest --test-dir build --output-on-failure
  ```

- 建置完成後可直接從建置目錄啟動：

  ```sh
  cd build
  ./server
  # 另一個終端，同樣位於 build/
  ./client
  ```

- Server 可覆寫監聽位置、port、逾時及執行期 log level，例如測試模式：

  ```sh
  ./server --host 127.0.0.1 --port 9734 \
    --idle-timeout 10 --log-level debug
  ```

- 編譯期 debug log 由 CMake option 控制；正式關閉方式為：

  ```sh
  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_DEBUG_LOG=OFF
  cmake --build build-release
  ```

## 連線模型

- 一般操作直接執行 `./server` 與 `./client`；預設使用 `127.0.0.1:9734`，並保留可選參數供跨主機或 port 衝突時覆寫。
- Port `9734` 是 server 的共同 listening endpoint，不是一條 connection；多個 clients 可同時連到同一 port，TCP 會以來源／目的 IP 與 port 的組合區分各 connection。
- 每次 `accept()` 成功後都建立一個 child process；同一 IP 建立多條 TCP connections 時，也必須建立多個 child processes。
- 同一條 connection 可連續遊玩多局；每局結束後可開始新局或離開。
- Client 僅以該條 TCP connection 識別；來源 IP 與 port 只用於 log 與執行證據，不作為玩家身分或共用 child 的依據。
- 連線成功後顯示主選單：開始新遊戲、查看 server system information、查看本次 connection 戰績、離開。每局結束後回到主選單。
- 「本次 connection 戰績」只是目前 client 視窗的臨時計數，包含完成局數、勝利、失敗及最佳猜測次數；不建立帳號、不與其他玩家比較，關閉 connection 後清空。

## 通訊協定 Header

每個 application message 都由固定 12-byte header 與其後的 payload 組成：

| Offset | 長度 | 欄位 | 說明 |
| --- | ---: | --- | --- |
| 0 | 4 bytes | Magic | 固定 ASCII `ABGM` |
| 4 | 1 byte | Version | 第一版固定為 `1` |
| 5 | 1 byte | Type | 訊息種類 |
| 6 | 2 bytes | Reserved | 第一版傳送時必須為 `0` |
| 8 | 4 bytes | Payload length | Payload byte 數，使用 network byte order |

- Header 必須逐欄位編碼與解碼，不得直接將 C `struct` 的記憶體內容送上網路，以避免 padding、alignment 與大小端差異。
- 收送函式必須處理 partial read／write 與 `EINTR`，並在配置或讀取 payload 前檢查協定版本及長度上限。
- 每個 payload 最多 4096 bytes；接收端必須在讀取或配置 payload buffer 前拒絕超過上限的 header。
- 猜測訊息的 payload 固定為恰好 4 bytes 數字字元，例如 `0123`，不包含字串終止字元。
- System information、connection 戰績與錯誤訊息使用 UTF-8 文字；header 的 payload length 不包含本機 C 字串使用的 `\0`。

Message type 使用以下固定值；Client 不得傳送 Server response type，Server 也不得傳送 Client request type：

| 數值 | 方向 | Message type |
| ---: | --- | --- |
| `0x01` | Client → Server | `START_GAME` |
| `0x02` | Client → Server | `GUESS` |
| `0x03` | Client → Server | `GET_SYSTEM_INFO` |
| `0x04` | Client → Server | `GET_STATS` |
| `0x05` | Client → Server | `QUIT` |
| `0x81` | Server → Client | `WELCOME` |
| `0x82` | Server → Client | `GAME_STARTED` |
| `0x83` | Server → Client | `GUESS_RESULT` |
| `0x84` | Server → Client | `GAME_WON` |
| `0x85` | Server → Client | `GAME_LOST` |
| `0x86` | Server → Client | `SYSTEM_INFO` |
| `0x87` | Server → Client | `STATS` |
| `0xFF` | Server → Client | `ERROR` |

## Server System Information

Client 可透過主選單向 server 請求以下唯讀資訊：

- Linux／kernel 版本
- CPU 架構與邏輯核心數
- 系統運行時間
- 總記憶體與可用記憶體

Server 必須使用 `uname()`、`sysinfo()`、`sysconf()` 等 Linux API 直接取得資料，不得透過 shell command 收集資訊。回應不得包含使用者名稱、私人路徑、IP address、環境變數或其他不必要的機器識別資訊。

## Robustness Defaults

- 每條 connection 的 idle／incomplete-frame timeout 預設為 120 秒；超時時 server child 傳送 `ERROR`（若 socket 狀態仍允許）後關閉 connection。
- Timeout 使用完整 frame 的 deadline，不因只收到少量 bytes 而無限延長，以防止 slow client 長期占用 child process。
- Server 提供 `--idle-timeout SECONDS` 覆寫值；正式執行維持 120 秒，timeout 測試與截圖固定使用 `--idle-timeout 10`。
- Server 最多允許 32 個同時進行中的正常 client sessions。每個已接受的 connection 仍建立 child；若已達上限，該短期 child 只回覆 `ERROR: server busy` 後關閉，不建立遊戲狀態，也不計入 active sessions。
- Parent 必須追蹤 active session children，透過 `SIGCHLD`／`waitpid()` 回收並遞減計數；若 `fork()` 失敗，必須關閉該 accepted socket、記錄錯誤並繼續服務既有 clients。
- 對於 framing 正確但內容或狀態無效的 request，server child 每次回覆明確 `ERROR`，最多容許連續 5 次；收到合法 request 後計數歸零，第 5 次連續錯誤後關閉 connection。
- Bad magic、unsupported version、nonzero reserved 或 payload length 超過 4096 等危險 header 不進入五次容錯，直接安全關閉 connection；若協定狀態仍可信才嘗試傳送 `ERROR`。

## 測試模式

- 開發與至少 10-client concurrency 證明使用 `127.0.0.1:9734`，並以 `tcpdump` 擷取 loopback traffic。
- 完成本機測試後，兩台主機固定連接同一個私人 Wi-Fi／LAN，例如讓學校電腦連接使用者提供的個人熱點；不使用學校 Wi-Fi，也不跨 public Internet。
- Server 可改為綁定 `0.0.0.0:9734` 或指定 LAN address，另一台主機的 client 以 server 的實際 IPv4 address 連線；loopback 測試仍保留為正式必交證據。
- 跨主機測試使用 Wireshark 擷取實體網路介面流量，並只在測試期間允許 server firewall 接受 TCP 9734。

## 已確認玩法

- 每局答案固定為 4 位數。
- 四個數字不可重複，例如 `1234` 有效、`1123` 無效。
- 允許以 `0` 開頭，例如 `0123` 是有效的四位遊戲代碼。
- 答案與猜測以固定長度字串保存及傳輸，不轉換成整數，因此不會遺失開頭的 `0`。
- 每局最多進行 15 次有效猜測；猜中時立即勝利，15 次仍未猜中時該局失敗並結束。
- 非法輸入會收到明確錯誤訊息，但不消耗 15 次有效猜測額度。
- 第 15 次有效猜測仍未猜中時，server 會在失敗回應中公開該局正確答案。

## 成績範圍

- 第一版不提供跨玩家或跨 connections 的全域排行榜。
- Child process 僅保存目前 connection 期間的完成局數、勝利數、失敗數與最佳猜測次數。
- Connection 關閉、child process 結束後，該次連線的成績不持久保存。

## 作業要求檢核

- [x] Server 對每個 client connection 使用 `fork()` 建立 child process（不可使用 thread）
- [x] 至少同時處理 10 個 clients，並提供可重跑的執行期證明
- [x] 以 `tcpdump`（loopback）擷取並分析封包
- [x] 提供編譯期與執行期兩層 debug log 控制
- [x] Client 與 server 共用 utility functions 必須封裝成動態共用函式庫 `.so`
- [x] Client 可向 server 請求 system information
- [x] 設計並實作異常情況的強健性機制
- [x] 至少三組「未實作／已實作強健性機制」的執行差異範例
- [x] 使用 CMake 建置，並可透過 CTest 執行自動測試
- [x] 報告包含系統設計、關鍵原始碼、執行截圖與完整原始碼
- [ ] 製作約 2 分鐘的操作展示影片

## 目前狀態

核心程式已完成：`server` 對每條 accepted connection 使用 `fork()`，child 獨立維護遊戲及連線戰績；`client` 提供遊戲、system information、戰績與離開選單。共用的遊戲、協定、完整網路 I/O 與 log 工具皆由 `libgame_common.so` 提供。

目前共有六組 CTest，涵蓋遊戲規則、protocol header、deadline-aware I/O、完整 frame、server session state machine，以及三組 robustness 改善前後對照。另有 `tests/test_loopback_concurrency.sh` 可重現 10 個 child processes 時間重疊的證明，`tests/test_debug_logging.sh` 可重現執行期 log level 差異；Debug clean build、CTest、Valgrind、動態連結、10-client loopback 與 debug log 測試均已通過。

Loopback `tcpdump` 封包與逐段分析已保存於 `evidence/packet-capture/`；三組 robustness 改善前後對照已保存於 `docs/testing/2026-11-08-robustness-comparison.md`。正式報告與五張執行截圖已完成，報告內已附約 2 分鐘影片的逐段腳本；目前只剩實際錄製影片。
