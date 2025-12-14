# Phoenix SDR Controller - Implementation Progress

**Project:** Phoenix SDR TCP Control Interface GUI
**Target:** Windows SDL2 Application
**Protocol Version:** 1.0
**Started:** December 14, 2025

---

## Current Status

| Component | Status | Files |
|-----------|--------|-------|
| Headers | ✅ Complete | 7 files in `include/` |
| Phase 1 Core | ✅ Complete | `tcp_client.c`, `app_state.c`, `sdr_protocol.c` |
| Phase 2 UI | ✅ Complete | `ui_core.c`, `ui_widgets.c`, `ui_layout.c` |
| Phase 3 Main | ✅ Complete | `main.c` |
| Build system | ✅ Ready | `CMakeLists.txt` |
| Phase 4 Testing | 🔄 In Progress | Build verification, testing |
| Documentation | ✅ Complete | `PROGRESS.md`, `.github/copilot-instructions.md` |

---

## Implementation Phases

### Phase 1: Core Infrastructure
- [x] **TCP Client** (`src/tcp_client.c`) ✅ Complete
  - Winsock initialization/cleanup
  - Connect to server (host:port)
  - Send command with newline
  - Receive response with timeout (5s)
  - Non-blocking check for async data
  - Graceful disconnect

- [x] **App State** (`src/app_state.c`) ✅ Complete
  - State structure management
  - Frequency formatting (grouped digits)
  - Frequency parsing (Hz/kHz/MHz)
  - Tuning step management
  - State sync from SDR status

- [x] **Protocol Handler - Core** (`src/sdr_protocol.c`) ✅ Complete
  - `PING` → `PONG`
  - `QUIT` → `BYE`
  - `SET_FREQ <hz>` / `GET_FREQ`
  - `SET_GAIN <db>` / `GET_GAIN`
  - `SET_LNA <state>` / `GET_LNA`
  - `START` / `STOP` / `STATUS`
  - Response parsing (OK, ERR codes)
  - Status string parsing (key=value pairs)

---

### Phase 2: SDL2 UI

- [x] **UI Core** (`src/ui_core.c`) ✅ Complete
  - SDL2 window (1024x700, resizable)
  - SDL2_ttf font loading
  - Color utilities (RGBA hex to SDL)
  - Drawing primitives:
    - Filled/outlined rectangles
    - Rounded rectangles
    - Lines
    - Horizontal/vertical gradients
  - Text rendering (left/center/right aligned)
  - Frame timing (60 FPS target)
  - Event polling

- [x] **UI Widgets** (`src/ui_widgets.c`) ✅ Complete
  - Button (normal/hover/active/disabled states)
  - Vertical slider (for gain/LNA)
  - Combo box dropdown (AGC/srate/BW/antenna)
  - Toggle switch (Bias-T/Notch)
  - LED indicator (connected/streaming/overload)
  - Frequency display (large digits, selectable)
  - S-Meter bar
  - Panel container with title

- [x] **UI Layout** (`src/ui_layout.c`) ✅ Complete
  - Layout regions calculation
  - Header bar (title, version, connection LED)
  - Frequency display panel
  - Tuning step controls (+/- buttons)
  - Gain panel (IF gain slider, LNA slider)
  - Config panel (AGC, sample rate, BW, antenna combos)
  - Hardware toggles (Bias-T with warning, FM Notch)
  - Streaming panel (Start/Stop, status LEDs)
  - Footer status bar

- [x] **Protocol Handler - Full** (`src/sdr_protocol.c`) ✅ Complete
  - `SET_AGC <mode>` / `GET_AGC` (OFF/5HZ/50HZ/100HZ)
  - `SET_SRATE <hz>` / `GET_SRATE`
  - `SET_BW <khz>` / `GET_BW`
  - `SET_ANTENNA <port>` / `GET_ANTENNA` (A/B/HIZ)
  - `SET_BIAST <ON|OFF>` (with CONFIRM)
  - `SET_NOTCH <ON|OFF>`
  - `VER` → version parsing
  - `CAPS` → capability structure parsing
  - `HELP` → command list

---

### Phase 3: Integration

- [x] **Main Application** (`src/main.c`) ✅ Complete
  - WinMain entry point
  - Initialize Winsock, SDL2, SDL2_ttf
  - Create tcp_client, sdr_protocol, app_state, ui_layout
  - Main event loop:
    - Poll SDL events
    - Update widgets from mouse/keyboard
    - Dispatch UI actions to protocol commands
    - Check for async notifications
    - Draw UI
    - Cap at 60 FPS
  - Periodic tasks:
    - Status poll every 500ms (when connected)
    - Keepalive PING every 60s (when idle)
  - Graceful shutdown (QUIT, cleanup)

