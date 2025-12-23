# Debugging Relay Commands

## New Logging Added

Commands and responses are now fully logged to help debug relay communication issues.

## How to View Logs

### Real-time Console Output
Logs appear in `phoenix_sdr_debug.log` in the same directory as the executable.

### View Recent Logs
```powershell
Get-Content phoenix_sdr_debug.log -Tail 50
```

### Follow Logs Live
```powershell
Get-Content phoenix_sdr_debug.log -Wait -Tail 20
```

## What You'll See

### When Clicking Antenna Button
```
[INFO] Setting antenna to B
[DEBUG] TCP TX: SET_ANTENNA B
[DEBUG] TCP RX: OK
[INFO] Antenna set to B successfully
```

### When Changing Frequency
```
[INFO] Setting frequency to 15000000 Hz
[DEBUG] TCP TX: SET_FREQ 15000000
[DEBUG] TCP RX: OK
[INFO] Frequency set successfully
```

### If Command Fails
```
[INFO] Setting antenna to B
[DEBUG] TCP TX: SET_ANTENNA B
[DEBUG] TCP RX: ERR 500 Server error
[ERROR] SET_ANTENNA failed: ERR 500 Server error
```

### If No Response (Timeout)
```
[INFO] Setting antenna to B
[DEBUG] TCP TX: SET_ANTENNA B
[ERROR] SET_ANTENNA command failed - no response
```

## Common Issues

### Commands Sent But No Response
**Log shows:**
```
[DEBUG] TCP TX: SET_ANTENNA B
[ERROR] SET_ANTENNA command failed - no response
```

**Cause:** Relay or sdr_server not responding
**Fix:** Check relay logs, check if sdr_server is running

### Response is Error
**Log shows:**
```
[DEBUG] TCP RX: ERR 404 Unknown command
```

**Cause:** Relay/server doesn't understand command
**Fix:** Check relay is forwarding commands correctly

### No TX/RX Logs at All
**Cause:** Not connected or commands not being sent
**Check:** Look for `Connected to <relay>:3001` message

## Testing Steps

1. **Run controller with relay flag:**
   ```powershell
   .\build\phoenix_sdr_controller.exe --relay 146.190.112.225
   ```

2. **Open log in another terminal:**
   ```powershell
   Get-Content phoenix_sdr_debug.log -Wait -Tail 30
   ```

3. **Click Connect button in GUI**
   - Should see: `Connected to 146.190.112.225:3001`

4. **Change antenna from A to B**
   - Should see: TX/RX logs + success message

5. **Change frequency**
   - Should see: TX/RX logs + success message

## Expected Behavior

✅ **Working correctly:**
- Every button click produces TX log
- Within 100-500ms, RX log appears with `OK`
- Success message confirms operation

❌ **Not working:**
- TX log appears but no RX log (timeout)
- RX log shows `ERR` response
- UI doesn't update after successful command

## Next Steps Based on Logs

### Scenario 1: No TX logs
→ Commands aren't being sent (UI event handling issue)

### Scenario 2: TX but no RX
→ Relay not responding (check relay server)

### Scenario 3: TX/RX with ERR
→ Server rejecting command (check sdr_server logs)

### Scenario 4: TX/RX with OK but UI doesn't update
→ UI sync issue (status polling not working)
