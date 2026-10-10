#include "xwa_remaster/artwork_restoration.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int near_rgb(const uint8_t* a, const uint8_t* b, int tolerance) {
	for (int c = 0; c < 3; ++c) {
		const int difference = (int)a[c] - (int)b[c];
		if (difference < -tolerance || difference > tolerance)
			return 0;
	}
	return 1;
}

size_t XwaArtworkRestoration_Dedither(uint8_t* rgba, int width, int height) {
	/* Reject small icons/glyphs and guard multiplication before allocating. */
	if (!rgba || width < 256 || height < 160 ||
		(size_t)width > SIZE_MAX / (size_t)height / 4u)
		return 0;

	const size_t bytes = (size_t)width * (size_t)height * 4u;
	uint8_t* const source = (uint8_t*)malloc(bytes);
	if (!source)
		return 0; /* Optional filter must never prevent the original from loading. */
	memcpy(source, rgba, bytes);

	size_t changed = 0;
	for (int y = 1; y < height - 1; ++y) {
		for (int x = 1; x < width - 1; ++x) {
			const size_t base = ((size_t)y * (size_t)width + (size_t)x) * 4u;
			const uint8_t* const center = source + base;
			const uint8_t* const neighbor[8] = {
				center - (size_t)width * 4u, center + (size_t)width * 4u,
				center - 4u, center + 4u,
				center - (size_t)width * 4u - 4u, center - (size_t)width * 4u + 4u,
				center + (size_t)width * 4u - 4u, center + (size_t)width * 4u + 4u
			};
			if (center[3] != 255u)
				continue;

			/* Exactly alternating locally: axial samples match one another,
			 * diagonals match the center. Reject edges and transparency. */
			int valid = 1;
			for (int i = 0; i < 8; ++i) {
				if (neighbor[i][3] != 255u ||
					!near_rgb(neighbor[i], i < 4 ? neighbor[0] : center, 12)) {
					valid = 0;
					break;
				}
			}
			if (!valid)
				continue;
			int contrast = 0;
			for (int c = 0; c < 3; ++c) {
				int d = (int)center[c] - (int)neighbor[0][c];
				if (d < 0) d = -d;
				if (d > contrast) contrast = d;
			}
			/* Avoid flattening intentional high-contrast pixel patterns. */
			if (contrast < 8 || contrast > 48)
				continue;

			uint8_t* const dst = rgba + base;
			for (int c = 0; c < 3; ++c) {
				int sum = (int)center[c];
				for (int i = 0; i < 8; ++i)
					sum += (int)neighbor[i][c];
				dst[c] = (uint8_t)((sum + 4) / 9);
			}
			if (!near_rgb(dst, center, 0))
				++changed;
		}
	}
	free(source);
	return changed;
}
