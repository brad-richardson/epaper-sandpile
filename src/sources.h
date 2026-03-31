#pragma once
#include <stdint.h>

// ---------------------------------------------------------------------------
// Sources — up to MAX_SOURCES grain sources placed equidistantly on a circle
// centred in the grid.  Each source has a normalised drop-weight so that
// heavier sources receive proportionally more grains per round.
// ---------------------------------------------------------------------------

#define MAX_SOURCES 8

struct Source {
    int     x;          // grid column
    int     y;          // grid row
    float   weight;     // relative drop weight (normalised to sum = 1.0)
};

// Place `n` sources (1 ≤ n ≤ MAX_SOURCES) equidistantly on a circle of
// radius `radius` centred at (cx, cy) in grid coordinates.
// Weights are drawn randomly and normalised to sum to 1.
// Uses the global Arduino random() / randomSeed() RNG — call randomSeed()
// before this function.
void sources_generate(int n, Source *sources,
                      float cx, float cy, float radius);

// Weighted random selection: returns an index in [0, n) proportional to
// each source's weight.  `r` must be a uniform float in [0, 1).
int sources_choose(const Source *sources, int n, float r);
