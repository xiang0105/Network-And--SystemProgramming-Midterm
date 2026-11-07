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
- 舊的 `Ubuntu-22.04` registration 已依使用者確認移除；遷移備份仍保留在 WSL 專案外作為 rollback。必須等 `Ubuntu-24.04` 完成第一次完整建置與測試且全部通過後，才可刪除備份。

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

## 連線模型

- 一般操作直接執行 `./server` 與 `./client`；預設使用 `127.0.0.1:9734`，並保留可選參數供跨主機或 port 衝突時覆寫。
- Port `9734` 是 server 的共同 listening endpoint，不是一條 connection；多個 clients 可同時連到同一 port，TCP 會以來源／目的 IP 與 port 的組合區分各 connection。
- 每次 `accept()` 成功後都建立一個 child process；同一 IP 建立多條 TCP connections 時，也必須建立多個 child processes。
- 同一條 connection 可連續遊玩多局；每局結束後可開始新局或離開。
- Client 僅以該條 TCP connection 識別；來源 IP 與 port 只用於 log 與執行證據，不作為玩家身分或共用 child 的依據。

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
- 訊息種類、payload 格式與最大 payload 長度仍待後續確認。

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

- [ ] Server 對每個 client connection 使用 `fork()` 建立 child process（不可使用 thread）
- [ ] 至少同時處理 10 個 clients，並提供執行期證明
- [ ] 以 `tcpdump`（loopback）或 Wireshark（跨主機）擷取並分析封包
- [ ] 提供編譯期與執行期兩層 debug log 控制
- [ ] Client 與 server 共用 utility functions 必須封裝成動態共用函式庫 `.so`
- [ ] Client 可向 server 請求 system information
- [ ] 設計並實作異常情況的強健性機制
- [ ] 至少三組「未實作／已實作強健性機制」的執行差異範例
- [ ] 使用 CMake 建置，並可透過 CTest 執行自動測試
- [ ] 報告包含系統設計、關鍵原始碼、執行截圖與完整原始碼
- [ ] 製作約 2 分鐘的操作展示影片

## 目前狀態

已完成課程教材與老師範例程式盤點，並確定主題、遊戲模式、基本數字規則及回合猜測上限。

第一個 CMake／CTest 程式切片已完成：`game_common` 動態共用函式庫目前提供四位遊戲代碼驗證與 A/B 計分，並已通過 Debug、Release、動態連結及 Valgrind 測試。`server`、`client`、完整通訊協定及其整合測試尚未實作，因此目前的 10 條 loopback smoke test 不作為正式 concurrency 證據，WSL 遷移備份也繼續保留。
