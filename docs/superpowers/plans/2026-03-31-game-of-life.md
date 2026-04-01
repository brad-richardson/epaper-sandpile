# Game of Life Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a bicolor (black/white) Game of Life simulation alongside the existing sandpile, selectable via build flag.

**Architecture:** GoL logic in `gol.cpp/h`, B&W renderer in `renderer_bw.cpp/h`. `main.cpp` uses `#ifdef SIM_GOL` to select between GoL and sandpile code paths. Existing sandpile files are untouched. GoL grid is 200x120 (1-bit-per-cell, 3000 bytes), persisted in RTC memory for deep-sleep survival. Staleness detected by comparing frame hashes; auto-reseeds when stable/oscillating.

**Tech Stack:** Arduino framework on ESP32-S3, Seeed_GFX EPaper driver, PlatformIO build system.

---

### Task 1: Create GoL simulation module (`gol.h` / `gol.cpp`)

**Files:**
- Create: `src/gol.h`
- Create: `src/gol.cpp`

- [ ] **Step 1: Write `src/gol.h`**

```cpp
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifndef GOL_W
#define GOL_W 200
#endif
#ifndef GOL_H
#define GOL_H 120
#endif

#define GOL_GRID_BYTES ((GOL_W * GOL_H + 7) / 8)

// Current generation grid (1 bit per cell, row-major, MSB-first packing)
extern uint8_t gol_grid[GOL_GRID_BYTES];

// Read/write individual cells
inline bool gol_get(const uint8_t *g, int x, int y) {
    int idx = y * GOL_W + x;
    return (g[idx / 8] >> (7 - (idx % 8))) & 1;
}

inline void gol_set(uint8_t *g, int x, int y, bool alive) {
    int idx = y * GOL_W + x;
    int byte_idx = idx / 8;
    int bit = 7 - (idx % 8);
    if (alive)
        g[byte_idx] |= (1 << bit);
    else
        g[byte_idx] &= ~(1 << bit);
}

// Seed the grid randomly. `density` is probability of alive (0.0-1.0).
void gol_seed(float density);

// Advance one generation. Uses wrapping (toroidal) boundary conditions.
void gol_step();

// Compute a simple hash of the current grid state (for staleness detection).
uint32_t gol_hash();

// Count of alive cells.
uint32_t gol_population();
```

- [ ] **Step 2: Write `src/gol.cpp`**

```cpp
#include "gol.h"
#include <string.h>
#include <esp_random.h>

uint8_t gol_grid[GOL_GRID_BYTES];

// Scratch buffer for next generation
static uint8_t gol_next[GOL_GRID_BYTES];

void gol_seed(float density) {
    memset(gol_grid, 0, GOL_GRID_BYTES);
    for (int y = 0; y < GOL_H; y++) {
        for (int x = 0; x < GOL_W; x++) {
            // Use hardware RNG, map to [0,1) and compare to density
            float r = (float)(esp_random() >> 1) / (float)0x7FFFFFFFu;
            if (r < density) {
                gol_set(gol_grid, x, y, true);
            }
        }
    }
}

static int count_neighbors(const uint8_t *g, int x, int y) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            // Toroidal wrapping
            int nx = (x + dx + GOL_W) % GOL_W;
            int ny = (y + dy + GOL_H) % GOL_H;
            if (gol_get(g, nx, ny)) count++;
        }
    }
    return count;
}

void gol_step() {
    memset(gol_next, 0, GOL_GRID_BYTES);
    for (int y = 0; y < GOL_H; y++) {
        for (int x = 0; x < GOL_W; x++) {
            int n = count_neighbors(gol_grid, x, y);
            bool alive = gol_get(gol_grid, x, y);
            // B3/S23: born if 3 neighbors, survive if 2 or 3
            if (alive ? (n == 2 || n == 3) : (n == 3)) {
                gol_set(gol_next, x, y, true);
            }
        }
    }
    memcpy(gol_grid, gol_next, GOL_GRID_BYTES);
}

uint32_t gol_hash() {
    // FNV-1a 32-bit
    uint32_t h = 2166136261u;
    for (int i = 0; i < GOL_GRID_BYTES; i++) {
        h ^= gol_grid[i];
        h *= 16777619u;
    }
    return h;
}

uint32_t gol_population() {
    uint32_t count = 0;
    for (int i = 0; i < GOL_GRID_BYTES; i++) {
        // popcount byte
        uint8_t b = gol_grid[i];
        b = b - ((b >> 1) & 0x55);
        b = (b & 0x33) + ((b >> 2) & 0x33);
        count += (b + (b >> 4)) & 0x0F;
    }
    return count;
}
```

- [ ] **Step 3: Commit**

```bash
git add src/gol.h src/gol.cpp
git commit -m "feat: add Game of Life simulation module"
```

---

### Task 2: Create B&W renderer (`renderer_bw.h` / `renderer_bw.cpp`)

