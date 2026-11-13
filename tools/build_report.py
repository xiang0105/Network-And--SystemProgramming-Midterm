import os
from pathlib import Path

from docx import Document
from docx.enum.section import WD_SECTION
from docx.enum.table import WD_CELL_VERTICAL_ALIGNMENT, WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Inches, Pt, RGBColor


ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT / "report"
LANGUAGE = os.environ.get("REPORT_LANGUAGE", "zh").lower()
if LANGUAGE not in {"zh", "en"}:
    raise ValueError("REPORT_LANGUAGE must be 'zh' or 'en'")
OUTPUT_PATH = OUTPUT_DIR / (
    "Network_Programming_Midterm_Report_EN.docx"
    if LANGUAGE == "en"
    else "Network_Programming_Midterm_Report.docx"
)
SCREENSHOT_DIR = ROOT / "evidence" / "screenshots"
SOURCE_FILES = [
    Path("CMakeLists.txt"),
    Path("include/game.h"),
    Path("include/log.h"),
    Path("include/network_io.h"),
    Path("include/protocol.h"),
    Path("include/server_session.h"),
    Path("src/client/main.c"),
    Path("src/common/game.c"),
    Path("src/common/log.c"),
    Path("src/common/network_io.c"),
    Path("src/common/protocol.c"),
    Path("src/server/main.c"),
    Path("src/server/session.c"),
]

BODY_FONT = "Arial" if LANGUAGE == "en" else "Microsoft JhengHei"
CODE_FONT = "Consolas"
NAVY = "17365D"
PALE_BLUE = "EAF2F8"
PALE_GRAY = "F3F4F6"
LIGHT_BORDER = "D9D9D9"


