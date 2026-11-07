# Application Frame Test Record — 2026-11-07

## Scope

本次測試把 protocol header codec 與 deadline-aware socket I/O 組合成 `protocol_send_frame()`／`protocol_receive_frame()`。Frame 使用同一個 absolute deadline 完成 header 與 payload 收送。

Server/client state machine、message direction 與 message-specific payload 規則留待後續整合層處理。

## Test-first behavior cycles

1. `GUESS` frame 寫出的 12-byte header 與 4-byte payload 完全符合 wire format。
2. 合法 frame 能完整 roundtrip，還原 version、type、length 與 payload。
3. 接收端為 payload 額外加入本機 `\0`，但不改變 wire payload length。
4. 錯誤 magic 會回傳 `PROTOCOL_STATUS_INVALID_MAGIC`。
5. 4097-byte payload length 會在讀取 payload 前被拒絕。
6. 合法 header 後缺少 payload 時，使用原 deadline 回傳 timeout。
7. Header 尚未收完就斷線時回傳 EOF。
8. Zero-payload menu request 能正常 roundtrip。
9. 長度非零但 payload 指標為空時，在寫出任何 wire bytes 前回傳 invalid argument。

底層 network I/O status 會明確映射為 protocol EOF、timeout、invalid argument 或 I/O error，供後續 server child 決定是否回覆 `ERROR` 或直接關閉。

## Final verification

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --clean-first
ctest --test-dir build --output-on-failure
ldd build/frame_tests
valgrind --leak-check=full --show-leak-kinds=all \
  --error-exitcode=99 build/frame_tests
```

結果：

- Debug clean build：PASS
- Debug CTest：4/4 PASS
- Release configure/build/CTest：4/4 PASS
- `frame_tests` 從 `build/libgame_common.so` 動態載入共用函式庫
- Valgrind：0 errors、結束時 0 bytes in 0 blocks

## Remaining work

下一個切片需建立 message direction／payload validation 與 connection game state machine，之後才能加入真正的 `server`、`client` 及 fork-per-connection 整合測試。本次結果仍不足以刪除 WSL 遷移備份。
