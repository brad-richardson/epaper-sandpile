#include "territory.h"
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Global Voronoi ownership map (14.4 KB)
// ---------------------------------------------------------------------------
uint8_t owner[GRID_W * GRID_H];

// ---------------------------------------------------------------------------
// Voronoi — nearest-source assignment (brute-force, run once per generation)
// ---------------------------------------------------------------------------
void voronoi_compute(const Source *sources, int n)
{
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            float best_dist = 1e30f;
            uint8_t best_src = OWNER_NONE;
            for (int s = 0; s < n; s++) {
                float dx = (float)(x - sources[s].x);
                float dy = (float)(y - sources[s].y);
                float d2 = dx * dx + dy * dy;
                if (d2 < best_dist) {
                    best_dist = d2;
                    best_src = (uint8_t)s;
                }
            }
            owner[y * GRID_W + x] = best_src;
        }
    }
}

// ---------------------------------------------------------------------------
// Union-Find
// ---------------------------------------------------------------------------
void uf_init(int *parent, int *rank_arr, int n)
{
    for (int i = 0; i < n; i++) {
        parent[i] = i;
        rank_arr[i] = 0;
    }
}

int uf_find(int *parent, int x)
{
    while (parent[x] != x) {
        parent[x] = parent[parent[x]]; // path halving
        x = parent[x];
    }
    return x;
}

void uf_union(int *parent, int *rank_arr, int a, int b)
{
    int ra = uf_find(parent, a);
    int rb = uf_find(parent, b);
    if (ra == rb) return;

    if (rank_arr[ra] < rank_arr[rb]) {
        parent[ra] = rb;
    } else if (rank_arr[ra] > rank_arr[rb]) {
        parent[rb] = ra;
    } else {
        parent[rb] = ra;
        rank_arr[ra]++;
    }
}

// ---------------------------------------------------------------------------
// Collision / merge detection
// ---------------------------------------------------------------------------
// A "collision" occurs when an occupied cell's Voronoi owner differs from
// the owner of an adjacent occupied cell — i.e., two territories touch.
// We union those two territory roots so their canonical colour collapses.
// ---------------------------------------------------------------------------
void territory_check_merge(int *parent, int *rank_arr, int n_sources)
{
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            if (grid[y * GRID_W + x] == 0) continue;

            uint8_t this_owner = owner[y * GRID_W + x];
            if (this_owner == OWNER_NONE || this_owner >= (uint8_t)n_sources) continue;

            // Check right neighbour
            if (x + 1 < GRID_W && grid[y * GRID_W + (x + 1)] > 0) {
                uint8_t nb_owner = owner[y * GRID_W + (x + 1)];
                if (nb_owner != OWNER_NONE && nb_owner < (uint8_t)n_sources
                        && nb_owner != this_owner) {
                    uf_union(parent, rank_arr, (int)this_owner, (int)nb_owner);
                }
            }
            // Check bottom neighbour
            if (y + 1 < GRID_H && grid[(y + 1) * GRID_W + x] > 0) {
                uint8_t nb_owner = owner[(y + 1) * GRID_W + x];
                if (nb_owner != OWNER_NONE && nb_owner < (uint8_t)n_sources
                        && nb_owner != this_owner) {
                    uf_union(parent, rank_arr, (int)this_owner, (int)nb_owner);
                }
            }
        }
    }
}