TRANSLATIONS = {
    "C Client Server 幾A幾B遊戲期中報告": "C Client Server Bulls and Cows Midterm Report",
    "網路系統程式開發與安全": "Network and System Programming Development and Security",
    "實作語言  C17": "Implementation Language  C17",
    "建置系統  CMake": "Build System  CMake",
    "執行環境  Ubuntu 24.04 on WSL2": "Runtime Environment  Ubuntu 24.04 on WSL2",
    "測試日期  2026 年 11 月 8 日": "Test Date  November 8 2026",
    (
        "本專案完成以 fork 處理每條 TCP connection 的單人幾A幾B遊戲，"
        "並以自動測試、10-client 執行證據、tcpdump 封包及三組異常對照驗證設計。"
    ): (
        "This project implements a single-player Bulls and Cows game in which the server "
        "uses fork to handle each TCP connection. Automated tests, a 10-client runtime "
        "demonstration, a tcpdump capture, and three abnormal-condition comparisons verify "
        "the design."
    ),
    "一 專案成果": "1 Project Results",
    (
        "本專案以 C 實作 client-server 幾A幾B遊戲。Server 在每次 accept 成功後立即 fork，"
        "每個 child process 獨立保存該 connection 的答案、遊戲回合及暫時戰績。Client 提供開始遊戲、"
        "查詢 server system information、查詢 connection 戰績與離開四個選項。正式程式不使用 thread、"
        "shell command、帳號系統或跨 connection 共用狀態。"
    ): (
        "This project implements a client-server Bulls and Cows game in C. The server calls "
        "fork immediately after each successful accept, and every child process stores the "
        "answer, game rounds, and temporary statistics for its own connection. The client menu "
        "provides four actions: start a game, request server system information, view "
        "connection-scoped statistics, and quit. The production program does not use threads, "
        "shell commands, accounts, or state shared across connections."
    ),
    (
        "功能與證據均已完成。Debug 與 Release 組態的六組 CTest 全數通過；10 條同時存在的 loopback "
        "connections 產生 10 個 child processes；server 與 client 都由 ldd 證實載入 libgame_common.so；"
        "Valgrind 未發現記憶體錯誤；tcpdump pcap 保存完整 TCP 交握、應用 frame 及關閉流程。"
    ): (
        "All planned functions and evidence are complete. All six CTest groups pass in both "
        "Debug and Release builds. Ten simultaneous loopback connections create ten child "
        "processes. ldd confirms that both the server and client load libgame_common.so. "
        "Valgrind reports no memory errors, and the tcpdump capture preserves the complete TCP "
        "handshake, application frames, and connection shutdown."
    ),
    "二 作業需求對照": "2 Requirements Compliance",
    "完成": "Complete",
    "每次 accept 後 fork；child 獨立執行 session": "Fork after every accept; each child runs an independent session",
    "至少 10 clients 併行": "At least 10 concurrent clients",
    "10 個 child PID 同時存在且 PPID 相同": "Ten child PIDs exist simultaneously with the same PPID",
    "封包擷取與分析": "Packet capture and analysis",
    "loopback tcpdump pcap 共 22 packets": "Loopback tcpdump capture containing 22 packets",
    "兩層 debug log": "Two-level debug logging",
    "CMake ENABLE_DEBUG_LOG 與 --log-level": "CMake ENABLE_DEBUG_LOG and --log-level",
    "動態共用函式庫": "Dynamic shared library",
    "server/client 共同載入 libgame_common.so": "Both server and client load libgame_common.so",
    "uname sysinfo sysconf 直接取得": "Collected directly through uname, sysinfo, and sysconf",
    "異常處理": "Abnormal-condition handling",
    "deadline 長度上限 SIGPIPE 錯誤次數 session 上限": "Deadline, length limit, SIGPIPE protection, error count, and session limit",
    "三組改善前後對照": "Three before-and-after comparisons",
    "CMake 與 CTest": "CMake and CTest",
    "out of source build 與六組測試": "Out-of-source build and six test groups",
    "報告與影片規劃": "Report and video plan",
    "本報告及第十節 2 分鐘腳本": "This report and the two-minute script in Section 10",
    "要求": "Requirement",
    "狀態": "Status",
    "證據": "Evidence",
    "三 系統設計": "3 System Design",
    "系統架構": "System Architecture",
    (
        "Parent server 只負責監聽、接受 connection、建立 child 與回收 child。每個 child 封閉 parent 的 "
        "listening socket 後，執行同一條 connection 的協定與遊戲狀態機。這個模型讓同一 IP 可以同時建立多條 "
        "connections，也讓某一個 client 的狀態與錯誤不會影響其他玩家。"
    ): (
        "The parent server only listens, accepts connections, creates children, and reaps "
        "terminated children. Each child closes its copy of the parent's listening socket and "
        "runs the protocol and game state machine for one connection. This model allows one IP "
        "address to create several simultaneous connections while isolating each client's state "
        "and errors from every other player."
    ),
    "多個 Client": "Multiple Clients",
    "每條 connection 一個 Child": "One Child per Connection",
    "選單與輸入驗證": "Menu and Input Validation",
    "accept 與 waitpid": "accept and waitpid",
    "獨立 PID": "Separate PID",
    "遊戲 狀態 戰績 System Info": "Game State Statistics System Info",
    "來源": "Source",
    "傳輸": "Transport",
    "父程序": "Parent Process",
    "建立": "Creation",
    "處理者": "Handler",
    "遊戲規則與狀態": "Game Rules and State",
    "答案與猜測固定為四個數字字元，允許 0 開頭，四個數字不可重複。": "Answers and guesses contain exactly four digit characters. A leading zero is allowed, and digits may not repeat.",
    "每局最多接受 15 次有效猜測；無效猜測回覆錯誤但不扣次數。": "Each game accepts at most 15 valid guesses. Invalid guesses return an error without consuming an attempt.",
    "4A0B 立即勝利；第 15 次仍未猜中時回覆 GAME_LOST 並公開答案。": "A 4A0B result wins immediately. If the fifteenth valid guess is incorrect, the server returns GAME_LOST and reveals the answer.",
    "同一 connection 可連玩多局，child 暫存完成局數、勝敗與最佳猜測次數。": "One connection may play several games. Its child temporarily stores completed games, wins, losses, and the best attempt count.",
    "Connection 關閉後 child 結束，資料不寫入檔案或共享記憶體。": "When the connection closes, the child exits. It does not write session data to a file or shared memory.",
    "Connection 生命週期": "Connection Lifecycle",
    "階段": "Stage",
    "記錄 IP port 後 fork": "Record IP and port, then fork",
    "關閉 listener": "Close listener",
    "歡迎": "Welcome",
    "記錄 child PID": "Record child PID",
    "送 WELCOME": "Send WELCOME",
    "顯示主選單": "Display main menu",
    "操作": "Operation",
    "繼續 accept": "Continue accepting",
    "處理 frame 狀態與活動 log": "Process frame, state, and activity log",
    "遊戲或查詢": "Game or query",
    "結束": "End",
    "SIGCHLD 後 waitpid": "waitpid after SIGCHLD",
    "關閉 connected socket": "Close connected socket",
    "QUIT 或 close": "QUIT or close",
    "四 通訊協定": "4 Communication Protocol",
    "固定長度 Header": "Fixed-Length Header",
    (
        "每個 application message 由 12-byte header 與 payload 組成。程式逐欄位編碼，不直接傳送 C struct，"
        "因此不受 padding、alignment 或主機位元序影響。Payload length 使用 network byte order，接收端會在讀取 "
        "payload 前拒絕超過 4096 bytes 的長度。"
    ): (
        "Each application message consists of a 12-byte header followed by a payload. The "
        "program encodes every field explicitly instead of transmitting a C struct, so padding, "
        "alignment, and host byte order do not affect the wire format. Payload length uses "
        "network byte order, and the receiver rejects lengths above 4096 bytes before reading "
        "the payload."
    ),
    "長度": "Length",
    "欄位": "Field",
    "規則": "Rule",
    "固定 1": "Fixed at 1",
    "Request 或 Response type": "Request or response type",
    "固定 0": "Fixed at 0",
    "Network byte order 最大 4096": "Network byte order, maximum 4096",
    "值": "Value",
    "方向": "Direction",
    "名稱": "Name",
    "完整讀寫與 Deadline": "Complete Reads and Writes with an Absolute Deadline",
    (
        "TCP 只提供 byte stream，不保留 application message boundary。network_read_exact 與 network_write_all "
        "使用 poll 搭配同一個 CLOCK_MONOTONIC absolute deadline，反覆處理 partial I/O、EINTR、EAGAIN 與 EOF。"
        "新的少量 bytes 不會重設 deadline，因此 slow client 無法無限延長 child 的生命。"
    ): (
        "TCP provides a byte stream and does not preserve application message boundaries. "
        "network_read_exact and network_write_all use poll with one CLOCK_MONOTONIC absolute "
        "deadline while repeatedly handling partial I/O, EINTR, EAGAIN, and EOF. A small amount "
        "of newly received data does not reset the deadline, so a slow client cannot keep a "
        "child alive indefinitely."
    ),
    "五 關鍵原始碼": "5 Key Source Code",
    "Fork 每條 Connection": "Fork for Every Connection",
    "Parent 接受一條 connection 後先以數字格式保存來源 IP 與 port，再決定是否進入正常 session，接著不論是否已達上限都建立 child。正常 child 執行 session；超額 child 只送出 server busy 後結束。": "After accepting a connection, the parent records the numeric source IP address and port, decides whether to admit a normal session, and creates a child even when the normal-session limit has been reached. An admitted child runs the session. An excess child only sends server busy and exits.",
    "四位數驗證": "Four-Digit Validation",
    "遊戲代碼始終以 char 陣列處理，不使用 atoi 或整數轉換，因此 0123 不會變成 123。Client 與 server 都驗證，server 的判定為最終結果。": "The program always stores game codes in char arrays and never uses atoi or another integer conversion, so 0123 does not become 123. Both the client and server validate guesses, and the server's decision is authoritative.",
    "兩層 Log 控制": "Two-Level Log Control",
    "CMake option ENABLE_DEBUG_LOG 決定 LOG_DEBUG 是否進入 binary；編譯啟用後，server 仍可透過 --log-level 在執行期選擇 error、info 或 debug。預設 info 會記錄 client IP、來源 port、child PID、遊戲與查詢操作及離線結果；答案不會提前寫入 log。每筆紀錄先完整組合再單次寫出，避免 parent 與多個 child 同時輸出時互相黏行。": "The CMake option ENABLE_DEBUG_LOG determines whether LOG_DEBUG is included in the binary. When compiled in, --log-level still selects error, info, or debug at runtime. The default info level records the client IP address, source port, child PID, game and query actions, and the disconnect result. It never logs the secret answer before the game ends. Each log record is assembled in memory and written once to prevent output from the parent and several children from interleaving.",
    "Child 使用 uname 取得 Linux 與 kernel，sysconf 取得邏輯核心數，sysinfo 取得 uptime 與記憶體。程式不執行 shell command，也不回傳使用者名稱、私人路徑、IP address 或環境變數。": "The child uses uname for Linux and kernel information, sysconf for the logical CPU count, and sysinfo for uptime and memory. The program does not execute shell commands or return usernames, private paths, IP addresses, or environment variables.",
    "六 強健性設計": "6 Robustness Design",
    "單次 recv 只得 4 of 12 bytes": "One recv returns only 4 of 12 bytes",
    "完整讀取迴圈重組 frame": "Complete-read loop reassembles the frame",
    "無 deadline 的迴圈永久阻塞": "A loop without a deadline blocks forever",
    "absolute deadline 回傳 timeout": "Absolute deadline returns timeout",
    "send 觸發 SIGPIPE 終止": "send triggers SIGPIPE termination",
    "MSG_NOSIGNAL 回傳可處理錯誤": "MSG_NOSIGNAL returns a manageable error",
    "相同輸入": "Same Input",
    "未改善行為": "Behavior Without Protection",
    "改善後行為": "Behavior With Protection",
    "圖 1 三組異常條件的改善前後執行結果": "Figure 1 Results Before and After Robustness Improvements",
    "其他異常處理": "Other Abnormal-Condition Handling",
    "Bad magic、unsupported version、nonzero reserved 與超長 payload 在讀取 payload 前拒絕。": "The receiver rejects bad magic, unsupported versions, nonzero reserved fields, and oversized payloads before reading the payload.",
    "Framing 正確但狀態或內容錯誤時回覆 ERROR；連續五次錯誤後關閉 connection。": "For a correctly framed request with invalid state or content, the server returns ERROR and closes the connection after five consecutive errors.",
    "每條 connection 預設 idle timeout 120 秒；測試使用 10 秒。": "Each connection has a default idle timeout of 120 seconds; tests use 10 seconds.",
    "正常 sessions 上限 32；超額 connection 仍 fork 短期 child 並回覆 server busy。": "The server allows at most 32 normal sessions. An excess connection still receives a short-lived child that returns server busy.",
    "SIGCHLD handler 通知 parent 使用 waitpid WNOHANG 回收 children，避免 zombie。": "The SIGCHLD handler notifies the parent to reap children with waitpid and WNOHANG, preventing zombies.",
    "fork 失敗時關閉 accepted socket，保留 listener 並繼續服務既有 clients。": "If fork fails, the parent closes the accepted socket, keeps the listener open, and continues serving existing clients.",
    "七 執行驗證": "7 Runtime Verification",
    "六組測試涵蓋遊戲規則、header codec、deadline I O、完整 frame、server session state machine 與三組 robustness 對照。Debug 與 Release 均為 6 of 6 PASS。": "Six test groups cover game rules, the header codec, deadline-based I/O, complete frames, the server session state machine, and three robustness comparisons. Both Debug and Release builds pass all 6 of 6 tests.",
    "圖 2 六組 CTest 全數通過": "Figure 2 All Six CTest Groups Passed",
    "十個 Clients 併行": "Ten Concurrent Clients",
    "測試同時保持 10 條 loopback connections 五秒，再以 pgrep 與 ps 讀取 server 的直接 children。畫面顯示 10 個不同 PID 具有相同 PPID，證明 sessions 在時間上重疊。": "The test keeps ten loopback connections open simultaneously for five seconds, then uses pgrep and ps to list the server's direct children. The output shows ten different PIDs with one common PPID, proving that the sessions overlap in time.",
    "圖 3 同一 parent server 下同時存在 10 個 child processes": "Figure 3 Ten Child Processes Running Under One Parent Server",
    "Client 與 System Information": "Client and System Information",
    "Client 成功接收 WELCOME，接著使用主選單要求 system information 與 connection stats。系統資訊只包含規定欄位，初始戰績為零。": "The client receives WELCOME and then uses the main menu to request system information and connection statistics. The system information contains only the required fields, and the initial statistics are zero.",
    "圖 4 Client 選單 System Information 與 Connection Stats": "Figure 4 Client Menu System Information and Connection Statistics",
    "動態連結與記憶體檢查": "Dynamic Linking and Memory Checks",
    "八 封包擷取與分析": "8 Packet Capture and Analysis",
    (
        "tcpdump 只擷取 loopback TCP 9734，共保存 22 packets。SHA 256 為 "
        "f7f5d5cda30088e85e50de741b8d30f35986044540d7d087ec110f84b087e307。"
        "封包包含 TCP three way handshake、WELCOME、GET_SYSTEM_INFO、SYSTEM_INFO、GET_STATS、STATS、QUIT 與雙方 FIN。"
    ): (
        "tcpdump captured only loopback TCP port 9734 and stored 22 packets. The SHA-256 digest "
        "is f7f5d5cda30088e85e50de741b8d30f35986044540d7d087ec110f84b087e307. "
        "The capture contains the TCP three-way handshake, WELCOME, GET_SYSTEM_INFO, "
        "SYSTEM_INFO, GET_STATS, STATS, QUIT, and FIN packets from both endpoints."
    ),
    "圖 5 tcpdump 十六進位資料中的 ABGM application header": "Figure 5 ABGM Application Header in Tcpdump Hexadecimal Output",
    "解碼": "Decoded Meaning",
    (
        "WELCOME、SYSTEM_INFO 與 STATS 的 12-byte header 和文字 payload 出現在不同 TCP segments。"
        "這項執行證據證明 TCP 不保留訊息邊界，也直接支持本專案使用 frame length 與完整讀寫迴圈的必要性。"
    ): (
        "The 12-byte headers and text payloads for WELCOME, SYSTEM_INFO, and STATS appear in "
        "different TCP segments. This runtime evidence shows that TCP does not preserve message "
        "boundaries and directly supports the need for frame lengths and complete I/O loops."
    ),
    "九 建置與執行": "9 Build and Execution",
    "建置與測試": "Build and Test",
    "啟動 Server 與 Client": "Start the Server and Client",
    "預設 endpoint 為 127.0.0.1:9734。跨主機測試時，server 可用 --host 0.0.0.0 --port 9734，client 則以 server 的私人 LAN IPv4 address 連線。": "The default endpoint is 127.0.0.1:9734. For a two-host test, the server can use --host 0.0.0.0 --port 9734, and the client connects to the server's private LAN IPv4 address.",
    "常用參數": "Common Options",
    "十 兩分鐘展示影片腳本": "10 Two-Minute Demonstration Script",
    "介紹架構": "Introduce Architecture",
    "說明 parent accept 後 fork，每條 connection 由獨立 child 處理": "Explain that the parent forks after accept and one child handles each connection",
    "建置與測試": "Build and Test",
    "執行 CMake 與 CTest，顯示 6 of 6 PASS": "Run CMake and CTest and show 6 of 6 PASS",
    "Client 功能": "Client Functions",
    "啟動 server/client，展示 system info、開始遊戲與 stats": "Start the server and client; show system info, start a game, and view stats",
    "執行 concurrency script，指出 10 個 child PID 與共同 PPID": "Run the concurrency script and identify ten child PIDs with one common PPID",
    "異常處理": "Abnormal Conditions",
    "執行 robustness demo，說明 partial read、timeout、SIGPIPE": "Run the robustness demo and explain partial reads, timeout, and SIGPIPE",
    "封包與結尾": "Packet Capture and Closing",
    "開啟 tcpdump 分析，指出 ABGM header 與 FIN close": "Open the tcpdump analysis and identify the ABGM header and FIN close",
    "時間": "Time",
    "畫面": "Screen",
    "旁白重點": "Narration",
    "錄影時正式 server 使用預設 120 秒 timeout；只有 timeout 示範與自動測試使用 10 秒。畫面應避免顯示密碼、使用者名稱、私人路徑或其他不相關視窗。": "During recording, the production server uses the default 120-second timeout. Only the timeout demonstration and automated tests use 10 seconds. The recording must not show passwords, usernames, private paths, or unrelated windows.",
    "十一 完整原始碼交付": "11 Complete Source Delivery",
    (
        "完整原始碼與測試隨本報告一同包含在專案目錄。下表列出所有交付程式檔；build output、執行 log、"
        "暫存檔與秘密資料均未納入 Git。Git 里程碑包含核心功能、protocol、server/client、封包證據與 robustness 證據。"
    ): (
        "The project directory includes the complete source code and tests with this report. "
        "The table lists all delivered program files. Build output, runtime logs, temporary "
        "files, and secrets are excluded from Git. Git milestones cover the core functions, "
        "protocol, server and client, packet evidence, and robustness evidence."
    ),
    "遊戲 I O protocol log 與 session headers": "Game, I/O, protocol, log, and session headers",
    "game log network_io protocol 實作": "Game, log, network_io, and protocol implementations",
    "Parent fork server 與 child session state machine": "Parent fork server and child session state machine",
    "Terminal menu 與遊戲 client": "Terminal menu and game client",
    "六組 CTest 整合 scripts 與三組隔離對照": "Six CTest groups, integration scripts, and three isolated comparisons",
    "pcap 分析與執行截圖": "Pcap analysis and runtime screenshots",
    "路徑": "Path",
    "內容": "Contents",
    "十二 結論": "12 Conclusion",
    (
        "本專案完成符合課程要求的 C client-server 應用。Server 對每條 connection 使用 fork 並可同時處理至少 "
        "10 個 clients；client 可玩幾A幾B並查詢 system information；共用 utility 由動態函式庫提供。"
        "Protocol framing、absolute deadline、payload 上限、SIGPIPE 防護、child 回收與 session 上限處理主要異常條件。"
        "自動測試、執行畫面與 pcap 彼此對應，可重現本報告中的結果。"
    ): (
        "The project delivers a C client-server application that satisfies the course "
        "requirements. The server forks for every connection and handles at least ten clients "
        "concurrently. The client plays Bulls and Cows and requests system information, while a "
        "dynamic library provides shared utilities. Protocol framing, an absolute deadline, the "
        "payload limit, SIGPIPE protection, child reaping, and the session limit address the main "
        "abnormal conditions. Automated tests, runtime screenshots, and the pcap capture provide "
        "matching evidence that reproduces the reported results."
    ),
    "附錄 A 完整程式原始碼": "Appendix A Complete Program Source Code",
    (
        "本附錄逐行收錄可建置 server、client 與 libgame_common.so 的全部 CMake、header 與 C 原始碼。"
        "自動測試、執行腳本、封包與原始截圖則以原始檔形式隨專案一併提交，以維持可直接執行與重現。"
    ): (
        "This appendix contains the complete CMake, header, and C source code needed to build "
        "the server, client, and libgame_common.so. Automated tests, execution scripts, packet "
        "captures, and original screenshots are submitted as project files so the results can "
        "be executed and reproduced directly."
    ),
}


