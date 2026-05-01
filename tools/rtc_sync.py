#!/usr/bin/env python3
"""Set RTC on k1-scaner to current PC time.

Usage:
    python3 rtc_sync.py /dev/ttyACM0
"""
import sys, time, serial

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    port = sys.argv[1]
    ser = serial.Serial(port, 115200, timeout=2)
    time.sleep(0.5)
    ser.write(b'h\n')
    resp = ser.read(200)
    print("Response:", resp.decode(errors='replace'))
    ts = int(time.time())
    ser.write(f"T {ts}\n".encode())
    resp = ser.read(100)
    print("Set RTC:", resp.decode(errors='replace'))
    ser.write(b'D\n')
    resp = ser.read(100)
    print("RTC date:", resp.decode(errors='replace'))
    ser.close()
    return 0

if __name__ == "__main__":
    sys.exit(main())
