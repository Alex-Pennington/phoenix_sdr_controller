# Relay Latency Optimization Notes

## Controller-Side Fixes Applied

### ✅ TCP_NODELAY Enabled
```c
// Disable Nagle's algorithm for immediate packet transmission
int nodelay = 1;
setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
```

**Impact:** Reduces latency by 40-200ms per command by disabling packet coalescing.

### ✅ Non-blocking Connect
Already implemented - prevents UI freeze during connection.

### ✅ Reduced Polling Frequency
Relay mode polls every 10 seconds instead of 500ms to avoid blocking UI thread.

## Expected Latency After Fixes

| Hop | Expected RTT | Notes |
|-----|-------------|-------|
| Controller → Relay | 50-150ms | Internet latency |
| Relay → Signal Splitter | 1-5ms | Should be fast passthrough |
| Splitter → sdr_server | 1-5ms | Local connection |
| **Total Round-Trip** | **55-160ms** | Acceptable for interactive use |

## Potential Issues in Proxy Chain

If latency is still >200ms per command, check:

### 1. Signal Relay (cloud hub)
- ❌ **Nagle's algorithm enabled** - Add TCP_NODELAY on both client and server sockets
- ❌ **Buffered I/O** - Use unbuffered reads/writes
- ❌ **Blocking operations** - Should use non-blocking or async I/O
- ❌ **Thread contention** - Each client should have dedicated thread

**Fix:**
```python
# In relay server code
client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
server_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
client_socket.setblocking(False)  # Use select/epoll for multiplexing
```

### 2. Signal Splitter (edge node)
- ❌ **Nagle's algorithm** - Set TCP_NODELAY
- ❌ **Synchronous passthrough** - Use async I/O or separate threads
- ❌ **Command parsing delay** - Should be zero-copy passthrough for control commands

**Fix:**
```c
// In signal_splitter control connection
setsockopt(ctrl_sock, IPPROTO_TCP, TCP_NODELAY, 1);
// Immediate relay - don't wait for buffer to fill
write(sdr_sock, buffer, n);
```

### 3. sdr_server
- ✅ Should be fast (local connection)
- Check if it's doing heavy processing during STATUS command

## Testing Latency

### Measure Controller → Relay
```bash
# From controller machine
ping -n 10 146.190.112.225
```

### Measure Each Hop
```bash
# On relay server
ping signal_splitter_ip

# On signal_splitter
ping sdr_server_ip
```

### Measure Command Round-Trip
Add timing to controller:
```c
uint32_t start = SDL_GetTicks();
sdr_get_status(proto);
uint32_t elapsed = SDL_GetTicks() - start;
LOG_INFO("STATUS command took %u ms", elapsed);
```

## Recommended Architecture Changes

### Option 1: Direct Relay Passthrough
```
Controller → Relay (TCP passthrough only)
              ↓
         sdr_server (direct)
```
Eliminates signal_splitter hop.

### Option 2: UDP Signaling
```
Controller → Relay (UDP for commands)
              ↓
         sdr_server
```
UDP avoids TCP overhead, trades reliability for latency.

### Option 3: WebSocket with Binary Framing
Lower overhead than TCP for small messages, built-in compression.

## Summary

**Expected improvement with TCP_NODELAY:**
- Before: 500ms+ per command
- After: 100-200ms per command
- Target: <100ms (requires proxy chain optimization)

**Next steps if still slow:**
1. Enable TCP_NODELAY in signal_relay
2. Enable TCP_NODELAY in signal_splitter
3. Profile each hop to find bottleneck
4. Consider eliminating signal_splitter hop