def tr(text):
    if LANGUAGE != "en":
        return text
    return TRANSLATIONS.get(text, text)


def tr_code(code):
    if LANGUAGE != "en":
        return code
    return code.replace("# 另一個終端", "# Another terminal")


def set_run_font(run, name=BODY_FONT, size=11, bold=None, color=None):
    run.font.name = name
    run._element.get_or_add_rPr().rFonts.set(qn("w:eastAsia"), name)
    run._element.get_or_add_rPr().rFonts.set(qn("w:ascii"), name)
    run._element.get_or_add_rPr().rFonts.set(qn("w:hAnsi"), name)
    run.font.size = Pt(size)
    if bold is not None:
        run.bold = bold
    if color is not None:
        run.font.color.rgb = RGBColor.from_string(color)


def set_cell_shading(cell, fill):
    properties = cell._tc.get_or_add_tcPr()
    shading = properties.find(qn("w:shd"))
    if shading is None:
        shading = OxmlElement("w:shd")
        properties.append(shading)
    shading.set(qn("w:fill"), fill)


def set_cell_margins(cell, top=100, start=120, bottom=100, end=120):
    properties = cell._tc.get_or_add_tcPr()
    margins = properties.first_child_found_in("w:tcMar")
    if margins is None:
        margins = OxmlElement("w:tcMar")
        properties.append(margins)
    for side, value in (("top", top), ("start", start), ("bottom", bottom), ("end", end)):
        node = margins.find(qn(f"w:{side}"))
        if node is None:
            node = OxmlElement(f"w:{side}")
            margins.append(node)
        node.set(qn("w:w"), str(value))
        node.set(qn("w:type"), "dxa")


