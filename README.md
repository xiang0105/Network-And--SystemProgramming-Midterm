# 幾A幾B Client-Server Game

期中專題：以 C 實作「幾A幾B」client-server 小遊戲。

目前確定採用獨立單人模式：每個 client 各自連線並挑戰由 server 產生的答案；server 對每個 connection 使用獨立的 child process 保存該局狀態並處理猜測。

## 連線模型

- 每次 `accept()` 成功後都建立一個 child process；同一 IP 建立多條 TCP connections 時，也必須建立多個 child processes。
- 同一條 connection 可連續遊玩多局；每局結束後可開始新局或離開。
- Client 僅以該條 TCP connection 識別；來源 IP 與 port 只用於 log 與執行證據，不作為玩家身分或共用 child 的依據。

## 已確認玩法

- 每局答案固定為 4 位數。
- 四個數字不可重複，例如 `1234` 有效、`1123` 無效。
- 允許以 `0` 開頭，例如 `0123` 是有效的四位遊戲代碼。
- 答案與猜測以固定長度字串保存及傳輸，不轉換成整數，因此不會遺失開頭的 `0`。
- 每局最多進行 15 次有效猜測；猜中時立即勝利，15 次仍未猜中時該局失敗並結束。
- 非法輸入會收到明確錯誤訊息，但不消耗 15 次有效猜測額度。
- 第 15 次有效猜測仍未猜中時，server 會在失敗回應中公開該局正確答案。

## 作業要求檢核

- [ ] Server 對每個 client connection 使用 `fork()` 建立 child process（不可使用 thread）
- [ ] 至少同時處理 10 個 clients，並提供執行期證明
- [ ] 以 `tcpdump`（loopback）或 Wireshark（跨主機）擷取並分析封包
- [ ] 提供編譯期與執行期兩層 debug log 控制
- [ ] Client 與 server 共用 utility functions 必須封裝成動態共用函式庫 `.so`
- [ ] Client 可向 server 請求 system information
- [ ] 設計並實作異常情況的強健性機制
- [ ] 至少三組「未實作／已實作強健性機制」的執行差異範例
- [ ] 使用 Makefile 或 CMake 建置
- [ ] 報告包含系統設計、關鍵原始碼、執行截圖與完整原始碼
- [ ] 製作約 2 分鐘的操作展示影片

## 目前狀態

已完成課程教材與老師範例程式盤點，並確定主題、遊戲模式、基本數字規則及回合猜測上限。其餘玩法、協定與功能仍在需求訪談中。