- [x] **Async Notifications** (`src/sdr_protocol.c`) ✅ Complete
  - Non-blocking socket check
  - Parse `!` prefixed messages:
    - `! OVERLOAD DETECTED` → set overload LED
    - `! OVERLOAD CLEARED` → clear overload LED
    - `! GAIN_CHANGE GAIN=x LNA=y` → update gain display
    - `! DISCONNECT reason` → show message, reset state

---

### Phase 4: Polish & Testing

- [ ] **Build Configuration**
  - Verify CMakeLists.txt with vcpkg
  - Install SDL2 and SDL2_ttf via vcpkg
  - Test build on Windows

- [ ] **Testing**
  - Mock server with netcat for manual testing
  - Test all command round-trips
  - Verify UI responsiveness
  - Test reconnection handling
  - Test error display

- [ ] **Documentation**
  - README.md with:
    - Prerequisites (vcpkg, SDL2)
    - Build instructions
    - Usage guide
    - Screenshots
  - Keyboard shortcuts
  - .github/copilot-instructions.md

---

## File Inventory

### Headers (✅ Complete)
```
include/
├── common.h        - Types, constants, colors, macros
├── tcp_client.h    - TCP client API
├── sdr_protocol.h  - Protocol handler API
├── app_state.h     - Application state API
├── ui_core.h       - SDL2 rendering API
├── ui_widgets.h    - Widget definitions
└── ui_layout.h     - Layout manager API
```

### Sources (❌ To Create)
```
src/
├── main.c          - Application entry point
├── tcp_client.c    - Winsock TCP implementation
├── sdr_protocol.c  - Command protocol implementation
├── app_state.c     - State management
├── ui_core.c       - SDL2 rendering functions
├── ui_widgets.c    - Widget drawing/update
└── ui_layout.c     - Main UI layout
```

---

## Protocol Commands Reference

| Command | Args | Response | Implemented |
|---------|------|----------|-------------|
| `SET_FREQ` | `<hz>` | `OK` | ❌ |
| `GET_FREQ` | - | `OK <hz>` | ❌ |
| `SET_GAIN` | `<db>` | `OK` | ❌ |
| `GET_GAIN` | - | `OK <db>` | ❌ |
| `SET_LNA` | `<state>` | `OK` | ❌ |
| `GET_LNA` | - | `OK <state>` | ❌ |
| `SET_AGC` | `<mode>` | `OK` | ❌ |
| `GET_AGC` | - | `OK <mode>` | ❌ |
| `SET_SRATE` | `<hz>` | `OK` | ❌ |
| `GET_SRATE` | - | `OK <hz>` | ❌ |
| `SET_BW` | `<khz>` | `OK` | ❌ |
| `GET_BW` | - | `OK <khz>` | ❌ |
| `SET_ANTENNA` | `<port>` | `OK` | ❌ |
| `GET_ANTENNA` | - | `OK <port>` | ❌ |
| `SET_BIAST` | `<ON\|OFF>` | `OK` | ❌ |
| `SET_NOTCH` | `<ON\|OFF>` | `OK` | ❌ |
| `START` | - | `OK` | ❌ |
| `STOP` | - | `OK` | ❌ |
| `STATUS` | - | `OK key=val...` | ❌ |
| `PING` | - | `PONG` | ❌ |
| `VER` | - | `OK PHOENIX_SDR=...` | ❌ |
| `CAPS` | - | Multi-line | ❌ |
| `HELP` | - | `OK COMMANDS:...` | ❌ |
| `QUIT` | - | `BYE` | ❌ |

---

## SDR Limits (RSP2 Pro)

| Parameter | Min | Max | Unit |
|-----------|-----|-----|------|
| Frequency | 1,000 | 2,000,000,000 | Hz |
| Gain Reduction | 20 | 59 | dB |
| LNA State | 0 | 8 | - |
| Sample Rate | 2,000,000 | 10,000,000 | Hz |
| Bandwidth | 200 | 8,000 | kHz |

**Bandwidths:** 200, 300, 600, 1536, 5000, 6000, 7000, 8000 kHz
**Antennas:** A, B, HIZ
**AGC Modes:** OFF, 5HZ, 50HZ, 100HZ

---

## Next Steps

1. Create `src/` directory
2. Implement `tcp_client.c` (Winsock TCP)
3. Implement `app_state.c` (state management)
4. Implement `sdr_protocol.c` (core commands)
5. Implement `ui_core.c` (SDL2 basics)
6. Continue with remaining files...

---

*Last Updated: December 14, 2025*
