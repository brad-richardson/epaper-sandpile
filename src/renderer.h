#pragma once
#include <stdint.h>
#include "sources.h"
#include "sandpile.h"

// ---------------------------------------------------------------------------
// Renderer — maps the sandpile grid to a Spectra 6 framebuffer
//
// Spectra 6 color indices (verify against the actual driver header):
//   BLACK  = 0
//   WHITE  = 1
//   GREEN  = 2
//   BLUE   = 3
//   RED    = 4
//   YELLOW = 5
//
// Each pixel is stored as a 4-bit nibble (upper nibble = even pixel, lower
// nibble = odd pixel) in the packed framebuffer expected by the Seeed driver.
// ---------------------------------------------------------------------------

#define SPECTRA6_BLACK  0
#define SPECTRA6_WHITE  1
#define SPECTRA6_GREEN  2
#define SPECTRA6_BLUE   3
#define SPECTRA6_RED    4
#define SPECTRA6_YELLOW 5

// Number of non-background Spectra 6 colours available for palette assignment
#define PALETTE_SIZE 4

// ---------------------------------------------------------------------------
// Initialise the renderer for a new generation.
//
// `n`       — number of active sources
// `sources` — source array (used only for count here; ownership via owner[])
//
// Shuffles a colour palette so each source gets a distinct colour index
// (colours cycle when n > PALETTE_SIZE).
void renderer_init(int n, const Source *sources);

// ---------------------------------------------------------------------------
// Copy the current source→colour assignment into `out` (length n).
// Call after renderer_init() to save the palette to RTC for later restore.
void renderer_get_palette(uint8_t *out, int n);

// ---------------------------------------------------------------------------
// Restore a previously saved source→colour assignment without reshuffling.
// Call instead of renderer_init() when waking from deep sleep.
void renderer_set_palette(const uint8_t *colors, int n);

// ---------------------------------------------------------------------------
// Build the packed Spectra 6 framebuffer from the current grid and Voronoi
// owner map.  Scales the 120×120 grid to DISP_W×DISP_H using nearest-neighbour.
//
// `buf`  — output buffer; must be at least (DISP_W * DISP_H / 2) bytes.
// `buf_size` — size of `buf` in bytes (used for a safety guard).
// `uf_parent` — union-find parent array so merged territories share a colour.
// `n_sources` — number of sources in the union-find structure.
void renderer_render(uint8_t *buf, int buf_size,
                     int *uf_parent, int n_sources);
