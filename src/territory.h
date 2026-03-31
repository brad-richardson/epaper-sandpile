#pragma once
#include <stdint.h>
#include "sources.h"
#include "sandpile.h"

// ---------------------------------------------------------------------------
// Territory — Voronoi ownership map + union-find territory merging
//
// Each cell in the grid is "owned" by the nearest source (Euclidean distance).
// Territories are merged when occupied cells from two different Voronoi regions
// become adjacent — i.e., the sandpile grains of two sources physically touch.
// ---------------------------------------------------------------------------

// owner[y * GRID_W + x] holds the source index (0..n-1) that owns the cell.
// OWNER_NONE is the unset sentinel; voronoi_compute() always assigns a valid
// source index for every cell (nearest-source wins ties deterministically).
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

// Walk every occupied grid cell.  When two adjacent occupied cells belong to
// different Voronoi territories, union those territories in `parent` so they
// share a canonical colour.  After calling this, use uf_find() to resolve IDs.
void territory_check_merge(int *parent, int *rank_arr,
                           int n_sources);
