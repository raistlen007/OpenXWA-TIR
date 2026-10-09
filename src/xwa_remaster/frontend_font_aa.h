#ifndef XWA_FRONTEND_FONT_AA_H
#define XWA_FRONTEND_FONT_AA_H

#include <stdint.h>

/* High-quality 4x reconstruction of decoded *frontend* ABP glyph coverage.
 * Returns straight-alpha white ink. The atlas loader premultiplies it once.
 * Caller owns the result; flight HUD fonts never use this function. */
uint8_t* XwaFrontendFontAA_Upscale(const uint8_t* rgba, int width, int height,
                                   int scale, int* out_width, int* out_height);

#endif