def set_table_borders(table):
    properties = table._tbl.tblPr
    borders = properties.first_child_found_in("w:tblBorders")
    if borders is None:
        borders = OxmlElement("w:tblBorders")
        properties.append(borders)
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        tag = borders.find(qn(f"w:{edge}"))
        if tag is None:
            tag = OxmlElement(f"w:{edge}")
            borders.append(tag)
        tag.set(qn("w:val"), "single")
        tag.set(qn("w:sz"), "4")
        tag.set(qn("w:color"), LIGHT_BORDER)


def set_repeat_table_header(row):
    properties = row._tr.get_or_add_trPr()
    repeat = OxmlElement("w:tblHeader")
    repeat.set(qn("w:val"), "true")
    properties.append(repeat)


def set_cell_text(cell, text, *, bold=False, color="000000", size=9.5, align=None):
    cell.text = ""
    paragraph = cell.paragraphs[0]
    if align is not None:
        paragraph.alignment = align
    paragraph.paragraph_format.space_after = Pt(0)
    paragraph.paragraph_format.line_spacing = 1.12
    run = paragraph.add_run(tr(str(text)))
    set_run_font(run, size=size, bold=bold, color=color)
    cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
    set_cell_margins(cell)


def add_table(document, headers, rows, widths=None, font_size=9.5):
    table = document.add_table(rows=1, cols=len(headers))
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = False
    set_table_borders(table)
    for index, header in enumerate(headers):
        set_cell_text(
            table.rows[0].cells[index],
            header,
            bold=True,
            color="FFFFFF",
            size=font_size,
            align=WD_ALIGN_PARAGRAPH.CENTER,
        )
        set_cell_shading(table.rows[0].cells[index], NAVY)
        if widths:
            table.rows[0].cells[index].width = Inches(widths[index])
    set_repeat_table_header(table.rows[0])

    for row_index, values in enumerate(rows):
        cells = table.add_row().cells
        for column_index, value in enumerate(values):
            align = WD_ALIGN_PARAGRAPH.CENTER if column_index == 0 and len(headers) > 2 else WD_ALIGN_PARAGRAPH.LEFT
            set_cell_text(cells[column_index], value, size=font_size, align=align)
            if row_index % 2 == 1:
                set_cell_shading(cells[column_index], PALE_BLUE)
            if widths:
                cells[column_index].width = Inches(widths[column_index])
    document.add_paragraph().paragraph_format.space_after = Pt(0)
    return table


