# Controller GUI --relay Mode Issue

**Date:** 2024-12-22  
**Status:** Open  
**Assigned:** Controller Dev Team

## Summary

When running with `--relay` flag, the GUI becomes unresponsive and has rendering/state issues. Window doesn't respond to close button.

## To Reproduce

```cmd
controller.exe --relay 146.190.112.225
```

## Expected Behavior

- GUI should render normally
- Should connect to relay:3001 for control passthrough
- Window should respond to close/minimize

## Actual Behavior

- GUI appears frozen or has rendering issues
- Unresponsive to window close
- Had to force kill via Task Manager

## Context

The `--relay` flag was added in commit a3fdcb1 to support remote SDR access through the cloud relay. The flag correctly sets:
- `server_host` = relay IP
- `server_port` = 3001 (instead of default 4535)

The relay infrastructure is working - signal_splitter successfully connects to relay:3001/3002/3003 and passes control commands.

## Relay Test Results (Working)

```
signal_splitter.exe --relay-host 146.190.112.225

[RELAY-CTRL] Connected (bidirectional passthrough)
[RELAY-DET] Connected: 50000 Hz float32 I/Q
[RELAY-DISP] Connected: 12000 Hz float32 I/Q
```

## Possible Causes

1. TCP connection blocking the UI thread during connect attempts
2. SDL event loop issue when connection fails/times out
3. State machine issue when not connected to a real sdr_server
4. Missing non-blocking socket handling for relay connections

## Files Changed

- `src/main.c` - Added --relay argument parsing and RELAY_CONTROL_PORT constant

## Workaround

For now, use the CLI sdr_controller for testing:
```cmd
sdr_controller.exe  # Works for local testing only
```

## Next Steps

1. Make TCP connect non-blocking or run in background thread
2. Add connection timeout handling that doesn't freeze UI
3. Test with relay server that has splitter connected on other end
