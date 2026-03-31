#include <Arduino.h>
#include <Preferences.h>
#include <esp_sleep.h>

#include "sandpile.h"
#include "sources.h"
#include "territory.h"
#include "renderer.h"

// ---------------------------------------------------------------------------
// Display driver — include the Seeed reTerminal E10xx header.
// Adjust the include path to match the installed library version.
// ---------------------------------------------------------------------------
// #include <ReTerminal_E10xx.h>   // uncomment once library is installed

// ---------------------------------------------------------------------------
// Display geometry and framebuffer
// ---------------------------------------------------------------------------
#ifndef DISP_W
#define DISP_W 648
#endif
#ifndef DISP_H
#define DISP_H 480
#endif

// Packed 4-bit-per-pixel Spectra 6 framebuffer: (W * H / 2) bytes
static uint8_t framebuf[(DISP_W * DISP_H + 1) / 2];

// ---------------------------------------------------------------------------
// RTC memory — survives deep sleep.  Holds lightweight metadata only; the
// full grid (14.4 KB) is too large for RTC SRAM and is stored in NVS flash.
// ---------------------------------------------------------------------------
RTC_DATA_ATTR static uint32_t rtc_seed   = 0; // persistent RNG seed
RTC_DATA_ATTR static uint32_t rtc_frame  = 0; // frames rendered this generation
RTC_DATA_ATTR static int      rtc_n_sources = 0;
RTC_DATA_ATTR static Source   rtc_sources[MAX_SOURCES];
RTC_DATA_ATTR static int      rtc_uf_parent[MAX_SOURCES];
RTC_DATA_ATTR static int      rtc_uf_rank[MAX_SOURCES];
RTC_DATA_ATTR static bool     rtc_valid = false; // set true once first frame is done

// ---------------------------------------------------------------------------
// NVS key used to persist the sandpile grid across deep sleep cycles
// ---------------------------------------------------------------------------
static const char NVS_NS[]   = "sandpile";
static const char NVS_GRID[] = "grid";

// ---------------------------------------------------------------------------
// Lifecycle tunables
// ---------------------------------------------------------------------------
#define FILL_THRESHOLD      0.82f   // start a new generation above this fill %
#define DROPS_PER_FRAME     2000    // grains to drop per display refresh cycle
#define SLEEP_DURATION_US   30000000ULL // 30 s deep sleep between refreshes
#define NUM_SOURCES_MIN     1
#define NUM_SOURCES_MAX     8
#define CIRCLE_RADIUS_FRAC  0.35f   // circle radius as fraction of min(W,H)

// ---------------------------------------------------------------------------
// Active generation state (mirrors the RTC copies for in-loop convenience)
// ---------------------------------------------------------------------------
static Source sources[MAX_SOURCES];
static int    n_sources = 0;
static int    uf_parent[MAX_SOURCES];
static int    uf_rank[MAX_SOURCES];

// ---------------------------------------------------------------------------
// Persist the grid to NVS flash so it survives deep sleep
// ---------------------------------------------------------------------------
static void grid_save()
{
    Preferences prefs;
    prefs.begin(NVS_NS, /*readOnly=*/false);
    prefs.putBytes(NVS_GRID, grid, sizeof(grid));
    prefs.end();
}

// ---------------------------------------------------------------------------
// Restore the grid from NVS flash.  Returns true if a valid grid was found.
// ---------------------------------------------------------------------------
static bool grid_restore()
{
    Preferences prefs;
    prefs.begin(NVS_NS, /*readOnly=*/true);
    size_t stored = prefs.getBytesLength(NVS_GRID);
    bool ok = (stored == sizeof(grid));
    if (ok) {
        prefs.getBytes(NVS_GRID, grid, sizeof(grid));
    }
    prefs.end();
    return ok;
}