def add_body(document, text, *, bold_lead=None):
    text = tr(text)
    if bold_lead is not None:
        bold_lead = tr(bold_lead)
    paragraph = document.add_paragraph()
    paragraph.paragraph_format.space_after = Pt(7)
    paragraph.paragraph_format.line_spacing = 1.35
    if bold_lead and text.startswith(bold_lead):
        lead = paragraph.add_run(bold_lead)
        set_run_font(lead, bold=True)
        rest = paragraph.add_run(text[len(bold_lead):])
        set_run_font(rest)
    else:
        run = paragraph.add_run(text)
        set_run_font(run)
    return paragraph


def add_bullets(document, items):
    for item in items:
        paragraph = document.add_paragraph(style="List Bullet")
        paragraph.paragraph_format.space_after = Pt(4)
        paragraph.paragraph_format.line_spacing = 1.25
        set_run_font(paragraph.add_run(tr(item)))


def add_code(document, code):
    code = tr_code(code)
    lines = code.strip("\n").splitlines()
    for index, line in enumerate(lines):
        paragraph = document.add_paragraph()
        paragraph.paragraph_format.space_after = Pt(0)
        paragraph.paragraph_format.line_spacing = 1.0
        paragraph.paragraph_format.left_indent = Inches(0.18)
        paragraph.paragraph_format.right_indent = Inches(0.12)
        paragraph.paragraph_format.keep_together = True
        paragraph.paragraph_format.keep_with_next = index < len(lines) - 1
        properties = paragraph._p.get_or_add_pPr()
        shading = OxmlElement("w:shd")
        shading.set(qn("w:fill"), PALE_GRAY)
        properties.append(shading)
        run = paragraph.add_run(line if line else " ")
        set_run_font(run, name=CODE_FONT, size=8.2)
    document.add_paragraph().paragraph_format.space_after = Pt(0)


def add_source_listing(document, relative_path):
    add_heading(document, relative_path.as_posix(), 2)
    source_path = ROOT / relative_path
    lines = source_path.read_text(encoding="utf-8").expandtabs(4).splitlines()
    numbered = "\n".join(f"{number:04d}  {line}" for number, line in enumerate(lines, 1))

    paragraph = document.add_paragraph()
    paragraph.paragraph_format.space_after = Pt(8)
    paragraph.paragraph_format.line_spacing = 0.82
    paragraph.paragraph_format.keep_together = False
    paragraph.paragraph_format.widow_control = False
    properties = paragraph._p.get_or_add_pPr()
    shading = OxmlElement("w:shd")
    shading.set(qn("w:fill"), PALE_GRAY)
    properties.append(shading)
    run = paragraph.add_run(numbered)
    set_run_font(run, name=CODE_FONT, size=6.2)


def add_picture(document, filename, caption, alt_text, width=6.55):
    path = SCREENSHOT_DIR / filename
    paragraph = document.add_paragraph()
    paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
    paragraph.paragraph_format.keep_with_next = True
    run = paragraph.add_run()
    inline = run.add_picture(str(path), width=Inches(width))
    inline._inline.docPr.set("descr", tr(alt_text))

    caption_paragraph = document.add_paragraph()
    caption_paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
    caption_paragraph.paragraph_format.space_after = Pt(10)
    caption_run = caption_paragraph.add_run(tr(caption))
    set_run_font(caption_run, size=9, bold=True)


def add_page_number(paragraph):
    paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = paragraph.add_run()
    begin = OxmlElement("w:fldChar")
    begin.set(qn("w:fldCharType"), "begin")
    instruction = OxmlElement("w:instrText")
    instruction.set(qn("xml:space"), "preserve")
    instruction.text = " PAGE "
    end = OxmlElement("w:fldChar")
    end.set(qn("w:fldCharType"), "end")
    run._r.extend([begin, instruction, end])
    set_run_font(run, size=9, color="666666")


def add_heading(document, text, level=1):
    paragraph = document.add_heading(tr(text), level=level)
    paragraph.paragraph_format.keep_with_next = True
    paragraph.paragraph_format.space_before = Pt(12 if level == 1 else 8)
    paragraph.paragraph_format.space_after = Pt(6)
    return paragraph


def configure_document(document):
    section = document.sections[0]
    section.page_width = Inches(8.5)
    section.page_height = Inches(11)
    section.top_margin = Inches(0.72)
    section.bottom_margin = Inches(0.68)
    section.left_margin = Inches(0.82)
    section.right_margin = Inches(0.82)

    styles = document.styles
    normal = styles["Normal"]
    normal.font.name = BODY_FONT
    normal._element.rPr.rFonts.set(qn("w:eastAsia"), BODY_FONT)
    normal.font.size = Pt(11)

    title = styles["Title"]
    title.font.name = BODY_FONT
    title._element.rPr.rFonts.set(qn("w:eastAsia"), BODY_FONT)
    title.font.size = Pt(27)
    title.font.bold = True
    title.font.color.rgb = RGBColor(0, 0, 0)
    title_properties = title.element.get_or_add_pPr()
    title_borders = title_properties.find(qn("w:pBdr"))
    if title_borders is not None:
        title_properties.remove(title_borders)

    for name, size in (("Heading 1", 17), ("Heading 2", 13.5), ("Heading 3", 11.5)):
        style = styles[name]
        style.font.name = BODY_FONT
        style._element.rPr.rFonts.set(qn("w:eastAsia"), BODY_FONT)
        style.font.size = Pt(size)
        style.font.bold = True
        style.font.color.rgb = RGBColor(0, 0, 0)

    add_page_number(section.footer.paragraphs[0])


