"""
Simple test to verify UDP telemetry reception from waterfall.exe
Run this while waterfall is running to see if CORR messages come through.
"""

import socket
import sys

TELEM_PORT = 3005

def main():
    print(f"Listening for telemetry on UDP port {TELEM_PORT}...")
    print("Make sure waterfall.exe is running and CORR telemetry is enabled.")
    print("Press Ctrl+C to stop.\n")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", TELEM_PORT))
    sock.settimeout(5.0)
    
    count = 0
    try:
        while True:
            try:
                data, addr = sock.recvfrom(4096)
                text = data.decode('utf-8').strip()
                
                # Filter for CORR messages
                if text.startswith("CORR"):
                    count += 1
                    print(f"[{count}] {text[:100]}...")
                elif count == 0:
                    # Show first few of any type so we know something is coming
                    print(f"  {text[:60]}...")
                    
            except socket.timeout:
                print("(waiting for data...)")
                
    except KeyboardInterrupt:
        print(f"\nReceived {count} CORR messages.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
