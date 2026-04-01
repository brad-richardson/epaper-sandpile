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
        uint8_t b = gol_grid[i];
        b = b - ((b >> 1) & 0x55);
        b = (b & 0x33) + ((b >> 2) & 0x33);
        count += (b + (b >> 4)) & 0x0F;
    }
    return count;
}
