# Phoenix SDR Controller - Build and Release System

## Overview

This project uses a PowerShell build script (`scripts/build.ps1`) for local development and GitHub Actions for automated releases. The system is designed to keep local version numbers **in sync** with released versions.

---

## Version Scheme

Version format: `MAJOR.MINOR.PATCH.BUILD-COMMIT`

| Component | Purpose | When Incremented |
|-----------|---------|------------------|
| **MAJOR** | Breaking changes | Manual (`.\build.ps1 major`) |
| **MINOR** | New features (backward compatible) | Manual (`.\build.ps1 minor`) |
| **PATCH** | Bug fixes | Manual (`.\build.ps1 patch`) |
| **BUILD** | Development builds | Automatic on each `.\build.ps1` |
| **COMMIT** | Git short hash | Automatic (read from git) |

**Examples:**
- `0.1.0` - Release version (displayed to users)
- `0.1.0.5` - Internal build 5 of version 0.1.0
- `0.1.0.5-abc1234` - Full version with commit hash

---

## Files

| File | Purpose |
|------|---------|
| `include/version.h` | Auto-generated C header with version defines |
| `scripts/build.ps1` | PowerShell build/version/release script |
| `.github/workflows/release.yml` | GitHub Actions CD workflow |

### version.h Defines

```c
#define VERSION_MAJOR       0
#define VERSION_MINOR       1
#define VERSION_PATCH       0
#define VERSION_BUILD       5
#define VERSION_COMMIT      "abc1234"
#define VERSION_STRING      "0.1.0"           // For display
#define VERSION_FULL        "0.1.0.5"         // With build number
#define VERSION_DETAILED    "0.1.0.5-abc1234" // Full debug info
```

---

## Build Script Commands

```powershell
cd d:\claude_sandbox\phoenix_sdr_controller

# Build (auto-increments build number)
.\scripts\build.ps1

# Bump versions (local only, commits to git)
.\scripts\build.ps1 patch    # 0.1.0 → 0.1.1
.\scripts\build.ps1 minor    # 0.1.0 → 0.2.0
.\scripts\build.ps1 major    # 0.1.0 → 1.0.0

# Create release (tags and pushes, triggers GitHub Actions)
.\scripts\build.ps1 tag

# Clean build directory
.\scripts\build.ps1 clean
```

---

## Workflows

### Daily Development

```powershell
# Make code changes...

.\scripts\build.ps1           # Build (build number: 0.1.0.1 → 0.1.0.2)
.\build\phoenix_sdr_controller.exe  # Test

# Make more changes...

.\scripts\build.ps1           # Build again (0.1.0.2 → 0.1.0.3)
```

The build number increments automatically. This helps track which exact build you're testing.

### Creating a Release

```powershell
# 1. Bump version (choose one)
.\scripts\build.ps1 patch     # For bug fixes
.\scripts\build.ps1 minor     # For new features
.\scripts\build.ps1 major     # For breaking changes

# This:
# - Updates version.h (e.g., 0.1.0 → 0.1.1)
# - Resets build number to 0
# - Commits the change locally

# 2. (Optional) Build and test locally
.\scripts\build.ps1
.\build\phoenix_sdr_controller.exe

# 3. Push the release tag
.\scripts\build.ps1 tag

# This:
# - Reads current version from version.h (0.1.1)
# - Creates annotated git tag (v0.1.1)
# - Pushes commits and tag to origin
# - GitHub Actions automatically builds and publishes release
```

### Key Principle: Version Sync

The `tag` command does **NOT** change the version number. It tags whatever version is currently in `version.h`. This ensures:

1. Your local `version.h` matches the released version
2. When users report bugs, their version matches your code
3. No version drift between local and remote

---

## GitHub Actions Release Workflow

When a tag matching `v*` is pushed, the workflow:

1. Checks out the tagged commit
2. Sets up vcpkg with SDL2 and SDL2_ttf
3. Builds the Release configuration
4. Packages executable + DLLs into a ZIP
5. Generates artifact attestation (provenance)
6. Creates a GitHub Release with the ZIP attached

### Workflow Triggers

```yaml
on:
  push:
    tags:
      - 'v*'  # v0.1.0, v1.0.0, etc.
```

### Artifact Attestation

The release includes cryptographic attestation proving:
- The artifact was built by GitHub Actions
- From this specific repository
- At this specific commit

Users can verify with:
```bash
gh attestation verify phoenix_sdr_controller-v0.1.1-windows-x64.zip \
  --owner Alex-Pennington
```

---

## Version Comparison Examples

| Scenario | Local version.h | Released Tag | In Sync? |
|----------|-----------------|--------------|----------|
| Just released v0.1.1 | 0.1.1.0 | v0.1.1 | ✅ Yes |
| After 3 dev builds | 0.1.1.3 | v0.1.1 | ✅ Yes (same base) |
| Bumped to 0.1.2 | 0.1.2.0 | v0.1.1 | ✅ Yes (ready to tag) |
| Wrong: tag incremented | 0.1.1.0 | v0.1.2 | ❌ No (out of sync!) |

---

## Error Handling

### `tag` command errors

**"Uncommitted changes detected"**
```
ERROR: You have uncommitted changes. Commit or stash them first.
```
Fix: Commit or stash your changes before tagging.

**"Tag already exists"**
```
ERROR: Tag v0.1.1 already exists!
```
Fix: Bump the version first (`.\build.ps1 patch`), then tag.

---

## For AI Coding Agents

When working on this project:

1. **Building**: Run `.\scripts\build.ps1` to build. This auto-increments the build number.

2. **Don't manually edit version.h**: It's auto-generated. Use the build script.

3. **Version bumps**: Use `.\scripts\build.ps1 patch/minor/major` to bump versions.

4. **Releases**: The human will run `.\scripts\build.ps1 tag` when ready to release.

5. **Version in code**: Use these constants from `common.h`:
   ```c
   APP_VERSION      // "0.1.1" - for display
   APP_VERSION_FULL // "0.1.1.5-abc1234" - for debug/logs
   ```

---

## Prerequisites

- **CMake** 3.16+
- **vcpkg** with packages:
  ```bash
  vcpkg install sdl2:x64-windows sdl2-ttf:x64-windows
  ```
- **Git** (for commit hash and tagging)
- **PowerShell** 5.1+ (included with Windows)

---

## Quick Reference

| Task | Command |
|------|---------|
| Build | `.\scripts\build.ps1` |
| Build (clean) | `.\scripts\build.ps1 clean; .\scripts\build.ps1` |
| Bump patch | `.\scripts\build.ps1 patch` |
| Bump minor | `.\scripts\build.ps1 minor` |
| Bump major | `.\scripts\build.ps1 major` |
| Release | `.\scripts\build.ps1 tag` |
| View current version | `Get-Content include\version.h` |

---

## Release Checklist

- [ ] All changes committed
- [ ] Tests pass locally
- [ ] Version bumped appropriately (`patch`/`minor`/`major`)
- [ ] CHANGELOG updated (if applicable)
- [ ] Run `.\scripts\build.ps1 tag`
- [ ] Verify release at GitHub Actions
- [ ] Download and test release artifact

---

*Last Updated: December 15, 2025*
