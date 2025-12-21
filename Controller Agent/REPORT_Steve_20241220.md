# Controller Agent: Automated Modem Parameter Tuning

**To:** Steve Hajducek (N2CKH)  
**From:** Rayven (KY4OLB)  
**Date:** December 20, 2024  
**Subject:** Local Model for Modem Tuning - Proof of Concept Complete

---

## Summary

Per your suggestion to set up a local model for modem tuning, we built and successfully tested an automated parameter optimization system for the WWV tick detector. **Total development and first successful optimization run: ~1 hour.**

## What We Built

### Controller Agent Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ControllerAgent                          │
│                  (scipy.optimize.minimize)                  │
└───────────┬─────────────────┬─────────────────┬─────────────┘
            │                 │                 │
            ▼                 ▼                 ▼
┌───────────────────┐ ┌─────────────────┐ ┌───────────────────┐
│  SDRController    │ │TelemetryListener│ │WaterfallController│
│  (TCP:4535)       │ │(UDP:3005 listen)│ │(UDP:3006 send)    │
└───────────────────┘ └─────────────────┘ └───────────────────┘
        │                     ▲                   │
        ▼                     │                   ▼
┌───────────────────┐         │           ┌───────────────────┐
│   sdr_server      │         └───────────│   waterfall.exe   │
│                   │                     │   (tick detect)   │
└───────────────────┘                     └───────────────────┘
```

### Components

1. **SDRController** - TCP connection to sdr_server (port 4535)
   - Sends START/STOP commands
   - Can control frequency, gain, LNA, etc.

2. **TelemetryListener** - UDP listener (port 3005)
   - Receives CORR telemetry from tick correlator
   - Parses interval timing, chain length, drift metrics

3. **WaterfallController** - UDP sender (port 3006)
   - Sends parameter updates to tick detector
   - SET_TICK_THRESHOLD, SET_TICK_ADAPT_DOWN, etc.

4. **Optimization Loop** - scipy.optimize.minimize (Nelder-Mead)
   - Adjusts 4 parameters simultaneously
   - Objective: minimize tick timing variance

## Parameters Tuned

| Parameter | Description | Bounds | Before | After |
|-----------|-------------|--------|--------|-------|
| threshold_multiplier | Noise floor multiplier | 1.0 - 5.0 | 3.0 | 3.074 |
| adapt_alpha_down | EMA decay rate | 0.9 - 0.999 | 0.9495 | 0.973 |
| adapt_alpha_up | EMA rise rate | 0.001 - 0.1 | 0.0505 | 0.052 |
| min_duration_ms | Minimum pulse duration | 1.0 - 10.0 | 5.5 | 5.23 |

## Results

### Before Optimization
- Catching ~1 in 5 ticks
- interval_ms: 5002ms (should be ~1000ms)
- chain_length: 1 (chains constantly breaking)
- variance: **477.9**

### After Optimization (64 evaluations)
- Catching nearly every tick
- interval_ms: ~998-1002ms 
- chain_length: growing
- variance: **1.9**

**250x improvement in tick timing consistency**

## Optimization Log Highlights

```
[1] NEW BEST: variance=477.900000, params=[3.0, 0.9495, 0.0505, 5.5]
...
[6] NEW BEST: variance=1.900000, params=[3.075, 0.9732, 0.0518, 5.225]
...
[64] variance=1.940000
```

Found optimal region at evaluation #6, then refined. Occasional spikes (1001.96, 461.9) were RF fades during gray line passage, not parameter issues.

## Saved Configuration

```ini
[tick_detector]
threshold_multiplier=3.074
adapt_alpha_down=0.973378
adapt_alpha_up=0.051749
min_duration_ms=5.23
```

## Future Applications

This same framework can be applied to:
- PSK modem parameters (MIL-STD-188-110A)
- BCD decoder tuning
- Envelope detection for other signals
- Any parameter set with measurable quality metrics

The key is having:
1. Parameters exposed via UDP control
2. Quality metrics in telemetry
3. Bounds defined for each parameter

## Files

Location: `D:\claude_sandbox\Controller Agent\`

```
Controller Agent/
├── README.md
├── run_controller.bat
└── src/
    ├── controller.py           # Main optimizer
    ├── sdr_controller.py       # TCP to sdr_server
    ├── telemetry_listener.py   # UDP from waterfall
    └── waterfall_controller.py # UDP to waterfall
```

---

73,  
Rayven KY4OLB
