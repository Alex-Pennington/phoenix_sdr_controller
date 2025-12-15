# Phoenix SDR Controller - Build and Release Guide

This document describes the versioning, build, and release workflow for Phoenix SDR Controller.

## Version Format

```
MAJOR.MINOR.PATCH+BUILD.COMMIT[-dirty]
```

**Examples:**
- `0.3.0+67.abc1234` - Clean build #67 from commit abc1234
- `0.3.0+67.abc1234-dirty` - Build with uncommitted local changes

| Component | Meaning |
|-----------|---------|
| MAJOR | Breaking changes, major features |
| MINOR | New features, backward compatible |
| PATCH | Bug fixes |
| BUILD | Auto-incremented every build |
| COMMIT | Git short hash (7 chars) |
| -dirty | Uncommitted changes present |

---

## Local Development

### Building

```powershell
.\build.ps1                      # Debug build, auto-increments BUILD
.\build.ps1 -Release             # Optimized build
.\build.ps1 -Clean               # Delete build/ directory first
.\build.ps1 -Clean -Release      # Clean rebuild in Release mode
```

### Incrementing Version

```powershell
.\build.ps1 -Increment patch     # 0.3.0 → 0.3.1, resets BUILD to 0
.\build.ps1 -Increment minor     # 0.3.0 → 0.4.0
.\build.ps1 -Increment major     # 0.3.0 → 1.0.0
```

### Version File

The version is stored in `include/version.h` and auto-updated by the build script:

```c
#define VERSION_MAJOR       0
#define VERSION_MINOR       3
#define VERSION_PATCH       0
#define VERSION_BUILD       67

#define VERSION_STRING      "0.3.0"
#define VERSION_FULL        "0.3.0+67.abc1234"
#define VERSION_COMMIT      "abc1234"
#define VERSION_DIRTY       false
```

---

## Creating a Release

### Prerequisites

1. All changes committed (no dirty state)
2. Tests passing
3. On the branch you want to release from

### Release Commands

```powershell
.\deploy_release.ps1             # Increment BUILD only
.\deploy_release.ps1 -Patch      # 0.3.0 → 0.3.1 (bug fix release)
.\deploy_release.ps1 -Minor      # 0.3.0 → 0.4.0 (feature release)
.\deploy_release.ps1 -Major      # 0.3.0 → 1.0.0 (breaking changes)
.\deploy_release.ps1 -DryRun     # Preview without pushing
```

### What `deploy_release.ps1` Does

1. **Validates** - Checks for uncommitted changes (fails if dirty)
2. **Updates** - Writes new version to `include/version.h`
3. **Commits** - Creates commit with message `v0.3.1 build 1`
4. **Amends** - Re-commits with correct git hash embedded
5. **Pushes** - Pushes to origin
6. **Tags** - Creates and pushes tag `v0.3.1+1.abc1234`

The tag push triggers GitHub Actions to build and publish the release.

---

## GitHub Actions CI/CD

### Release Build (`release.yml`)

**Triggers:**
- Push of tags matching `v*` (e.g., `v0.3.0+5.abc1234`)

**Steps:**
1. Checkout code
2. Setup vcpkg with SDL2 dependencies
3. Build with Release configuration
4. Package release:
   - `phoenix_sdr_controller.exe`
   - `SDL2.dll`, `SDL2_ttf.dll`
   - `README.txt`
5. Create zip archive
6. Generate artifact attestation
7. Create GitHub Release with attached zip

**Prerelease Detection:**
Tags containing `-alpha`, `-beta`, or `-rc` are marked as prereleases.

---

## Workflow Diagram

```
Local Development                    GitHub
─────────────────                    ──────

.\build.ps1                          (no CI on push currently)
    │
    ▼
Build #++ locally
    │
git add & commit
    │
    ▼
.\deploy_release.ps1 -Patch
    │
    ├── Update version.h
    ├── Commit "v0.3.1 build 1"
    ├── Create tag v0.3.1+1.abc1234
    └── Push tag ─────────────────────► release.yml runs
                                            │
                                            ├── Build -Release
                                            ├── Package zip
                                            └── Create GitHub Release
                                                     │
                                                     ▼
                                            phoenix_sdr_controller-v0.3.1-win64.zip
                                            available for download
```

---

## Quick Reference

| Task | Command |
|------|---------|
| Debug build | `.\build.ps1` |
| Release build | `.\build.ps1 -Release` |
| Clean build | `.\build.ps1 -Clean` |
| Bump patch version | `.\build.ps1 -Increment patch` |
| Bump minor version | `.\build.ps1 -Increment minor` |
| Preview release | `.\deploy_release.ps1 -DryRun` |
| Bug fix release | `.\deploy_release.ps1 -Patch` |
| Feature release | `.\deploy_release.ps1 -Minor` |
| Breaking change release | `.\deploy_release.ps1 -Major` |

---

## Version Verification

After a release, verify the tag and version match:

```powershell
# Check local version
Get-Content include\version.h | Select-String "VERSION_FULL"

# Check git tags
git tag -l "v*"

# Check remote releases
gh release list
```
