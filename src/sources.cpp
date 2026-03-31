#include "sources.h"
#include "sandpile.h"
#include <math.h>
#include <Arduino.h>

// ---------------------------------------------------------------------------
void sources_generate(int n, Source *sources,
                      float cx, float cy, float radius)
{
    // Equidistant angular placement starting at a random phase
    float phase = (float)random(0, 1000) / 1000.0f * 2.0f * (float)M_PI;
    float angle_step = 2.0f * (float)M_PI / (float)n;

    // Generate raw random weights in [1, 10]
    float total = 0.0f;
    for (int i = 0; i < n; i++) {
        float angle = phase + i * angle_step;
        sources[i].x = (int)(cx + radius * cosf(angle) + 0.5f);
        sources[i].y = (int)(cy + radius * sinf(angle) + 0.5f);

        // Clamp to grid bounds
        if (sources[i].x < 0)        sources[i].x = 0;
        if (sources[i].x >= GRID_W)  sources[i].x = GRID_W - 1;
        if (sources[i].y < 0)        sources[i].y = 0;
        if (sources[i].y >= GRID_H)  sources[i].y = GRID_H - 1;

        sources[i].weight = (float)(random(1, 11)); // 1..10
        total += sources[i].weight;
    }

    // Normalise weights to sum to 1.0
    for (int i = 0; i < n; i++) {
        sources[i].weight /= total;
    }
}

// ---------------------------------------------------------------------------
int sources_choose(const Source *sources, int n, float r)
{
    float cumulative = 0.0f;
    for (int i = 0; i < n; i++) {
        cumulative += sources[i].weight;
        if (r < cumulative) {
            return i;
        }
    }
    // Floating-point rounding safety: return last source
    return n - 1;
}
