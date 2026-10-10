#include "xwa_remaster/artwork_restoration.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { W = 320, H = 200 };

/* Multi-level 4x4 dither: unlike a perfect checkerboard, this must be
 * reconstructed by the real algorithm, not silently left untouched. */
static const int dither[4][4] = {
	{ -11, 5, -7, 9 },
	{ 7, -9, 11, -5 },
	{ -6, 10, -10, 6 },
	{ 12, -4, 4, -12 }
};

int main(void) {
	const size_t bytes = (size_t)W * (size_t)H * 4u;
	uint8_t* source = (uint8_t*)malloc(bytes);
	uint8_t* restored = (uint8_t*)malloc(bytes);
	assert(source && restored);

	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			uint8_t* pixel = source + ((size_t)y * W + x) * 4u;
			const int noise = dither[y % 4][x % 4];
			pixel[0] = (uint8_t)(110 + noise);
			pixel[1] = (uint8_t)(128 + noise);
			pixel[2] = (uint8_t)(154 + noise);
			pixel[3] = 255u;
			if (x >= 100 && x < 120)
				pixel[0] = pixel[1] = pixel[2] = 20u;
			if (x >= 150 && x < 160 && y >= 50 && y < 70)
				pixel[3] = 0u;
		}
	}
	memcpy(restored, source, bytes);
	const size_t changed = XwaArtworkRestoration_Dedither(restored, W, H);
	assert(changed > 10000u);

	long long before = 0, after = 0;
	for (int y = 10; y < H - 10; ++y) {
		for (int x = 20; x < 80; ++x) {
			const size_t index = ((size_t)y * W + x) * 4u;
			before += abs((int)source[index] - 110);
			after += abs((int)restored[index] - 110);
		}
	}
	printf("4x4 ordered-dither residual: %lld -> %lld, changed %zu pixels\n",
		before, after, changed);
	/* Prove the outcome is materially cleaner, not simply a nonzero counter. */
	assert(after * 2 < before);
	for (int y = 10; y < H - 10; ++y)
		for (int x = 102; x < 118; ++x)
			assert(restored[((size_t)y * W + x) * 4u] == 20u);
	for (size_t i = 0; i < bytes; i += 4u)
		assert(source[i + 3u] == restored[i + 3u]);
	for (int y = 50; y < 70; ++y) {
		for (int x = 150; x < 160; ++x) {
			const size_t index = ((size_t)y * W + x) * 4u;
			assert(memcmp(source + index, restored + index, 4u) == 0);
		}
	}
	assert(XwaArtworkRestoration_Dedither(NULL, W, H) == 0);
	assert(XwaArtworkRestoration_Dedither(restored, 24, 32) == 0);
	free(source);
	free(restored);
	return 0;
}
