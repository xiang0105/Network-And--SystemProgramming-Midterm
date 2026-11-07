# Protocol Header Test Record — 2026-11-07

## Scope

本次測試驗證 `game_common` 動態共用函式庫內的 12-byte protocol header codec。Socket 收送、partial read／write、deadline timeout、server/client state machine 與整合測試不在本次範圍。

## Test-first behavior cycles

每個行為都先執行會失敗的測試，再加入最小實作並確認轉為通過：

1. Encoder 產生 `ABGM`、version `1`、message type、zero reserved 與 payload length。
2. Payload length 以 network byte order 編碼。
3. Encoder 拒絕超過 4096 bytes 的 payload。
4. Encoder 拒絕空指標與不足 12 bytes 的輸出 buffer。
5. Encoder 拒絕未定義的 message type。
6. Decoder 能還原合法 header 的 version、message type 與 payload length。
7. Decoder 分別拒絕錯誤 magic、unsupported version、nonzero reserved、未知 type 與超長 payload。
8. Decoder 拒絕空輸入、空輸出與不足 12 bytes 的 buffer。

空指標防護加入前，對應 RED 測試可重現 SegFault；加入參數驗證後正常回傳 `PROTOCOL_STATUS_INVALID_ARGUMENT`。

## Final verification

Debug 驗證：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --clean-first
ctest --test-dir build --output-on-failure
ldd build/protocol_tests
valgrind --leak-check=full --show-leak-kinds=all \
  --error-exitcode=99 build/protocol_tests
```

結果：

- Debug clean build：PASS
- Debug CTest：2/2 PASS
- Release configure/build/CTest：2/2 PASS
- `protocol_tests` 從 `build/libgame_common.so` 動態載入共用函式庫
- Valgrind：0 errors、結束時 0 bytes in 0 blocks

## Remaining work

下一個切片需實作 socket 完整讀寫與 absolute deadline，之後才能建立真正的 `server`、`client` 及正式 concurrency／robustness 整合測試。本次結果不足以刪除 WSL 遷移備份。
