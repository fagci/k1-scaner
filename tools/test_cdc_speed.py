#!/usr/bin/env python3
"""CDC speed test: send bulk data, measure throughput.

Usage:
    python3 tools/test_cdc_speed.py /dev/ttyACM0
"""
import sys, time, serial

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM0'
    ser = serial.Serial(port, 115200, timeout=10)
    time.sleep(0.5)
    ser.reset_input_buffer()
    ser.read_all()

    # 1. send 1KB via command output (CH_LS with many channels)
    print("=== CDC Speed Test ===")

    # fill 200 loot entries via S (range scan with fast noise threshold)
    ser.write(b'LW_CLR\n')
    time.sleep(0.2)
    ser.read_all()

    # measure throughput: read a lot of data
    t0 = time.time()
    ser.write(b'CH_LS\n')
    data = ser.read_until(b'\n', 5)
    t1 = time.time()
    dt = t1 - t0
    print(f"CH_LS response: {len(data)} bytes in {dt*1000:.1f}ms = {len(data)/dt/1024:.1f} KB/s")

    # 2. SCR benchmark
    t0 = time.time()
    ser.write(b'SCR\n')
    # read all data until timeout
    out = b''
    while time.time() - t0 < 10:
        b = ser.read(1024)
        if b:
            out += b
            # PBM ends with last line
            if out.count(b'\n') >= 66:
                break
        else:
            break
    t1 = time.time()
    dt = t1 - t0
    print(f"SCR: {len(out)} bytes in {dt*1000:.1f}ms = {len(out)/dt/1024:.1f} KB/s")
    if out[:4] == b'P1\n':
        lines = out.decode(errors='replace').count('\n')
        print(f"  Lines: {lines} (expect 66)")
        if lines >= 66:
            print("  PASS: full PBM received")
        else:
            print("  FAIL: truncated PBM")
    else:
        print(f"  FAIL: bad header {out[:20]}")

    print("=== Done ===")
    return 0

if __name__ == '__main__':
    sys.exit(main())