**Files:**
- Create: `src/renderer_bw.h`
- Create: `src/renderer_bw.cpp`

- [ ] **Step 1: Write `src/renderer_bw.h`**

```cpp
#pragma once
#include <stdint.h>
#include "gol.h"

#ifndef DISP_W
#define DISP_W 800
#endif
#ifndef DISP_H
#define DISP_H 480
#endif

// Spectra 6 color indices (same as renderer.h)
#define BW_BLACK 0
#define BW_WHITE 1

// Render the GoL grid into a packed 4-bit-per-pixel framebuffer.
// Each cell maps to a (DISP_W/GOL_W) x (DISP_H/GOL_H) block.
// Alive = black, dead = white.
void renderer_bw_render(uint8_t *buf, int buf_size);
```

- [ ] **Step 2: Write `src/renderer_bw.cpp`**

```cpp
#include "renderer_bw.h"
#include <string.h>

void renderer_bw_render(uint8_t *buf, int buf_size) {
    int required = (DISP_W * DISP_H + 1) / 2;
    if (buf_size < required) return;

    // Fill with white
    memset(buf, (BW_WHITE << 4) | BW_WHITE, (size_t)buf_size);

    for (int dy = 0; dy < DISP_H; dy++) {
        int gy = (dy * GOL_H) / DISP_H;
        if (gy >= GOL_H) gy = GOL_H - 1;

        for (int dx = 0; dx < DISP_W; dx++) {
            int gx = (dx * GOL_W) / DISP_W;
            if (gx >= GOL_W) gx = GOL_W - 1;

            uint8_t color = gol_get(gol_grid, gx, gy) ? BW_BLACK : BW_WHITE;

            int byte_idx = (dy * DISP_W + dx) / 2;
            if ((dx & 1) == 0) {
                buf[byte_idx] = (uint8_t)((color << 4) | (buf[byte_idx] & 0x0F));
            } else {
                buf[byte_idx] = (uint8_t)((buf[byte_idx] & 0xF0) | (color & 0x0F));
            }
        }
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add src/renderer_bw.h src/renderer_bw.cpp
git commit -m "feat: add black/white renderer for GoL"
```

---

### Task 3: Refactor `main.cpp` for GoL with `#ifdef SIM_GOL`

**Files:**
- Modify: `src/main.cpp`
- Modify: `platformio.ini`

- [ ] **Step 1: Add GoL includes and RTC state to `main.cpp`**

Under the existing sandpile includes, add a `#ifdef SIM_GOL` block that includes `gol.h` and `renderer_bw.h`, defines GoL-specific RTC variables (grid packed in RTC, frame hash history for staleness, frame count), and the GoL-specific constants (seed density 0.37, sleep duration 15s, stale history depth 4, max frames per generation).

- [ ] **Step 2: Add GoL lifecycle functions to `main.cpp`**

Add `#ifdef SIM_GOL` versions of:
- `gol_new_generation()` — calls `gol_seed(GOL_SEED_DENSITY)`, copies grid to RTC, resets frame count and hash history
- `gol_restore()` — copies RTC grid back to `gol_grid`
- `gol_push_display()` — calls `renderer_bw_render()`, then pushes framebuffer to EPaper pixel-by-pixel using only `TFT_BLACK`/`TFT_WHITE`, calls `epaper->update()`
- `gol_is_stale()` — computes `gol_hash()`, checks against last N hashes, returns true if match found or population is 0

- [ ] **Step 3: Add GoL `setup()` / `loop()` paths**

Inside `setup()`, under `#ifdef SIM_GOL`: if `rtc_valid`, call `gol_restore()`; else call `gol_new_generation()` and set `rtc_valid = true`.

Inside `loop()`, under `#ifdef SIM_GOL`: check staleness → reseed if stale; call `gol_step()`; copy grid to RTC; call `gol_push_display()`; increment frame; deep sleep for `GOL_SLEEP_DURATION_US`.

The sandpile code paths stay in the corresponding `#else` / `#elif defined(SIM_SANDPILE)` blocks.

- [ ] **Step 4: Update `platformio.ini`**

Switch build flags from sandpile to GoL:
- Remove `-DGRID_W=120`, `-DGRID_H=72`
- Add `-DSIM_GOL`, `-DGOL_W=200`, `-DGOL_H=120`
- Keep `-DDISP_W=800`, `-DDISP_H=480`

- [ ] **Step 5: Build, flash, verify**

```bash
pio run -t upload
```

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp platformio.ini
git commit -m "feat: wire up Game of Life in main loop with deep sleep"
```

---

### Task 4: Rename repo and create PR

- [ ] **Step 1: Rename GitHub repo**

```bash
gh repo rename epaper-sim
```

- [ ] **Step 2: Update git remote**

```bash
git remote set-url origin https://github.com/brad-richardson/epaper-sim.git
```

- [ ] **Step 3: Create PR**

Push branch and create PR against main.
