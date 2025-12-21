# sdr_controller.py
# TCP connection to sdr_server control port (4535)
# Protocol: Text-based commands like SET_FREQ, SET_GAIN, START, STOP, etc.

import socket
from typing import Optional, Tuple

class SDRController:
    """
    TCP client for sdr_server control.
    
    Protocol (from tcp_server.h):
        Commands: SET_FREQ, GET_FREQ, SET_GAIN, GET_GAIN, SET_LNA, GET_LNA,
                  SET_AGC, GET_AGC, SET_SRATE, GET_SRATE, SET_BW, GET_BW,
                  SET_ANTENNA, GET_ANTENNA, SET_BIAST, SET_NOTCH,
                  SET_DECIM, GET_DECIM, SET_IFMODE, GET_IFMODE,
                  SET_DCOFFSET, GET_DCOFFSET, SET_IQCORR, GET_IQCORR,
                  SET_AGC_SETPOINT, GET_AGC_SETPOINT,
                  START, STOP, STATUS, PING, VER, CAPS, HELP, QUIT
        
        Response: "OK [value]" or "ERR <code> [message]"
    """
    
    DEFAULT_PORT = 4535
    
    def __init__(self, host: str = "127.0.0.1", port: int = DEFAULT_PORT):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        
    def connect(self) -> bool:
        """Connect to sdr_server control port"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.socket.settimeout(5.0)
            return True
        except socket.error as e:
            print(f"SDR connection failed: {e}")
            return False
        
    def disconnect(self):
        """Disconnect from sdr_server"""
        if self.socket:
            try:
                self._send_command("QUIT")
            except:
                pass
            self.socket.close()
            self.socket = None
            
    def _send_command(self, cmd: str) -> Tuple[bool, str]:
        """Send command and receive response"""
        if not self.socket:
            return False, "Not connected"
        
        try:
            self.socket.sendall(f"{cmd}\n".encode('utf-8'))
            response = self.socket.recv(1024).decode('utf-8').strip()
            
            # Handle different response formats
            if response.startswith("OK"):
                return True, response[3:].strip() if len(response) > 2 else ""
            elif response == "PONG":
                return True, "PONG"
            elif response == "BYE":
                return True, "BYE"
            elif response.startswith("ERR"):
                return False, response
            else:
                # Unknown format - return as-is
                return True, response
        except socket.error as e:
            return False, str(e)
    
    # Frequency
    def set_frequency(self, freq_hz: float) -> Tuple[bool, str]:
        """Set center frequency (1000 - 2000000000 Hz)"""
        return self._send_command(f"SET_FREQ {freq_hz:.0f}")
        
    def get_frequency(self) -> Tuple[bool, str]:
        """Get current frequency"""
        return self._send_command("GET_FREQ")
    
    # Gain
    def set_gain(self, gain_reduction: int) -> Tuple[bool, str]:
        """Set gain reduction (20-59 dB)"""
        return self._send_command(f"SET_GAIN {gain_reduction}")
        
    def get_gain(self) -> Tuple[bool, str]:
        """Get current gain reduction"""
        return self._send_command("GET_GAIN")
        
    def set_lna(self, state: int) -> Tuple[bool, str]:
        """Set LNA state (0-8 for A/B, 0-4 for Hi-Z)"""
        return self._send_command(f"SET_LNA {state}")
        
    def get_lna(self) -> Tuple[bool, str]:
        """Get LNA state"""
        return self._send_command("GET_LNA")
        
    def set_agc(self, mode: str) -> Tuple[bool, str]:
        """Set AGC mode (OFF, 5HZ, 50HZ, 100HZ)"""
        return self._send_command(f"SET_AGC {mode}")
        
    def get_agc(self) -> Tuple[bool, str]:
        """Get AGC mode"""
        return self._send_command("GET_AGC")
    
    def set_agc_setpoint(self, dbfs: int) -> Tuple[bool, str]:
        """Set AGC setpoint (-72 to 0 dBFS)"""
        return self._send_command(f"SET_AGC_SETPOINT {dbfs}")
    
    # Sample Rate / Bandwidth
    def set_sample_rate(self, rate: int) -> Tuple[bool, str]:
        """Set sample rate (2000000 - 10000000 Hz)"""
        return self._send_command(f"SET_SRATE {rate}")
        
    def get_sample_rate(self) -> Tuple[bool, str]:
        """Get sample rate"""
        return self._send_command("GET_SRATE")
        
    def set_bandwidth(self, bw_khz: int) -> Tuple[bool, str]:
        """Set bandwidth (200, 300, 600, 1536, 5000, 6000, 7000, 8000 kHz)"""
        return self._send_command(f"SET_BW {bw_khz}")
        
    def get_bandwidth(self) -> Tuple[bool, str]:
        """Get bandwidth"""
        return self._send_command("GET_BW")
    
    # Hardware
    def set_antenna(self, port: str) -> Tuple[bool, str]:
        """Set antenna port (A, B, HIZ)"""
        return self._send_command(f"SET_ANTENNA {port}")
        
    def get_antenna(self) -> Tuple[bool, str]:
        """Get antenna port"""
        return self._send_command("GET_ANTENNA")
    
    def set_decimation(self, factor: int) -> Tuple[bool, str]:
        """Set decimation factor (1, 2, 4, 8, 16, 32)"""
        return self._send_command(f"SET_DECIM {factor}")
    
    # Streaming
    def start(self) -> Tuple[bool, str]:
        """Start streaming"""
        return self._send_command("START")
        
    def stop(self) -> Tuple[bool, str]:
        """Stop streaming"""
        return self._send_command("STOP")
        
    def status(self) -> Tuple[bool, str]:
        """Get status"""
        return self._send_command("STATUS")
        
    def ping(self) -> Tuple[bool, str]:
        """Ping server"""
        return self._send_command("PING")
        
    def version(self) -> Tuple[bool, str]:
        """Get version"""
        return self._send_command("VER")
