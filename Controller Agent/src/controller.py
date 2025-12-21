# controller.py
# Main controller agent - coordinates SDR, telemetry, and waterfall control
# Integrates scipy.optimize for automated parameter tuning

from sdr_controller import SDRController
from telemetry_listener import TelemetryListener
from waterfall_controller import WaterfallController
from typing import Dict, Any, List, Optional, Callable
import numpy as np
import time
import os
import configparser

class ControllerAgent:
    """
    Parameter optimization controller for WWV tick detection tuning.
    
    Uses scipy.optimize.minimize to automatically tune envelope detection
    parameters by minimizing tick timing variance.
    
    Architecture:
        ControllerAgent
            ├── SDRController (TCP:4535) → sdr_server
            ├── TelemetryListener (UDP:3005) ← waterfall.exe  
            └── WaterfallController (UDP:3006) → waterfall.exe
    """
    
    def __init__(self):
        self.sdr = SDRController()
        self.telemetry = TelemetryListener()
        self.waterfall = WaterfallController()
        
        # Optimization state
        self.optimization_running = False
        self.eval_count = 0
        self.best_score = float('inf')
        self.best_params: List[float] = []
        self._opt_mode = 'tick'  # Current optimization mode
        
        # Timing for telemetry collection
        self.settle_time = 2.0  # seconds to wait for telemetry after param change
        self.marker_eval_time = 180.0  # seconds to collect markers (~3 markers)
        self.marker_count_target = 3  # number of markers to wait for
        self.sync_eval_time = 60.0  # seconds to collect sync state for evaluation
        
        # INI file paths for saving results
        self.ini_path = os.path.join(os.path.dirname(__file__), '..', 'optimized_params.ini')  # Overall best
        self.last_run_path = os.path.join(os.path.dirname(__file__), '..', 'last_run.ini')  # Last run best
        
    def start(self) -> bool:
        """Initialize all connections"""
        # Connect to SDR
        sdr_ok = self.sdr.connect()
        if not sdr_ok:
            print("Note: SDR not connected (optimization can still work)")
        else:
            # Start streaming
            ok, resp = self.sdr.start()
            if ok:
                print("SDR streaming started")
            else:
                print(f"SDR start failed: {resp}")
            
        self.waterfall.connect()
        self.telemetry.start(callback=self._on_telemetry)
        
        return True
        
    def stop(self):
        """Shutdown all connections"""
        # Stop SDR streaming first
        if self.sdr.socket:
            self.sdr.stop()
            print("SDR streaming stopped")
        
        self.telemetry.stop()
        self.waterfall.disconnect()
        self.sdr.disconnect()
        
    def _on_telemetry(self, msg):
        """Handle incoming telemetry from waterfall"""
        # Could add logging or real-time display here
        pass
    
    def save_to_ini(self, params: List[float], param_names: List[str], mode: str, score: float):
        """
        Save optimized parameters to INI files.
        
        - last_run.ini: Always saves (current run's best)
        - optimized_params.ini: Only saves if better than existing score for this mode
        """
        # Always save to last_run.ini
        self._write_ini(self.last_run_path, params, param_names, mode, score)
        print(f"Saved to {self.last_run_path}")
        
        # Check if we should update overall best
        existing_score = self._get_existing_score(self.ini_path, mode)
        if existing_score is None or score < existing_score:
            self._write_ini(self.ini_path, params, param_names, mode, score)
            print(f"NEW OVERALL BEST! Saved to {self.ini_path}")
        else:
            print(f"Score {score:.4f} not better than existing {existing_score:.4f} - overall best unchanged")
    
    def _get_existing_score(self, filepath: str, mode: str) -> Optional[float]:
        """Get existing best score from INI file for given mode"""
        if not os.path.exists(filepath):
            return None
        
        config = configparser.ConfigParser()
        config.read(filepath)
        
        # Check for mode-specific score first
        score_key = f"{mode}_best_score"
        if 'optimization_meta' in config and score_key in config['optimization_meta']:
            try:
                return float(config['optimization_meta'][score_key])
            except ValueError:
                pass
        
        # Fallback to last_score if mode matches
        if 'optimization_meta' in config:
            if config['optimization_meta'].get('last_mode') == mode:
                try:
                    return float(config['optimization_meta'].get('last_score', 'inf'))
                except ValueError:
                    pass
        
        return None
    
    def _write_ini(self, filepath: str, params: List[float], param_names: List[str], mode: str, score: float):
        """
        Write parameters to INI file.
        
        Creates/updates sections based on mode:
        - tick: [tick_detector]
        - corr: [tick_correlator]
        - marker: [marker_detector]
        - all: all sections
        """
        config = configparser.ConfigParser()
        
        # Read existing file if present
        if os.path.exists(filepath):
            config.read(filepath)
        
        # Build param dict
        param_dict = dict(zip(param_names, params))
        
        # Map to INI sections and keys
        if mode == 'tick' or mode == 'all':
            if 'tick_detector' not in config:
                config['tick_detector'] = {}
            if 'threshold' in param_dict:
                config['tick_detector']['threshold_multiplier'] = f"{param_dict['threshold']:.6f}"
            if 'adapt_down' in param_dict:
                config['tick_detector']['adapt_alpha_down'] = f"{param_dict['adapt_down']:.6f}"
            if 'adapt_up' in param_dict:
                config['tick_detector']['adapt_alpha_up'] = f"{param_dict['adapt_up']:.6f}"
            if 'min_duration' in param_dict:
                config['tick_detector']['min_duration_ms'] = f"{param_dict['min_duration']:.2f}"
        
        if mode == 'corr' or mode == 'all':
            if 'tick_correlator' not in config:
                config['tick_correlator'] = {}
            if 'confidence' in param_dict:
                config['tick_correlator']['epoch_confidence_threshold'] = f"{param_dict['confidence']:.6f}"
            if 'max_misses' in param_dict:
                config['tick_correlator']['max_consecutive_misses'] = f"{int(param_dict['max_misses'])}"
        
        if mode == 'marker' or mode == 'all':
            if 'marker_detector' not in config:
                config['marker_detector'] = {}
            if 'marker_threshold' in param_dict:
                config['marker_detector']['threshold_multiplier'] = f"{param_dict['marker_threshold']:.6f}"
            if 'marker_adapt_rate' in param_dict:
                config['marker_detector']['noise_adapt_rate'] = f"{param_dict['marker_adapt_rate']:.6f}"
            if 'marker_min_duration' in param_dict:
                config['marker_detector']['min_duration_ms'] = f"{param_dict['marker_min_duration']:.2f}"
        
        if mode == 'sync' or mode == 'all':
            if 'sync_detector' not in config:
                config['sync_detector'] = {}
            # Evidence weights
            if 'sync_weight_tick' in param_dict:
                config['sync_detector']['weight_tick'] = f"{param_dict['sync_weight_tick']:.6f}"
            if 'sync_weight_marker' in param_dict:
                config['sync_detector']['weight_marker'] = f"{param_dict['sync_weight_marker']:.6f}"
            if 'sync_weight_p_marker' in param_dict:
                config['sync_detector']['weight_p_marker'] = f"{param_dict['sync_weight_p_marker']:.6f}"
            if 'sync_weight_tick_hole' in param_dict:
                config['sync_detector']['weight_tick_hole'] = f"{param_dict['sync_weight_tick_hole']:.6f}"
            if 'sync_weight_combined' in param_dict:
                config['sync_detector']['weight_combined_hole_marker'] = f"{param_dict['sync_weight_combined']:.6f}"
            # Confidence thresholds
            if 'sync_locked_threshold' in param_dict:
                config['sync_detector']['confidence_locked_threshold'] = f"{param_dict['sync_locked_threshold']:.6f}"
            if 'sync_min_retain' in param_dict:
                config['sync_detector']['confidence_min_retain'] = f"{param_dict['sync_min_retain']:.6f}"
            if 'sync_tentative_init' in param_dict:
                config['sync_detector']['confidence_tentative_init'] = f"{param_dict['sync_tentative_init']:.6f}"
            # Decay rates
            if 'sync_decay_normal' in param_dict:
                config['sync_detector']['confidence_decay_normal'] = f"{param_dict['sync_decay_normal']:.6f}"
            if 'sync_decay_recovering' in param_dict:
                config['sync_detector']['confidence_decay_recovering'] = f"{param_dict['sync_decay_recovering']:.6f}"
            # Tolerances
            if 'sync_tick_tolerance' in param_dict:
                config['sync_detector']['tick_phase_tolerance_ms'] = f"{param_dict['sync_tick_tolerance']:.2f}"
            if 'sync_marker_tolerance' in param_dict:
                config['sync_detector']['marker_tolerance_ms'] = f"{param_dict['sync_marker_tolerance']:.2f}"
            if 'sync_p_marker_tolerance' in param_dict:
                config['sync_detector']['p_marker_tolerance_ms'] = f"{param_dict['sync_p_marker_tolerance']:.2f}"
        
        # Add metadata section
        if 'optimization_meta' not in config:
            config['optimization_meta'] = {}
        config['optimization_meta']['last_mode'] = mode
        config['optimization_meta']['last_score'] = f"{score:.6f}"
        config['optimization_meta']['last_run'] = time.strftime('%Y-%m-%d %H:%M:%S')
        
        # Track per-mode best scores
        score_key = f"{mode}_best_score"
        config['optimization_meta'][score_key] = f"{score:.6f}"
        
        # Write file
        with open(filepath, 'w') as f:
            config.write(f)
        
    def objective_function(self, params: np.ndarray) -> float:
        """
        Objective function for scipy.optimize.
        
        Args:
            params: Parameter array (length depends on self._opt_mode)
            
        Returns:
            Metric to minimize (tick timing variance, lower = better)
        """
        mode = getattr(self, '_opt_mode', 'tick')
        
        # Marker and sync modes use different evaluation
        if mode == 'marker':
            return self.marker_objective_function(params)
        if mode == 'sync':
            return self.sync_objective_function(params)
        
        self.eval_count += 1
        
        # Apply parameters to waterfall using current mode
        self.waterfall.set_parameters_from_array(params.tolist(), mode)
        
        # Wait for system to settle and collect telemetry
        time.sleep(self.settle_time)
        
        # Get timing metrics from correlator
        metrics = self.telemetry.get_correlation_metrics()
        
        if not metrics:
            # No telemetry received - return high penalty
            print(f"  [{self.eval_count}] No telemetry - penalty")
            return 1000.0
            
        # Primary metric: interval error from nominal 1000ms (lower = better)
        interval_error = metrics.get('interval_error_ms', 1000.0)
        
        # Secondary: cumulative drift (lower = better long-term stability)
        drift = abs(metrics.get('cumulative_drift_ms', 0.0))
        
        # Tertiary: chain length (higher = better, so we subtract from score)
        chain_length = metrics.get('chain_length', 0)
        chain_bonus = min(chain_length * 0.1, 5.0)  # Cap at 5ms reduction
        
        # Correlation ratio (higher = better matched filter response)
        corr_ratio = metrics.get('corr_ratio', 0.0)
        corr_penalty = max(0, (1.0 - corr_ratio) * 2.0)  # Penalize low correlation
        
        # Combined score
        variance = interval_error + drift * 0.1 - chain_bonus + corr_penalty
            
        # Track best
        if variance < self.best_score:
            self.best_score = variance
            self.best_params = params.tolist()
            print(f"  [{self.eval_count}] NEW BEST: variance={variance:.6f}, params={params}")
            
            # Save immediately so Ctrl+C doesn't lose progress
            self.save_to_ini(self.best_params, self.waterfall.get_param_names(mode), mode, self.best_score)
        else:
            print(f"  [{self.eval_count}] variance={variance:.6f}")
            
        return variance
    
    def marker_objective_function(self, params: np.ndarray) -> float:
        """
        Objective function for marker detector optimization.
        
        Waits for multiple markers (~3 minutes) to evaluate detection quality.
        """
        self.eval_count += 1
        mode = 'marker'
        
        # Apply parameters
        self.waterfall.set_parameters_from_array(params.tolist(), mode)
        
        # Clear marker history
        self.telemetry.clear_marker_events()
        
        print(f"  [{self.eval_count}] Waiting for {self.marker_count_target} markers...")
        
        # Wait for markers with progress updates
        start_time = time.time()
        last_count = 0
        while True:
            elapsed = time.time() - start_time
            count = self.telemetry.get_marker_count()
            
            # Progress update when new marker detected
            if count > last_count:
                print(f"    Marker {count}/{self.marker_count_target} at {elapsed:.0f}s")
                last_count = count
            
            # Got enough markers
            if count >= self.marker_count_target:
                break
            
            # Timeout
            if elapsed >= self.marker_eval_time:
                print(f"    Timeout at {elapsed:.0f}s with {count} markers")
                break
            
            time.sleep(1.0)
        
        # Get metrics
        metrics = self.telemetry.get_marker_metrics()
        count = metrics.get('count', 0)
        
        if count == 0:
            # No markers detected - high penalty
            print(f"  [{self.eval_count}] No markers - penalty")
            return 1000.0
        
        # Score components:
        # 1. Detection rate penalty (want all expected markers)
        expected = int((time.time() - start_time) / 60.0) + 1
        detection_penalty = (expected - count) * 100.0  # Big penalty per missed marker
        
        # 2. Interval error (deviation from 60s)
        interval_error = metrics.get('interval_error_sec', 60.0) * 10.0  # Scale to ms-like range
        
        # 3. Duration variance (should be consistent ~800ms)
        duration_var = metrics.get('duration_variance', 0) / 100.0  # Normalize
        
        # 4. Duration accuracy (should be ~800ms)
        avg_duration = metrics.get('avg_duration_ms', 0)
        duration_error = abs(avg_duration - 800.0) / 10.0  # Penalize deviation from 800ms
        
        # Combined score
        score = detection_penalty + interval_error + duration_var + duration_error
        
        print(f"  [{self.eval_count}] count={count} interval_err={interval_error:.1f} dur_err={duration_error:.1f} score={score:.1f}")
        
        # Track best
        if score < self.best_score:
            self.best_score = score
            self.best_params = params.tolist()
            print(f"  [{self.eval_count}] NEW BEST: score={score:.6f}, params={params}")
            self.save_to_ini(self.best_params, self.waterfall.get_param_names(mode), mode, self.best_score)
        
        return score
    
    def sync_objective_function(self, params: np.ndarray) -> float:
        """
        Objective function for sync detector optimization.
        
        Collects sync state over ~60 seconds and optimizes for:
        - Maximum LOCKED time percentage
        - Fewer state transitions (stability)
        - Higher average confidence
        """
        self.eval_count += 1
        mode = 'sync'
        
        # Apply parameters
        self.waterfall.set_parameters_from_array(params.tolist(), mode)
        
        # Clear sync state history
        self.telemetry.clear_sync_states()
        
        print(f"  [{self.eval_count}] Collecting sync data for {self.sync_eval_time:.0f}s...")
        
        # Wait and show progress
        start_time = time.time()
        while True:
            elapsed = time.time() - start_time
            
            # Progress every 15 seconds
            if int(elapsed) % 15 == 0 and int(elapsed) > 0:
                metrics = self.telemetry.get_sync_metrics()
                pct = metrics.get('locked_pct', 0)
                if int(elapsed) == 15 or int(elapsed) == 30 or int(elapsed) == 45:
                    print(f"    {elapsed:.0f}s: LOCKED {pct:.1f}%")
            
            if elapsed >= self.sync_eval_time:
                break
            
            time.sleep(1.0)
        
        # Get final metrics
        metrics = self.telemetry.get_sync_metrics()
        locked_pct = metrics.get('locked_pct', 0.0)
        avg_confidence = metrics.get('avg_confidence', 0.0)
        state_changes = metrics.get('state_changes', 0)
        time_to_lock = metrics.get('time_to_lock', float('inf'))
        
        # Score: minimize (lower is better)
        # Primary: maximize LOCKED percentage (100 - pct)
        # Secondary: penalize excessive state changes (instability)
        # Tertiary: reward faster lock acquisition
        
        pct_penalty = 100.0 - locked_pct  # 0 if always LOCKED, 100 if never
        stability_penalty = state_changes * 2.0  # Each transition costs 2 points
        lock_time_penalty = min(time_to_lock, 60.0) / 6.0  # Max 10 points for slow lock
        confidence_bonus = avg_confidence * 5.0  # Reward higher confidence (subtract)
        
        score = pct_penalty + stability_penalty + lock_time_penalty - confidence_bonus
        
        print(f"  [{self.eval_count}] locked={locked_pct:.1f}% changes={state_changes} ttl={time_to_lock:.1f}s score={score:.1f}")
        
        # Track best
        if score < self.best_score:
            self.best_score = score
            self.best_params = params.tolist()
            print(f"  [{self.eval_count}] NEW BEST: score={score:.6f}")
            self.save_to_ini(self.best_params, self.waterfall.get_param_names(mode), mode, self.best_score)
        
        return score
        
    def optimize(self, 
                 initial_params: Optional[List[float]] = None,
                 method: str = 'Nelder-Mead',
                 max_iterations: int = 100,
                 mode: str = 'tick') -> Dict[str, Any]:
        """
        Run scipy optimization on detection/correlation parameters.
        
        Args:
            initial_params: Starting parameter values
            method: Optimization method ('Nelder-Mead', 'Powell', 'L-BFGS-B')
            max_iterations: Maximum function evaluations
            mode: Which parameters to tune:
                  'tick' - tick detector only (threshold, adapt_down, adapt_up, min_duration)
                  'corr' - tick correlator only (confidence, max_misses)
                  'all'  - all 6 parameters
            
        Returns:
            Dict with 'success', 'params', 'score', 'iterations'
        """
        from scipy.optimize import minimize
        
        self._opt_mode = mode
        
        # Default starting point (middle of bounds)
        if initial_params is None:
            bounds = self.waterfall.get_bounds(mode)
            initial_params = [(b[0] + b[1]) / 2 for b in bounds]
            
        x0 = np.array(initial_params)
        bounds = self.waterfall.get_bounds(mode)
        
        print(f"Starting optimization with method={method}, mode={mode}")
        print(f"Parameters: {self.waterfall.get_param_names(mode)}")
        print(f"Initial params: {initial_params}")
        print(f"Bounds: {bounds}")
        print(f"Max iterations: {max_iterations}")
        print("-" * 50)
        
        self.optimization_running = True
        self.eval_count = 0
        self.best_score = float('inf')
        self.best_params = initial_params.copy()
        
        try:
            # For Nelder-Mead, bounds are handled differently
            if method == 'Nelder-Mead':
                result = minimize(
                    self.objective_function,
                    x0,
                    method=method,
                    options={
                        'maxiter': max_iterations,
                        'xatol': 0.001,
                        'fatol': 0.0001,
                        'disp': True
                    }
                )
            else:
                # L-BFGS-B and others support bounds directly
                result = minimize(
                    self.objective_function,
                    x0,
                    method=method,
                    bounds=bounds,
                    options={
                        'maxiter': max_iterations,
                        'disp': True
                    }
                )
                
        finally:
            self.optimization_running = False
            
        print("-" * 50)
        print(f"Optimization complete!")
        print(f"Mode: {mode}")
        print(f"Best params: {self.best_params}")
        print(f"Param names: {self.waterfall.get_param_names(mode)}")
        print(f"Best score: {self.best_score}")
        print(f"Evaluations: {self.eval_count}")
        
        # Apply best parameters (already saved to INI during run)
        self.waterfall.set_parameters_from_array(self.best_params, mode)
        
        return {
            'success': result.success if hasattr(result, 'success') else True,
            'params': self.best_params,
            'param_names': self.waterfall.get_param_names(mode),
            'mode': mode,
            'score': self.best_score,
            'iterations': self.eval_count,
            'scipy_result': result
        }


