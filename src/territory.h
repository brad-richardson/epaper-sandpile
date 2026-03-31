#pragma once
#include <stdint.h>
#include "sources.h"
#include "sandpile.h"

// ---------------------------------------------------------------------------
// Territory — Voronoi ownership map + union-find territory merging
//
// Each cell in the grid is "owned" by the nearest source (Euclidean distance).
// When grains from two different territories share a cell (detected during
// rendering or after a topple pass), the territories are merged via union-find
// so their colours become uniform.
// ---------------------------------------------------------------------------

// owner[y * GRID_W + x] holds the source index (0..n-1) that owns the cell.
// Value OWNER_NONE means no source has been assigned (cell is equidistant).
#define OWNER_NONE 0xFF
extern uint8_t owner[GRID_W * GRID_H];

// ---------------------------------------------------------------------------
// Voronoi
// ---------------------------------------------------------------------------

// Precompute the Voronoi ownership map for `n` sources.
// Must be called after sources_generate().
void voronoi_compute(const Source *sources, int n);

// ---------------------------------------------------------------------------
// Union-Find (path-compressed, rank-unioned)
// ---------------------------------------------------------------------------

// Initialise the parent/rank arrays for `n` elements.
void uf_init(int *parent, int *rank_arr, int n);

// Find the root of element x with path compression.
int uf_find(int *parent, int x);

// Union the sets containing a and b (union by rank).
void uf_union(int *parent, int *rank_arr, int a, int b);

// ---------------------------------------------------------------------------
// Collision / merge detection
// ---------------------------------------------------------------------------

// Walk every occupied grid cell.  If a cell's grain came from a different
// source than its Voronoi owner, union the two territories in `parent`.
// After calling this, use uf_find() to resolve canonical territory IDs.
void territory_check_merge(int *parent, int *rank_arr,
                           int n_sources);
