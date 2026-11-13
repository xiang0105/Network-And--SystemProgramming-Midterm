import re
from pathlib import Path

from docx import Document
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.shared import Inches, Pt, RGBColor
from docx.oxml import OxmlElement
from docx.oxml.ns import qn


ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / "report" / "Network_Programming_Midterm_Report.docx"
OUTPUT = ROOT / "report" / "Network_Programming_Midterm_Report_EN.docx"

TRANSLATIONS = {
    "C Client Server 幾A幾B遊戲期中報告": "C Client Server Bulls and Cows Midterm Report",
    "網路系統程式開發與安全": "Network and System Programming Development and Security",
    "實作語言  C17": "Implementation Language  C17",
    "建置系統  CMake": "Build System  CMake",
    "執行環境  Ubuntu 24.04 on WSL2": "Runtime Environment  Ubuntu 24.04 on WSL2",
    "測試日期  2026 年 11 月 8 日": "Test Date  November 8 2026",
    "本專案完成以 fork 處理每條 TCP connection 的單人幾A幾B遊戲，並以自動測試、10-client 執行證據、tcpdump 封包及三組異常對照驗證設計。": "This project implements a single-player Bulls and Cows game in which the server uses fork to handle each TCP connection. Automated tests, a 10-client runtime demonstration, a tcpdump capture, and three abnormal-condition comparisons verify the design.",
    "一 專案成果": "1 Project Results",
    "本專案以 C 實作 client-server 幾A幾B遊戲。Server 在每次 accept 成功後立即 fork，每個 child process 獨立保存該 connection 的答案、遊戲回合及暫時戰績。Client 提供開始遊戲、查詢 server system information、查詢 connection 戰績與離開四個選項。正式程式不使用 thread、shell command、帳號系統或跨 connection 共用狀態。": "This project implements a client-server Bulls and Cows game in C. The server calls fork immediately after each successful accept, and each child process independently stores the answer, game rounds, and temporary statistics for its connection. The client provides four options: start a game, request server system information, view connection statistics, and quit. The production program does not use threads, shell commands, an account system, or state shared across connections.",
    "功能與證據均已完成。Debug 與 Release 組態的六組 CTest 全數通過；10 條同時存在的 loopback connections 產生 10 個 child processes；server 與 client 都由 ldd 證實載入 libgame_common.so；Valgrind 未發現記憶體錯誤；tcpdump pcap 保存完整 TCP 交握、應用 frame 及關閉流程。": "All planned functions and evidence are complete. All six CTest groups pass in both Debug and Release configurations; ten simultaneous loopback connections create ten child processes; ldd confirms that both the server and client load libgame_common.so; Valgrind reports no memory errors; and the tcpdump pcap preserves the complete TCP handshake, application frames, and close sequence.",
    "二 作業需求對照": "2 Requirements Compliance",
    "三 系統設計": "3 System Design",
    "系統架構": "System Architecture",
    "Parent server 只負責監聽、接受 connection、建立 child 與回收 child。每個 child 封閉 parent 的 listening socket 後，執行同一條 connection 的協定與遊戲狀態機。這個模型讓同一 IP 可以同時建立多條 connections，也讓某一個 client 的狀態與錯誤不會影響其他玩家。": "The parent server only listens, accepts connections, creates children, and reaps terminated children. Each child closes its copy of the parent's listening socket and runs the protocol and game state machine for one connection. This model allows one IP address to create several simultaneous connections while isolating each client's state and errors from every other player.",
    "遊戲規則與狀態": "Game Rules and State",
    "答案與猜測固定為四個數字字元，允許 0 開頭，四個數字不可重複。": "Answers and guesses contain exactly four digit characters. A leading zero is allowed, and digits may not repeat.",
    "每局最多接受 15 次有效猜測；無效猜測回覆錯誤但不扣次數。": "Each game accepts at most 15 valid guesses. Invalid guesses return an error without consuming an attempt.",
    "4A0B 立即勝利；第 15 次仍未猜中時回覆 GAME_LOST 並公開答案。": "A 4A0B result wins immediately. If the fifteenth valid guess is incorrect, the server returns GAME_LOST and reveals the answer.",
    "同一 connection 可連玩多局，child 暫存完成局數、勝敗與最佳猜測次數。": "One connection may play several games. Its child temporarily stores completed games, wins, losses, and the best attempt count.",
    "Connection 關閉後 child 結束，資料不寫入檔案或共享記憶體。": "When the connection closes, the child exits. It does not write session data to a file or shared memory.",
    "Connection 生命週期": "Connection Lifecycle",
    "四 通訊協定": "4 Communication Protocol",
    "固定長度 Header": "Fixed-Length Header",
    "每個 application message 由 12-byte header 與 payload 組成。程式逐欄位編碼，不直接傳送 C struct，因此不受 padding、alignment 或主機位元序影響。Payload length 使用 network byte order，接收端會在讀取 payload 前拒絕超過 4096 bytes 的長度。": "Each application message consists of a 12-byte header followed by a payload. The program encodes every field explicitly instead of transmitting a C struct, so padding, alignment, and host byte order do not affect the wire format. Payload length uses network byte order, and the receiver rejects lengths above 4096 bytes before reading the payload.",
    "Message Type": "Message Type",
    "完整讀寫與 Deadline": "Complete Reads and Writes with an Absolute Deadline",
    "TCP 只提供 byte stream，不保留 application message boundary。network_read_exact 與 network_write_all 使用 poll 搭配同一個 CLOCK_MONOTONIC absolute deadline，反覆處理 partial I/O、EINTR、EAGAIN 與 EOF。新的少量 bytes 不會重設 deadline，因此 slow client 無法無限延長 child 的生命。": "TCP provides a byte stream and does not preserve application message boundaries. network_read_exact and network_write_all use poll with one CLOCK_MONOTONIC absolute deadline while repeatedly handling partial I/O, EINTR, EAGAIN, and EOF. A small amount of newly received data does not reset the deadline, so a slow client cannot keep a child alive indefinitely.",
    "五 關鍵原始碼": "5 Key Source Code",
    "Fork 每條 Connection": "Fork for Every Connection",
    "Parent 接受一條 connection 後先以數字格式保存來源 IP 與 port，再決定是否進入正常 session，接著不論是否已達上限都建立 child。正常 child 執行 session；超額 child 只送出 server busy 後結束。": "After accepting a connection, the parent records the numeric source IP address and port, decides whether to admit a normal session, and creates a child even when the normal-session limit has been reached. An admitted child runs the session. An excess child only sends server busy and exits.",
    "四位數驗證": "Four-Digit Validation",
    "遊戲代碼始終以 char 陣列處理，不使用 atoi 或整數轉換，因此 0123 不會變成 123。Client 與 server 都驗證，server 的判定為最終結果。": "The program always stores game codes in char arrays and never uses atoi or another integer conversion, so 0123 does not become 123. Both the client and server validate guesses, and the server's decision is authoritative.",
    "兩層 Log 控制": "Two-Level Log Control",
    "CMake option ENABLE_DEBUG_LOG 決定 LOG_DEBUG 是否進入 binary；編譯啟用後，server 仍可透過 --log-level 在執行期選擇 error、info 或 debug。預設 info 會記錄 client IP、來源 port、child PID、遊戲與查詢操作及離線結果；答案不會提前寫入 log。每筆紀錄先完整組合再單次寫出，避免 parent 與多個 child 同時輸出時互相黏行。": "The CMake option ENABLE_DEBUG_LOG determines whether LOG_DEBUG is included in the binary. When compiled in, --log-level still selects error, info, or debug at runtime. The default info level records the client IP address, source port, child PID, game and query actions, and the disconnect result. It never logs the secret answer before the game ends. Each log record is assembled in memory and written once to prevent output from the parent and several children from interleaving.",
    "System Information": "System Information",
    "Child 使用 uname 取得 Linux 與 kernel，sysconf 取得邏輯核心數，sysinfo 取得 uptime 與記憶體。程式不執行 shell command，也不回傳使用者名稱、私人路徑、IP address 或環境變數。": "The child uses uname for Linux and kernel information, sysconf for the logical CPU count, and sysinfo for uptime and memory. The program does not execute shell commands or return usernames, private paths, IP addresses, or environment variables.",
    "六 強健性設計": "6 Robustness Design",
    "圖 1 三組異常條件的改善前後執行結果": "Figure 1 Results Before and After Robustness Improvements",
    "其他異常處理": "Other Abnormal-Condition Handling",
    "Bad magic、unsupported version、nonzero reserved 與超長 payload 在讀取 payload 前拒絕。": "The receiver rejects bad magic, unsupported versions, nonzero reserved fields, and oversized payloads before reading the payload.",
    "Framing 正確但狀態或內容錯誤時回覆 ERROR；連續五次錯誤後關閉 connection。": "For a correctly framed request with invalid state or content, the server returns ERROR and closes the connection after five consecutive errors.",
    "每條 connection 預設 idle timeout 120 秒；測試使用 10 秒。": "Each connection has a default idle timeout of 120 seconds; tests use 10 seconds.",
    "正常 sessions 上限 32；超額 connection 仍 fork 短期 child 並回覆 server busy。": "The server allows at most 32 normal sessions. An excess connection still receives a short-lived child that returns server busy.",
    "SIGCHLD handler 通知 parent 使用 waitpid WNOHANG 回收 children，避免 zombie。": "The SIGCHLD handler notifies the parent to reap children with waitpid and WNOHANG, preventing zombies.",
    "fork 失敗時關閉 accepted socket，保留 listener 並繼續服務既有 clients。": "If fork fails, the parent closes the accepted socket, keeps the listener open, and continues serving existing clients.",
    "七 執行驗證": "7 Runtime Verification",
    "八 封包擷取與分析": "8 Packet Capture and Analysis",
    "六組測試涵蓋遊戲規則、header codec、deadline I O、完整 frame、server session state machine 與三組 robustness 對照。Debug 與 Release 均為 6 of 6 PASS。": "Six test groups cover game rules, header encoding and decoding, deadline-based I/O, complete frames, the server session state machine, and three robustness comparisons. Both Debug and Release builds pass 6 of 6 tests.",
    "圖 2 六組 CTest 全數通過": "Figure 2 All Six CTest Groups Passed",
    "十個 Clients 併行": "Ten Concurrent Clients",
    "測試同時保持 10 條 loopback connections 五秒，再以 pgrep 與 ps 讀取 server 的直接 children。畫面顯示 10 個不同 PID 具有相同 PPID，證明 sessions 在時間上重疊。": "The test keeps ten loopback connections open simultaneously for five seconds, then uses pgrep and ps to list the server's direct children. The output shows ten different PIDs with one common PPID, proving that the sessions overlap in time.",
    "圖 3 同一 parent server 下同時存在 10 個 child processes": "Figure 3 Ten Child Processes Running Under One Parent Server",
    "Client 與 System Information": "Client and System Information",
    "Client 成功接收 WELCOME，接著使用主選單要求 system information 與 connection stats。系統資訊只包含規定欄位，初始戰績為零。": "The client receives WELCOME and then uses the main menu to request system information and connection statistics. The system information contains only the required fields, and the initial statistics are zero.",
    "圖 4 Client 選單 System Information 與 Connection Stats": "Figure 4 Client Menu System Information and Connection Statistics",
    "動態連結與記憶體檢查": "Dynamic Linking and Memory Checks",
    "tcpdump 只擷取 loopback TCP 9734，共保存 22 packets。SHA 256 為 f7f5d5cda30088e85e50de741b8d30f35986044540d7d087ec110f84b087e307。封包包含 TCP three way handshake、WELCOME、GET_SYSTEM_INFO、SYSTEM_INFO、GET_STATS、STATS、QUIT 與雙方 FIN。": "tcpdump captured only loopback TCP port 9734 and stored 22 packets. The SHA-256 digest is f7f5d5cda30088e85e50de741b8d30f35986044540d7d087ec110f84b087e307. The capture contains the TCP three-way handshake, WELCOME, GET_SYSTEM_INFO, SYSTEM_INFO, GET_STATS, STATS, QUIT, and FIN packets from both endpoints.",
    "圖 5 tcpdump 十六進位資料中的 ABGM application header": "Figure 5 ABGM Application Header in Tcpdump Hexadecimal Output",
    "WELCOME、SYSTEM_INFO 與 STATS 的 12-byte header 和文字 payload 出現在不同 TCP segments。這項執行證據證明 TCP 不保留訊息邊界，也直接支持本專案使用 frame length 與完整讀寫迴圈的必要性。": "The 12-byte headers and text payloads for WELCOME, SYSTEM_INFO, and STATS appear in different TCP segments. This runtime evidence shows that TCP does not preserve message boundaries and directly supports the need for frame lengths and complete I/O loops.",
    "九 建置與執行": "9 Build and Execution",
    "建置與測試": "Build and Test",
    "啟動 Server 與 Client": "Start the Server and Client",
    "# 另一個終端": "# Another terminal",
    "預設 endpoint 為 127.0.0.1:9734。跨主機測試時，server 可用 --host 0.0.0.0 --port 9734，client 則以 server 的私人 LAN IPv4 address 連線。": "The default endpoint is 127.0.0.1:9734. For a two-host test, the server can use --host 0.0.0.0 --port 9734, and the client connects to the server's private LAN IPv4 address.",
    "常用參數": "Common Options",
    "十 完整原始碼交付": "10 Complete Source Delivery",
    "完整原始碼與測試隨本報告一同包含在專案目錄。下表列出所有交付程式檔；build output、執行 log、暫存檔與秘密資料均未納入 Git。Git 里程碑包含核心功能、protocol、server/client、封包證據與 robustness 證據。": "The project directory includes the complete source code and tests with this report. The table lists all delivered program files. Build output, runtime logs, temporary files, and secrets are excluded from Git. Git milestones cover the core functions, protocol, server and client, packet evidence, and robustness evidence.",
    "十一 結論": "11 Conclusion",
    "本專案完成符合課程要求的 C client-server 應用。Server 對每條 connection 使用 fork 並可同時處理至少 10 個 clients；client 可玩幾A幾B並查詢 system information；共用 utility 由動態函式庫提供。Protocol framing、absolute deadline、payload 上限、SIGPIPE 防護、child 回收與 session 上限處理主要異常條件。自動測試、執行畫面與 pcap 彼此對應，可重現本報告中的結果。": "The project delivers a C client-server application that satisfies the course requirements. The server forks for every connection and handles at least ten clients concurrently. The client plays Bulls and Cows and requests system information, while a dynamic library provides shared utilities. Protocol framing, an absolute deadline, the payload limit, SIGPIPE protection, child reaping, and the session limit address the main abnormal conditions. Automated tests, runtime screenshots, and the pcap capture provide matching evidence that reproduces the reported results.",
}

