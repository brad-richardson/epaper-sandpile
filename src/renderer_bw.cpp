#include "renderer_bw.h"
#include "gol.h"
#include <string.h>

void renderer_bw_render(uint8_t *buf, int buf_size) {
    int required = (DISP_W * DISP_H + 1) / 2;
    if (buf_size < required) return;

    memset(buf, (BW_WHITE << 4) | BW_WHITE, (size_t)buf_size);

    for (int dy = 0; dy < DISP_H; dy++) {
        int gy = (dy * GOL_H) / DISP_H;
        if (gy >= GOL_H) gy = GOL_H - 1;

        for (int dx = 0; dx < DISP_W; dx++) {
            int gx = (dx * GOL_W) / DISP_W;
            if (gx >= GOL_W) gx = GOL_W - 1;

            if (gol_get(gol_grid, gx, gy)) {
                int byte_idx = (dy * DISP_W + dx) / 2;
                if ((dx & 1) == 0) {
                    buf[byte_idx] = (uint8_t)((BW_BLACK << 4) | (buf[byte_idx] & 0x0F));
                } else {
                    buf[byte_idx] = (uint8_t)((buf[byte_idx] & 0xF0) | BW_BLACK);
                }
            }
        }
    }
}
