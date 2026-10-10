#include "xwa_remaster/artwork_restoration.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Reconstruct only alternating/isolated dither samples. Unlike bilateral blur,
 * this has no effect on smooth gradients or ordinary continuous shading.
 * Both horizontal and vertical neighbours must agree with each other but
 * differ from the center: the characteristic high-frequency dither pattern.
 * The immutable source prevents multiple smoothing passes during traversal. */
size_t XwaArtworkRestoration_Dedither(uint8_t* rgba, int width, int height) {
	if (!rgba || width < 3 || height < 3 ||
		(size_t)width > SIZE_MAX / (size_t)height / 4u)
		return 0;
	const size_t bytes = (size_t)width * (size_t)height * 4u;
	uint8_t* original = (uint8_t*)malloc(bytes);
	if (!original)
		return 0;
	memcpy(original, rgba, bytes);

	size_t changed = 0;
	const size_t pitch = (size_t)width * 4u;
	for (int y = 1; y < height - 1; ++y) {
		for (int x = 1; x < width - 1; ++x) {
			const size_t at = (size_t)y * pitch + (size_t)x * 4u;
			const uint8_t* const center = original + at;
			const uint8_t* const left = center - 4u;
			const uint8_t* const right = center + 4u;
			const uint8_t* const up = center - pitch;
			const uint8_t* const down = center + pitch;
			if (center[3] != 255u || left[3] != 255u || right[3] != 255u ||
				up[3] != 255u || down[3] != 255u)
				continue;

			int horizontal_difference = 0;
			int vertical_difference = 0;
			int horizontal_contrast = 0;
			int vertical_contrast = 0;
			for (int c = 0; c < 3; ++c) {
				int d = (int)left[c] - (int)right[c];
				if (d < 0) d = -d;
				if (d > horizontal_difference) horizontal_difference = d;
				d = (int)up[c] - (int)down[c];
				if (d < 0) d = -d;
				if (d > vertical_difference) vertical_difference = d;
				d = (int)center[c] * 2 - (int)left[c] - (int)right[c];
				if (d < 0) d = -d;
				if (d > horizontal_contrast) horizontal_contrast = d;
				d = (int)center[c] * 2 - (int)up[c] - (int)down[c];
				if (d < 0) d = -d;
				if (d > vertical_contrast) vertical_contrast = d;
			}
			/* A plain gradient has diverging opposite neighbours, so it is
			 * unchanged. Avoid strong lines and distinct surface colors. */
			if (horizontal_difference > 9 || vertical_difference > 9 ||
				horizontal_contrast < 10 || horizontal_contrast > 64 ||
				vertical_contrast < 10 || vertical_contrast > 64)
				continue;

			uint8_t* const dest = rgba + at;
			int modified = 0;
			for (int c = 0; c < 3; ++c) {
				const int adjacent = (int)left[c] + (int)right[c] +
									 (int)up[c] + (int)down[c];
				/* 75% interpolation toward the neighbour consensus.
				 * RGB can never bleed into alpha or transparency. */
				const uint8_t value = (uint8_t)((4 * (int)center[c] +
												 3 * adjacent + 8) / 16);
				modified |= dest[c] != value;
				dest[c] = value;
			}
			changed += (size_t)modified;
		}
	}
	free(original);
	return changed;
}
