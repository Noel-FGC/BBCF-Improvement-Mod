# Package to send for controller hotplug RE help

Use this pack when replying to the “ancient cursed C++ input wizard” so they can give code-level guidance on forcing controller hotplug recognition.

## What to share from the repo
- **DirectInput proxy and wrappers:** `src/Core/dllmain.cpp` (DirectInput8Create export/forwarder) and `src/Core/DirectInputWrapper.cpp`/`.h` (IDirectInput8A/W wrappers, EnumDevices ordering, CreateDevice tracking).
- **Refresh button plumbing:** `src/Core/ControllerOverrideManager.cpp`/`.h` (device enumeration, tracked device list, RefreshDevicesAndReinitializeGame, WM_DEVICECHANGE broadcast) and `src/Overlay/Window/MainWindow.cpp` (UI hook for the Refresh controllers button and the keyboard/controller swap toggle).
- **Reverse-engineering headers:** `notes.h` and `src/Game/GhidraDefs.h` (base-address helper, RE offsets) plus the inline `battle_key_controller` pointer swap in `MainWindow.cpp` for context on prior RE work.
- **Logs/diagnostics:** Any runtime log captured when pressing Refresh (shows device enumeration + acquire/unacquire results) from `BBCF_IM/log.txt` if available; otherwise, mention current logging gaps.

## What to answer from the RE tooling side
- **Binary/load info:** Confirm you’re loading `BBCF.exe` as a 32-bit process and using its default base address (matching existing `GetBbcfBaseAdress` usage).
- **Tools you can use:** Let them know if you can run Cheat Engine, x64dbg/VS debugger, and Ghidra; these are needed for pointer scans + stack traces.
- **Current knowledge of refresh hook:** Explain that RefreshDevicesAndReinitializeGame currently only re-enumerates, re-acquires tracked `IDirectInputDevice8` handles, and posts `WM_DEVICECHANGE`, but BBCF still ignores new devices.

## How to respond to each numbered request in their message
1. **Provide code paths they asked for (1.1 A–D):** point them to the files above, noting that CreateDevice hooks live in `DirectInputWrapper.cpp` and the refresh routine lives in `ControllerOverrideManager::RefreshDevicesAndReinitializeGame`, triggered from the overlay button in `MainWindow.cpp`.
2. **Share RE assets (1.2 E):** confirm `src/Game/GhidraDefs.h` and `notes.h` are the only shipped RE artifacts; no full decompiled BBCF source exists.
3. **State your tooling comfort (1.3):** list which of Cheat Engine / x64dbg / VS debugger / Ghidra you can run, and confirm the game build (Win32) to guide their pointer-size assumptions.
4. **Align on the gameplan:** Acknowledge their checklist and say you’ll instrument CreateDevice/Acquire logs, scan for `IDirectInputDevice8*` pointers to locate the game’s device table, and map that address into Ghidra to find init/refresh functions before trying Strategy A (call the init) or Strategy B (patch the table).

## Optional extras to send if available
- Screenshots or snippets of `BBCF_IM/log.txt` showing CreateDevice + Acquire logs before/after pressing Refresh.
- Any pointer addresses you’ve already spotted for `battle_key_controller` or suspected pad tables (RVA and absolute address), even if unconfirmed.

Sending the above should give them everything they asked for so they can recommend the exact hook/patch points for rebuilding BBCF’s internal controller list when Refresh is used.
