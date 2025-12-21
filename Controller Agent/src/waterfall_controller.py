# waterfall_controller.py
# UDP sender for waterfall.exe parameter control (port 3006)
#
# Commands (from waterfall.c process_modem_command):
#   ENABLE_TELEM <channel>   - Enable telemetry channel (TICK, MARK, CORR, etc.)
#   DISABLE_TELEM <channel>  - Disable telemetry channel
#
# Tick detector parameters (for optimization):
#   SET_TICK_THRESHOLD <value>     - Detection threshold
#   SET_TICK_ADAPT_DOWN <value>    - Adaptive threshold decrease rate
#   SET_TICK_ADAPT_UP <value>      - Adaptive threshold increase rate  
#   SET_TICK_MIN_DURATION <value>  - Minimum tick duration

import socket
from typing import Optional, Dict, Any, List
from dataclasses import dataclass

@dataclass
class TickParameters:
    """Current tick detector parameters"""
    threshold: float = 0.0
    adapt_down: float = 0.0
    adapt_up: float = 0.0
    min_duration: float = 0.0

@dataclass
class CorrelatorParameters:
    """Current tick correlator parameters"""
    confidence: float = 0.8
    max_misses: int = 5

@dataclass
class MarkerParameters:
    """Current marker detector parameters"""
    threshold: float = 3.0
    adapt_rate: float = 0.001
    min_duration: float = 500.0

@dataclass
class SyncParameters:
    """Current sync detector parameters"""
    # Evidence weights
    weight_tick: float = 0.05
    weight_marker: float = 0.40
    weight_p_marker: float = 0.15
    weight_tick_hole: float = 0.20
    weight_combined: float = 0.50
    # Confidence thresholds
    locked_threshold: float = 0.70
    min_retain: float = 0.05
    tentative_init: float = 0.30
    # Decay rates
    decay_normal: float = 0.9999
    decay_recovering: float = 0.98
    # Tolerances
    tick_tolerance: float = 100.0
    marker_tolerance: float = 500.0
    p_marker_tolerance: float = 200.0