TABLE_TRANSLATIONS = {
    "要求": "Requirement", "狀態": "Status", "證據": "Evidence", "完成": "Complete",
    "每次 accept 後 fork；child 獨立執行 session": "Fork after every accept; each child runs an independent session",
    "至少 10 clients 併行": "At least 10 concurrent clients", "10 個 child PID 同時存在且 PPID 相同": "Ten child PIDs exist simultaneously with the same PPID",
    "封包擷取與分析": "Packet capture and analysis", "loopback tcpdump pcap 共 22 packets": "Loopback tcpdump capture containing 22 packets",
    "兩層 debug log": "Two-level debug logging", "CMake ENABLE_DEBUG_LOG 與 --log-level": "CMake ENABLE_DEBUG_LOG and --log-level",
    "動態共用函式庫": "Dynamic shared library", "server/client 共同載入 libgame_common.so": "Both server and client load libgame_common.so",
    "異常處理": "Abnormal-condition handling", "deadline 長度上限 SIGPIPE 錯誤次數 session 上限": "Deadline, length limit, SIGPIPE protection, error count, and session limit",
    "三組改善前後對照": "Three before-and-after comparisons", "fragmented incomplete closed peer": "Fragmented input, incomplete input, and closed peer",
    "CMake 與 CTest": "CMake and CTest", "out of source build 與六組測試": "Out-of-source build and six test groups",
    "來源": "Source", "傳輸": "Transport", "父程序": "Parent Process", "建立": "Creation", "處理者": "Handler",
    "多個 Client": "Multiple Clients", "Parent Server": "Parent Server", "每條 connection 一個 Child": "One Child per Connection",
    "選單與輸入驗證": "Menu and Input Validation", "ABGM frame": "ABGM frame", "accept 與 waitpid": "accept and waitpid", "獨立 PID": "Separate PID", "遊戲 狀態 戰績 System Info": "Game State Statistics System Info",
    "階段": "Stage", "Parent": "Parent", "Child": "Child", "Client": "Client", "記錄 IP port 後 fork": "Record IP and port, then fork", "關閉 listener": "Close listener", "connect": "connect", "歡迎": "Welcome", "記錄 child PID": "Record child PID", "送 WELCOME": "Send WELCOME", "顯示主選單": "Display main menu", "操作": "Operation", "繼續 accept": "Continue accepting", "處理 frame 狀態與活動 log": "Process frame, state, and activity log", "遊戲或查詢": "Game or query", "結束": "End", "SIGCHLD 後 waitpid": "waitpid after SIGCHLD", "關閉 connected socket": "Close connected socket", "QUIT 或 close": "QUIT or close",
    "長度": "Length", "欄位": "Field", "規則": "Rule", "固定 1": "Fixed at 1", "Request 或 Response type": "Request or response type", "固定 0": "Fixed at 0", "Network byte order 最大 4096": "Network byte order, maximum 4096",
    "值": "Value", "方向": "Direction", "名稱": "Name", "解碼": "Decoded Meaning", "Client to Server": "Client to Server", "Server to Client": "Server to Client",
    "uname sysinfo sysconf 直接取得": "Collected directly through uname, sysinfo, and sysconf",
    "相同輸入": "Same Input", "未改善行為": "Behavior Without Protection", "改善後行為": "Behavior With Protection", "單次 recv 只得 4 of 12 bytes": "One recv returns only 4 of 12 bytes", "完整讀取迴圈重組 frame": "Complete-read loop reassembles the frame", "無 deadline 的迴圈永久阻塞": "A loop without a deadline blocks forever", "absolute deadline 回傳 timeout": "Absolute deadline returns timeout", "send 觸發 SIGPIPE 終止": "send triggers SIGPIPE termination", "MSG_NOSIGNAL 回傳可處理錯誤": "MSG_NOSIGNAL returns a manageable error",
    "Offset": "Offset", "Payload length": "Payload length", "ASCII ABGM": "ASCII ABGM", "Magic": "Magic", "Version": "Version", "Type": "Type", "Reserved": "Reserved",
    "路徑": "Path", "內容": "Contents", "遊戲 I O protocol log 與 session headers": "Game, I/O, protocol, log, and session headers", "game log network_io protocol 實作": "Game, log, network_io, and protocol implementations", "Parent fork server 與 child session state machine": "Parent fork server and child session state machine", "Terminal menu 與遊戲 client": "Terminal menu and game client", "六組 CTest 整合 scripts 與三組隔離對照": "Six CTest groups, integration scripts, and three isolated comparisons", "pcap 分析與執行截圖": "Pcap analysis and runtime screenshots",
}


