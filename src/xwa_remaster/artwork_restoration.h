#ifndef XWA_REMASTER_ARTWORK_RESTORATION_H
#define XWA_REMASTER_ARTWORK_RESTORATION_H

#include <stddef.h>
#include <stdint.h>

/* Selectively reconstruct alternating two-colour dithering in opaque RGBA8
 * artwork. Changes RGB in memory only, preserves alpha and original disk files.
 * Returns the number of altered pixels. */
size_t XwaArtworkRestoration_Dedither(uint8_t* rgba, int width, int height);

#endif
