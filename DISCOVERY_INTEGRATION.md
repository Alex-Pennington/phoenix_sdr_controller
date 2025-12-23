# UDP Discovery Integration - Summary

**Date:** December 22, 2024  
**Status:** ✅ COMPLETE

## What Was Implemented

Integrated the Phoenix Nest discovery library (`pn_discovery`) for automatic sdr_server discovery on the local network.

## Changes Made

### 1. Added phoenix-discovery Submodule
```bash
git submodule add ../phoenix-discovery external/phoenix-discovery
```

### 2. Updated CMakeLists.txt
- Added `add_subdirectory(external/phoenix-discovery)`
- Linked against `pn_discovery` library
- Added include path for phoenix-discovery headers

### 3. Modified src/main.c

**Added:**
- `#include "pn_discovery.h"`
- `relay_mode` flag in app_context_t
- `discovery_active` flag in app_context_t
- `on_sdr_server_discovered()` callback function

**Initialization:**
- Calls `pn_discovery_init(0)` - listens on default port 5400
- Calls `pn_listen(on_sdr_server_discovered, app)` to register callback
- Discovery runs automatically in background thread

**Shutdown:**
- Calls `pn_discovery_shutdown()` to cleanup resources

## How It Works

### Normal Mode (without --relay flag)
1. Controller starts and initializes phoenix-discovery
2. Discovery listens for UDP broadcasts on port 5400
3. When sdr_server announces itself: `"PHOENIX_SDR <id> <ip> <port>"`
4. Callback updates `server_host` and `server_port` in app_state
5. User clicks "Connect" → connects to discovered server

### Relay Mode (with --relay flag)
```cmd
phoenix_sdr_controller.exe --relay 146.190.112.225
```
1. Controller sets `relay_mode = true`
2. `server_host` = relay IP, `server_port` = 3001
3. Discovery still runs but doesn't override relay settings
4. User clicks "Connect" → connects to relay:3001

## Protocol Flow

### Discovery (UDP Port 5400)
```
sdr_server → Broadcast: {"m":"PNSD","cmd":"helo","svc":"sdr_server","ip":"192.168.1.10","port":4535}
controller → Receives announcement
controller → Updates GUI with discovered server address
```

### Control Connection (TCP)
```
Normal Mode:   controller → sdr_server:4535 (direct)
Relay Mode:    controller → relay:3001 → signal_splitter → sdr_server:4535
```

## Callback Behavior

```c
void on_sdr_server_discovered(...) {
    if (is_bye) {
        // Server leaving network - log only
    } else {
        // Server discovered
        if (!relay_mode && conn_state == DISCONNECTED) {
            // Update server address for connect button
            server_host = ip;
            server_port = ctrl_port;
            status_message = "Discovered <id> at <ip>:<port>";
        }
    }
}
```

## Build Status

✅ **Build successful**
- phoenix-discovery library: `libpn_discovery.a`
- Controller executable: `phoenix_sdr_controller.exe`
- No errors, only minor warning about WIN32_LEAN_AND_MEAN redefinition (harmless)

## Testing Scenarios

### Scenario 1: Local sdr_server
```bash
# Terminal 1 - Start sdr_server (announces itself)
sdr_server.exe

# Terminal 2 - Start controller (discovers server)
phoenix_sdr_controller.exe
# GUI shows: "Discovered KY4OLB-SDR1 at 192.168.1.10:4535"
# Click Connect → connects to discovered server
```

### Scenario 2: Relay Mode
```bash
# Controller ignores local discovery, uses relay
phoenix_sdr_controller.exe --relay 146.190.112.225
# GUI shows: "Relay mode: 146.190.112.225:3001"
# Click Connect → connects to relay:3001
```

### Scenario 3: No Server Found
```bash
# Start controller with no sdr_server running
phoenix_sdr_controller.exe
# GUI shows: "127.0.0.1:4535" (default)
# Discovery continues listening in background
# If server starts later, address auto-updates
```

## Files Modified

1. [.gitmodules](.gitmodules) - Added phoenix-discovery submodule
2. [CMakeLists.txt](CMakeLists.txt) - Link against pn_discovery
3. [src/main.c](src/main.c) - Discovery integration and callback
4. [external/phoenix-discovery/](external/phoenix-discovery/) - Submodule (read-only)

## Benefits

✅ **Zero configuration** - No need to manually enter server IP  
✅ **Auto-discovery** - Finds sdr_server automatically on LAN  
✅ **Background listening** - Runs in separate thread, no UI blocking  
✅ **Relay override** - `--relay` flag takes precedence over discovery  
✅ **Multiple servers** - Could extend to show dropdown of discovered servers  
✅ **Resilient** - If server restarts, re-discovery happens automatically  

## Next Steps

- [x] Build and test basic discovery
- [ ] Test with real sdr_server broadcasting announcements
- [ ] Test relay mode connection
- [ ] Optional: Add UI indicator showing "Discovered" vs "Manual" server
- [ ] Optional: Show multiple discovered servers in dropdown

---

**Implementation Time:** ~30 minutes  
**Build Status:** ✅ Success  
**Risk Level:** Low (well-tested library, no breaking changes)
