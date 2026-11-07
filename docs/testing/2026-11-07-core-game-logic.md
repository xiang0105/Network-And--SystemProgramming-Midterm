# Core Game Logic Test Record — 2026-11-07

## Scope

本次測試驗證 Ubuntu 24.04 開發環境、CMake／CTest 建置流程、`game_common` 動態共用函式庫，以及已確認的四位數驗證與 A/B 計分規則。

本次不包含尚未實作的 `server`、`client`、`fork()` connection handling、正式 10-client concurrency、封包擷取、debug log、system information 或完整 robustness 整合測試。

## Environment smoke test

- Ubuntu 24.04.5 LTS、x86_64、使用者與專案存取正常。
- GCC 13.3、CMake／CTest 3.28.3、Valgrind 3.22、socat、tcpdump 與 GDB 可執行。
- 臨時 C 程式可建立並動態載入 `.so`；Valgrind 結果為 0 errors。
- 使用 socat 在 `127.0.0.1:19735` 同時建立 10 條 TCP echo connections，結果 10/10 通過。這只驗證環境能力，不是正式專案 concurrency 證據。

第一次 smoke 指令曾因 PowerShell 將 Linux shell 變數錯誤解析而失敗；改用不跨 shell 展開的固定臨時路徑後測試通過。該失敗不涉及專案程式或 Ubuntu 工具鏈。

## Test-first behavior cycles

每個行為都先執行會失敗的測試，再加入最小實作並確認轉為通過：

1. `0123` 是有效的四位代碼，前導 `0` 不會遺失。
2. 長度不是 4 時拒絕。
3. 輸入 buffer 為空指標時拒絕。
4. 包含非數字字元時拒絕。
5. 包含重複數字時拒絕。
6. `0123` 與 `0123` 計為 `4A0B`。
7. `0123` 與 `0132` 計為 `2A2B`。
8. 計分介面拒絕非法猜測。
9. 計分輸出指標為空時拒絕；加入防護前測試得到 SegFault，加入防護後正常回傳失敗。

## Final verification

使用以下命令進行 Debug 建置與測試：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --clean-first
ctest --test-dir build --output-on-failure
```

結果：

- Debug clean build：PASS
- Debug CTest：1/1 PASS
- Release clean configure/build/CTest：PASS
- `ldd build/game_common_tests`：從 `build/libgame_common.so` 載入 `game_common`
- Valgrind：0 errors、結束時 0 bytes in 0 blocks

## Backup decision

這不是完整 client-server 專案測試。WSL 遷移備份必須繼續保留，直到 `server`、`client`、CTest 與必要整合測試全部完成並保存結果。
