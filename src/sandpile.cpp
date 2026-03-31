#include "sandpile.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Global grid storage (14.4 KB, lives in SRAM)
// ---------------------------------------------------------------------------
uint8_t grid[GRID_W * GRID_H];

// ---------------------------------------------------------------------------
void sandpile_reset()
{
    memset(grid, 0, sizeof(grid));
}

// ---------------------------------------------------------------------------
void sandpile_drop(int x, int y)
{
    grid[y * GRID_W + x]++;
}

// ---------------------------------------------------------------------------
// Iterative toppling.  We use a simple scan-and-repeat strategy: walk every
// cell; if it is unstable fire it immediately and mark that we made progress.
// Repeat until a full pass finds nothing to fire.  This is O(n * k) where k
// is the cascade depth, but on a 120×120 grid at 240 MHz it is fast enough.
// ---------------------------------------------------------------------------
uint32_t sandpile_topple()
{
    uint32_t total_topples = 0;
    bool fired;

    do {
        fired = false;
        for (int y = 0; y < GRID_H; y++) {
            for (int x = 0; x < GRID_W; x++) {
                uint8_t *cell = &grid[y * GRID_W + x];
                if (*cell >= TOPPLE_THRESHOLD) {
                    uint8_t times = *cell / TOPPLE_THRESHOLD;
                    *cell -= times * TOPPLE_THRESHOLD;
                    total_topples += times;

                    uint8_t grains = times; // grains shed per neighbour
                    if (x > 0)          grid[y * GRID_W + (x - 1)] += grains;
                    if (x < GRID_W - 1) grid[y * GRID_W + (x + 1)] += grains;
                    if (y > 0)          grid[(y - 1) * GRID_W + x] += grains;
                    if (y < GRID_H - 1) grid[(y + 1) * GRID_W + x] += grains;
                    fired = true;
                }
            }
        }
    } while (fired);

    return total_topples;
}

// ---------------------------------------------------------------------------
uint32_t sandpile_occupied_cells()
{
    uint32_t count = 0;
    for (int i = 0; i < GRID_W * GRID_H; i++) {
        if (grid[i] > 0) count++;
    }
    return count;
}

// ---------------------------------------------------------------------------
float sandpile_fill_percent()
{
    return (float)sandpile_occupied_cells() / (float)(GRID_W * GRID_H);
}
