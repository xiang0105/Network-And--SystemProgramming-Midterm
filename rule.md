# Midterm Project Rules

本檔案是本專案的工作準則。進行任何設計、實作、測試、文件、簡報或展示工作前後，都必須檢查本檔案。

## 1. 必要作業要求

1. 使用 C 實作 client-server 應用程式。
2. Server 必須對每一個 client connection 呼叫 `fork()`，由 child process 處理該 client；禁止以 thread 取代。
3. Server 必須能同時處理至少 10 個 clients，並保存可重現的執行期證明。
4. 必須提供封包擷取與分析：loopback 通訊使用 `tcpdump`，跨電腦通訊可使用 Wireshark。
5. Debug log 必須同時具備：
   - 編譯期控制，例如 `-DDEBUG`／`#ifdef DEBUG`。
   - 執行期控制，例如命令列參數或環境變數設定 log level。
6. Client 與 server 共用的 utility functions 必須封裝成動態共用函式庫 `.so`，並由兩者動態連結。
7. Client 至少必須能向 server 請求 system information。
8. 必須設計並實作 abnormal／exceptional conditions 的強健性機制。
9. 必須提供至少三組「未使用強健性機制」與「使用強健性機制」的行為差異執行範例。
10. 專案必須使用 Makefile 或 CMake 建置。
11. 報告必須包含系統設計、關鍵原始碼說明、執行截圖與完整原始碼。
12. 必須準備約 2 分鐘的操作展示影片。

## 2. 實作與安全準則

1. 所有重要 system calls 與 library calls 都必須檢查回傳值並提供可理解的錯誤訊息。
2. 網路協定必須有明確的 message framing、版本、命令種類、payload 長度與最大訊息大小；不可假設一次 `read()`／`recv()` 就會收到完整訊息。
3. 共用函式庫應至少封裝協定收送、完整讀寫、輸入驗證與 log 等 client/server 共用功能。
4. 必須處理 partial read/write、`EINTR`、client 提前中斷、idle/slow client、惡意或錯誤長度、未知命令與資源耗盡。
5. Server 必須正確關閉 parent/child 不需要的 socket descriptor，並使用可靠的 `SIGCHLD`／`waitpid()` 機制避免 zombie process。
6. 必須避免因 client 中斷造成 `SIGPIPE` 終止 server，並設計合理 timeout 與連線／child process 上限。
7. Server 預設只提供完成主題所需的最小權限；除非設計經過明確確認，不提供任意 shell command、任意檔案寫入或未限制的路徑存取。
8. 不得把密碼、token、私密金鑰、個人資料或機器敏感資訊寫入原始碼、log、封包檔、截圖或 Git history。

## 3. 驗證與證據準則

1. 每項功能都必須有可重現的建置與執行方式。
2. 至少 10-client concurrency 測試必須能證明多個 child processes 在時間上重疊，而不只是快速依序完成。
3. 封包分析至少應辨認 TCP connection establishment、application request/response framing 與 connection close；保存使用的 filter、指令與 `.pcap`／截圖。
4. 三組 robustness 前後對照必須使用相同輸入條件，清楚記錄未改善版本的失敗行為與改善版本的處理結果。
5. 每次重要修改後至少執行建置、相關自動測試與必要的手動整合測試；不得把「能編譯」視為全部驗證完成。
6. 最終報告、原始碼、截圖、封包分析與影片內容必須彼此一致。

## 4. Git 與檔案管理準則

1. `main` 必須保持可建置；每個邏輯里程碑建立清楚、單一目的的 commit。
2. 不提交 build output、暫存檔、core dump、一般執行 log 或任何秘密資訊。
3. 應提交可重現所需的原始碼、header、Makefile/CMake、測試腳本、文件與經整理的報告證據。
4. 修改前先檢查 working tree；不得覆寫或移除不屬於目前工作的既有變更。

## 5. 每次工作的強制檢查流程

1. **開始前**：閱讀本檔案，確認本次工作涉及哪些規則。
2. **進行中**：若需求或實作與本檔案衝突，立即停止衝突部分並向使用者說明，不可自行忽略規則。
3. **完成前**：逐項檢視本次變更是否違反本檔案、老師要求、安全準則、驗證準則或 Git 準則。
4. **驗證後**：只有在所有適用規則都通過，或已明確列出尚未解決的衝突／限制時，才可宣告工作完成。
5. **回覆時**：簡要回報規則檢查結果；若有衝突，必須列出衝突規則、影響與需要的決策。

