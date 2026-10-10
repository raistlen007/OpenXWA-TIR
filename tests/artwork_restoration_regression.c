#include "xwa_remaster/artwork_restoration.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Smaller than the old 512x320 cut-off. Stand-alone item/medal sprites
 * must not be silently excluded on the basis of their dimensions. */
enum { W = 128, H = 128 };
static const int ordered_dither[4][4] = {
	{ -11, 5, -7, 9 }, { 7, -9, 11, -5 },
	{ -6, 10, -10, 6 }, { 12, -4, 4, -12 }
};

int main(void) {
	const size_t bytes = (size_t)W * H * 4u;
	uint8_t* original = (uint8_t*)malloc(bytes);
	uint8_t* restored = (uint8_t*)malloc(bytes);
	assert(original && restored);

	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			uint8_t* pixel = original + ((size_t)y * W + x) * 4u;
			const int noise = ordered_dither[y % 4][x % 4];
			if (x < 80) {
				pixel[0] = (uint8_t)(110 + noise);
				pixel[1] = (uint8_t)(128 + noise);
				pixel[2] = (uint8_t)(154 + noise);
			} else if (x < 104) {
				pixel[0] = pixel[1] = pixel[2] = (uint8_t)(40 + (x - 80) * 2);
			} else {
				pixel[0] = pixel[1] = pixel[2] = 220u;
			}
			pixel[3] = x >= 5 && x < 15 && y >= 40 && y < 60 ? 0 : 255;
		}
	}

	memcpy(restored, original, bytes);
	const size_t changed = XwaArtworkRestoration_Dedither(restored, W, H);
	long long before = 0;
	long long after = 0;
	for (int y = 10; y < H - 10; ++y) {
		for (int x = 20; x < 70; ++x) {
			const size_t offset = ((size_t)y * W + x) * 4u;
			before += abs((int)original[offset] - 110);
			after += abs((int)restored[offset] - 110);
		}
	}
	printf("small multi-level dither: changed %zu pixels, residual %lld -> %lld\n",
		   changed, before, after);
	assert(changed > 3000);
	assert(after * 5 <= before * 3); /* >=40% reduction */

	/* True gradients and line-free flat colors must remain bit-exact. */
	for (int y = 12; y < 115; ++y) {
		for (int x = 85; x < 99; ++x) {
			const size_t at = ((size_t)y * W + x) * 4u;
			assert(memcmp(original + at, restored + at, 4u) == 0);
		}
	}
	for (int y = 5; y < 120; ++y) {
		for (int x = 108; x < 123; ++x) {
			const size_t at = ((size_t)y * W + x) * 4u;
			assert(memcmp(original + at, restored + at, 4u) == 0);
		}
	}
	for (size_t at = 0; at < bytes; at += 4u)
		assert(original[at + 3u] == restored[at + 3u]);

	assert(XwaArtworkRestoration_Dedither(NULL, W, H) == 0);
	assert(XwaArtworkRestoration_Dedither(restored, 2, H) == 0);
	assert(XwaArtworkRestoration_Dedither(restored, W, 2) == 0);
	free(original);
	free(restored);
	return 0;
}
