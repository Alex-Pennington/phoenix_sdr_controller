#!/usr/bin/env python3
"""
Phoenix SDR Relay Controller
Connects to signal relay and controls remote SDR server
"""

import socket
import time
import sys

# Relay configuration
RELAY_HOST = "146.190.112.225"
RELAY_PORT = 3001

# SDR configuration
TARGET_FREQ_HZ = 10000000      # 10 MHz
TARGET_ANTENNA = "HIZ"          # Hi-Z antenna
TARGET_LNA = 2                  # LNA state 2
TARGET_GAIN = 50                # Gain reduction 50 dB

# Timing
STATUS_POLL_INTERVAL = 10       # Poll status every 10 seconds (relay mode)
KEEPALIVE_INTERVAL = 60         # Send PING every 60 seconds
SOCKET_TIMEOUT = 5              # Socket timeout in seconds


class SDRRelayController:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.connected = False
        self.last_status_poll = 0
        self.last_keepalive = 0
        
    def connect(self):
        """Connect to the relay server"""
        try:
            print(f"Connecting to relay at {self.host}:{self.port}...")
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(SOCKET_TIMEOUT)
            
            # Enable TCP_NODELAY for low latency
            self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            
            self.sock.connect((self.host, self.port))
            self.connected = True
            print(f"Connected to relay at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from relay"""
        if self.sock:
            try:
                self.send_command("QUIT")
            except:
                pass
            self.sock.close()
            self.sock = None
        self.connected = False
        print("Disconnected")
    
    def send_command(self, command):
        """Send command and receive response"""
        if not self.connected or not self.sock:
            return None
        
        try:
            # Send command with newline
            msg = f"{command}\n".encode('ascii')
            self.sock.sendall(msg)
            print(f"TX: {command}")
            
            # Receive response (read until newline)
            response = b""
            while b'\n' not in response:
                chunk = self.sock.recv(1)
                if not chunk:
                    raise ConnectionError("Connection closed by remote")
                response += chunk
            
            # Decode and strip newline
            response = response.decode('ascii').strip()
            print(f"RX: {response}")
            return response
            
        except socket.timeout:
            print("ERROR: Command timeout")
            return None
        except Exception as e:
            print(f"ERROR: {e}")
            self.connected = False
            return None
    
    def check_response(self, response):
        """Check if response is OK"""
        if not response:
            return False
        return response.startswith("OK")
    
    def configure_sdr(self):
        """Configure SDR to target settings"""
        print("\nConfiguring SDR...")
        
        # Set frequency
        response = self.send_command(f"SET_FREQ {TARGET_FREQ_HZ}")
        if not self.check_response(response):
            print(f"  Failed to set frequency: {response}")
            return False
        print(f"  ✓ Frequency set to {TARGET_FREQ_HZ} Hz (10 MHz)")
        
        # Set antenna
        response = self.send_command(f"SET_ANTENNA {TARGET_ANTENNA}")
        if not self.check_response(response):
            print(f"  Failed to set antenna: {response}")
            return False
        print(f"  ✓ Antenna set to {TARGET_ANTENNA}")
        
        # Set LNA state
        response = self.send_command(f"SET_LNA {TARGET_LNA}")
        if not self.check_response(response):
            print(f"  Failed to set LNA: {response}")
            return False
        print(f"  ✓ LNA state set to {TARGET_LNA}")
        
        # Set gain reduction
        response = self.send_command(f"SET_GAIN {TARGET_GAIN}")
        if not self.check_response(response):
            print(f"  Failed to set gain: {response}")
            return False
        print(f"  ✓ Gain reduction set to {TARGET_GAIN} dB")
        
        print("Configuration complete!\n")
        return True
    
    def get_status(self):
        """Get current SDR status"""
        response = self.send_command("STATUS")
        if response and response.startswith("OK"):
            # Parse key=value pairs
            status = {}
            parts = response.split()[1:]  # Skip "OK"
            for part in parts:
                if '=' in part:
                    key, value = part.split('=', 1)
                    status[key] = value
            return status
        return None
    
    def maintain_connection(self):
        """Maintain connection with periodic status polls and keepalives"""
        now = time.time()
        
        # Status poll
        if now - self.last_status_poll >= STATUS_POLL_INTERVAL:
            self.last_status_poll = now
            status = self.get_status()
            if status:
                print(f"\n[Status] FREQ={status.get('FREQ', '?')} Hz, "
                      f"GAIN={status.get('GAIN', '?')} dB, "
                      f"LNA={status.get('LNA', '?')}, "
                      f"ANT={status.get('ANT', '?')}, "
                      f"STREAM={status.get('STREAM', '?')}")
        
        # Keepalive ping
        if now - self.last_keepalive >= KEEPALIVE_INTERVAL:
            self.last_keepalive = now
            response = self.send_command("PING")
            if response != "PONG":
                print("WARNING: Keepalive failed")
                return False
        
        return True
    
    def run(self):
        """Main control loop"""
        if not self.connect():
            return False
        
        try:
            # Configure SDR to target settings
            if not self.configure_sdr():
                print("Configuration failed!")
                return False
            
            # Initialize timers
            self.last_status_poll = time.time()
            self.last_keepalive = time.time()
            
            # Main loop - maintain connection and monitor status
            print("Maintaining connection... (Press Ctrl+C to exit)")
            while self.connected:
                if not self.maintain_connection():
                    print("Connection lost!")
                    break
                time.sleep(1)
            
        except KeyboardInterrupt:
            print("\n\nShutdown requested by user")
        except Exception as e:
            print(f"\nError: {e}")
        finally:
            self.disconnect()
        
        return True


def main():
    print("=" * 60)
    print("Phoenix SDR Relay Controller")
    print("=" * 60)
    print(f"Target: {RELAY_HOST}:{RELAY_PORT}")
    print(f"Config: {TARGET_FREQ_HZ/1e6:.1f} MHz, {TARGET_ANTENNA} antenna, "
          f"LNA={TARGET_LNA}, GAIN={TARGET_GAIN} dB")
    print("=" * 60)
    
    controller = SDRRelayController(RELAY_HOST, RELAY_PORT)
    success = controller.run()
    
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
