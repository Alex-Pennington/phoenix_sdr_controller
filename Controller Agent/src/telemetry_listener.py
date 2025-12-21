# telemetry_listener.py
# UDP listener for telemetry data from waterfall.exe (port 3005)
#
# Telemetry channels (from waterfall_telemetry.h):
#   CHAN - Channel quality: carrier_db, snr_db, noise_db
#   TICK - Tick pulse events
#   MARK - Minute marker events
#   CARR - Carrier frequency tracking
#   SYNC - Sync state changes
#   SUBC - Subcarrier analysis
#   CORR - Tick correlation data (timing metrics for optimization)
#   T500 - 500 Hz tone tracker
#   T600 - 600 Hz tone tracker
#   BCDS - BCD decoder symbols and time
#   CONS - Console messages
#   CTRL - Control commands received
#   RESP - Responses to control commands

import socket
import threading
from typing import Callable, Optional, Dict, Any
from dataclasses import dataclass
from queue import Queue
import time

@dataclass
class TelemetryMessage:
    channel: str
    timestamp: float
    data: str
    parsed: Dict[str, Any]

class TelemetryListener:
    """
    UDP listener for waterfall telemetry.
    
    Telemetry format: "PREFIX,csv_data\n"
    Example: "CORR,0.123,0.045,12.5\n"
    """
    
    DEFAULT_PORT = 3005
    
    def __init__(self, host: str = "0.0.0.0", port: int = DEFAULT_PORT):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.running = False
        self.thread: Optional[threading.Thread] = None
        self.callback: Optional[Callable[[TelemetryMessage], None]] = None
        self.message_queue: Queue = Queue(maxsize=1000)
        
        # Latest telemetry by channel
        self.latest: Dict[str, TelemetryMessage] = {}
        self.lock = threading.Lock()
        
        # Marker event history for multi-marker evaluation
        self.marker_events: list = []
        self.marker_lock = threading.Lock()
        
        # Sync state tracking for LOCKED time measurement
        self.sync_states: list = []  # List of (timestamp, state, confidence)
        self.sync_lock = threading.Lock()
        
    def start(self, callback: Optional[Callable[[TelemetryMessage], None]] = None):
        """Start listening for telemetry data"""
        self.callback = callback
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind((self.host, self.port))
        self.socket.settimeout(1.0)  # Allow periodic check of running flag
        self.running = True
        self.thread = threading.Thread(target=self._listen_loop, daemon=True)
        self.thread.start()
        
    def stop(self):
        """Stop listening"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=2.0)
        if self.socket:
            self.socket.close()
            self.socket = None
            
    def _parse_message(self, data: bytes) -> Optional[TelemetryMessage]:
        """Parse telemetry message"""
        try:
            text = data.decode('utf-8').strip()
            if not text:
                return None
                
            # Format: "PREFIX,data..."
            parts = text.split(',', 1)
            if len(parts) < 1:
                return None
                
            channel = parts[0]
            csv_data = parts[1] if len(parts) > 1 else ""
            
            # Parse channel-specific data
            parsed = self._parse_channel_data(channel, csv_data)
            
            return TelemetryMessage(
                channel=channel,
                timestamp=time.time(),
                data=csv_data,
                parsed=parsed
            )
        except Exception as e:
            return None
            
    def _parse_channel_data(self, channel: str, data: str) -> Dict[str, Any]:
        """Parse channel-specific CSV data"""
        parsed = {}
        fields = data.split(',') if data else []
        
        if channel == "CORR":
            # Tick correlation data from tick_correlator.c
            # Format: time_str,timestamp_ms,tick_num,expected,energy_peak,duration_ms,
            #         interval_ms,avg_interval_ms,noise_floor,corr_peak,corr_ratio,
            #         chain_id,chain_length,chain_start_ms,cumulative_drift_ms
            try:
                if len(fields) >= 15:
                    parsed['time_str'] = fields[0]
                    parsed['timestamp_ms'] = float(fields[1]) if fields[1] else 0.0
                    parsed['tick_num'] = int(fields[2]) if fields[2] else 0
                    parsed['expected'] = fields[3]
                    parsed['energy_peak'] = float(fields[4]) if fields[4] else 0.0
                    parsed['duration_ms'] = float(fields[5]) if fields[5] else 0.0
                    parsed['interval_ms'] = float(fields[6]) if fields[6] else 0.0
                    parsed['avg_interval_ms'] = float(fields[7]) if fields[7] else 0.0
                    parsed['noise_floor'] = float(fields[8]) if fields[8] else 0.0
                    parsed['corr_peak'] = float(fields[9]) if fields[9] else 0.0
                    parsed['corr_ratio'] = float(fields[10]) if fields[10] else 0.0
                    parsed['chain_id'] = int(fields[11]) if fields[11] else 0
                    parsed['chain_length'] = int(fields[12]) if fields[12] else 0
                    parsed['chain_start_ms'] = float(fields[13]) if fields[13] else 0.0
                    parsed['cumulative_drift_ms'] = float(fields[14]) if fields[14] else 0.0
                    
                    # Computed metrics for optimization
                    # Timing variance: difference from nominal 1000ms interval
                    parsed['interval_error_ms'] = abs(parsed['interval_ms'] - 1000.0)
            except (ValueError, IndexError) as e:
                pass  # Parsing error, leave parsed empty
                
        elif channel == "TICK":
            # Tick detection events
            if len(fields) >= 1:
                parsed['tick_type'] = fields[0]
            if len(fields) >= 2:
                parsed['sample_offset'] = int(fields[1]) if fields[1] else 0
                
        elif channel == "CHAN":
            # Channel quality
            if len(fields) >= 1:
                parsed['carrier_db'] = float(fields[0]) if fields[0] else 0.0
            if len(fields) >= 2:
                parsed['snr_db'] = float(fields[1]) if fields[1] else 0.0
            if len(fields) >= 3:
                parsed['noise_db'] = float(fields[2]) if fields[2] else 0.0
                
        elif channel == "RESP":
            # Response from control command
            parsed['response'] = data
            
        elif channel == "MARK":
            # Marker detection events
            # Format: time_str,timestamp_ms,marker_num,wwv_second,expected,peak_energy,duration_ms,since_last_sec,baseline,threshold
            try:
                if len(fields) >= 8:
                    parsed['time_str'] = fields[0]
                    parsed['timestamp_ms'] = float(fields[1]) if fields[1] else 0.0
                    parsed['marker_num'] = int(fields[2]) if fields[2] else 0
                    parsed['peak_energy'] = float(fields[3]) if fields[3] else 0.0
                    parsed['duration_ms'] = float(fields[4]) if fields[4] else 0.0
                    parsed['since_last_sec'] = float(fields[5]) if fields[5] else 0.0
                    parsed['confidence'] = fields[6] if len(fields) > 6 else 'UNKNOWN'
            except (ValueError, IndexError):
                pass
        
        elif channel == "SYNC":
            # Sync state telemetry
            # Format varies - look for state and confidence
            try:
                if len(fields) >= 2:
                    parsed['state'] = fields[0]  # SEARCHING, TENTATIVE, LOCKED
                    parsed['confidence'] = float(fields[1]) if fields[1] else 0.0
                if len(fields) >= 3:
                    parsed['reason'] = fields[2]
            except (ValueError, IndexError):
                pass
            
        return parsed
            
    def _listen_loop(self):
        """Main listen loop"""
        while self.running:
            try:
                data, addr = self.socket.recvfrom(4096)
                msg = self._parse_message(data)
                if msg:
                    with self.lock:
                        self.latest[msg.channel] = msg
                    
                    # Track marker events
                    if msg.channel == "MARK" and msg.parsed:
                        with self.marker_lock:
                            self.marker_events.append({
                                'timestamp': msg.timestamp,
                                'parsed': msg.parsed
                            })
                    
                    # Track sync state changes
                    if msg.channel == "SYNC" and msg.parsed:
                        with self.sync_lock:
                            self.sync_states.append({
                                'timestamp': msg.timestamp,
                                'state': msg.parsed.get('state', 'UNKNOWN'),
                                'confidence': msg.parsed.get('confidence', 0.0)
                            })
                    
                    if not self.message_queue.full():
                        self.message_queue.put(msg)
                    
                    if self.callback:
                        self.callback(msg)
                        
            except socket.timeout:
                continue
            except socket.error:
                if self.running:
                    raise
                    
    def get_latest(self, channel: str) -> Optional[TelemetryMessage]:
        """Get latest message for a channel"""
        with self.lock:
            return self.latest.get(channel)
            
    def get_correlation_metrics(self) -> Dict[str, float]:
        """Get latest tick correlation metrics for optimization"""
        msg = self.get_latest("CORR")
        if msg and msg.parsed:
            return msg.parsed
        return {}
    
    def clear_marker_events(self):
        """Clear marker event history - call before starting marker collection"""
        with self.marker_lock:
            self.marker_events.clear()
    
    def get_marker_events(self) -> list:
        """Get list of marker events since last clear"""
        with self.marker_lock:
            return list(self.marker_events)
    
    def get_marker_count(self) -> int:
        """Get number of markers detected since last clear"""
        with self.marker_lock:
            return len(self.marker_events)
    
    def get_marker_metrics(self) -> Dict[str, Any]:
        """
        Calculate marker detection metrics for optimization.
        
        Returns dict with:
        - count: number of markers detected
        - avg_duration_ms: average pulse duration
        - duration_variance: variance in duration (should be low)
        - avg_interval_sec: average time between markers
        - interval_error_sec: deviation from expected 60s interval
        """
        events = self.get_marker_events()
        
        if not events:
            return {'count': 0, 'avg_duration_ms': 0, 'duration_variance': 0,
                    'avg_interval_sec': 0, 'interval_error_sec': 60.0}
        
        count = len(events)
        durations = [e['parsed'].get('duration_ms', 0) for e in events]
        
        # Calculate duration stats
        avg_duration = sum(durations) / count if count > 0 else 0
        duration_var = sum((d - avg_duration) ** 2 for d in durations) / count if count > 1 else 0
        
        # Calculate interval stats (time between consecutive markers)
        intervals = []
        for i in range(1, len(events)):
            interval = events[i]['timestamp'] - events[i-1]['timestamp']
            intervals.append(interval)
        
        if intervals:
            avg_interval = sum(intervals) / len(intervals)
            interval_error = abs(avg_interval - 60.0)  # Expected: 60 seconds
        else:
            avg_interval = 0
            interval_error = 60.0  # Max penalty if no intervals
        
        return {
            'count': count,
            'avg_duration_ms': avg_duration,
            'duration_variance': duration_var,
            'avg_interval_sec': avg_interval,
            'interval_error_sec': interval_error
        }
    
    def clear_sync_states(self):
        """Clear sync state history - call before starting sync collection"""
        with self.sync_lock:
            self.sync_states.clear()
    
    def get_sync_states(self) -> list:
        """Get list of sync state changes since last clear"""
        with self.sync_lock:
            return list(self.sync_states)
    
    def get_sync_metrics(self) -> Dict[str, Any]:
        """
        Calculate sync detector metrics for optimization.
        
        Returns dict with:
        - locked_pct: percentage of time in LOCKED state
        - avg_confidence: average confidence level
        - state_changes: number of state transitions
        - time_to_lock: time from start to first LOCKED (if any)
        """
        states = self.get_sync_states()
        
        if not states:
            return {'locked_pct': 0.0, 'avg_confidence': 0.0, 
                    'state_changes': 0, 'time_to_lock': float('inf')}
        
        # Calculate time in each state
        locked_time = 0.0
        total_time = 0.0
        confidences = []
        time_to_lock = float('inf')
        start_time = states[0]['timestamp']
        
        for i, s in enumerate(states):
            confidences.append(s['confidence'])
            
            # Track first LOCKED
            if s['state'] == 'LOCKED' and time_to_lock == float('inf'):
                time_to_lock = s['timestamp'] - start_time
            
            # Calculate duration in this state
            if i < len(states) - 1:
                duration = states[i + 1]['timestamp'] - s['timestamp']
            else:
                duration = 0.1  # Assume last state continues briefly
            
            total_time += duration
            if s['state'] == 'LOCKED':
                locked_time += duration
        
        locked_pct = (locked_time / total_time * 100.0) if total_time > 0 else 0.0
        avg_confidence = sum(confidences) / len(confidences) if confidences else 0.0
        
        # Count state changes (transitions)
        state_changes = 0
        for i in range(1, len(states)):
            if states[i]['state'] != states[i-1]['state']:
                state_changes += 1
        
        return {
            'locked_pct': locked_pct,
            'avg_confidence': avg_confidence,
            'state_changes': state_changes,
            'time_to_lock': time_to_lock
        }
