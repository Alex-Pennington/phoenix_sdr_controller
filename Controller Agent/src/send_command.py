"""
Send a command to waterfall.exe to enable CORR telemetry.
"""

import socket
import sys

CMD_PORT = 3006

def send_command(cmd: str, host: str = "127.0.0.1"):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(f"{cmd}\n".encode('utf-8'), (host, CMD_PORT))
        print(f"Sent: {cmd}")
    finally:
        sock.close()

def main():
    if len(sys.argv) > 1:
        cmd = " ".join(sys.argv[1:])
    else:
        cmd = "ENABLE_TELEM CORR"
    
    send_command(cmd)
    print(f"Command sent to UDP port {CMD_PORT}")

if __name__ == "__main__":
    main()
