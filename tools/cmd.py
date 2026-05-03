#!/usr/bin/env python3
"""Send a single CDC command and print response.

Usage:
    ./tools/cmd.py 'CH_LS'                           # list channels
    ./tools/cmd.py 'SPECTRUM 400 470 25' > spec.csv  # dump spectrum
    ./tools/cmd.py 'SCR' > shot.pbm                  # screenshot (~16KB)
    ./tools/cmd.py 'LS' | head -5                    # list files
"""
import sys, time, serial

def main():
    port = sys.argv[1] if len(sys.argv) >= 3 else '/dev/ttyACM0'
    cmd  = sys.argv[2] if len(sys.argv) >= 3 else sys.argv[1]
    timeout = 30 if cmd.startswith('SPECTRUM') else 5

    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(0.3)
    ser.reset_input_buffer()
    ser.read_all()
    ser.write((cmd + '\n').encode())

    out = b''
    t0 = time.time()
    # читаем блоками по 4KB
    while time.time() - t0 < timeout:
        b = ser.read(4096)
        if b:
            out += b
            # для SCR: 16KB достаточно
            if cmd == 'SCR' and len(out) >= 16384:
                break
            # для остальных: ждём маркер
            if b'DONE' in out or b'OK\n' in out or b'?\n' in out:
                time.sleep(0.1)
                out += ser.read_all()
                break
        else:
            if out: break
            time.sleep(0.05)

    sys.stdout.buffer.write(out)
    return 0

if __name__ == '__main__':
    sys.exit(main())
