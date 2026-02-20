# GC9D01 Zephyr Examples

LVGL examples for the **Waveshare 0.71" Round LCD** (GC9D01 controller, 160×160) running on **Zephyr RTOS** with an **nRF52832** custom board (`bruno_nrf52832`).

---

## Examples

| Branch | Description |
|--------|-------------|
| `main` | **Animated Eye** — animated eyeball with iris, sclera, eyelids and blink |
| [`acelerometer-chart`](../../tree/acelerometer-chart) | **Accelerometer Chart** — real-time 3-axis LVGL chart via FXOS8700CQ |

---

## Animated Eye (`main`)

Port of the [Adafruit UncannyEyes](https://github.com/adafruit/Uncanny_Eyes) / Bodmer TFT_eSPI adaptation to **Zephyr RTOS + LVGL**, targeting the GC9D01 160×160 round display.

```
        ┌─────────────────────┐
        │   160×160 display   │
        │                     │
        │  ┌───────────────┐  │
        │  │  128×128 eye  │  │
        │  │    ○  sclera  │  │
        │  │   ◉  iris     │  │
        │  │  ███ eyelid   │  │
        │  └───────────────┘  │
        │                     │
        └─────────────────────┘
```

### Features

- **Smooth eye movement** — pupil wanders randomly with inertia
- **Iris animation** — size pulses autonomously (simulates light response)
- **Eyelid tracking** — upper eyelid follows pupil vertically (`TRACKING`)
- **Auto-blink** — random autonomous blinking (`AUTOBLINK`, disabled by default)
- **Manual blink** — press `button0` (P0.25) to trigger a blink
- **Pure LVGL canvas** — 128×128 RGB565 canvas, no direct display writes
- **~157 KB eye textures** — sclera (200×200), iris map (256×64), polar map (80×80), eyelid tables (128×128) stored in flash

### Configuration

All options are `#define` flags at the top of `app/src/eye_renderer.c`:

```c
#define EYE_1_XPOSITION  20   /* x offset of the 128×128 canvas on the 160px-wide display */

//#define AUTOBLINK             /* autonomous blinking — uncomment to enable */
#define TRACKING              /* upper eyelid follows pupil movement       */

#define IRIS_MIN   90         /* iris size in bright light (smaller)       */
#define IRIS_MAX   130        /* iris size in darkness (larger)            */
```

### Flash layout

The eye texture tables (~157 KB) require expanding the application flash slot:

| Partition | Start | Size |
|-----------|-------|------|
| MCUboot | `0x00000` | 48 KB |
| **slot0 (app)** | `0x0C000` | **440 KB** |
| storage | `0x7A000` | 24 KB |

> **Note:** slot1 (OTA) is removed. Restore it in the overlay if OTA is needed.

---

## Accelerometer Chart (`acelerometer-chart` branch)

Real-time 3-axis accelerometer chart using the **NXP FXOS8700CQ** sensor over I2C and **LVGL**'s chart widget.

- X axis → red line
- Y axis → blue line
- Z axis → green line

Labels are inset from the display edges to remain within the round screen area.

---

## Hardware

### Board

**`bruno_nrf52832`** — custom nRF52832 board
64 KB RAM · 512 KB Flash · 64 MHz Cortex-M4F

### Display

| Property | Value |
|----------|-------|
| Product | [Waveshare 0.71" Round LCD Module](https://www.waveshare.com/0.71inch-lcd-module.htm) |
| Controller | GC9D01N |
| Resolution | 160×160 px (round IPS, 65K colors) |
| Interface | 4-wire SPI via MIPI DBI |
| Connector | SH1.0 8-pin |
| Supply | 3.3 V / 5 V |

> **Important:** The built-in Zephyr driver `galaxycore,gc9x01x` uses the GC9A01 init sequence, which is **incompatible** with the GC9D01N.
> This repo includes a custom out-of-tree driver in `custom_driver_module/` with the correct BOE panel init.

### Pinout — Display SH1.0 → Bruno nRF52832

| Module pin | Signal | nRF52832 pin |
|-----------|--------|-------------|
| VCC | Power | 3.3 V |
| GND | Ground | GND |
| DIN | MOSI | P0.12 |
| CLK | SCK | P0.11 |
| CS | Chip Select | P0.19 |
| DC | Data/Command | P0.20 |
| RST | Reset | P0.22 |
| BL | Backlight | 3.3 V (always on) |

### Button (Animated Eye only)

| Function | Pin |
|----------|-----|
| Blink (manual) | P0.25 (`button0` / `sw0`) |

---

## Building & Flashing

```bash
# Clone with west
west init -m <this-repo-url>
west update

# Build (from the app/ directory)
west build -b bruno_nrf52832/nrf52832 app/

# Flash
west flash
```

---

## Project Structure

```
GC9D01_ZEPHYR_EXAMPLES/
├── app/
│   ├── src/
│   │   ├── main.c               # Entry point, LVGL loop
│   │   ├── eye_renderer.c       # Animated eye logic (port of UncannyEyes)
│   │   ├── eye_renderer.h
│   │   └── eye_data/
│   │       └── defaultEye.h     # Eye texture tables (~157 KB flash)
│   ├── boards/
│   │   └── bruno_nrf52832_nrf52832.overlay
│   ├── CMakeLists.txt
│   ├── Kconfig
│   └── prj.conf
└── custom_driver_module/        # Out-of-tree GC9D01 Zephyr driver
```

---

## License

MIT — © 2025 Bruno Tondin
See [LICENSE.md](LICENSE.md).

Eye texture data adapted from [Adafruit UncannyEyes](https://github.com/adafruit/Uncanny_Eyes) (MIT).
