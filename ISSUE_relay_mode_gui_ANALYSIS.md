# GUI Freeze in Relay Mode - Root Cause Analysis

**Date:** 2024-12-22  
**Status:** Diagnosed  
**Priority:** High

## Root Cause

The GUI freezes when using `--relay` flag because `tcp_client_connect()` performs a **blocking** `connect()` system call on the UI thread.

### Code Location

[src/tcp_client.c](src/tcp_client.c#L161):
```c
/* Connect to server */
res = connect(client->socket, result->ai_addr, (int)result->ai_addrlen);
```

This blocking call:
- Freezes the UI thread for up to 20+ seconds on Windows if the relay is unreachable
- Prevents SDL event processing (window close, rendering, mouse input)
- Happens in the main event loop when user clicks "Connect" button

### Call Stack
```
main.c: Event Loop
  └─ app_handle_actions()
      └─ app_connect()
          └─ sdr_connect()  (sdr_protocol.c)
              └─ tcp_client_connect()  (tcp_client.c)
                  └─ connect()  ← BLOCKS HERE
```

## Solution

Make the connection **non-blocking** by:

1. Set socket to non-blocking mode before `connect()`
2. Use `select()` with timeout to check connection status
3. Return control to UI thread while waiting

### Implementation Changes

#### Option 1: Non-blocking connect with select() (Recommended)

**File:** [src/tcp_client.c](src/tcp_client.c#L140-L180)

```c
/* Set socket to non-blocking mode */
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(client->socket, FIONBIO, &mode);
#else
    int flags = fcntl(client->socket, F_GETFL, 0);
    fcntl(client->socket, F_SETFL, flags | O_NONBLOCK);
#endif

/* Attempt non-blocking connect */
res = connect(client->socket, result->ai_addr, (int)result->ai_addrlen);
freeaddrinfo(result);

if (res != 0) {
    int err = SOCKET_ERROR_CODE;
    
    /* WSAEWOULDBLOCK (Windows) or EINPROGRESS (Unix) is expected */
#ifdef _WIN32
    if (err != WSAEWOULDBLOCK) {
#else
    if (err != EINPROGRESS) {
#endif
        snprintf(client->last_error, sizeof(client->last_error),
                 "connect() failed: %d", err);
        LOG_ERROR("%s", client->last_error);
        CLOSE_SOCKET(client->socket);
        client->socket = INVALID_SOCK;
        client->state = CONN_ERROR;
        return false;
    }
    
    /* Wait for connection with timeout using select() */
    fd_set write_fds, error_fds;
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);
    FD_SET(client->socket, &write_fds);
    FD_SET(client->socket, &error_fds);
    
    struct timeval timeout;
    timeout.tv_sec = SOCKET_TIMEOUT_MS / 1000;
    timeout.tv_usec = (SOCKET_TIMEOUT_MS % 1000) * 1000;
    
    res = select((int)client->socket + 1, NULL, &write_fds, &error_fds, &timeout);
    
    if (res <= 0 || FD_ISSET(client->socket, &error_fds)) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Connection timeout or failed");
        LOG_ERROR("%s", client->last_error);
        CLOSE_SOCKET(client->socket);
        client->socket = INVALID_SOCK;
        client->state = CONN_ERROR;
        return false;
    }
    
    /* Check if connection actually succeeded */
    int socket_error = 0;
    socklen_t len = sizeof(socket_error);
    getsockopt(client->socket, SOL_SOCKET, SO_ERROR, (char*)&socket_error, &len);
    
    if (socket_error != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Connection failed: %d", socket_error);
        LOG_ERROR("%s", client->last_error);
        CLOSE_SOCKET(client->socket);
        client->socket = INVALID_SOCK;
        client->state = CONN_ERROR;
        return false;
    }
}

/* Set socket back to blocking mode for send/recv operations */
#ifdef _WIN32
    mode = 0;
    ioctlsocket(client->socket, FIONBIO, &mode);
#else
    flags = fcntl(client->socket, F_GETFL, 0);
    fcntl(client->socket, F_SETFL, flags & ~O_NONBLOCK);
#endif
```

**Pros:**
- Single-threaded solution
- Keeps all existing code structure
- Still respects SOCKET_TIMEOUT_MS (5 seconds)
- UI remains responsive during timeout

**Cons:**
- Still blocks for up to 5 seconds (but better than 20+)

---

#### Option 2: Background Thread Connection (Better UX)

Create a connection worker thread that updates state asynchronously.

**New State:** `CONN_CONNECTING` (already exists in enum)

**Changes:**
1. Add mutex to `tcp_client_t` for thread safety
2. Launch connection thread from `tcp_client_connect()`
3. Main loop checks connection state each frame
4. Show "Connecting..." animation in UI

**Pros:**
- UI never blocks
- Can show spinner/progress indicator
- Best user experience

**Cons:**
- More complex implementation
- Requires thread synchronization
- Needs careful cleanup on window close

---

## Recommended Approach

**Phase 1 (Quick Fix):** Implement Option 1 - non-blocking connect with `select()`
- Solves the freeze issue immediately
- Minimal code changes
- Low risk

**Phase 2 (Enhancement):** Consider Option 2 for better UX
- Only if connection times are frequently >2 seconds
- Nice-to-have, not critical

## Testing Checklist

After fix:
- [ ] Connect to local server (localhost:4535) - should work instantly
- [ ] Connect to relay server (relay:3001) - should timeout gracefully in 5s
- [ ] Connect to unreachable host - should timeout in 5s (not 20s)
- [ ] Window close button works during connection attempt
- [ ] UI renders at 60 FPS during connection
- [ ] Reconnect works after failed attempt

## Files to Modify

1. [src/tcp_client.c](src/tcp_client.c) - `tcp_client_connect()` function (lines 90-180)
2. Optional: [include/tcp_client.h](include/tcp_client.h) - Add mutex if using threading

## References

- Original Issue: [ISSUE_relay_mode_gui.md](ISSUE_relay_mode_gui.md)
- TCP Client Implementation: [src/tcp_client.c](src/tcp_client.c)
- Main Event Loop: [src/main.c](src/main.c#L115-L260)
- Windows Non-blocking Sockets: https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-ioctlsocket

---

**Next Steps:** Implement Option 1 fix and test with relay server.