def translate_text(text: str) -> str:
    if text in TRANSLATIONS:
        return TRANSLATIONS[text]
    if text in TABLE_TRANSLATIONS:
        return TABLE_TRANSLATIONS[text]
    return text


def set_run_text(paragraph, text):
    if paragraph.runs:
        paragraph.runs[0].text = text
        for run in paragraph.runs[1:]:
            run.text = ""
    else:
        paragraph.add_run(text)


def shade(cell, fill):
    props = cell._tc.get_or_add_tcPr()
    shd = props.find(qn("w:shd"))
    if shd is None:
        shd = OxmlElement("w:shd")
        props.append(shd)
    shd.set(qn("w:fill"), fill)


def cell_margins(cell, top=100, start=120, bottom=100, end=120):
    props = cell._tc.get_or_add_tcPr()
    margins = props.first_child_found_in("w:tcMar")
    if margins is None:
        margins = OxmlElement("w:tcMar")
        props.append(margins)
    for side, value in (("top", top), ("start", start), ("bottom", bottom), ("end", end)):
        node = margins.find(qn(f"w:{side}"))
        if node is None:
            node = OxmlElement(f"w:{side}")
            margins.append(node)
        node.set(qn("w:w"), str(value))
        node.set(qn("w:type"), "dxa")


