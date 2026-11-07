# Robustness Before/After Comparison — 2026-11-08

## Purpose

老師要求至少三組「沒有強健性機制」與「加入強健性機制」的相同輸入對照。本專案以 `examples/robustness_demo.c` 在本機 `socketpair()` 中隔離執行不安全基準，正式 server/client 不會連結或呼叫這些基準邏輯；改善後版本直接使用正式 `libgame_common.so` 的網路函式。

執行方式：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/robustness_demo
# 或由 CTest 自動判定
ctest --test-dir build --output-on-failure
```

實際輸出：

```text
case=fragmented-input unsafe=FAIL(short-read) robust=PASS(reassembled)
case=incomplete-input unsafe=HANG(blocked) robust=PASS(timeout)
case=closed-peer unsafe=CRASH(SIGPIPE) robust=PASS(error-returned)
robustness-comparison: PASS
```

## Example 1 — Fragmented TCP input

相同輸入：12-byte `GET_STATS` header 先送 4 bytes，100 ms 後再送剩餘 8 bytes。

| 版本 | 行為 |
| --- | --- |
| 未改善 | 只呼叫一次 `recv(fd, header, 12, 0)`，實際只取得 4 bytes；若直接解碼，header 不完整。 |
| 已改善 | `protocol_receive_frame()` → `network_read_exact()` 持續累積資料，在同一 absolute deadline 內重組完整 12 bytes，正確解碼 `GET_STATS`。 |

結論：TCP 是 byte stream，不保證一次 `recv()` 對應一個 application message；完整讀取迴圈可處理 partial read。

## Example 2 — Incomplete/slow client

相同輸入：送出 header 前 2 bytes `AB` 後保持 connection 開啟，不再傳送剩餘 10 bytes。

| 版本 | 行為 |
| --- | --- |
| 未改善 | 雖然使用迴圈等待讀滿 12 bytes，但沒有 deadline；觀察 200 ms 後 process 仍永久阻塞，示範程式必須隔離終止它。 |
| 已改善 | `protocol_receive_frame()` 使用 absolute deadline；150 ms 到期後回傳 `PROTOCOL_STATUS_TIMEOUT`，child 可回覆錯誤並釋放 connection。 |

結論：只處理 partial read 仍不足以抵抗 slow/incomplete client；absolute deadline 可防止對方用少量 bytes 無限占用 child。

## Example 3 — Peer closes before write

相同輸入：對方已關閉 socket，本端嘗試送出 4 bytes `ABGM`。

| 版本 | 行為 |
| --- | --- |
| 未改善 | 一般 `send(..., 0)` 觸發 `SIGPIPE`，process 以 signal 13 終止。若發生在 server child，該 session 會非預期崩潰。 |
| 已改善 | `network_write_all()` 使用 `MSG_NOSIGNAL`；process 保持存活並取得 `NETWORK_IO_STATUS_SYSTEM_ERROR`，能執行清理與正常結束。 |

結論：client 可在任何時點斷線；抑制 `SIGPIPE` 並把狀況轉成可處理的錯誤，能避免非預期 process termination。

## Verification result

- 每個案例都使用相同輸入分別執行 unsafe baseline 與 production mechanism。
- 驗收腳本：`tests/test_robustness_demo.sh`
- 為確認 timing 穩定性，三組案例連續執行 10 次皆 PASS。
- 完整 CTest：6/6 PASS。
- 不安全程式碼只存在於獨立 `robustness_demo` target，不會進入 `server` 或 `client`。
