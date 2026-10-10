#include "xwa_remaster/frontend_font_aa.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* Mitchell-Netravali (B=C=1/3) reconstructs the 4x enlarged original glyph
 * coverage without nearest-neighbour's flat pixel steps. The original ABP
 * decoder packs glyphs with gutters; neighbouring shapes cannot bleed into
 * one another over this 2-original-pixel support radius. */
static float aa_kernel(float x) {
    x = fabsf(x);
    if (x < 1.0f)
        return ((7.0f*x - 12.0f)*x*x + (16.0f / 3.0f)) / 6.0f;
    if (x < 2.0f)
        return (((-7.0f / 3.0f)*x + 12.0f)*x*x - 20.0f*x + (32.0f/3.0f)) / 6.0f;
    return 0.0f;
}

uint8_t* XwaFrontendFontAA_Upscale(const uint8_t* rgba, int width, int height,
                                   int scale, int* out_width, int* out_height) {
    if (!rgba || !out_width || !out_height || width < 1 || height < 1 ||
        scale < 1 || width > INT32_MAX / scale || height > INT32_MAX / scale)
        return NULL;
    const int w = width * scale, h = height * scale;
    if ((size_t)w > SIZE_MAX / (size_t)h / 4u)
        return NULL;
    uint8_t* out = (uint8_t*)malloc((size_t)w * (size_t)h * 4u);
    if (!out) return NULL;
    for (int y = 0; y < h; ++y) {
        const float sy = ((float)y + 0.5f) / (float)scale - 0.5f;
        const int cy = (int)floorf(sy);
        float wy[4];
        for (int j = 0; j < 4; ++j)
            wy[j] = aa_kernel(sy - (float)(cy + j - 1));
        for (int x = 0; x < w; ++x) {
            const float sx = ((float)x + 0.5f) / (float)scale - 0.5f;
            const int cx = (int)floorf(sx);
            float weighted_alpha = 0.0f;
            for (int j = 0; j < 4; ++j) {
                const int yy = cy + j - 1;
                if (yy < 0 || yy >= height) continue;
                for (int i = 0; i < 4; ++i) {
                    const int xx = cx + i - 1;
                    if (xx < 0 || xx >= width) continue;
                    weighted_alpha += wy[j] * aa_kernel(sx - (float)xx) *
                        (float)rgba[((size_t)yy * (size_t)width + (size_t)xx) * 4u + 3u];
                }
            }
            if (weighted_alpha < 0.0f) weighted_alpha = 0.0f;
            if (weighted_alpha > 255.0f) weighted_alpha = 255.0f;
            uint8_t* pixel = out + ((size_t)y * (size_t)w + (size_t)x) * 4u;
            pixel[0] = pixel[1] = pixel[2] = 255u;
            pixel[3] = (uint8_t)(weighted_alpha + 0.5f);
        }
    }
    *out_width = w;
    *out_height = h;
    return out;
}