// ---------------------------------------------------------------------------
// Start a brand-new generation: new sources, palette, Voronoi map, clear grid
// ---------------------------------------------------------------------------
static void new_generation()
{
    sandpile_reset();

    n_sources = (int)random(NUM_SOURCES_MIN, NUM_SOURCES_MAX + 1);

    float cx     = GRID_W / 2.0f;
    float cy     = GRID_H / 2.0f;
    float radius = CIRCLE_RADIUS_FRAC * (float)(GRID_W < GRID_H ? GRID_W : GRID_H);

    sources_generate(n_sources, sources, cx, cy, radius);
    voronoi_compute(sources, n_sources);
    renderer_init(n_sources, sources);
    uf_init(uf_parent, uf_rank, n_sources);

    // Mirror into RTC memory so the state survives the next deep sleep
    rtc_n_sources = n_sources;
    for (int i = 0; i < n_sources; i++) {
        rtc_sources[i]   = sources[i];
        rtc_uf_parent[i] = uf_parent[i];
        rtc_uf_rank[i]   = uf_rank[i];
    }
    rtc_frame = 0;

    Serial.println("[lifecycle] new generation started");
}

// ---------------------------------------------------------------------------
// Restore generation state from RTC memory after a deep sleep wake
// ---------------------------------------------------------------------------
static void generation_restore()
{
    n_sources = rtc_n_sources;
    for (int i = 0; i < n_sources; i++) {
        sources[i]   = rtc_sources[i];
        uf_parent[i] = rtc_uf_parent[i];
        uf_rank[i]   = rtc_uf_rank[i];
    }
    voronoi_compute(sources, n_sources);
    renderer_init(n_sources, sources);
}

// ---------------------------------------------------------------------------
// Drop DROPS_PER_FRAME grains, then topple to stability
// ---------------------------------------------------------------------------
static void drop_and_topple()
{
    for (int i = 0; i < DROPS_PER_FRAME; i++) {
        float r = (float)random(0, 10000) / 10000.0f;
        int src = sources_choose(sources, n_sources, r);
        sandpile_drop(sources[src].x, sources[src].y);
    }
    sandpile_topple();
}

// ---------------------------------------------------------------------------
// Render the framebuffer and push it to the display
// ---------------------------------------------------------------------------
static void push_display()
{
    renderer_render(framebuf, (int)sizeof(framebuf), uf_parent, n_sources);

    // Push to the Seeed E10xx display driver.
    // Replace the stub below with the actual driver API once the library is
    // integrated:
    //
    //   epaper.writeBuffer(framebuf, sizeof(framebuf));
    //   epaper.refresh();            // triggers the ~15–30 s ePaper refresh
    //
    Serial.printf("[frame %lu] sources=%d fill=%.1f%%\n",
                  (unsigned long)rtc_frame,
                  n_sources,
                  sandpile_fill_percent() * 100.0f);
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    // Seed the RNG.  On first power-on rtc_seed == 0, so use the hardware RNG.
    if (rtc_seed == 0) {
        rtc_seed = (uint32_t)esp_random();
    }
    randomSeed(rtc_seed + rtc_frame);

    if (rtc_valid) {
        // Waking from deep sleep — restore simulation state
        generation_restore();
        if (!grid_restore()) {
            // NVS read failed (e.g. first boot after a new generation); reset
            sandpile_reset();
        }
    } else {
        // True first boot — start fresh
        new_generation();
        rtc_valid = true;
    }
}

void loop()
{
    // Check fill threshold → start a new generation if the grid is saturated
    if (sandpile_fill_percent() > FILL_THRESHOLD) {
        Serial.println("[lifecycle] fill threshold reached — new generation");
        new_generation();
        // Advance the seed so the next generation is visually distinct
        rtc_seed ^= (uint32_t)esp_random();
        randomSeed(rtc_seed);
    }

    // 1. Drop grains and topple to stability
    drop_and_topple();

    // 2. Detect territory collisions and merge
    territory_check_merge(uf_parent, uf_rank, n_sources);

    // 3. Mirror union-find state into RTC so merges survive deep sleep
    for (int i = 0; i < n_sources; i++) {
        rtc_uf_parent[i] = uf_parent[i];
        rtc_uf_rank[i]   = uf_rank[i];
    }

    // 4. Render and push to display
    push_display();
    rtc_frame++;

    // 5. Persist the grid to NVS before sleeping
    grid_save();

    // 6. Deep sleep until the next refresh cycle.
    //    The ePaper display holds its image with zero power draw during sleep.
    Serial.printf("[sleep] deep sleep for %llu ms\n", SLEEP_DURATION_US / 1000ULL);
    Serial.flush();
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
    esp_deep_sleep_start();

    // esp_deep_sleep() does not return; the MCU wakes up and re-runs setup().
}