## 6. 目前決策狀態

1. 應用主題已確定為「幾A幾B」client-server 小遊戲。
2. 第一階段採用獨立單人模式：每個 client 各自挑戰由 server 產生的答案，不實作玩家配對或玩家對戰。
3. 每次 `accept()` 成功取得一條 TCP connection 後，都必須呼叫一次 `fork()`，由一個 child process 獨立維護該 connection 的答案、猜測紀錄與遊戲狀態；不同 connections 的遊戲狀態不得互相影響。
4. 不得以來源 IP 合併 connections 或重用同一個 child process；同一 IP（包含 `127.0.0.1`）建立多條 connections 時，必須建立對應數量的 child processes。來源 IP 與來源 port 僅用於 log 與執行證據。
5. 同一條 TCP connection 可以連續遊玩多局；一局勝利或失敗後，玩家可選擇產生新答案並重設回合狀態，或離開並關閉 connection，使 child process 正常結束。
6. System information 是同一個 server 必須提供的附加命令，不是遊戲的唯一功能。
7. 每局答案固定為 4 位數；client 的猜測也必須恰好包含 4 位數字才可進入 A/B 判定。
8. 答案與猜測中的四個數字都不可重複；重複數字屬於無效輸入，不得進入 A/B 判定。
9. 允許以 `0` 開頭；`0123` 必須被視為有效的四位遊戲代碼，而不是三位整數。
10. 答案與猜測在通訊協定中以恰好 4 bytes 的數字字元序列表示，在 C 程式內以可容納終止字元的 `char[5]` 表示；不得使用 `atoi()`、`strtol()` 或其他整數轉換保存遊戲代碼。
11. Client 與 server 都必須驗證輸入長度恰好為 4、每個字元介於 `0` 至 `9` 且四個字元互不重複；server 的驗證結果為最終依據。
12. 每局最多接受 15 次有效猜測；得到 `4A0B` 時立即勝利，完成第 15 次有效猜測仍未猜中時該局失敗並結束。
13. 無效輸入不得消耗 15 次有效猜測額度；server 必須回覆明確錯誤原因，且不得進行 A/B 判定。
14. 第 15 次有效猜測仍未猜中時，server 必須在失敗回應中公開該局正確答案，然後將該局標記為結束。
15. 第一版不實作跨玩家或跨 connections 的全域排行榜，也不使用共享記憶體或共用檔案保存成績。
16. 每個 child process 僅在記憶體中保存該 connection 期間的完成局數、勝利數、失敗數與最佳猜測次數；connection 關閉後不持久保存。
17. 一般操作必須能直接執行 `./server` 與 `./client`；預設 server endpoint 為 `127.0.0.1:9734`，並保留可選的 host／port 參數供跨主機或 port 衝突時覆寫。
18. 單一 server listening port 必須支援多條同時存在的 TCP connections；不得把 port 誤當成單一 connection。每條 connection 由來源 IP、來源 port、目的 IP 與目的 port 的組合區分。
19. 開發與至少 10-client concurrency 證明使用 `127.0.0.1:9734`；完成 loopback 測試後，必須能讓 server 綁定 `0.0.0.0:9734` 或指定 LAN address，讓另一台主機的 client 以 server 實際 IPv4 address 連線。
20. Loopback 封包證據使用 `tcpdump`；跨主機證據使用 Wireshark 擷取實體網路介面。若需調整 firewall，只允許測試所需的 TCP 9734，並避免不必要的對外網路暴露。
21. 跨主機測試固定使用同一個私人 Wi-Fi／LAN，例如讓學校電腦連接使用者提供的個人熱點；不使用學校 Wi-Fi，也不以 public Internet、router port forwarding 或放寬 firewall 作為測試方案。
22. Loopback 測試及其證據必須可獨立完成老師要求；跨主機測試屬額外驗證，不得成為專案完成的單一失敗點。
23. 正式基準開發環境為本機 WSL2：預設 distribution 名稱 `Ubuntu-24.04`，實際系統 Ubuntu 24.04.5 LTS、x86_64，專案路徑 `/mnt/d/Data/Project/Network-And--SystemProgramming-midterm`。建置、執行、測試與證據指令必須先在此環境驗證。
24. 本機 WSL 目前使用 NAT；loopback 開發先沿用 NAT，跨主機測試前再採用 mirrored networking 或等效且最小開放範圍的安全轉送方式。不得讓 WSL 網路設定阻擋 loopback 必交證據。
25. 已驗證 GCC、Make、CMake、Git、GDB、`ldd`、`nc`、tcpdump、valgrind 與 socat 可用。
26. 舊的 `Ubuntu-22.04` registration 已依使用者明確同意移除；遷移備份必須保留到 `Ubuntu-24.04` 完成第一次完整建置、CTest、自動測試與必要整合測試且全部通過。只有保存測試結果後，才可依使用者指示刪除備份。
27. 正式建置系統採用 CMake，不另外維護手寫 Makefile。必須採用 out-of-source build；targets 至少包含 `game_common` 動態共用函式庫、`server`、`client` 與自動測試，並支援 Debug／Release 組態及 CTest。
28. 每個 application message 採用固定 12-byte header：bytes 0–3 為 ASCII magic `ABGM`、byte 4 為 protocol version、byte 5 為 message type、bytes 6–7 為 reserved、bytes 8–11 為 payload length。第一版 version 固定為 `1`、reserved 傳送時固定為 `0`，payload length 使用 network byte order。
29. Protocol header 必須逐欄位編碼與解碼，不得直接傳送 C `struct` 的記憶體內容。接收端在配置或讀取 payload 前，必須驗證 magic、version、reserved 與 payload length 上限，並以完整讀寫函式處理 partial read／write 及 `EINTR`。
30. 第二台主機的 Windows／WSL 版本、連續無效請求的防濫用上限、connection／child 上限及額外功能仍在需求訪談中；未確認的項目不得自行視為已定案。
31. Client 連線成功後的主選單固定包含：開始新遊戲、查看 server system information、查看本次 connection 戰績、離開；每局結束後回到主選單。戰績選項只讀取規則 16 已定義的 child process 記憶體資料，不得擴張為帳號、持久化資料或跨 connection 排行榜。
32. Server system information 固定包含 Linux／kernel 版本、CPU 架構與邏輯核心數、系統運行時間、總記憶體與可用記憶體。必須使用 `uname()`、`sysinfo()`、`sysconf()` 等 Linux API 直接取得，不得執行 shell command。
33. System information response 不得包含使用者名稱、私人路徑、IP address、環境變數或其他非必要機器識別資訊；測試證據、封包與截圖也必須遵守相同限制。
34. 每個 message payload 的硬性上限為 4096 bytes。接收端必須在讀取 payload 或依長度配置記憶體之前驗證此上限；超過上限的 frame 必須拒絕並安全結束或依後續錯誤策略處理。
35. Guess payload 固定為恰好 4 bytes 的數字字元且不包含 `\0`。System information、connection 戰績與錯誤訊息 payload 使用 UTF-8 文字；header 的 payload length 不包含接收端為 C 字串額外加入的終止字元。
36. Message type 固定如下：Client requests 為 `START_GAME=0x01`、`GUESS=0x02`、`GET_SYSTEM_INFO=0x03`、`GET_STATS=0x04`、`QUIT=0x05`；Server responses 為 `WELCOME=0x81`、`GAME_STARTED=0x82`、`GUESS_RESULT=0x83`、`GAME_WON=0x84`、`GAME_LOST=0x85`、`SYSTEM_INFO=0x86`、`STATS=0x87`、`ERROR=0xFF`。任一端收到方向不符或未定義的 type 時，必須視為 protocol error。
37. 每條 connection 的 idle／incomplete-frame timeout 預設為 120 秒，並提供 server 參數 `--idle-timeout SECONDS` 覆寫。Timeout 必須以完整 frame 的 absolute deadline 實作，不可因只收到少量 bytes 而無限延長；正式執行使用 120 秒，測試與截圖固定使用 10 秒。超時後 child 應在 socket 狀態允許時回覆 `ERROR`，再關閉 connection 並正常結束。
