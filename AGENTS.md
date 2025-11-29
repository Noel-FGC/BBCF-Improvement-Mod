# BBCF Improvement Mod – Agent Handbook

This repository builds the `dinput8.dll` proxy that injects the BBCF Improvement Mod into BlazBlue Centralfiction. The mod hooks the game's DirectInput entry point, loads user settings, installs runtime hooks, and initializes overlay/gameplay managers before handing control back to the game.

## Build requirements and outputs
- **Tooling:** Visual Studio 2019 with the v142 toolset and Windows 10 SDK. The solution is already configured for Win32 builds of the DLL target named `dinput8`.
- **Included third-party libraries:** Detours (hooking), legacy DirectX 9 SDK headers/libs, Steamworks SDK headers and redistributable, ImGui sources, and a WinHTTP client wrapper; include and library paths are wired in the project file, so no external installs are needed.
- **Outputs:** The build places `dinput8.dll` into `bin/<Configuration>/` (e.g., `bin/Release/`). Copy that DLL plus `settings.ini` and `palettes.ini` (templates in `resource/`) into the BBCF installation directory alongside `BBCF.exe`.
- **Exports:** The `export/dinput8.def` file defines the forwarded DirectInput entry so the proxy can stand in for the system `dinput8.dll`.

## Repository layout
- **README.md / USER_README.txt:** High-level feature list, installation/usage notes, and historical changelog for users.
- **depends/**: Vendored third-party headers and libraries (Detours, DirectX 9, Steam API, ImGui, WinHTTP client).
- **export/**: DEF file for the exported DirectInput symbol.
- **resource/**: Sample `settings.ini` and `palettes.ini` distributed with the mod.
- **src/Core/**: Entry point (`dllmain.cpp`), settings loader, logger, crash dump handler, utility helpers, and shared interface structures.
- **src/Hooks/**: Signature-scanned and address-based function hooks plus hook registration helpers.
- **src/D3D9EXWrapper/**: Lightweight wrappers around Direct3D9Ex device/sprite/effect interfaces used by the overlay and rendering tweaks.
- **src/Overlay/**: ImGui-based UI manager and utilities for rendering the mod's windows and HUD overlays.
- **src/Palette/**: Palette manager and helpers for loading, storing, and applying character palettes/effects.
- **src/CustomGameMode/**: Definitions and manager for custom game modes and their runtime control.
- **src/Network/**: Networking helpers for online palettes, game mode coordination, room handling, and replay uploads.
- **src/SteamApiWrapper/**: Thin wrappers around the Steamworks API used by networking/room features.
- **src/Web/**: Update-check and replay-upload HTTP helpers.
- **src/Game/**: Game-side data structures (characters, stages, match state) and replay/room helpers.
- **notes.h**: Developer research notes about scene control and networking addresses.

## Initialization and shutdown flow
1. `DllMain` spawns a startup thread that opens the logger, creates the `BBCF_IM` data folder, and installs a crash handler.
2. Settings are loaded from `settings.ini`; saved settings are initialized and logged for debugging.
3. The proxy loads the original (or user-specified) `dinput8.dll` and resolves `DirectInput8Create`. Failure to load aborts startup with a message box.
4. Detour hooks are applied via `placeHooks_detours()`, then the palette manager is constructed. The exported `DirectInput8Create` forwards calls to the original DLL while logging.
5. On process detach, overlay and manager singletons are shut down, interfaces are cleaned up, and the original `dinput8.dll` handle is freed.

## Configuration
- Place `settings.ini` next to `BBCF.exe`; the mod reads it on startup. Templates live in `resource/` for convenience.
- `src/Core/settings.def` defines every recognized setting and its default. Highlights:
  - UI and feature toggles (e.g., `ToggleButton`, `ToggleOnlineButton`, `ToggleHUDButton`).
  - State-saving and replay control hotkeys (`SaveStateKeybind`, `LoadStateKeybind`, `LoadReplayStateKeybind`, `freezeFrameKeybind`, `stepFramesKeybind`).
  - Rendering overrides (`RenderingWidth`, `RenderingHeight`, `Viewport`, `AntiAliasing`, `V-sync`).
  - Overlay sizing and notifications (`MenuSize`, `Notifications`, `FrameHistory*`).
  - Network/replay upload options (`UploadReplayData*`, `autoArchive`, `LoadForeignPalettesToggleDefault`).
- `settings.ini` changes do not require recompilation; restart the game to apply.

## Hooking and interfaces
- `src/Hooks/HookManager.*` centralizes signature scanning and JMP patching. Hooks can be registered by pattern or explicit address, toggled on/off, and store original bytes for restoration.
- `src/Core/interfaces.h` aggregates pointers to major managers (Steam API wrappers, network handlers, palette/game mode managers, replay systems, and Direct3D device wrappers) along with live game-state pointers (HUD visibility flags, match timer/rounds, entity list, stage/music selectors, etc.). These shared structures are populated during startup so hooks and overlays can coordinate game state.

## Major features and where they live
- **Overlay & HUD tools:** ImGui-driven UI for toggling mod features, frame history meter, hitbox overlays, and configuration panels (`src/Overlay/`).
- **Palette system:** Loading, assigning, and synchronizing custom palettes/effects both locally and online (`src/Palette/`, `src/Network/OnlinePaletteManager.*`).
- **Custom game modes:** Offline/online coordination of alternate rulesets via `src/CustomGameMode/` and `src/Network/OnlineGameModeManager.*`.
- **Replay tooling:** Replay takeover, upload/archiving, and rewind utilities in `src/Network/ReplayUploadManager.*` and `src/Game/ReplayRewind/`.
- **Networking/rooms:** Steam matchmaking/networking helpers and room coordination in `src/Network/` plus Steam API wrappers in `src/SteamApiWrapper/`.
- **Rendering tweaks:** Direct3D9Ex wrapper layer enabling resolution, viewport, and HUD visibility adjustments (`src/D3D9EXWrapper/` alongside relevant hook sites).

## Release preparation checklist
1. Build `Release|Win32` to generate `bin/Release/dinput8.dll`.
2. Copy `dinput8.dll`, `settings.ini`, and `palettes.ini` into the BBCF install directory. The mod will create its own `BBCF_IM` folder for palettes and saved data on first launch.
3. Verify hooks and overlay load by launching the game and toggling the mod menu (default `F1`).
