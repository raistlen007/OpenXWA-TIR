/* Standalone deterministic checks for the modern gimbal-lock orientation hook.
 * Run: cc -std=c99 -Isrc tests/orientation_regression.c
 *             src/xwa_runtime/hooks/orientation_hook.c -lm -o /tmp/xwa-orientation-test
 *      /tmp/xwa-orientation-test
 */
#include "xwa_runtime/hooks/orientation_hook.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846
#define Q16RAD (PI / 32768.0)

static void fail(const char* message, int iteration) {
	fprintf(stderr, "orientation regression: %s at iteration %d\n", message, iteration);
	exit(1);
}

static int same(XwaOrientationAngles a, XwaOrientationAngles b) {
	return a.yaw == b.yaw && a.pitch == b.pitch && a.roll == b.roll;
}

static void rotate_local(double m[3][3], int axis, double angle) {
	double x = m[axis][0], y = m[axis][1], z = m[axis][2];
	double c = cos(angle), s = sin(angle), k = 1.0 - c;
	double r[3][3] = {
		{ c+x*x*k, x*y*k-z*s, x*z*k+y*s },
		{ y*x*k+z*s, c+y*y*k, y*z*k-x*s },
		{ z*x*k-y*s, z*y*k+x*s, c+z*z*k }
	};
	double n[3][3];
	for (int col = 0; col < 3; ++col)
		for (int row = 0; row < 3; ++row)
			n[col][row] = r[row][0]*m[col][0] + r[row][1]*m[col][1] + r[row][2]*m[col][2];
	for (int col = 0; col < 3; ++col)
		for (int row = 0; row < 3; ++row)
			m[col][row] = n[col][row];
}

static void native_basis(XwaOrientationAngles a, double m[3][3]) {
	for (int col = 0; col < 3; ++col)
		for (int row = 0; row < 3; ++row)
			m[col][row] = col == row ? 1.0 : 0.0;
	rotate_local(m, 1, -(int16_t)a.yaw * Q16RAD);
	rotate_local(m, 0, -PI * 0.5 - (int16_t)a.pitch * Q16RAD);
	rotate_local(m, 2, -(int16_t)a.roll * Q16RAD);
}

static double matrix_distance(XwaOrientationAngles a, XwaOrientationAngles b) {
	double x[3][3], y[3][3], error = 0.0;
	native_basis(a, x);
	native_basis(b, y);
	for (int col = 0; col < 3; ++col)
		for (int row = 0; row < 3; ++row) {
			const double d = x[col][row] - y[col][row];
			error += d * d;
		}
	return sqrt(error);
}

int main(void) {
	XwaOrientationAngles a = { 0, 0, 0 };
	const XwaOrientationAngles initial = a;

	/* A no-op must not rewrite stored angles at or near the singularity. */
	if (!same(a, XwaOrientation_ApplyPitchYaw(a, 0, 0)))
		fail("zero-angle input altered orientation", 0);

	/* Pitch-only through the former singularity must never invent roll/yaw.
	 * A 32-unit pitch should produce exactly 32 units of native pitch. */
	for (int i = 1; i <= 128; ++i) {
		a = XwaOrientation_ApplyPitchYaw(a, 32, 0);
		if (a.yaw != 0 || a.roll != 0 || (int16_t)a.pitch != -32 * i)
			fail("pitch-only input creates yaw/roll or loses pitch steps", i);
	}
	for (int i = 1; i <= 128; ++i) {
		a = XwaOrientation_ApplyPitchYaw(a, -32, 0);
		if (a.yaw != 0 || a.roll != 0 || (int16_t)a.pitch != -4096 + 32 * i)
			fail("reverse pitch input does not unwind cleanly", i);
	}
	if (!same(a, initial)) fail("pitch cycle failed to return to origin", 0);

	/* An arbitrary no-input attitude should also be left byte-for-byte alone. */
	a = (XwaOrientationAngles){ 24100, 0, 11300 };
	if (!same(a, XwaOrientation_ApplyPitchYaw(a, 0, 0)))
		fail("zero input rewrote noncanonical native angles", 0);

	/* Stress each native representation, especially extreme pitch and roll.
	 * The physical rotation of the craft must stay small for small input,
	 * even when the closest native Euler representation changes branch. */
	uint32_t seed = 0x1784BEEF;
	for (int i = 0; i < 4096; ++i) {
		seed = seed * 1664525u + 1013904223u;
		a.yaw = (Q16Angle)(seed >> 16);
		seed = seed * 1664525u + 1013904223u;
		a.pitch = (Q16Angle)(i % 9 == 0 ? 0 : i % 9 == 1 ? 0x4000 : i % 9 == 2 ? 0xc000 : seed >> 16);
		seed = seed * 1664525u + 1013904223u;
		a.roll = (Q16Angle)(seed >> 16);
		const int pitch = (i % 5 - 2) * 48;
		const int yaw = (i % 7 - 3) * 32;
		const XwaOrientationAngles b = XwaOrientation_ApplyPitchYaw(a, pitch, yaw);
		/* 0.025 rad exceeds the 0.021 rad combined maximum requested
		 * here, permitting float extraction error but rejecting flips. */
		if (!isfinite(matrix_distance(a, b)) || matrix_distance(a, b) > 0.025)
			fail("small input caused large physical rotation", i);
	}
	puts("orientation regression passed (pitch reversal, branch continuity, 4096 extreme attitudes)");
	return 0;
}