def borders(table, color="D9D9D9"):
    props = table._tbl.tblPr
    edge_set = props.first_child_found_in("w:tblBorders")
    if edge_set is None:
        edge_set = OxmlElement("w:tblBorders")
        props.append(edge_set)
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        node = edge_set.find(qn(f"w:{edge}"))
        if node is None:
            node = OxmlElement(f"w:{edge}")
            edge_set.append(node)
        node.set(qn("w:val"), "single")
        node.set(qn("w:sz"), "6")
        node.set(qn("w:color"), color)


def make_relationship_diagram():
    from PIL import Image, ImageDraw, ImageFont

    out = ROOT / "report" / "assets" / "tcp_server_client_relationship.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    width, height = 1800, 980
    image = Image.new("RGB", (width, height), "white")
    draw = ImageDraw.Draw(image)
    navy = "#1F416A"
    blue = "#EAF2F8"
    border = "#1F416A"
    dark = "#111827"
    try:
        regular = ImageFont.truetype(r"C:\Windows\Fonts\arial.ttf", 34)
        bold = ImageFont.truetype(r"C:\Windows\Fonts\arialbd.ttf", 40)
        small = ImageFont.truetype(r"C:\Windows\Fonts\arial.ttf", 27)
        small_bold = ImageFont.truetype(r"C:\Windows\Fonts\arialbd.ttf", 30)
    except OSError:
        regular = bold = small = small_bold = ImageFont.load_default()

    def centered(text, box, font, fill=dark, spacing=8):
        lines = text.split("\n")
        heights = [draw.textbbox((0, 0), line, font=font)[3] for line in lines]
        total = sum(heights) + spacing * (len(lines) - 1)
        y = (box[1] + box[3] - total) / 2
        for line, h in zip(lines, heights):
            bbox = draw.textbbox((0, 0), line, font=font)
            x = (box[0] + box[2] - (bbox[2] - bbox[0])) / 2
            draw.text((x, y), line, font=font, fill=fill)
            y += h + spacing

    def box(x0, y0, x1, y1, title, body):
        draw.rounded_rectangle((x0, y0, x1, y1), radius=24, fill=blue, outline=border, width=6)
        draw.rectangle((x0, y0, x1, y0 + 78), fill=navy)
        centered(title, (x0, y0, x1, y0 + 78), bold, "white")
        centered(body, (x0 + 24, y0 + 95, x1 - 24, y1 - 24), regular, dark, 14)

    def arrow(x0, y0, x1, y1, label):
        draw.line((x0, y0, x1, y1), fill=navy, width=7)
        direction = 1 if x1 > x0 else -1
        tip = (x1, y1)
        wing = 22
        draw.polygon([(tip[0], tip[1]), (tip[0] - direction * wing, tip[1] - wing / 2), (tip[0] - direction * wing, tip[1] + wing / 2)], fill=navy)
        bbox = draw.textbbox((0, 0), label, font=small_bold)
        lx = (x0 + x1 - (bbox[2] - bbox[0])) / 2
        ly = y0 - 58 if y0 < 500 else y0 + 22
        draw.rounded_rectangle((lx - 12, ly - 5, lx + (bbox[2] - bbox[0]) + 12, ly + (bbox[3] - bbox[1]) + 5), radius=8, fill="white")
        draw.text((lx, ly), label, font=small_bold, fill=navy)

    centered("TCP Server and Client Relationship", (0, 35, width, 110), bold, navy)
    box(90, 245, 510, 735, "CLIENT", "Application process\n./client\n\nconnect()\n\nSends request frames")
    box(645, 200, 1155, 780, "TCP", "Transport layer\n127.0.0.1:9734\n\nSYN / SYN-ACK / ACK\n\nReliable ordered\nfull-duplex byte stream\n\nABGM header + payload\n\nFIN / ACK")
    box(1290, 245, 1710, 735, "SERVER", "Application process\n./server\n\nlisten()\naccept()\n\nfork()\n\nChild handles one\nconnection session")
    arrow(510, 340, 645, 340, "connect")
    arrow(1155, 340, 1290, 340, "accept")
    arrow(510, 480, 645, 480, "request")
    arrow(1290, 570, 1155, 570, "response")
    footer = "Framed messages over TCP; one child per accepted connection."
    bbox = draw.textbbox((0, 0), footer, font=small)
    draw.text(((width - (bbox[2] - bbox[0])) / 2, height - 55), footer, font=small, fill=dark)
    image.save(out)
    return out


