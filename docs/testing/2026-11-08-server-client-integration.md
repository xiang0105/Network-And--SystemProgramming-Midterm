# Server/Client Integration Test Record — 2026-11-08

## Scope

本切片完成可執行的 `server`、`client`、server connection session state machine、Linux system information、隨機答案產生、fork-per-connection、32-session 上限，以及編譯期／執行期兩層 log 控制。

## Covered behavior

1. 答案以 Linux `getrandom()` 產生，固定四位、可為 `0` 開頭且數字不重複。
2. 連線後先收到 `WELCOME`，主選單可開始遊戲、查 system information、查連線戰績或離開。
3. 無效猜測回覆 `ERROR` 且不消耗回合；15 次有效猜測未成功會回覆答案。
4. 同一 connection 可連續遊玩，完成局數、勝敗及最佳猜測次數由該 child 暫存。
5. System information 直接使用 Linux API，僅回傳規定欄位。
6. 連續五次 application-level 錯誤後關閉；idle timeout 回覆錯誤後關閉。
7. Parent 每次 `accept()` 後使用 `fork()`；10 條同時存在的 connections 實際觀察到 10 個不同 child PIDs。
8. `ENABLE_DEBUG_LOG=OFF` 會在編譯期移除 debug log；啟用後仍可用 `--log-level error|info|debug` 在執行期過濾。

## Reproducible commands

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG_LOG=ON
cmake --build build --clean-first
ctest --test-dir build --output-on-failure
bash tests/test_loopback_concurrency.sh
bash tests/test_debug_logging.sh
ldd build/server
ldd build/client
valgrind --error-exitcode=99 --leak-check=full \
  --trace-children=yes build/session_tests
```

編譯期關閉 debug log 的獨立驗證：

```sh
cmake -S . -B /tmp/network-midterm-no-debug \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_DEBUG_LOG=OFF
cmake --build /tmp/network-midterm-no-debug
ctest --test-dir /tmp/network-midterm-no-debug --output-on-failure
```

## Results

- Debug clean build：PASS，無 compiler warning
- Debug CTest：5/5 PASS
- Release、`ENABLE_DEBUG_LOG=OFF` CTest：5/5 PASS
- Valgrind `session_tests`：PASS，無 memory error
- `server` 與 `client` 均由 `ldd` 證實載入 `build/libgame_common.so`
- Client system-information／stats smoke test：PASS
- Loopback concurrent children：10，所有 child 的 PPID 均為同一 server parent，PASS
- Runtime `debug` level 產生 INFO/DEBUG；runtime `error` level不產生 INFO/DEBUG，PASS

## Remaining evidence

功能本身已完成；正式交付證據仍需補上 `tcpdump` loopback `.pcap` 與 frame 分析，以及三組 robustness「改善前／改善後」的相同輸入對照。WSL 遷移備份在這些完整整合證據完成前繼續保留。
