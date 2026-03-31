#include "renderer.h"
#include "territory.h"
#include <string.h>
#include <Arduino.h>

#ifndef DISP_W
#define DISP_W 800
#endif
#ifndef DISP_H
#define DISP_H 480
#endif

// ---------------------------------------------------------------------------
// Per-source colour assignment (shuffled each generation, persisted in RTC)
// ---------------------------------------------------------------------------
static uint8_t source_color[MAX_SOURCES]; // colour index for each source

// Non-background Spectra 6 colours available for palette assignment
static const uint8_t PALETTE_COLORS[PALETTE_SIZE] = {
    SPECTRA6_RED,
    SPECTRA6_BLUE,
    SPECTRA6_GREEN,
    SPECTRA6_YELLOW,
};

// ---------------------------------------------------------------------------
// Fisher-Yates shuffle on a small array of indices
// ---------------------------------------------------------------------------
static void shuffle_palette(uint8_t *arr, int len)
{
    for (int i = len - 1; i > 0; i--) {
        int j = (int)(random(0, i + 1));
        uint8_t tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

// ---------------------------------------------------------------------------
void renderer_init(int n, const Source *sources)
{
    (void)sources; // unused beyond count

    // Build a shuffled copy of available palette colours
    uint8_t shuffled[PALETTE_SIZE];
    for (int i = 0; i < PALETTE_SIZE; i++) {
        shuffled[i] = PALETTE_COLORS[i];
    }
    shuffle_palette(shuffled, PALETTE_SIZE);

    // Assign colours to sources, cycling through the shuffled palette
    for (int i = 0; i < n && i < MAX_SOURCES; i++) {
        source_color[i] = shuffled[i % PALETTE_SIZE];
    }
}

// ---------------------------------------------------------------------------
void renderer_get_palette(uint8_t *out, int n)
{
    for (int i = 0; i < n && i < MAX_SOURCES; i++) {
        out[i] = source_color[i];
    }
}

// ---------------------------------------------------------------------------
void renderer_set_palette(const uint8_t *colors, int n)
{
    for (int i = 0; i < n && i < MAX_SOURCES; i++) {
        source_color[i] = colors[i];
    }
}

// ---------------------------------------------------------------------------
// Render the sandpile grid into a packed 4-bit-per-pixel Spectra 6 buffer.
//
// Layout: pixel (x, y) at display coordinates maps back to grid coordinates
// via nearest-neighbour scaling.  Each byte holds two pixels:
//   byte[(y * DISP_W + x) / 2]:
//     upper nibble = pixel at even x
//     lower nibble = pixel at odd x
// ---------------------------------------------------------------------------
void renderer_render(uint8_t *buf, int buf_size,
                     int *uf_parent, int n_sources)
{
    int required = (DISP_W * DISP_H + 1) / 2;
    if (buf_size < required) return; // safety guard

    memset(buf, (SPECTRA6_WHITE << 4) | SPECTRA6_WHITE, (size_t)buf_size);

    for (int dy = 0; dy < DISP_H; dy++) {
        // Map display row → grid row (nearest-neighbour)
        int gy = (dy * GRID_H) / DISP_H;
        if (gy >= GRID_H) gy = GRID_H - 1;

        for (int dx = 0; dx < DISP_W; dx++) {
            // Map display column → grid column (nearest-neighbour)
            int gx = (dx * GRID_W) / DISP_W;
            if (gx >= GRID_W) gx = GRID_W - 1;

            uint16_t grains = grid[gy * GRID_W + gx];
            uint8_t color;

            if (grains == 0) {
                color = SPECTRA6_WHITE;
            } else {
                uint8_t src_idx = owner[gy * GRID_W + gx];
                if (src_idx == OWNER_NONE || src_idx >= (uint8_t)n_sources) {
                    color = SPECTRA6_BLACK;
                } else {
                    // Resolve merged territories via union-find
                    int canonical = uf_find(uf_parent, (int)src_idx);
                    if (canonical < 0 || canonical >= MAX_SOURCES) {
                        canonical = (int)src_idx;
                    }
                    color = source_color[canonical % MAX_SOURCES];

                    // Dim cells with fewer grains to black to show texture
                    if (grains == 1) {
                        color = SPECTRA6_BLACK;
                    }
                    // grains == 2 or 3 → full source colour (already set above)
                }
            }

            // Pack two pixels per byte
            int byte_idx = (dy * DISP_W + dx) / 2;
            if ((dx & 1) == 0) {
                // Even pixel → upper nibble
                buf[byte_idx] = (uint8_t)((color << 4) | (buf[byte_idx] & 0x0F));
            } else {
                // Odd pixel → lower nibble
                buf[byte_idx] = (uint8_t)((buf[byte_idx] & 0xF0) | (color & 0x0F));
            }
        }
    }
}