class WaterfallController:
    """
    UDP control interface for waterfall.exe tick detection parameters.
    """
    
    DEFAULT_PORT = 3006
    
    # Parameter definitions with bounds (from tick_detector.h and tick_correlator.h)
    PARAM_BOUNDS = {
        # Tick detector parameters
        'threshold':    (1.0, 5.0),       # threshold_mult: noise floor multiplier
        'adapt_down':   (0.9, 0.999),     # adapt_alpha_down: EMA decay when below threshold
        'adapt_up':     (0.001, 0.1),     # adapt_alpha_up: EMA rise when above threshold  
        'min_duration': (1.0, 10.0),      # min_duration_ms: minimum pulse duration
        # Tick correlator parameters
        'confidence':   (0.5, 0.95),      # epoch confidence threshold for lock acquisition
        'max_misses':   (2, 10),          # max consecutive misses before dropping lock
        # Marker detector parameters
        'marker_threshold':    (2.0, 5.0),    # threshold multiplier for 800ms marker
        'marker_adapt_rate':   (0.0001, 0.01), # noise adaptation rate
        'marker_min_duration': (300.0, 700.0), # minimum pulse duration ms
        # Sync detector parameters - evidence weights
        'sync_weight_tick':     (0.01, 0.2),   # tick evidence contribution
        'sync_weight_marker':   (0.1, 0.6),    # marker evidence contribution
        'sync_weight_p_marker': (0.05, 0.3),   # P-marker evidence contribution
        'sync_weight_tick_hole': (0.05, 0.4),  # tick hole (sec 29/59) evidence
        'sync_weight_combined': (0.2, 0.8),    # combined hole+marker evidence
        # Sync detector parameters - confidence thresholds
        'sync_locked_threshold': (0.5, 0.9),   # CRITICAL - gate for LOCKED state
        'sync_min_retain':       (0.01, 0.2),  # minimum to stay TENTATIVE/LOCKED
        'sync_tentative_init':   (0.1, 0.5),   # initial TENTATIVE confidence
        # Sync detector parameters - decay rates
        'sync_decay_normal':     (0.99, 0.9999), # slow decay when locked
        'sync_decay_recovering': (0.90, 0.99),   # fast decay when recovering
        # Sync detector parameters - tolerances (currently unused but tunable)
        'sync_tick_tolerance':    (50.0, 200.0),  # tick timing validation
        'sync_marker_tolerance':  (200.0, 800.0), # marker timing validation
        'sync_p_marker_tolerance': (100.0, 400.0), # P-marker timing validation
    }
    
    def __init__(self, host: str = "127.0.0.1", port: int = DEFAULT_PORT):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.tick_params = TickParameters()
        self.corr_params = CorrelatorParameters()
        self.marker_params = MarkerParameters()
        self.sync_params = SyncParameters()
        
    def connect(self):
        """Create UDP socket for sending"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
    def disconnect(self):
        """Close socket"""
        if self.socket:
            self.socket.close()
            self.socket = None
            
    def _send(self, cmd: str) -> bool:
        """Send command to waterfall"""
        if not self.socket:
            return False
        try:
            self.socket.sendto(f"{cmd}\n".encode('utf-8'), (self.host, self.port))
            return True
        except socket.error:
            return False
    
    # Telemetry control
    def enable_telemetry(self, channel: str) -> bool:
        """Enable a telemetry channel (TICK, MARK, CORR, CHAN, etc.)"""
        return self._send(f"ENABLE_TELEM {channel}")
        
    def disable_telemetry(self, channel: str) -> bool:
        """Disable a telemetry channel"""
        return self._send(f"DISABLE_TELEM {channel}")
    
    # Tick detector parameters
    def set_tick_threshold(self, value: float) -> bool:
        """Set tick detection threshold"""
        self.tick_params.threshold = value
        return self._send(f"SET_TICK_THRESHOLD {value}")
        
    def set_tick_adapt_down(self, value: float) -> bool:
        """Set adaptive threshold decrease rate"""
        self.tick_params.adapt_down = value
        return self._send(f"SET_TICK_ADAPT_DOWN {value}")
        
    def set_tick_adapt_up(self, value: float) -> bool:
        """Set adaptive threshold increase rate"""
        self.tick_params.adapt_up = value
        return self._send(f"SET_TICK_ADAPT_UP {value}")
        
    def set_tick_min_duration(self, value: float) -> bool:
        """Set minimum tick duration (ms)"""
        self.tick_params.min_duration = value
        return self._send(f"SET_TICK_MIN_DURATION {value}")
    
    # Tick correlator parameters
    def set_corr_confidence(self, value: float) -> bool:
        """Set epoch confidence threshold (0.5-0.95)"""
        self.corr_params.confidence = value
        return self._send(f"SET_CORR_CONFIDENCE {value}")
        
    def set_corr_max_misses(self, value: int) -> bool:
        """Set max consecutive misses before dropping lock (2-10)"""
        self.corr_params.max_misses = int(value)
        return self._send(f"SET_CORR_MAX_MISSES {int(value)}")
    
    # Marker detector parameters
    def set_marker_threshold(self, value: float) -> bool:
        """Set marker detection threshold multiplier (2.0-5.0)"""
        self.marker_params.threshold = value
        return self._send(f"SET_MARKER_THRESHOLD {value}")
        
    def set_marker_adapt_rate(self, value: float) -> bool:
        """Set marker noise adaptation rate (0.0001-0.01)"""
        self.marker_params.adapt_rate = value
        return self._send(f"SET_MARKER_ADAPT_RATE {value}")
        
    def set_marker_min_duration(self, value: float) -> bool:
        """Set marker minimum duration ms (300.0-700.0)"""
        self.marker_params.min_duration = value
        return self._send(f"SET_MARKER_MIN_DURATION {value}")
    
    # Sync detector parameters - evidence weights
    def set_sync_weight_tick(self, value: float) -> bool:
        """Set tick evidence weight (0.01-0.2)"""
        self.sync_params.weight_tick = value
        return self._send(f"SET_SYNC_WEIGHT_TICK {value}")
    
    def set_sync_weight_marker(self, value: float) -> bool:
        """Set marker evidence weight (0.1-0.6)"""
        self.sync_params.weight_marker = value
        return self._send(f"SET_SYNC_WEIGHT_MARKER {value}")
    
    def set_sync_weight_p_marker(self, value: float) -> bool:
        """Set P-marker evidence weight (0.05-0.3)"""
        self.sync_params.weight_p_marker = value
        return self._send(f"SET_SYNC_WEIGHT_P_MARKER {value}")
    
    def set_sync_weight_tick_hole(self, value: float) -> bool:
        """Set tick hole evidence weight (0.05-0.4)"""
        self.sync_params.weight_tick_hole = value
        return self._send(f"SET_SYNC_WEIGHT_TICK_HOLE {value}")
    
    def set_sync_weight_combined(self, value: float) -> bool:
        """Set combined hole+marker evidence weight (0.2-0.8)"""
        self.sync_params.weight_combined = value
        return self._send(f"SET_SYNC_WEIGHT_COMBINED {value}")
    
    # Sync detector parameters - confidence thresholds
    def set_sync_locked_threshold(self, value: float) -> bool:
        """Set LOCKED state confidence threshold (0.5-0.9) - CRITICAL"""
        self.sync_params.locked_threshold = value
        return self._send(f"SET_SYNC_LOCKED_THRESHOLD {value}")
    
    def set_sync_min_retain(self, value: float) -> bool:
        """Set minimum confidence to stay TENTATIVE/LOCKED (0.01-0.2)"""
        self.sync_params.min_retain = value
        return self._send(f"SET_SYNC_MIN_RETAIN {value}")
    
    def set_sync_tentative_init(self, value: float) -> bool:
        """Set initial TENTATIVE confidence (0.1-0.5)"""
        self.sync_params.tentative_init = value
        return self._send(f"SET_SYNC_TENTATIVE_INIT {value}")
    
    # Sync detector parameters - decay rates
    def set_sync_decay_normal(self, value: float) -> bool:
        """Set normal state confidence decay (0.99-0.9999)"""
        self.sync_params.decay_normal = value
        return self._send(f"SET_SYNC_DECAY_NORMAL {value}")
    
    def set_sync_decay_recovering(self, value: float) -> bool:
        """Set recovering state confidence decay (0.90-0.99)"""
        self.sync_params.decay_recovering = value
        return self._send(f"SET_SYNC_DECAY_RECOVERING {value}")
    
    # Sync detector parameters - tolerances
    def set_sync_tick_tolerance(self, value: float) -> bool:
        """Set tick timing tolerance ms (50.0-200.0)"""
        self.sync_params.tick_tolerance = value
        return self._send(f"SET_SYNC_TICK_TOLERANCE {value}")
    
    def set_sync_marker_tolerance(self, value: float) -> bool:
        """Set marker timing tolerance ms (200.0-800.0)"""
        self.sync_params.marker_tolerance = value
        return self._send(f"SET_SYNC_MARKER_TOLERANCE {value}")
    
    def set_sync_p_marker_tolerance(self, value: float) -> bool:
        """Set P-marker timing tolerance ms (100.0-400.0)"""
        self.sync_params.p_marker_tolerance = value
        return self._send(f"SET_SYNC_P_MARKER_TOLERANCE {value}")
    
    # Batch parameter update
    def set_parameters(self, params: Dict[str, float]) -> bool:
        """Set multiple parameters at once"""
        success = True
        # Tick detector
        if 'threshold' in params:
            success &= self.set_tick_threshold(params['threshold'])
        if 'adapt_down' in params:
            success &= self.set_tick_adapt_down(params['adapt_down'])
        if 'adapt_up' in params:
            success &= self.set_tick_adapt_up(params['adapt_up'])
        if 'min_duration' in params:
            success &= self.set_tick_min_duration(params['min_duration'])
        # Tick correlator
        if 'confidence' in params:
            success &= self.set_corr_confidence(params['confidence'])
        if 'max_misses' in params:
            success &= self.set_corr_max_misses(params['max_misses'])
        # Marker detector
        if 'marker_threshold' in params:
            success &= self.set_marker_threshold(params['marker_threshold'])
        if 'marker_adapt_rate' in params:
            success &= self.set_marker_adapt_rate(params['marker_adapt_rate'])
        if 'marker_min_duration' in params:
            success &= self.set_marker_min_duration(params['marker_min_duration'])
        # Sync detector - evidence weights
        if 'sync_weight_tick' in params:
            success &= self.set_sync_weight_tick(params['sync_weight_tick'])
        if 'sync_weight_marker' in params:
            success &= self.set_sync_weight_marker(params['sync_weight_marker'])
        if 'sync_weight_p_marker' in params:
            success &= self.set_sync_weight_p_marker(params['sync_weight_p_marker'])
        if 'sync_weight_tick_hole' in params:
            success &= self.set_sync_weight_tick_hole(params['sync_weight_tick_hole'])
        if 'sync_weight_combined' in params:
            success &= self.set_sync_weight_combined(params['sync_weight_combined'])
        # Sync detector - confidence thresholds
        if 'sync_locked_threshold' in params:
            success &= self.set_sync_locked_threshold(params['sync_locked_threshold'])
        if 'sync_min_retain' in params:
            success &= self.set_sync_min_retain(params['sync_min_retain'])
        if 'sync_tentative_init' in params:
            success &= self.set_sync_tentative_init(params['sync_tentative_init'])
        # Sync detector - decay rates
        if 'sync_decay_normal' in params:
            success &= self.set_sync_decay_normal(params['sync_decay_normal'])
        if 'sync_decay_recovering' in params:
            success &= self.set_sync_decay_recovering(params['sync_decay_recovering'])
        # Sync detector - tolerances
        if 'sync_tick_tolerance' in params:
            success &= self.set_sync_tick_tolerance(params['sync_tick_tolerance'])
        if 'sync_marker_tolerance' in params:
            success &= self.set_sync_marker_tolerance(params['sync_marker_tolerance'])
        if 'sync_p_marker_tolerance' in params:
            success &= self.set_sync_p_marker_tolerance(params['sync_p_marker_tolerance'])
        return success
        
    def set_parameters_from_array(self, values: List[float], mode: str = 'all') -> bool:
        """
        Set parameters from array (for scipy.optimize).
        
        mode='tick': [threshold, adapt_down, adapt_up, min_duration]
        mode='corr': [confidence, max_misses]
        mode='marker': [marker_threshold, marker_adapt_rate, marker_min_duration]
        mode='sync': [13 sync params in order]
        mode='all':  all 22 parameters
        """
        if mode == 'tick':
            param_names = ['threshold', 'adapt_down', 'adapt_up', 'min_duration']
        elif mode == 'corr':
            param_names = ['confidence', 'max_misses']
        elif mode == 'marker':
            param_names = ['marker_threshold', 'marker_adapt_rate', 'marker_min_duration']
        elif mode == 'sync':
            param_names = [
                'sync_weight_tick', 'sync_weight_marker', 'sync_weight_p_marker',
                'sync_weight_tick_hole', 'sync_weight_combined',
                'sync_locked_threshold', 'sync_min_retain', 'sync_tentative_init',
                'sync_decay_normal', 'sync_decay_recovering',
                'sync_tick_tolerance', 'sync_marker_tolerance', 'sync_p_marker_tolerance'
            ]
        else:  # 'all'
            param_names = [
                'threshold', 'adapt_down', 'adapt_up', 'min_duration',
                'confidence', 'max_misses',
                'marker_threshold', 'marker_adapt_rate', 'marker_min_duration',
                'sync_weight_tick', 'sync_weight_marker', 'sync_weight_p_marker',
                'sync_weight_tick_hole', 'sync_weight_combined',
                'sync_locked_threshold', 'sync_min_retain', 'sync_tentative_init',
                'sync_decay_normal', 'sync_decay_recovering',
                'sync_tick_tolerance', 'sync_marker_tolerance', 'sync_p_marker_tolerance'
            ]
        
        params = {name: val for name, val in zip(param_names, values)}
        return self.set_parameters(params)
        
    def get_bounds(self, mode: str = 'all') -> List[tuple]:
        """Get parameter bounds for scipy.optimize"""
        if mode == 'tick':
            return [
                self.PARAM_BOUNDS['threshold'],
                self.PARAM_BOUNDS['adapt_down'],
                self.PARAM_BOUNDS['adapt_up'],
                self.PARAM_BOUNDS['min_duration'],
            ]
        elif mode == 'corr':
            return [
                self.PARAM_BOUNDS['confidence'],
                self.PARAM_BOUNDS['max_misses'],
            ]
        elif mode == 'marker':
            return [
                self.PARAM_BOUNDS['marker_threshold'],
                self.PARAM_BOUNDS['marker_adapt_rate'],
                self.PARAM_BOUNDS['marker_min_duration'],
            ]
        elif mode == 'sync':
            return [
                self.PARAM_BOUNDS['sync_weight_tick'],
                self.PARAM_BOUNDS['sync_weight_marker'],
                self.PARAM_BOUNDS['sync_weight_p_marker'],
                self.PARAM_BOUNDS['sync_weight_tick_hole'],
                self.PARAM_BOUNDS['sync_weight_combined'],
                self.PARAM_BOUNDS['sync_locked_threshold'],
                self.PARAM_BOUNDS['sync_min_retain'],
                self.PARAM_BOUNDS['sync_tentative_init'],
                self.PARAM_BOUNDS['sync_decay_normal'],
                self.PARAM_BOUNDS['sync_decay_recovering'],
                self.PARAM_BOUNDS['sync_tick_tolerance'],
                self.PARAM_BOUNDS['sync_marker_tolerance'],
                self.PARAM_BOUNDS['sync_p_marker_tolerance'],
            ]
        else:  # 'all'
            return [
                self.PARAM_BOUNDS['threshold'],
                self.PARAM_BOUNDS['adapt_down'],
                self.PARAM_BOUNDS['adapt_up'],
                self.PARAM_BOUNDS['min_duration'],
                self.PARAM_BOUNDS['confidence'],
                self.PARAM_BOUNDS['max_misses'],
                self.PARAM_BOUNDS['marker_threshold'],
                self.PARAM_BOUNDS['marker_adapt_rate'],
                self.PARAM_BOUNDS['marker_min_duration'],
                self.PARAM_BOUNDS['sync_weight_tick'],
                self.PARAM_BOUNDS['sync_weight_marker'],
                self.PARAM_BOUNDS['sync_weight_p_marker'],
                self.PARAM_BOUNDS['sync_weight_tick_hole'],
                self.PARAM_BOUNDS['sync_weight_combined'],
                self.PARAM_BOUNDS['sync_locked_threshold'],
                self.PARAM_BOUNDS['sync_min_retain'],
                self.PARAM_BOUNDS['sync_tentative_init'],
                self.PARAM_BOUNDS['sync_decay_normal'],
                self.PARAM_BOUNDS['sync_decay_recovering'],
                self.PARAM_BOUNDS['sync_tick_tolerance'],
                self.PARAM_BOUNDS['sync_marker_tolerance'],
                self.PARAM_BOUNDS['sync_p_marker_tolerance'],
            ]
        
    def get_param_names(self, mode: str = 'all') -> List[str]:
        """Get parameter names in order"""
        if mode == 'tick':
            return ['threshold', 'adapt_down', 'adapt_up', 'min_duration']
        elif mode == 'corr':
            return ['confidence', 'max_misses']
        elif mode == 'marker':
            return ['marker_threshold', 'marker_adapt_rate', 'marker_min_duration']
        elif mode == 'sync':
            return [
                'sync_weight_tick', 'sync_weight_marker', 'sync_weight_p_marker',
                'sync_weight_tick_hole', 'sync_weight_combined',
                'sync_locked_threshold', 'sync_min_retain', 'sync_tentative_init',
                'sync_decay_normal', 'sync_decay_recovering',
                'sync_tick_tolerance', 'sync_marker_tolerance', 'sync_p_marker_tolerance'
            ]
        else:  # 'all'
            return [
                'threshold', 'adapt_down', 'adapt_up', 'min_duration',
                'confidence', 'max_misses',
                'marker_threshold', 'marker_adapt_rate', 'marker_min_duration',
                'sync_weight_tick', 'sync_weight_marker', 'sync_weight_p_marker',
                'sync_weight_tick_hole', 'sync_weight_combined',
                'sync_locked_threshold', 'sync_min_retain', 'sync_tentative_init',
                'sync_decay_normal', 'sync_decay_recovering',
                'sync_tick_tolerance', 'sync_marker_tolerance', 'sync_p_marker_tolerance'
            ]
