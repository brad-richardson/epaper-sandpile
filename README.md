# epaper-sandpile

An ESP32-S3 firmware that renders an animated [Abelian sandpile model](https://en.wikipedia.org/wiki/Abelian_sandpile_model) onto a Seeed reTerminal E1002 Spectra 6 ePaper display.  Multiple grain sources compete for territory, producing vivid coloured fractal patterns that refresh every 30 seconds while the MCU deep-sleeps between frames.

---

## Hardware

| Component | Details |
|-----------|---------|
| MCU | Seeed reTerminal E1002 (ESP32-S3, 240 MHz, 512 KB SRAM) |
| Display | Spectra 6 ePaper (6-colour: black, white, green, blue, red, yellow) |
| Display driver | [Seeed Arduino reTerminal E10xx](https://github.com/Seeed-Studio/Seeed_Arduino_ReTerminal_E10xx) |
| Build system | [PlatformIO](https://platformio.org) |
| Framework | Arduino |

---

## Repository Layout

```
e1002-sandpile/
├── src/
│   ├── main.cpp          # setup/loop, deep-sleep lifecycle
│   ├── sandpile.h/cpp    # 120×120 grid, drop, topple cascade
│   ├── territory.h/cpp   # Voronoi ownership, union-find merge
│   ├── renderer.h/cpp    # grid → packed Spectra 6 framebuffer
│   └── sources.h/cpp     # equidistant placement, weighted drops
├── platformio.ini        # ESP32-S3 board + library deps
└── README.md
```

---

## Algorithm

### Sandpile (sandpile.h/cpp)
A flat `uint8_t grid[120×120]` holds the grain count per cell.  When a cell reaches 4 grains it **topples**: it loses 4 grains and each orthogonal neighbour gains 1.  Grains that fall off the edge are discarded (open boundary).

### Sources (sources.h/cpp)
Each generation picks 1–8 sources placed equidistantly on a circle (random phase, random weighted drops).  Every frame `DROPS_PER_FRAME` grains are distributed according to the normalised per-source weights.

### Territory (territory.h/cpp)
A **Voronoi map** (`uint8_t owner[120×120]`) is precomputed once per generation by assigning each cell to its nearest source.  A compact **union-find** structure tracks territory merges: when occupied cells from two different Voronoi regions become adjacent, their territories are unioned and rendered with the same colour.

### Renderer (renderer.h/cpp)
A shuffled colour palette assigns one of four non-background Spectra 6 colours (red, blue, green, yellow) to each source.  Merged territories share the canonical root's colour.  Cells with 1 grain render as black (edge/shadow effect); cells with 2–3 grains render in the source colour.  The 120×120 grid is scaled to the full display resolution via nearest-neighbour interpolation and packed into a 4-bit-per-pixel buffer.

### Lifecycle (main.cpp)
```
boot/wake
  │
  ├─ first boot → new_generation()
  │
  loop:
  ├─ fill > 82%? → new_generation()
  ├─ drop DROPS_PER_FRAME grains (weighted)
  ├─ topple to stability
  ├─ check territory collisions → merge
  ├─ render → push to display
  └─ deep sleep 30 s  (display holds image at zero power)
```

`RTC_DATA_ATTR` variables survive deep sleep so the RNG seed and frame counter persist across cycles.

---

## Getting Started

### Prerequisites
- [PlatformIO Core](https://docs.platformio.org/en/latest/core/index.html) or the PlatformIO IDE extension for VS Code

### Build & Flash
```bash
git clone https://github.com/brad-richardson/epaper-sandpile.git
cd epaper-sandpile
pio run --target upload
```

### Monitor Serial Output
```bash
pio device monitor
```

---

## Configuration

Key tunables at the top of `src/main.cpp`:

| Constant | Default | Description |
|----------|---------|-------------|
| `FILL_THRESHOLD` | `0.82` | Occupied-cell fraction that triggers a new generation |
| `DROPS_PER_FRAME` | `2000` | Grains dropped per refresh cycle |
| `SLEEP_DURATION_US` | `30 000 000` | Deep-sleep duration in microseconds (30 s) |
| `NUM_SOURCES_MIN/MAX` | `1` / `8` | Range for random source count |
| `CIRCLE_RADIUS_FRAC` | `0.35` | Source-circle radius as a fraction of `min(GRID_W, GRID_H)` |

Display resolution is set in `platformio.ini` via `-DDISP_W` and `-DDISP_H` build flags.

---

## Display Driver Integration

The Seeed reTerminal E10xx library is listed in `platformio.ini` as a `lib_deps` entry.  Once the library is installed, uncomment the driver include and API calls in `src/main.cpp`:

```cpp
#include <ReTerminal_E10xx.h>

// Inside push_display():
epaper.writeBuffer(framebuf, sizeof(framebuf));
epaper.refresh();
```

Verify the Spectra 6 colour enum values in the driver header and update the `SPECTRA6_*` constants in `src/renderer.h` if they differ.

---

## License

MIT — see [LICENSE](LICENSE).
