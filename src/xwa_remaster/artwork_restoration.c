#include "xwa_remaster/artwork_restoration.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Edge-aware reconstruction of ordered and irregular colour dithering.
 * Use an immutable source to avoid repeated blur. The colour similarity
 * threshold keeps foreground silhouettes and panel edges distinct.
 * No GPU shader or per-frame cost: this runs only when an original 2D
 * illustration is decoded, before constructing its existing GPU atlas. */
size_t XwaArtworkRestoration_Dedither(uint8_t* rgba, int width, int height) {
	if (!rgba || width < 256 || height < 160 ||
		(size_t)width > SIZE_MAX / (size_t)height / 4u)
		return 0;

	const size_t bytes = (size_t)width * (size_t)height * 4u;
	uint8_t* source = (uint8_t*)malloc(bytes);
	if (!source)
		return 0; /* Fail safe to the untouched source artwork. */
	memcpy(source, rgba, bytes);

	static const int spatial[5] = { 1, 4, 6, 4, 1 };
	const int similarity_limit = 32;
	size_t changed = 0;

	for (int y = 2; y < height - 2; ++y) {
		for (int x = 2; x < width - 2; ++x) {
			const size_t index = ((size_t)y * (size_t)width + (size_t)x) * 4u;
			const uint8_t* center = source + index;
			if (center[3] != 255u)
				continue;

			/* Keep high-contrast horizontal and vertical edges sharp. */
			const uint8_t* left = center - 4u;
			const uint8_t* right = center + 4u;
			const uint8_t* up = center - (size_t)width * 4u;
			const uint8_t* down = center + (size_t)width * 4u;
			int large_edge = 0;
			for (int c = 0; c < 3; ++c) {
				const int dx = (int)left[c] - (int)right[c];
				const int dy = (int)up[c] - (int)down[c];
				if (dx > 40 || dx < -40 || dy > 40 || dy < -40)
					large_edge = 1;
			}
			if (large_edge)
				continue;

			/* Weighted local colour reconstruction. Unlike an exact 3x3
			 * checker detector, this also handles 4x4 multi-level Bayer
			 * patterns and gently irregular dithering. Pixels unlike the
			 * center are excluded, so hard boundaries cannot bleed through. */
			uint64_t sum[3] = { 0, 0, 0 };
			uint64_t total_weight = 0;
			for (int dy = -2; dy <= 2; ++dy) {
				for (int dx = -2; dx <= 2; ++dx) {
					const uint8_t* pixel = source +
						((size_t)(y + dy) * (size_t)width + (size_t)(x + dx)) * 4u;
					if (pixel[3] != 255u)
						continue;
					int contrast = 0;
					for (int c = 0; c < 3; ++c) {
						int d = (int)pixel[c] - (int)center[c];
						if (d < 0)
							d = -d;
						if (d > contrast)
							contrast = d;
					}
					if (contrast >= similarity_limit)
						continue;
					const uint64_t weight = (uint64_t)spatial[dy + 2] *
						(uint64_t)spatial[dx + 2] * (uint64_t)(similarity_limit - contrast);
					for (int c = 0; c < 3; ++c)
						sum[c] += weight * pixel[c];
					total_weight += weight;
				}
			}
			if (!total_weight)
				continue;
			uint8_t* output = rgba + index;
			int modified = 0;
			for (int c = 0; c < 3; ++c) {
				const uint8_t value = (uint8_t)((sum[c] + total_weight / 2) / total_weight);
				modified |= output[c] != value;
				output[c] = value;
			}
			changed += (size_t)modified;
		}
	}
	free(source);
	return changed;
}
