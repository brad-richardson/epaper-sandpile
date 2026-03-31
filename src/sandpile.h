#pragma once
#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Abelian sandpile model — 120×120 grid
// Topple rule: if cell value >= 4, shed one grain to each of the 4 orthogonal
// neighbours and subtract 4 from the cell.  Grains that fall off the edge of
// the grid are simply discarded (open boundary conditions).
// ---------------------------------------------------------------------------

#ifndef GRID_W
#define GRID_W 120
#endif
#ifndef GRID_H
#define GRID_H 120
#endif

// Maximum stable grain count per cell (topple fires when value >= TOPPLE_THRESHOLD)
#define TOPPLE_THRESHOLD 4u

// Flat row-major grid: grid[y * GRID_W + x]
extern uint8_t grid[GRID_W * GRID_H];

// Reset every cell to 0
void sandpile_reset();

// Add one grain at (x, y).  No bounds check — caller must ensure validity.
void sandpile_drop(int x, int y);

// Run the toppling cascade until the grid is stable.
// Returns the total number of topples that fired.
uint32_t sandpile_topple();

// Count of cells that have at least one grain
uint32_t sandpile_occupied_cells();

// Fill percentage: (occupied cells) / (GRID_W * GRID_H)
float sandpile_fill_percent();
