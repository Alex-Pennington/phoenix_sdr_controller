# Phoenix SDR Controller - Build Instructions

This document describes how to set up your development environment and build Phoenix SDR Controller from source.

## Prerequisites

### Required Software

| Software | Version | Purpose |
|----------|---------|---------|
| CMake | 3.16+ | Build system |
| vcpkg | Latest | Package manager |
| C Compiler | MSVC or MinGW | C11 support |
| Git | Any | Version control |
| PowerShell | 5.1+ | Build scripts |

### Required Libraries (via vcpkg)

| Library | Purpose |
|---------|---------|
| SDL2 | Window, graphics, input |
| SDL2_ttf | Font rendering |

---

## Development Environment Setup

### 1. Install vcpkg

```powershell
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap
.\bootstrap-vcpkg.bat

# Add to PATH (or set VCPKG_ROOT environment variable)
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
```

### 2. Install Dependencies

```powershell
vcpkg install sdl2:x64-windows sdl2-ttf:x64-windows
```

Or use manifest mode (automatic with CMake):
```json
// vcpkg.json (already in project root)
{
  "name": "phoenix-sdr-controller",
  "dependencies": ["sdl2", "sdl2-ttf"]
}
```

### 3. Install CMake

Option A - Using winget:
```powershell
winget install Kitware.CMake
```

Option B - Download from [cmake.org](https://cmake.org/download/)

---

## Building

### Quick Build (PowerShell)

```powershell
.\build.ps1                      # Debug build, increments build number
.\build.ps1 -Release             # Optimized build
.\build.ps1 -Clean               # Clean build directory first
.\build.ps1 -Clean -Release      # Clean rebuild in Release mode
```

### Manual CMake Build

```powershell
# Configure (first time)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

# Build
cmake --build build --config Release

# Run
.\build\phoenix_sdr_controller.exe
```

### Build Output

| File | Description |
|------|-------------|
| `build\phoenix_sdr_controller.exe` | Main application |
| `build\SDL2.dll` | SDL2 runtime |
| `build\SDL2_ttf.dll` | SDL2_ttf runtime |

---

## Project Structure

```
phoenix_sdr_controller/
├── build.ps1              # Local build script
├── deploy_release.ps1     # Release deployment script
├── CMakeLists.txt         # CMake configuration
├── vcpkg.json             # vcpkg dependencies
├── include/               # Header files
│   ├── version.h          # Auto-generated version info
│   ├── common.h           # Types, constants, colors
│   ├── tcp_client.h       # TCP client API
│   ├── sdr_protocol.h     # Protocol handler API
│   ├── app_state.h        # Application state API
│   ├── ui_core.h          # SDL2 rendering API
│   ├── ui_widgets.h       # Widget definitions
│   ├── ui_layout.h        # Layout manager API
│   └── process_manager.h  # External process control
├── src/                   # Source files
│   ├── main.c             # Application entry point
│   ├── tcp_client.c       # Winsock TCP implementation
│   ├── sdr_protocol.c     # Command protocol
│   ├── app_state.c        # State management
│   ├── ui_core.c          # SDL2 rendering
│   ├── ui_widgets.c       # Widget drawing/update
│   ├── ui_layout.c        # Main UI layout
│   └── process_manager.c  # Process spawning
├── docs/                  # Documentation
│   └── BUILD_AND_RELEASE.md
├── scripts/               # Utility scripts
└── .github/
    └── workflows/
        └── release.yml    # GitHub Actions CD
```

---

## Version Information

Version format: `MAJOR.MINOR.PATCH+BUILD.COMMIT[-dirty]`

Example: `0.2.0+5.abc1234` or `0.2.0+5.abc1234-dirty`

```powershell
# Bump version (resets build number to 0)
.\build.ps1 -Increment patch   # 0.2.0 → 0.2.1
.\build.ps1 -Increment minor   # 0.2.0 → 0.3.0
.\build.ps1 -Increment major   # 0.2.0 → 1.0.0

# Every build auto-increments the build number
.\build.ps1                     # 0.2.0+1 → 0.2.0+2
```

Version is stored in `include/version.h`:
```c
#define VERSION_MAJOR       0
#define VERSION_MINOR       2
#define VERSION_PATCH       0
#define VERSION_BUILD       5
#define VERSION_STRING      "0.2.0"
#define VERSION_FULL        "0.2.0+5.abc1234"
#define VERSION_COMMIT      "abc1234"
#define VERSION_DIRTY       false
```

---

## Running

### Start the Controller

```powershell
.\build\phoenix_sdr_controller.exe
```

The application connects to a Phoenix SDR server on `localhost:4535`.

### Configuration

Settings are saved to `phoenix_sdr_presets.ini` in the executable directory:
- Memory presets (M1-M5)
- Last used frequency, gain, etc.
- External process paths (Server, Waterfall)

### UI Controls

| Control | Action |
|---------|--------|
| Frequency display | Click digits to select, use tuning buttons |
| WWV buttons | Quick tune to WWV frequencies |
| M1-M5 | Memory presets (Ctrl+click to save) |
| DC button | Toggle +450 Hz DC offset |
| Server/Wfall | Launch/stop external processes |
| Connect | Connect to SDR server |
| Start/Stop | Control I/Q streaming |

---

## Troubleshooting

### "Cannot find SDL2.dll"
Ensure vcpkg installed SDL2 and CMake was configured with the vcpkg toolchain file.

### "CMake configuration failed"
Check that VCPKG_ROOT is set and the toolchain file path is correct:
```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

### "Connection failed" in application
Ensure the Phoenix SDR server is running on port 4535:
```powershell
# Test connection
Test-NetConnection -ComputerName localhost -Port 4535
```

### Build errors after pulling updates
Try a clean rebuild:
```powershell
.\build.ps1 -Clean
```

### Font rendering issues
Ensure SDL2_ttf is installed. The application looks for a system font (Consolas or Segoe UI).

---

## Releasing

See [docs/BUILD_AND_RELEASE.md](docs/BUILD_AND_RELEASE.md) for the full release workflow.

Quick release:
```powershell
.\deploy_release.ps1 -Patch      # Bug fix release (0.2.0 → 0.2.1)
.\deploy_release.ps1 -Minor      # Feature release (0.2.0 → 0.3.0)
.\deploy_release.ps1 -DryRun     # Preview without pushing
```

---

## Platform Support

Currently **Windows-only** due to:
- Winsock2 for TCP networking
- Windows Job Objects for process management
- Windows-specific path handling

Future cross-platform support would require:
- POSIX socket abstraction
- Platform-specific process management
- CMake platform detection
