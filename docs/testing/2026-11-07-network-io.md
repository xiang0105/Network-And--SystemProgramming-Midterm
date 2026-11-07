# Network I/O Test Record — 2026-11-07

## Scope

本次測試驗證 `game_common` 動態共用函式庫中的完整 socket read/write、partial I/O、`EINTR`、EOF、absolute deadline、blocked write 與 `SIGPIPE` 防護。

測試使用本機 `socketpair()` 與 forked child processes，不使用 thread。Application frame、server/client state machine 與 TCP concurrency 不在本次範圍。

## Test-first behavior cycles

1. 已在 socket 中的完整資料可以正確讀取。
2. Child 分兩次傳送的資料能在同一 deadline 內重組。
3. 沒有資料時，read 會等待到 deadline 後回傳 timeout。
4. 只收到部分資料後 peer 關閉時回傳 EOF。
5. Slow client 逐 byte 傳送不會重設 absolute deadline。
6. `poll()` 被 signal 以 `EINTR` 中斷後，仍使用原 deadline 繼續等待。
7. 一般 write 能完整送出資料。
8. 受限 send buffer 造成的 partial write 能循環完成，child 驗證所有 bytes 內容一致。
9. Peer 不讀、send buffer 填滿時，write 會在 deadline 回傳 timeout。
10. Peer 已關閉時，write 回傳 system error；`MSG_NOSIGNAL` 防止 process 被 `SIGPIPE` 終止。

第一個測試建置也發現 C17 模式下缺少 POSIX feature declaration；CMake 已明確設定 `_POSIX_C_SOURCE=200809L`，避免依賴編譯器預設擴充。

## Final verification

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --clean-first
ctest --test-dir build --output-on-failure
ldd build/network_io_tests
valgrind --leak-check=full --show-leak-kinds=all \
  --error-exitcode=99 build/network_io_tests
```

結果：

- Debug clean build：PASS
- Debug CTest：3/3 PASS
- Release configure/build/CTest：3/3 PASS
- `network_io_tests` 從 `build/libgame_common.so` 動態載入共用函式庫
- Valgrind parent 與所有 forked children：0 errors、0 bytes in 0 blocks

## Remaining work

下一個切片需把 header codec 與完整 I/O 組合成 application frame send/receive，再建立 `server`、`client` 與正式整合測試。本次結果仍不足以刪除 WSL 遷移備份。
