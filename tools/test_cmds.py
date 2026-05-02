#!/usr/bin/env python3
"""Автотест всех команд k1-scaner."""
import sys, time, serial

class Radio:
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=3)
        time.sleep(0.5)
        self.flush()
    def flush(self):
        self.ser.reset_input_buffer()
        self.ser.read_all()
        time.sleep(0.1)
        self.ser.read_all()
    def cmd(self, c, timeout=2):
        self.flush()
        self.ser.write((c + '\n').encode())
        end = time.time() + timeout
        out = b''
        while time.time() < end:
            b = self.ser.read(1)
            if b:
                out += b
                # ждём \n
                if b == b'\n':
                    # ещё один шанс на вторую строку
                    time.sleep(0.2)
                    out += self.ser.read_all()
                    break
            else:
                time.sleep(0.1)
        return out.decode(errors='replace').strip()

    def check(self, name, cmd, expect_substr):
        r = self.cmd(cmd)
        if expect_substr in r:
            print(f"  OK {name}: {r[:60]}")
            return True
        else:
            print(f"FAIL {name}: '{cmd}' → {repr(r[:80])} (expected '{expect_substr}')")
            return False

    def run(self):
        ok = 0; total = 0
        def t(n, c, e, to=2):
            nonlocal ok, total
            total += 1
            if self.check(n, c, e): ok += 1

        t("ready", "U", "ms")
        print("\n=== CHANNELS ===")
        t("CH_ADD freq", "CH_ADD 172.3", "CH0")
        t("CH_ADD range", "SCAN_ADD 430 440 25", "OK")
        t("CH_LS", "CH_LS", "channels")
        t("CH_RM", "CH_RM 1", "OK")
        t("CH_LS after rm", "CH_LS", "1 channels")

        print("\n=== SCANLISTS ===")
        t("SL_ADD", "SL_ADD 0", "SL 1")
        t("SL_LS", "SL_LS", "active SL")
        t("SL_WR", "SL_WR test 0", "scanlist_test 1")
        t("G", "G", "DONE")
        t("SCAN_SL", "SCAN_SL test", "DONE")

        print("\n=== BLACKLIST ===")
        t("BL_ADD freq", "BL_ADD 172.3", "OK")
        t("BL_ADD range", "BL_ADD 410 420", "OK")
        t("BL_WR", "BL_WR", "OK")
        t("BL_RD", "BL_RD", "2")

        print("\n=== LOOT ===")
        t("LW_LS", "LW_LS", "total=")
        t("LW_CLR", "LW_CLR", "OK")

        print("\n=== PROFILES ===")
        t("PR_WR", "PR_WR 0 144.5 3 2 0", "OK")
        t("PR_LS", "PR_LS", "14450000")

        print("\n=== SYSTEM ===")
        t("U", "U", "ms")
        t("K", "K 38", "R38=0x")

        print(f"\n{'='*20}\n{ok}/{total} passed")
        return total - ok

if __name__ == '__main__':
    r = Radio(sys.argv[1])
    sys.exit(r.run())
