#include "xwa_remaster/artwork_restoration.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { W = 320, H = 200 };

int main(void) {
	const size_t bytes = (size_t)W * H * 4u;
	uint8_t* pixels = (uint8_t*)malloc(bytes);
	uint8_t* original = (uint8_t*)malloc(bytes);
	assert(pixels && original);
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			uint8_t* p = pixels + ((size_t)y * W + x) * 4u;
			p[0] = p[1] = p[2] = ((x + y) & 1) ? 80 : 96;
			p[3] = 255u;
		}
	}
	/* Preserve real details and full transparency. */
	for (int y = 50; y < 60; ++y) for (int x = 50; x < 70; ++x) {
		uint8_t* p = pixels + ((size_t)y * W + x) * 4u;
		p[0] = p[1] = p[2] = 220u;
	}
	for (int y = 80; y < 90; ++y) for (int x = 80; x < 90; ++x)
		pixels[((size_t)y * W + x) * 4u + 3u] = 0u;
	memcpy(original, pixels, bytes);
	const size_t changed = XwaArtworkRestoration_Dedither(pixels, W, H);
	assert(changed > 10000);
	assert(pixels[((size_t)20 * W + 20) * 4u] > 80);
	assert(pixels[((size_t)20 * W + 20) * 4u] < 96);
	for (size_t i = 0; i < bytes; i += 4u)
		assert(original[i + 3u] == pixels[i + 3u]);
	for (int y = 52; y < 58; ++y) for (int x = 52; x < 68; ++x)
		assert(pixels[((size_t)y * W + x) * 4u] == 220u);
	assert(memcmp(pixels + ((size_t)85 * W + 85) * 4u,
		original + ((size_t)85 * W + 85) * 4u, 4u) == 0);
	assert(XwaArtworkRestoration_Dedither(NULL, W, H) == 0);
	assert(XwaArtworkRestoration_Dedither(pixels, 0, H) == 0);
	assert(XwaArtworkRestoration_Dedither(pixels, 24, 32) == 0);
	printf("artwork restoration passed: %zu reconstructed pixels\n", changed);
	free(original);
	free(pixels);
	return 0;
}
