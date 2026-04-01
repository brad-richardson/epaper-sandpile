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
