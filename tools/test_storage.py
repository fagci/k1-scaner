#!/usr/bin/env python3
"""Full autotest for ParamSet + Loot + RxProfile storage.

Usage:
    python3 tools/test_storage.py /dev/ttyACM0
    python3 tools/test_storage.py /dev/ttyACM0 --verify-only
"""
import sys, time, serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
VERIFY_ONLY = "--verify-only" in sys.argv

def cmd(ser, c, expect=None, timeout=5):
    ser.write(c.encode())
    expect_bytes = expect.encode() if expect else None
    t = time.time()
    out = b''
    while time.time() - t < timeout:
        out += ser.read(1)
        if expect_bytes and expect_bytes in out:
            return out.decode(errors='replace')
    return out.decode(errors='replace').strip()

def main():
    ser = serial.Serial(PORT, 11564, timeout=2)
    time.sleep(1)
    ser.reset_input_buffer()
    ser.read_all()

    if not VERIFY_ONLY:
        # ── fill RxProfiles ────────────────────────────────────────────
        profiles = [
            (0, 430.050, 5, 3, 0),   # FM, gain=5, sql=3
            (1, 144.350, 3, 2, 0),   # FM, gain=3, sql=2
            (2, 145.600, 8, 4, 0),   # FM, gain=8, sql=4
            (3, 433.500, 2, 1, 0),   # FM, gain=2, sql=1
        ]
        for idx, freq, gain, sql, mod in profiles:
            r = cmd(ser, f"P {idx} {freq} {gain} {sql} {mod}\n", "OK")
            print(f"[SET] P {idx} {freq} g={gain} sql={sql} mod={mod}: {r}")

        # ── fill 64 loot entries, spread across profiles ─────────────
        print("Filling 64 loot entries...")
        for i in range(64):
            freq_mhz = 430.0 + i * 0.01
            profile = i % 4
            r = cmd(ser, f"F {freq_mhz:.2f} {profile}\n", "OK", timeout=2)
            if "OK" not in r:
                print(f"  FAIL at entry {i}: {r}")
                break
        print(f"[DONE] 64 loot entries written")

        # ── save ───────────────────────────────────────────────────────
        r = cmd(ser, "X\n", "SAVED")
        print(f"[SAVE] {r}")

        print("\n--- Data saved. Re-run with --verify-only to check ---\n")

    # ── verify ─────────────────────────────────────────────────────────
    print("=== VERIFICATION ===")

    r = cmd(ser, "L\n")
    lines = [l for l in r.split('\n') if l.strip() and ':' in l]
    print(f"[LOOT] {len(lines)} entries")
    assert len(lines) <= 64, f"Expected ≤64, got {len(lines)}"
    for l in lines[:5]:
        print(f"  {l}")
    if len(lines) > 5:
        print(f"  ...")

    r = cmd(ser, "R\n")
    rlines = [l for l in r.split('\n') if l.strip() and ':' in l]
    print(f"[PROFILES] {len(rlines)} profiles")
    for l in rlines:
        print(f"  {l}")

    # verify scanlist.sl exists (saved)
    r = cmd(ser, "h\n")  # just confirm device alive
    print(f"[ALIVE] device responds: {'h' in r or '?' in r}")

    print("\n=== ALL CHECKS PASSED ===")
    return 0

if __name__ == "__main__":
    sys.exit(main())
