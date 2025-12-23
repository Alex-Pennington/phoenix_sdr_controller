# Phoenix SDR Controller

A Windows SDL2 GUI application for remote control of the Phoenix SDR (SDRplay RSP2 Pro) via a text-based TCP protocol on port 4535.

## Build & Release Workflow

This project uses the phoenix-build-scripts submodule for all install, build, and release operations.

- Submodule: `external/phoenix-build-scripts`
- CMake integration: add after `project(...)` → `include(external/phoenix-build-scripts/cmake/phoenix-build.cmake)`

### Install / Initialize

```powershell
./external/phoenix-build-scripts/scripts/init-project.ps1 \
  -ProjectName "phoenix-sdr-controller" \
  -GitHubRepo "Alex-Pennington/phoenix_sdr_controller" \
  -Executables @("phoenix_sdr_controller.exe") \
  -DLLs @("SDL2.dll","SDL2_ttf.dll") \
  -PackageFiles @("README.md","LICENSE","docs/")
```

### Development Build

```powershell
./external/phoenix-build-scripts/scripts/deploy-release.ps1
```

### Release Build

```powershell
./external/phoenix-build-scripts/scripts/deploy-release.ps1 -IncrementPatch
```

### Deploy to GitHub

```powershell
./external/phoenix-build-scripts/scripts/deploy-release.ps1 -IncrementPatch -Deploy
```

Version format: MAJOR.MINOR.PATCH+BUILD.COMMIT[-dirty]
