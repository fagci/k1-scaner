# k1-scaner

Minimalist spectrum scanner firmware for UV-K1/K5/K6 radios (PY32F071 + BK4819).  
Designed for fast, automated scanning with USB CDC control and on-device UI.

## Features

- **USB CDC virtual COM port** — control via serial terminal (115200 baud, DTR required)
- **Range scanning** — preset ranges (144-176, 400-470 MHz) or custom via CDC
- **Named scanlists** — create and scan subsets of channels
- **Blacklist** — exclude frequencies and ranges from scanning (BLOOM / LPD filters)
- **Loot** — discovered frequencies, deduplicated, auto-save to channels
- **Channels** — persistent storage, add/remove/list
- **RxProfiles** — per-frequency gain, squelch, modulation presets
- **On-device UI** — 5 Tabs (Range, Loot, Channels, Scanlist, Blacklist) + Settings
- **BK4819 register read/write** — `K 38`, `K_WR 40 1F40`
- **Autotests** — `tools/test_cmds.py`, 21/21 passed

## Hardware

- MCU: PY32F071 (Cortex-M0+, 48MHz, 16KB RAM, 128KB Flash)
- Radio: BK4829 (BK4819 compatible)
- Display: ST7565 128x64
- Storage: PY25Q16 SPI flash + LittleFS
- USB: CDC ACM via CherryUSB stack

## Quick Start

### Build

```bash
make
```

Flash `bin/k1-scaner.bin` using [flash.py](flash.py) or your preferred tool.

### Connect

```bash
screen /dev/ttyACM0 115200
# or
python3 tools/test_cmds.py /dev/ttyACM0
```

Requires DTR enabled (most terminals do this automatically).

## CDC Commands

| Command | Description |
|---------|-------------|
| `S a b st [k=v]` | Range scan: `S 430 440 25 sql=3 wl=1` |
| `CH_ADD f [name]` | Add channel: `CH_ADD 172.3 DPS` |
| `CH_LS` | List channels |
| `CH_RM i` | Remove channel |
| `SCAN_ADD l h st` | Add range as channel: `SCAN_ADD 430 440 25` |
| `SL_ADD i..` | Add channels to active scanlist |
| `SL_WR n i..` | Write named scanlist |
| `SL_LS` | List active scanlist |
| `G` | Scan active scanlist |
| `SCAN_SL n` | Scan named scanlist |
| `LOOP n` | Loop scan named scanlist (infinite) |
| `BL_ADD f [f2]` | Blacklist freq or range |
| `BL_LS / BL_WR / BL_RD` | Blacklist control |
| `LW_LS / LW_CLR` | Loot list / clear |
| `PR_WR i f g s m` | Write RxProfile |
| `PR_LS` | List RxProfiles |
| `K hex` | Read BK4829 register |
| `K_WR hex val` | Write BK4829 register |
| `U` | Uptime in ms |
| `RESET` | Clear all stored data |

## On-Device UI

5 tabs switched by **MENU** button:

| Tab | Controls |
|-----|----------|
| **Rng** | UP/DOWN select range, SIDE2 start scan |
| **Loot** | UP/DOWN navigate, SIDE1 → BL, SIDE2 → WL, EXIT delete |
| **CH** | UP/DOWN navigate, SIDE1 listen, EXIT delete |
| **SL** | UP/DOWN navigate, SIDE2 scan list |
| **BL** | UP/DOWN navigate, EXIT delete |
| **Set** | UP/DOWN select field, STAR (+), F (-) |

Scan progress shown in list area. Any key stops scanning.

## Project Structure

```
src/
  main.c             — init, main loop, CDC console
  app/app.c          — FSM: IDLE / SCAN / LISTEN
  app/ui.c           — display, tabs, key handling
  driver/            — BK4829, ST7565, USB CDC, GPIO, IWDG, RTC, timers
  helper/            — measurements, storage (LittleFS)
  inc/               — radio_types.h, common types
  ui/                — graphics primitives, fonts
  external/          — CMSIS, CherryUSB, PY32 HAL, littlefs, printf
tools/
  test_cmds.py       — autotest 21/21 CDC commands
  rtc_sync.py        — set RTC via USB
```

## License

Apache 2.0
