#!/usr/bin/env python3
"""
Phoenix SDR TCP Protocol Test Script

Tests the protocol commands against a running server (or netcat mock).
First action: START the IQ stream, then test all other commands.

Usage:
    1. Start a mock server with netcat:
       ncat -l -p 4535 -k
       (Type responses manually like: OK, PONG, BYE, etc.)
    
    2. Or run against the real Phoenix SDR server
    
    3. Run this script:
       python test_protocol.py
"""

import socket
import sys
import time

HOST = 'localhost'
PORT = 4535
TIMEOUT = 5.0

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def log_send(cmd):
    print(f"{Colors.CYAN}>>> SEND:{Colors.RESET} {cmd}")

def log_recv(resp):
    print(f"{Colors.YELLOW}<<< RECV:{Colors.RESET} {resp}")

def log_pass(msg):
    print(f"{Colors.GREEN}[PASS]{Colors.RESET} {msg}")

def log_fail(msg):
    print(f"{Colors.RED}[FAIL]{Colors.RESET} {msg}")

def log_info(msg):
    print(f"{Colors.BOLD}[INFO]{Colors.RESET} {msg}")


class PhoenixSDRTester:
    def __init__(self, host=HOST, port=PORT):
        self.host = host
        self.port = port
        self.sock = None
        self.tests_passed = 0
        self.tests_failed = 0
    
    def connect(self):
        """Connect to the server."""
        log_info(f"Connecting to {self.host}:{self.port}...")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(TIMEOUT)
        try:
            self.sock.connect((self.host, self.port))
            log_pass(f"Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            log_fail(f"Connection failed: {e}")
            return False
    
    def send_command(self, cmd):
        """Send a command and return the response."""
        full_cmd = cmd + '\n'
        log_send(cmd)
        self.sock.sendall(full_cmd.encode('ascii'))
        
        # Read response (may be multiple lines for CAPS)
        response = b''
        try:
            while True:
                chunk = self.sock.recv(1024)
                if not chunk:
                    break
                response += chunk
                # Check if we have a complete response
                if response.endswith(b'\n'):
                    # For most commands, one line is enough
                    # For CAPS, wait for END
                    decoded = response.decode('ascii')
                    if cmd == 'CAPS':
                        if 'END' in decoded:
                            break
                    else:
                        break
        except socket.timeout:
            log_info("(timeout waiting for more data)")
        
        decoded = response.decode('ascii').strip()
        log_recv(decoded)
        return decoded
    
    def check_response(self, response, expected_start, test_name):
        """Check if response starts with expected value."""
        if response.startswith(expected_start):
            log_pass(f"{test_name}: Response starts with '{expected_start}'")
            self.tests_passed += 1
            return True
        else:
            log_fail(f"{test_name}: Expected '{expected_start}', got '{response}'")
            self.tests_failed += 1
            return False
    
    def check_response_exact(self, response, expected, test_name):
        """Check if response exactly matches expected value."""
        if response == expected:
            log_pass(f"{test_name}: Response is '{expected}'")
            self.tests_passed += 1
            return True
        else:
            log_fail(f"{test_name}: Expected '{expected}', got '{response}'")
            self.tests_failed += 1
            return False
    
    def run_tests(self):
        """Run all protocol tests."""
        print("\n" + "="*60)
        print(" PHOENIX SDR TCP PROTOCOL TEST")
        print("="*60 + "\n")
        
        if not self.connect():
            return False
        
        try:
            # ============================================================
            # FIRST: START the IQ stream (as requested)
            # ============================================================
            print("\n--- STREAMING: START (First Priority) ---")
            resp = self.send_command("START")
            self.check_response(resp, "OK", "START streaming")
            
            # ============================================================
            # Test STATUS while streaming
            # ============================================================
            print("\n--- STATUS (while streaming) ---")
            resp = self.send_command("STATUS")
            if self.check_response(resp, "OK", "STATUS command"):
                # Verify it contains STREAMING=1
                if "STREAMING=1" in resp:
                    log_pass("STATUS shows STREAMING=1")
                    self.tests_passed += 1
                else:
                    log_fail("STATUS should show STREAMING=1")
                    self.tests_failed += 1
            
            # ============================================================
            # Test PING
            # ============================================================
            print("\n--- PING ---")
            resp = self.send_command("PING")
            self.check_response_exact(resp, "PONG", "PING")
            
            # ============================================================
            # Test VER
            # ============================================================
            print("\n--- VER ---")
            resp = self.send_command("VER")
            self.check_response(resp, "OK", "VER")
            if "PHOENIX_SDR=" in resp and "PROTOCOL=" in resp:
                log_pass("VER contains version info")
                self.tests_passed += 1
            else:
                log_fail("VER should contain PHOENIX_SDR= and PROTOCOL=")
                self.tests_failed += 1
            
            # ============================================================
            # Test Frequency Commands (while streaming - may fail on some SDRs)
            # ============================================================
            print("\n--- FREQUENCY COMMANDS (while streaming) ---")
            
            # SET_FREQ - may return ERR HARDWARE if SDR doesn't support live updates
            resp = self.send_command("SET_FREQ 14100000")
            if resp.startswith("OK") or resp.startswith("ERR HARDWARE"):
                log_pass("SET_FREQ 14.1 MHz: Response OK or ERR HARDWARE (live update not supported)")
                self.tests_passed += 1
            else:
                log_fail(f"SET_FREQ: Unexpected response: {resp}")
                self.tests_failed += 1
            
            # GET_FREQ
            resp = self.send_command("GET_FREQ")
            if self.check_response(resp, "OK", "GET_FREQ"):
                # Should return the frequency we just set
                if "14100000" in resp:
                    log_pass("GET_FREQ returned correct frequency")
                    self.tests_passed += 1
                else:
                    log_info(f"GET_FREQ returned: {resp}")
            
            # Test out-of-range frequency
            resp = self.send_command("SET_FREQ 50")
            self.check_response(resp, "ERR", "SET_FREQ out of range (50 Hz)")
            
            # ============================================================
            # Test Gain Commands (while streaming - may fail on some SDRs)
            # ============================================================
            print("\n--- GAIN COMMANDS (while streaming) ---")
            
            # SET_GAIN - may return ERR HARDWARE if SDR doesn't support live updates
            resp = self.send_command("SET_GAIN 35")
            if resp.startswith("OK") or resp.startswith("ERR HARDWARE"):
                log_pass("SET_GAIN 35: Response OK or ERR HARDWARE (live update not supported)")
                self.tests_passed += 1
            else:
                log_fail(f"SET_GAIN: Unexpected response: {resp}")
                self.tests_failed += 1
            
            # GET_GAIN
            resp = self.send_command("GET_GAIN")
            self.check_response(resp, "OK", "GET_GAIN")
            
            # Test out-of-range gain
            resp = self.send_command("SET_GAIN 15")
            self.check_response(resp, "ERR", "SET_GAIN out of range (15)")
            
            # ============================================================
            # Test LNA Commands
            # ============================================================
            print("\n--- LNA COMMANDS ---")
            
            resp = self.send_command("SET_LNA 4")
            self.check_response(resp, "OK", "SET_LNA 4")
            
            resp = self.send_command("GET_LNA")
            self.check_response(resp, "OK", "GET_LNA")
            
            # ============================================================
            # Test AGC Commands (while streaming - may fail on some SDRs)
            # ============================================================
            print("\n--- AGC COMMANDS (while streaming) ---")
            
            # SET_AGC - may return ERR HARDWARE if SDR doesn't support live updates
            resp = self.send_command("SET_AGC OFF")
            if resp.startswith("OK") or resp.startswith("ERR HARDWARE"):
                log_pass("SET_AGC OFF: Response OK or ERR HARDWARE (live update not supported)")
                self.tests_passed += 1
            else:
                log_fail(f"SET_AGC OFF: Unexpected response: {resp}")
                self.tests_failed += 1
            
            resp = self.send_command("SET_AGC 50HZ")
            if resp.startswith("OK") or resp.startswith("ERR HARDWARE"):
                log_pass("SET_AGC 50HZ: Response OK or ERR HARDWARE")
                self.tests_passed += 1
            else:
                log_fail(f"SET_AGC 50HZ: Unexpected response: {resp}")
                self.tests_failed += 1
            
            resp = self.send_command("GET_AGC")
            self.check_response(resp, "OK", "GET_AGC")
            
            # Invalid AGC mode
            resp = self.send_command("SET_AGC AUTO")
            self.check_response(resp, "ERR", "SET_AGC invalid mode")
            
            # ============================================================
            # STOP streaming before testing antenna/srate/bw changes
            # (These require stopped state per SDR hardware constraints)
            # ============================================================
            print("\n--- STOP STREAMING ---")
            resp = self.send_command("STOP")
            self.check_response(resp, "OK", "STOP streaming")
            
            # Brief delay for hardware cooldown
            time.sleep(0.5)
            
            # ============================================================
            # Test Antenna Commands (must be stopped first)
            # ============================================================
            print("\n--- ANTENNA COMMANDS (stopped) ---")
            
            resp = self.send_command("SET_ANTENNA A")
            self.check_response(resp, "OK", "SET_ANTENNA A")
            
            resp = self.send_command("SET_ANTENNA B")
            self.check_response(resp, "OK", "SET_ANTENNA B")
            
            resp = self.send_command("SET_ANTENNA HIZ")
            self.check_response(resp, "OK", "SET_ANTENNA HIZ")
            
            resp = self.send_command("GET_ANTENNA")
            self.check_response(resp, "OK", "GET_ANTENNA")
            
            # ============================================================
            # Test Sample Rate Commands (only when not streaming)
            # ============================================================
            print("\n--- SAMPLE RATE COMMANDS ---")
            
            resp = self.send_command("SET_SRATE 6000000")
            self.check_response(resp, "OK", "SET_SRATE 6 MSPS")
            
            resp = self.send_command("GET_SRATE")
            self.check_response(resp, "OK", "GET_SRATE")
            
            # ============================================================
            # Test Bandwidth Commands (only when not streaming)
            # ============================================================
            print("\n--- BANDWIDTH COMMANDS ---")
            
            resp = self.send_command("SET_BW 1536")
            self.check_response(resp, "OK", "SET_BW 1536 kHz")
            
            resp = self.send_command("GET_BW")
            self.check_response(resp, "OK", "GET_BW")
            
            # ============================================================
            # Test Notch and Bias-T
            # ============================================================
            print("\n--- HARDWARE TOGGLES ---")
            
            resp = self.send_command("SET_NOTCH ON")
            self.check_response(resp, "OK", "SET_NOTCH ON")
            
            resp = self.send_command("SET_NOTCH OFF")
            self.check_response(resp, "OK", "SET_NOTCH OFF")
            
            # Bias-T OFF is safe
            resp = self.send_command("SET_BIAST OFF")
            self.check_response(resp, "OK", "SET_BIAST OFF")
            
            # ============================================================
            # Test RESTART streaming (with delay for hardware cooldown)
            # ============================================================
            print("\n--- RESTART STREAMING ---")
            
            # Wait for hardware cooldown
            log_info("Waiting 1s for hardware cooldown...")
            time.sleep(1.0)
            
            resp = self.send_command("START")
            if resp.startswith("OK"):
                log_pass("START again: Successfully restarted")
                self.tests_passed += 1
                
                # Try START while already streaming - should fail with ERR STATE
                resp = self.send_command("START")
                self.check_response(resp, "ERR", "START while streaming (should error)")
            elif resp.startswith("ERR HARDWARE"):
                log_pass("START again: ERR HARDWARE (SDR needs longer cooldown or reinit)")
                self.tests_passed += 1
                log_info("Skipping double-START test since streaming didn't start")
            else:
                log_fail(f"START again: Unexpected response: {resp}")
                self.tests_failed += 1
            
            # ============================================================
            # Final STATUS check
            # ============================================================
            print("\n--- FINAL STATUS ---")
            resp = self.send_command("STATUS")
            self.check_response(resp, "OK", "Final STATUS")
            log_info(f"Full status: {resp}")
            
            # ============================================================
            # Test HELP
            # ============================================================
            print("\n--- HELP ---")
            resp = self.send_command("HELP")
            self.check_response(resp, "OK", "HELP")
            
            # ============================================================
            # CLEANUP - STOP (may already be stopped) and QUIT
            # ============================================================
            print("\n--- CLEANUP ---")
            resp = self.send_command("STOP")
            # Accept both OK and ERR STATE (not streaming) as valid
            if resp.startswith("OK") or "not streaming" in resp:
                log_pass("Final STOP: OK or already stopped")
                self.tests_passed += 1
            else:
                log_fail(f"Final STOP: Unexpected response: {resp}")
                self.tests_failed += 1
            
            resp = self.send_command("QUIT")
            self.check_response_exact(resp, "BYE", "QUIT")
            
        except Exception as e:
            log_fail(f"Test error: {e}")
            self.tests_failed += 1
        
        finally:
            self.sock.close()
        
        # ============================================================
        # Summary
        # ============================================================
        print("\n" + "="*60)
        print(" TEST SUMMARY")
        print("="*60)
        total = self.tests_passed + self.tests_failed
        print(f"  Passed: {Colors.GREEN}{self.tests_passed}{Colors.RESET}")
        print(f"  Failed: {Colors.RED}{self.tests_failed}{Colors.RESET}")
        print(f"  Total:  {total}")
        print("="*60 + "\n")
        
        return self.tests_failed == 0


def interactive_mode():
    """Interactive mode for manual testing."""
    print("\n" + "="*60)
    print(" PHOENIX SDR INTERACTIVE MODE")
    print("="*60)
    print(" Type commands to send to the server.")
    print(" Type 'quit' or 'exit' to disconnect.")
    print("="*60 + "\n")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(TIMEOUT)
    
    try:
        sock.connect((HOST, PORT))
        log_pass(f"Connected to {HOST}:{PORT}")
    except Exception as e:
        log_fail(f"Connection failed: {e}")
        return
    
    try:
        while True:
            try:
                cmd = input(f"{Colors.CYAN}Command> {Colors.RESET}").strip()
            except EOFError:
                break
            
            if not cmd:
                continue
            
            if cmd.lower() in ['quit', 'exit']:
                sock.sendall(b"QUIT\n")
                resp = sock.recv(256).decode('ascii').strip()
                log_recv(resp)
                break
            
            sock.sendall((cmd + '\n').encode('ascii'))
            
            try:
                resp = sock.recv(1024).decode('ascii').strip()
                log_recv(resp)
            except socket.timeout:
                log_info("(no response - timeout)")
    
    finally:
        sock.close()
        log_info("Disconnected")


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '-i':
        interactive_mode()
    else:
        tester = PhoenixSDRTester()
        success = tester.run_tests()
        sys.exit(0 if success else 1)