def main():
    """Main entry point"""
    agent = ControllerAgent()
    
    if not agent.start():
        print("Failed to start controller")
        return
        
    try:
        print("\nController Agent started")
        print("Commands:")
        print("  o  - Optimize tick detector (threshold, adapt, duration)")
        print("  oc - Optimize correlator (confidence, max_misses)")
        print("  om - Optimize marker detector (threshold, adapt_rate, min_duration)")
        print("  os - Optimize sync detector (13 params, ~60s/eval)")
        print("  oa - Optimize all 22 parameters")
        print("  s  - Show current CORR telemetry") 
        print("  t  - Show all latest telemetry channels")
        print("  i  - Show saved INI values (overall best + last run)")
        print("  p  - Ping SDR")
        print("  v  - Get SDR version")
        print("  x  - Get SDR status")
        print("  q  - Quit")
        print()
        
        while True:
            cmd = input("> ").strip().lower()
            
            if cmd == 'q':
                break
            elif cmd == 'o':
                result = agent.optimize(max_iterations=50, mode='tick')
                print(f"\nOptimization result: {result}")
            elif cmd == 'oc':
                result = agent.optimize(max_iterations=50, mode='corr')
                print(f"\nOptimization result: {result}")
            elif cmd == 'om':
                # Marker optimization is slow (~3 min per eval)
                print("Note: Marker optimization takes ~3 min per evaluation")
                print("      15 iterations = ~45 minutes")
                result = agent.optimize(max_iterations=15, mode='marker')
                print(f"\nOptimization result: {result}")
            elif cmd == 'os':
                # Sync optimization (~60s per eval)
                print("Note: Sync optimization takes ~60s per evaluation")
                print("      30 iterations = ~30 minutes")
                result = agent.optimize(max_iterations=30, mode='sync')
                print(f"\nOptimization result: {result}")
            elif cmd == 'oa':
                result = agent.optimize(max_iterations=200, mode='all')
                print(f"\nOptimization result: {result}")
            elif cmd == 's':
                metrics = agent.telemetry.get_correlation_metrics()
                print(f"Correlation metrics: {metrics}")
            elif cmd == 't':
                print("Latest telemetry by channel:")
                with agent.telemetry.lock:
                    if not agent.telemetry.latest:
                        print("  (no telemetry received yet)")
                    for ch, msg in agent.telemetry.latest.items():
                        print(f"  {ch}: {msg.data[:60]}...")
            elif cmd == 'i':
                print("\n=== OVERALL BEST ===")
                if os.path.exists(agent.ini_path):
                    print(f"({agent.ini_path}):")
                    with open(agent.ini_path, 'r') as f:
                        print(f.read())
                else:
                    print("No overall best yet.")
                
                print("\n=== LAST RUN ===")
                if os.path.exists(agent.last_run_path):
                    print(f"({agent.last_run_path}):")
                    with open(agent.last_run_path, 'r') as f:
                        print(f.read())
                else:
                    print("No last run data yet.")
            elif cmd == 'p':
                ok, resp = agent.sdr.ping()
                print(f"Ping: {ok}, {resp}")
            elif cmd == 'v':
                ok, resp = agent.sdr.version()
                print(f"Version: {ok}, {resp}")
            elif cmd == 'x':
                ok, resp = agent.sdr.status()
                print(f"Status: {ok}, {resp}")
            else:
                print(f"Unknown command: {cmd}")
                
    except KeyboardInterrupt:
        print("\nInterrupted")
    finally:
        agent.stop()


if __name__ == "__main__":
    main()
