# Controller Agent

Automated parameter optimization for WWV tick detection using scipy.optimize.

## Architecture

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
        │                     │                   │
        ▼                     │                   ▼
┌───────────────────┐         │           ┌───────────────────┐
│   sdr_server      │         │           │   waterfall.exe   │
│   (radio ctrl)    │         └───────────│   (detection)     │
└───────────────────┘                     └───────────────────┘
```

## Components

### SDRController (TCP:4535)
TCP connection to sdr_server for radio control:
- SET_FREQ, GET_FREQ - Center frequency (1kHz - 2GHz)
- SET_GAIN, GET_GAIN - Gain reduction (20-59 dB)
- SET_LNA, GET_LNA - LNA state (0-8)
- SET_AGC, GET_AGC - AGC mode (OFF, 5HZ, 50HZ, 100HZ)
- SET_SRATE, GET_SRATE - Sample rate (2-10 MHz)
- SET_BW, GET_BW - Bandwidth (200, 300, 600, 1536, 5000, 6000, 7000, 8000 kHz)
- SET_ANTENNA, GET_ANTENNA - Antenna port (A, B, HIZ)
- START, STOP, STATUS - Streaming control
- PING, VER, CAPS, HELP - Utility

### TelemetryListener (UDP:3005)
Receives broadcast telemetry from waterfall.exe:
- CORR - Tick correlation data (timing variance, confidence) - **KEY FOR OPTIMIZATION**
- TICK - Tick detection events
- CHAN - Channel quality (carrier, SNR, noise)
- MARK - Minute markers
- RESP - Responses to control commands

### WaterfallController (UDP:3006)
Sends parameter updates to waterfall.exe:
- SET_TICK_THRESHOLD - Detection threshold
- SET_TICK_ADAPT_DOWN - Adaptive threshold decrease
- SET_TICK_ADAPT_UP - Adaptive threshold increase
- SET_TICK_MIN_DURATION - Minimum tick duration
- ENABLE_TELEM / DISABLE_TELEM - Telemetry channel control

## Optimization Flow

1. scipy.optimize.minimize calls objective_function with trial parameters
2. Parameters sent to waterfall.exe via UDP (port 3006)
3. Wait for system to settle (~2 seconds)
4. Read CORR telemetry for timing variance metrics (port 3005)
5. Return variance as optimization target (lower = better)
6. Optimizer adjusts parameters and repeats

## Usage

```python
from controller import ControllerAgent

agent = ControllerAgent()
agent.start()

# Run optimization with default settings
result = agent.optimize(max_iterations=50)
print(f"Best params: {result['params']}")
print(f"Best variance: {result['score']}")

agent.stop()
```

## Dependencies

```
pip install numpy scipy
```

## Parameters Being Optimized

| Parameter | Description | Bounds |
|-----------|-------------|--------|
| threshold | Noise floor multiplier for detection threshold | 1.0 - 5.0 |
| adapt_down | EMA decay rate when below threshold | 0.9 - 0.999 |
| adapt_up | EMA rise rate when above threshold | 0.001 - 0.1 |
| min_duration | Minimum tick pulse duration (ms) | 1.0 - 10.0 |

## TODO
- [ ] Tune settle_time based on waterfall response
- [x] Add CORR telemetry format parsing
- [ ] Consider adding more parameters from other detectors
- [ ] Collect multiple CORR samples during settle period and average
