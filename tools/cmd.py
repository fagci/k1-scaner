#!/usr/bin/env python3
"""Send a single CDC command to k1-scaner and print the response.

Usage:
    ./tools/cmd.py 'CH_LS'                           # list channels
    ./tools/cmd.py 'SPECTRUM 400 470 25' > spec.csv  # dump spectrum
    ./tools/cmd.py 'SCR' > shot.pbm                  # screenshot
    ./tools/cmd.py 'LS'                              # list files
    ./tools/cmd.py 'CAT settings.cfg'                # read file
"""
import sys, time, serial

def main():
    if len(sys.argv) < 3:
        port = '/dev/ttyACM0'
        cmd_idx = 1
    else:
        port = sys.argv[1]
        cmd_idx = 2

    command = sys.argv[cmd_idx]
    timeout = 30 if command.startswith('SPECTRUM') or command.startswith('S ') or command == 'SCR' else 5

    ser = serial.Serial(port, 115200, timeout=2)
    time.sleep(0.3)
    ser.reset_input_buffer()
    ser.read_all()

    ser.write((command + '\n').encode())

    out = b''
    end = time.time() + timeout
    while time.time() < end:
        b = ser.read(1)
        if b:
            out += b
            if b == b'\n' and (b'DONE' in out or b'OK' in out or b'?' in out):
                # ждём ещё немного возможных данных
                time.sleep(0.2)
                out += ser.read_all()
                break
        else:
            time.sleep(0.05)

    sys.stdout.buffer.write(out)
    return 0

if __name__ == '__main__':
    sys.exit(main())