def build_report():
    document = Document()
    configure_document(document)

    title = document.add_paragraph(style="Title")
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title.paragraph_format.space_before = Pt(95)
    title.paragraph_format.space_after = Pt(24)
    title_run = title.add_run(tr("C Client Server 幾A幾B遊戲期中報告"))
    set_run_font(title_run, size=27, bold=True)

    subtitle = document.add_paragraph()
    subtitle.alignment = WD_ALIGN_PARAGRAPH.CENTER
    subtitle.paragraph_format.space_after = Pt(42)
    subtitle_run = subtitle.add_run(tr("網路系統程式開發與安全"))
    set_run_font(subtitle_run, size=16, bold=True, color=NAVY)

    cover_items = [
        "實作語言  C17",
        "建置系統  CMake",
        "執行環境  Ubuntu 24.04 on WSL2",
        "測試日期  2026 年 11 月 8 日",
    ]
    for item in cover_items:
        paragraph = document.add_paragraph()
        paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
        paragraph.paragraph_format.space_after = Pt(9)
        set_run_font(paragraph.add_run(tr(item)), size=12)

    conclusion = document.add_paragraph()
    conclusion.alignment = WD_ALIGN_PARAGRAPH.CENTER
    conclusion.paragraph_format.space_before = Pt(56)
    conclusion.paragraph_format.left_indent = Inches(0.65)
    conclusion.paragraph_format.right_indent = Inches(0.65)
    conclusion.paragraph_format.line_spacing = 1.4
    set_run_font(
        conclusion.add_run(tr(
            "本專案完成以 fork 處理每條 TCP connection 的單人幾A幾B遊戲，"
            "並以自動測試、10-client 執行證據、tcpdump 封包及三組異常對照驗證設計。"
        )),
        size=11.5,
    )

    document.add_page_break()

    add_heading(document, "一 專案成果", 1)
    add_body(
        document,
        "本專案以 C 實作 client-server 幾A幾B遊戲。Server 在每次 accept 成功後立即 fork，"
        "每個 child process 獨立保存該 connection 的答案、遊戲回合及暫時戰績。Client 提供開始遊戲、"
        "查詢 server system information、查詢 connection 戰績與離開四個選項。正式程式不使用 thread、"
        "shell command、帳號系統或跨 connection 共用狀態。",
    )
    add_body(
        document,
        "功能與證據均已完成。Debug 與 Release 組態的六組 CTest 全數通過；10 條同時存在的 loopback "
        "connections 產生 10 個 child processes；server 與 client 都由 ldd 證實載入 libgame_common.so；"
        "Valgrind 未發現記憶體錯誤；tcpdump pcap 保存完整 TCP 交握、應用 frame 及關閉流程。",
    )

    add_heading(document, "二 作業需求對照", 1)
    requirements = [
        ("fork per connection", "完成", "每次 accept 後 fork；child 獨立執行 session"),
        ("至少 10 clients 併行", "完成", "10 個 child PID 同時存在且 PPID 相同"),
        ("封包擷取與分析", "完成", "loopback tcpdump pcap 共 22 packets"),
        ("兩層 debug log", "完成", "CMake ENABLE_DEBUG_LOG 與 --log-level"),
        ("動態共用函式庫", "完成", "server/client 共同載入 libgame_common.so"),
        ("System information", "完成", "uname sysinfo sysconf 直接取得"),
        ("異常處理", "完成", "deadline 長度上限 SIGPIPE 錯誤次數 session 上限"),
        ("三組改善前後對照", "完成", "fragmented incomplete closed peer"),
        ("CMake 與 CTest", "完成", "out of source build 與六組測試"),
        ("報告與影片規劃", "完成", "本報告及第十節 2 分鐘腳本"),
    ]
    add_table(document, ["要求", "狀態", "證據"], requirements, widths=[1.65, 0.75, 3.95], font_size=9.3)

    add_heading(document, "三 系統設計", 1)
    add_heading(document, "系統架構", 2)
    add_body(
        document,
        "Parent server 只負責監聽、接受 connection、建立 child 與回收 child。每個 child 封閉 parent 的 "
        "listening socket 後，執行同一條 connection 的協定與遊戲狀態機。這個模型讓同一 IP 可以同時建立多條 "
        "connections，也讓某一個 client 的狀態與錯誤不會影響其他玩家。",
    )
    architecture = [
        ("多個 Client", "TCP 9734", "Parent Server", "fork", "每條 connection 一個 Child"),
        ("選單與輸入驗證", "ABGM frame", "accept 與 waitpid", "獨立 PID", "遊戲 狀態 戰績 System Info"),
    ]
    add_table(document, ["來源", "傳輸", "父程序", "建立", "處理者"], architecture, widths=[1.25, 1.05, 1.35, 0.8, 2.05], font_size=9.0)

    add_heading(document, "遊戲規則與狀態", 2)
    add_bullets(document, [
        "答案與猜測固定為四個數字字元，允許 0 開頭，四個數字不可重複。",
        "每局最多接受 15 次有效猜測；無效猜測回覆錯誤但不扣次數。",
        "4A0B 立即勝利；第 15 次仍未猜中時回覆 GAME_LOST 並公開答案。",
        "同一 connection 可連玩多局，child 暫存完成局數、勝敗與最佳猜測次數。",
        "Connection 關閉後 child 結束，資料不寫入檔案或共享記憶體。",
    ])

    add_heading(document, "Connection 生命週期", 2)
    add_table(
        document,
        ["階段", "Parent", "Child", "Client"],
        [
            ("建立", "記錄 IP port 後 fork", "關閉 listener", "connect"),
            ("歡迎", "記錄 child PID", "送 WELCOME", "顯示主選單"),
            ("操作", "繼續 accept", "處理 frame 狀態與活動 log", "遊戲或查詢"),
            ("結束", "SIGCHLD 後 waitpid", "關閉 connected socket", "QUIT 或 close"),
        ],
        widths=[0.85, 1.75, 2.05, 1.75],
        font_size=9.2,
    )

    add_heading(document, "四 通訊協定", 1)
    add_heading(document, "固定長度 Header", 2)
    add_body(
        document,
        "每個 application message 由 12-byte header 與 payload 組成。程式逐欄位編碼，不直接傳送 C struct，"
        "因此不受 padding、alignment 或主機位元序影響。Payload length 使用 network byte order，接收端會在讀取 "
        "payload 前拒絕超過 4096 bytes 的長度。",
    )
    add_table(
        document,
        ["Offset", "長度", "欄位", "規則"],
        [
            ("0", "4", "Magic", "ASCII ABGM"),
            ("4", "1", "Version", "固定 1"),
            ("5", "1", "Type", "Request 或 Response type"),
            ("6", "2", "Reserved", "固定 0"),
            ("8", "4", "Payload length", "Network byte order 最大 4096"),
        ],
        widths=[0.7, 0.7, 1.55, 3.65],
        font_size=9.3,
    )

    add_heading(document, "Message Type", 2)
    message_rows = [
        ("0x01", "Client to Server", "START_GAME"),
        ("0x02", "Client to Server", "GUESS"),
        ("0x03", "Client to Server", "GET_SYSTEM_INFO"),
        ("0x04", "Client to Server", "GET_STATS"),
        ("0x05", "Client to Server", "QUIT"),
        ("0x81", "Server to Client", "WELCOME"),
        ("0x82", "Server to Client", "GAME_STARTED"),
        ("0x83", "Server to Client", "GUESS_RESULT"),
        ("0x84", "Server to Client", "GAME_WON"),
        ("0x85", "Server to Client", "GAME_LOST"),
        ("0x86", "Server to Client", "SYSTEM_INFO"),
        ("0x87", "Server to Client", "STATS"),
        ("0xFF", "Server to Client", "ERROR"),
    ]
    add_table(document, ["值", "方向", "名稱"], message_rows, widths=[0.8, 2.1, 3.4], font_size=9.1)

    add_heading(document, "完整讀寫與 Deadline", 2)
    add_body(
        document,
        "TCP 只提供 byte stream，不保留 application message boundary。network_read_exact 與 network_write_all "
        "使用 poll 搭配同一個 CLOCK_MONOTONIC absolute deadline，反覆處理 partial I/O、EINTR、EAGAIN 與 EOF。"
        "新的少量 bytes 不會重設 deadline，因此 slow client 無法無限延長 child 的生命。",
    )

    add_heading(document, "五 關鍵原始碼", 1)
    add_heading(document, "Fork 每條 Connection", 2)
    add_body(document, "Parent 接受一條 connection 後先以數字格式保存來源 IP 與 port，再決定是否進入正常 session，接著不論是否已達上限都建立 child。正常 child 執行 session；超額 child 只送出 server busy 後結束。")
    add_code(document, """
client_fd = accept(listener,
                   (struct sockaddr *)&peer_address,
                   &peer_address_length);
format_peer_label((const struct sockaddr *)&peer_address,
                  peer_address_length, peer_label, sizeof(peer_label));
admitted = active_count < MAX_ACTIVE_CLIENTS;
child = fork();

if (child == 0) {
    close(listener);
    if (admitted) {
        result = server_session_run(client_fd, timeout, peer_label,
                                    generate_answer, NULL);
    } else {
        protocol_send_frame(client_fd, PROTOCOL_MSG_ERROR,
                            "server busy", 11U, &deadline);
    }
    close(client_fd);
    _exit(result == 0 ? 0 : 1);
}

close(client_fd);
""")

    add_heading(document, "四位數驗證", 2)
    add_body(document, "遊戲代碼始終以 char 陣列處理，不使用 atoi 或整數轉換，因此 0123 不會變成 123。Client 與 server 都驗證，server 的判定為最終結果。")
    add_code(document, """
bool game_code_is_valid(const char *code, size_t length)
{
    if (code == NULL || length != 4U) return false;
    for (size_t i = 0U; i < length; ++i) {
        if (code[i] < '0' || code[i] > '9') return false;
        for (size_t j = 0U; j < i; ++j) {
            if (code[i] == code[j]) return false;
        }
    }
    return true;
}
""")

    add_heading(document, "兩層 Log 控制", 2)
    add_body(document, "CMake option ENABLE_DEBUG_LOG 決定 LOG_DEBUG 是否進入 binary；編譯啟用後，server 仍可透過 --log-level 在執行期選擇 error、info 或 debug。預設 info 會記錄 client IP、來源 port、child PID、遊戲與查詢操作及離線結果；答案不會提前寫入 log。每筆紀錄先完整組合再單次寫出，避免 parent 與多個 child 同時輸出時互相黏行。")
    add_code(document, """
#ifdef ENABLE_DEBUG_LOG
#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

./server --log-level debug
""")
    add_code(document, """
[INFO pid=801] client_connected peer=[127.0.0.1]:60394 child=809 active_sessions=1
[INFO pid=809] peer=[127.0.0.1]:60394 action=GET_SYSTEM_INFO
[INFO pid=809] peer=[127.0.0.1]:60394 action=GET_STATS completed=0 wins=0 losses=0 best_attempts=0
[INFO pid=809] peer=[127.0.0.1]:60394 action=QUIT
[INFO pid=809] client_disconnected peer=[127.0.0.1]:60394 result=normal
""")

    add_heading(document, "System Information", 2)
    add_body(
        document,
        "Child 使用 uname 取得 Linux 與 kernel，sysconf 取得邏輯核心數，sysinfo 取得 uptime 與記憶體。"
        "程式不執行 shell command，也不回傳使用者名稱、私人路徑、IP address 或環境變數。",
    )

    add_heading(document, "六 強健性設計", 1)
    robustness_rows = [
        ("Fragmented input", "單次 recv 只得 4 of 12 bytes", "完整讀取迴圈重組 frame"),
        ("Incomplete input", "無 deadline 的迴圈永久阻塞", "absolute deadline 回傳 timeout"),
        ("Closed peer", "send 觸發 SIGPIPE 終止", "MSG_NOSIGNAL 回傳可處理錯誤"),
    ]
    add_table(document, ["相同輸入", "未改善行為", "改善後行為"], robustness_rows, widths=[1.55, 2.35, 2.45], font_size=9.2)
    add_picture(
        document,
        "03-robustness-comparison.png",
        "圖 1 三組異常條件的改善前後執行結果",
        "Terminal showing fragmented input, incomplete input, and closed peer comparisons passing",
    )

    add_heading(document, "其他異常處理", 2)
    add_bullets(document, [
        "Bad magic、unsupported version、nonzero reserved 與超長 payload 在讀取 payload 前拒絕。",
        "Framing 正確但狀態或內容錯誤時回覆 ERROR；連續五次錯誤後關閉 connection。",
        "每條 connection 預設 idle timeout 120 秒；測試使用 10 秒。",
        "正常 sessions 上限 32；超額 connection 仍 fork 短期 child 並回覆 server busy。",
        "SIGCHLD handler 通知 parent 使用 waitpid WNOHANG 回收 children，避免 zombie。",
        "fork 失敗時關閉 accepted socket，保留 listener 並繼續服務既有 clients。",
    ])

    add_heading(document, "七 執行驗證", 1)
    add_heading(document, "CTest", 2)
    add_body(document, "六組測試涵蓋遊戲規則、header codec、deadline I O、完整 frame、server session state machine 與三組 robustness 對照。Debug 與 Release 均為 6 of 6 PASS。")
    add_picture(
        document,
        "01-ctest-6-of-6.png",
        "圖 2 六組 CTest 全數通過",
        "Terminal showing six out of six CTest tests passed",
    )

    add_heading(document, "十個 Clients 併行", 2)
    add_body(document, "測試同時保持 10 條 loopback connections 五秒，再以 pgrep 與 ps 讀取 server 的直接 children。畫面顯示 10 個不同 PID 具有相同 PPID，證明 sessions 在時間上重疊。")
    add_picture(
        document,
        "02-ten-client-concurrency.png",
        "圖 3 同一 parent server 下同時存在 10 個 child processes",
        "Terminal listing ten concurrent server child processes with a common parent PID",
    )

    add_heading(document, "Client 與 System Information", 2)
    add_body(document, "Client 成功接收 WELCOME，接著使用主選單要求 system information 與 connection stats。系統資訊只包含規定欄位，初始戰績為零。")
    add_picture(
        document,
        "05-client-system-info.png",
        "圖 4 Client 選單 System Information 與 Connection Stats",
        "Client menu displaying safe server system information and zeroed connection statistics",
    )

    add_heading(document, "動態連結與記憶體檢查", 2)
    add_code(document, """
$ ldd build/server | grep game_common
libgame_common.so => .../build/libgame_common.so

$ ldd build/client | grep game_common
libgame_common.so => .../build/libgame_common.so

$ valgrind --error-exitcode=99 --leak-check=full build/session_tests
valgrind-session-tests: PASS
""")

    add_heading(document, "八 封包擷取與分析", 1)
    add_body(
        document,
        "tcpdump 只擷取 loopback TCP 9734，共保存 22 packets。SHA 256 為 "
        "f7f5d5cda30088e85e50de741b8d30f35986044540d7d087ec110f84b087e307。"
        "封包包含 TCP three way handshake、WELCOME、GET_SYSTEM_INFO、SYSTEM_INFO、GET_STATS、STATS、QUIT 與雙方 FIN。",
    )
    add_picture(
        document,
        "04-packet-frame-analysis.png",
        "圖 5 tcpdump 十六進位資料中的 ABGM application header",
        "Tcpdump hexadecimal output showing ABGM headers on loopback TCP 9734",
    )
    add_table(
        document,
        ["方向", "Header Hex", "解碼"],
        [
            ("Server to Client", "4142474d 01 81 0000 0000001b", "WELCOME 27 bytes"),
            ("Client to Server", "4142474d 01 03 0000 00000000", "GET_SYSTEM_INFO"),
            ("Server to Client", "4142474d 01 86 0000 000000a4", "SYSTEM_INFO 164 bytes"),
            ("Client to Server", "4142474d 01 04 0000 00000000", "GET_STATS"),
            ("Server to Client", "4142474d 01 87 0000 0000002c", "STATS 44 bytes"),
            ("Client to Server", "4142474d 01 05 0000 00000000", "QUIT"),
        ],
        widths=[1.35, 3.3, 1.75],
        font_size=8.7,
    )
    add_body(
        document,
        "WELCOME、SYSTEM_INFO 與 STATS 的 12-byte header 和文字 payload 出現在不同 TCP segments。"
        "這項執行證據證明 TCP 不保留訊息邊界，也直接支持本專案使用 frame length 與完整讀寫迴圈的必要性。",
    )

    add_heading(document, "九 建置與執行", 1)
    add_heading(document, "建置與測試", 2)
    add_code(document, """
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG_LOG=ON
cmake --build build --clean-first
ctest --test-dir build --output-on-failure
""")
    add_heading(document, "啟動 Server 與 Client", 2)
    add_code(document, """
cd build
./server

# 另一個終端
cd build
./client
""")
    add_body(document, "預設 endpoint 為 127.0.0.1:9734。跨主機測試時，server 可用 --host 0.0.0.0 --port 9734，client 則以 server 的私人 LAN IPv4 address 連線。")
    add_heading(document, "常用參數", 2)
    add_code(document, """
./server --host 127.0.0.1 --port 9734 \\
  --idle-timeout 10 --log-level debug

./client 127.0.0.1 9734
""")

    add_heading(document, "十 兩分鐘展示影片腳本", 1)
    video_rows = [
        ("0:00 to 0:15", "介紹架構", "說明 parent accept 後 fork，每條 connection 由獨立 child 處理"),
        ("0:15 to 0:35", "建置與測試", "執行 CMake 與 CTest，顯示 6 of 6 PASS"),
        ("0:35 to 1:05", "Client 功能", "啟動 server/client，展示 system info、開始遊戲與 stats"),
        ("1:05 to 1:25", "10-client", "執行 concurrency script，指出 10 個 child PID 與共同 PPID"),
        ("1:25 to 1:45", "異常處理", "執行 robustness demo，說明 partial read、timeout、SIGPIPE"),
        ("1:45 to 2:00", "封包與結尾", "開啟 tcpdump 分析，指出 ABGM header 與 FIN close"),
    ]
    add_table(document, ["時間", "畫面", "旁白重點"], video_rows, widths=[1.05, 1.45, 3.9], font_size=9.3)
    add_body(document, "錄影時正式 server 使用預設 120 秒 timeout；只有 timeout 示範與自動測試使用 10 秒。畫面應避免顯示密碼、使用者名稱、私人路徑或其他不相關視窗。")

    add_heading(document, "十一 完整原始碼交付", 1)
    add_body(
        document,
        "完整原始碼與測試隨本報告一同包含在專案目錄。下表列出所有交付程式檔；build output、執行 log、"
        "暫存檔與秘密資料均未納入 Git。Git 里程碑包含核心功能、protocol、server/client、封包證據與 robustness 證據。",
    )
    source_rows = [
        ("CMakeLists.txt", "CMake targets Debug Release CTest"),
        ("include", "遊戲 I O protocol log 與 session headers"),
        ("src common", "game log network_io protocol 實作"),
        ("src server", "Parent fork server 與 child session state machine"),
        ("src client", "Terminal menu 與遊戲 client"),
        ("tests and examples", "六組 CTest 整合 scripts 與三組隔離對照"),
        ("evidence", "pcap 分析與執行截圖"),
    ]
    add_table(document, ["路徑", "內容"], source_rows, widths=[2.35, 4.0], font_size=9.1)

    add_heading(document, "十二 結論", 1)
    add_body(
        document,
        "本專案完成符合課程要求的 C client-server 應用。Server 對每條 connection 使用 fork 並可同時處理至少 "
        "10 個 clients；client 可玩幾A幾B並查詢 system information；共用 utility 由動態函式庫提供。"
        "Protocol framing、absolute deadline、payload 上限、SIGPIPE 防護、child 回收與 session 上限處理主要異常條件。"
        "自動測試、執行畫面與 pcap 彼此對應，可重現本報告中的結果。",
    )

    document.add_page_break()
    add_heading(document, "附錄 A 完整程式原始碼", 1)
    add_body(
        document,
        "本附錄逐行收錄可建置 server、client 與 libgame_common.so 的全部 CMake、header 與 C 原始碼。"
        "自動測試、執行腳本、封包與原始截圖則以原始檔形式隨專案一併提交，以維持可直接執行與重現。",
    )
    for relative_path in SOURCE_FILES:
        add_source_listing(document, relative_path)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    document.save(OUTPUT_PATH)
    print(OUTPUT_PATH)


if __name__ == "__main__":
    build_report()
