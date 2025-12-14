# Phoenix SDR Controller - AI Agent Instructions

## Project Overview

A Windows SDL2 GUI application for remote control of the Phoenix SDR (SDRplay RSP2 Pro) via a text-based TCP protocol on port 4535. The project is in early development—headers are complete but source files in `src/` need to be implemented.

## Architecture

```
┌──────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│   UI Layer       │────▶│  Protocol Layer  │────▶│  TCP Client     │
│  (SDL2/ui_*.c)   │     │ (sdr_protocol.c) │     │ (tcp_client.c)  │
└──────────────────┘     └──────────────────┘     └─────────────────┘
         ▲                        ▲
         │                        │
         └────────┬───────────────┘
                  │
          ┌───────▼───────┐
          │   App State   │ (app_state.c - central state management)
          └───────────────┘
```

- **tcp_client**: Winsock-based TCP socket handling with platform abstractions in `common.h`
- **sdr_protocol**: Command/response protocol parser implementing the spec in [Phoenix SDR TCP Control Interface.md](../Phoenix%20SDR%20TCP%20Control%20Interface.md)
- **app_state**: Central state management—SDR parameters, connection status, UI mode
- **ui_core/widgets/layout**: SDL2-based rendering with reusable widget system

## Key Patterns

### Protocol Commands
All commands follow: `COMMAND [ARGS]\n` → `OK [value]\n` or `ERR <CODE> [msg]\n`
- Frequencies are always in **Hz** (not kHz/MHz)
- Async notifications are prefixed with `!` (e.g., `! OVERLOAD DETECTED\n`)
- Parse responses expecting: `OK`, `OK <value>`, `OK key=val key=val`, `ERR CODE msg`, `PONG`, `BYE`

### SDR Limits (RSP2 Pro)
```c
FREQ_MIN=1000, FREQ_MAX=2000000000 (Hz)
GAIN_MIN=20, GAIN_MAX=59 (dB reduction)
LNA_MIN=0, LNA_MAX=8
SRATE_MIN=2000000, SRATE_MAX=10000000 (Hz)
```

### Platform Abstraction
Use `common.h` socket typedefs for cross-platform compatibility:
```c
socket_t       // SOCKET on Windows, int on Unix
INVALID_SOCK   // INVALID_SOCKET or -1
CLOSE_SOCKET() // closesocket() or close()
```

### UI Color Constants
Use predefined colors from `common.h`: `COLOR_BG_DARK`, `COLOR_ACCENT`, `COLOR_GREEN`, etc.

## Build System

```bash
# Prerequisites: vcpkg with SDL2 and SDL2_ttf
vcpkg install sdl2:x64-windows sdl2-ttf:x64-windows

# Configure and build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## Implementation Status

Check [PROGRESS.md](../PROGRESS.md) for current implementation status. Headers in `include/` define all APIs—source files in `src/` need to be created following these interfaces exactly.

## Critical Implementation Notes

1. **Single-client model**: Server accepts one connection; additional connections get `ERR BUSY\n`
2. **Bias-T safety**: `SET_BIAST ON` requires `SET_BIAST ON CONFIRM` to actually enable (supplies DC voltage)
3. **Streaming state**: Sample rate/bandwidth cannot be changed while streaming—call `STOP` first
4. **Status polling**: Poll `STATUS` every 500ms when connected; send `PING` every 60s when idle
5. **Widget pattern**: Init widget → update with mouse state → draw. Updates return `true` if value changed.

## File Conventions

- All source files should `#include` their corresponding header first
- Use `LOG_INFO/WARN/ERROR/DEBUG` macros for logging
- Use `CLAMP()` and `ARRAY_SIZE()` macros from `common.h`
- Function naming: `module_action()` (e.g., `tcp_client_connect()`, `sdr_set_freq()`)