def add_relationship_diagram(doc, target):
    diagram_path = make_relationship_diagram()
    prose = doc.add_paragraph(
        "TCP is the transport layer between the Client and Server. The Client initiates a connection to the Server's listening socket; TCP provides reliable, ordered, full-duplex byte delivery; the Server accepts the connection and forks a child process to handle that session."
    )
    prose.paragraph_format.space_after = Pt(9)
    for r in prose.runs:
        r.font.name = "Arial"
        r.font.size = Pt(10.5)

    heading = doc.add_paragraph("TCP Server and Client Relationship")
    heading.style = "Heading 1"
    heading.paragraph_format.space_after = Pt(8)
    picture = doc.add_paragraph()
    picture.alignment = WD_ALIGN_PARAGRAPH.CENTER
    picture.paragraph_format.space_after = Pt(8)
    picture.add_run().add_picture(str(diagram_path), width=Inches(6.55))
    caption = doc.add_paragraph("TCP Server and Client Relationship Diagram")
    caption.alignment = WD_ALIGN_PARAGRAPH.CENTER
    caption.paragraph_format.space_after = Pt(8)
    for r in caption.runs:
        r.font.name = "Arial"
        r.font.size = Pt(9.5)
        r.font.bold = True

    parent = target._p.getparent()
    insert_at = parent.index(target._p)
    parent.insert(insert_at, heading._p)
    parent.insert(insert_at + 1, prose._p)
    parent.insert(insert_at + 2, picture._p)
    parent.insert(insert_at + 3, caption._p)


def add_github_link(doc):
    target = next(p for p in doc.paragraphs if p.text.strip().startswith("This project implements a single-player"))
    p = target.insert_paragraph_before("GitHub Repository  https://github.com/xiang0105/Network-And--SystemProgramming-Midterm")
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_after = Pt(12)
    for r in p.runs:
        r.font.name = "Arial"
        r.font.size = Pt(10.5)
        r.font.color.rgb = RGBColor(31, 65, 106)


def main():
    doc = Document(INPUT)
    for paragraph in doc.paragraphs:
        translated = translate_text(paragraph.text)
        if translated != paragraph.text:
            set_run_text(paragraph, translated)

    for table in doc.tables:
        for row in table.rows:
            for cell in row.cells:
                for paragraph in cell.paragraphs:
                    translated = translate_text(paragraph.text)
                    if translated != paragraph.text:
                        set_run_text(paragraph, translated)

    add_github_link(doc)
    first_section_heading = next(p for p in doc.paragraphs if p.text.strip() == "1 Project Results")
    add_relationship_diagram(doc, first_section_heading)
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    doc.save(OUTPUT)
    print(OUTPUT)


if __name__ == "__main__":
    main()
