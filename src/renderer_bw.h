#pragma once
#include <stdint.h>

#ifndef DISP_W
#define DISP_W 800
#endif
#ifndef DISP_H
#define DISP_H 480
#endif

#define BW_BLACK 0
#define BW_WHITE 1

// Render the GoL grid into a packed 4-bit-per-pixel framebuffer.
// Alive = black, dead = white.
void renderer_bw_render(uint8_t *buf, int buf_size);
