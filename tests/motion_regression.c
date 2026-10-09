/* Fixed-step motion regressions: deterministic for 1/4/8 tick steps,
 * including extremely slow docking and low-rate maneuvering.
 *
 * cc -std=c99 -Wall -Wextra -Werror -Isrc tests/motion_regression.c -o /tmp/xwa-motion-test
 * /tmp/xwa-motion-test
 */
#include "xwa/flight/object/motion_math.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void require(int condition, const char* reason, unsigned rate, unsigned step) {
    if (!condition) {
        fprintf(stderr, "motion regression: %s (rate=%u step=%u)\n", reason, rate, step);
        exit(1);
    }
}

static uint32_t total_ticks(uint32_t clock, uint32_t ticks, uint32_t rate, uint32_t granularity) {
    uint32_t total = 0;
    while (ticks > 0) {
        uint16_t step = (uint16_t)(ticks < granularity ? ticks : granularity);
        total += XwaObject_ScaledTickRate(clock, step, rate);
        clock += step;
        ticks -= step;
    }
    return total;
}

static uint32_t travel_ticks(uint32_t clock, uint32_t ticks, uint16_t speed, uint32_t granularity) {
    uint32_t total = 0;
    while (ticks > 0) {
        uint16_t step = (uint16_t)(ticks < granularity ? ticks : granularity);
        total += XwaObject_ScaledForwardMove(clock, step, speed);
        clock += step;
        ticks -= step;
    }
    return total;
}

int main(void) {
    const uint16_t speeds[] = {0, 1, 2, 3, 5, 10, 13, 20, 40, 100, 300};
    const uint32_t rates[] = {0, 1, 2, 10, 25, 50, 120, 200, 500, 1200, 4096};
    const uint32_t steps[] = {1, 4, 8, 16, 59, 236};
    for (unsigned i = 0; i < sizeof(speeds)/sizeof(speeds[0]); ++i) {
        uint32_t target = (4660u * speeds[i] + 128u) >> 8;
        for (unsigned j = 0; j < sizeof(steps)/sizeof(steps[0]); ++j) {
            require(travel_ticks(0, 236, speeds[i], steps[j]) == target,
                    "one-second forward travel does not conserve total distance", speeds[i], steps[j]);
            require(travel_ticks(87, 472, speeds[i], steps[j]) == target * 2u,
                    "fractional travel depends on time phase or timestep", speeds[i], steps[j]);
            require(travel_ticks(167, 0, speeds[i], steps[j]) == 0,
                    "zero elapsed ticks moved the craft", speeds[i], steps[j]);
        }
    }
    for (unsigned i = 0; i < sizeof(rates)/sizeof(rates[0]); ++i) {
        for (unsigned j = 0; j < sizeof(steps)/sizeof(steps[0]); ++j) {
            require(total_ticks(113, 236, rates[i], steps[j]) == rates[i],
                    "AI turning loses sub-tick angle", rates[i], steps[j]);
            require(total_ticks(113, 472, rates[i], steps[j]) == rates[i] * 2,
                    "AI turning depends on frame granularity", rates[i], steps[j]);
        }
    }
    require(XwaObject_MinimumSignedPush(1, 0) == 1, "positive single-unit push reversed", 1, 0);
    require(XwaObject_MinimumSignedPush(-1, 0) == -1, "negative single-unit push reversed", 1, 0);
    require(XwaObject_MinimumSignedPush(100, 7) == 7, "push step not preserved", 100, 7);
    require(XwaObject_MinimumSignedPush(-100, -7) == -7, "negative push step not preserved", 100, 7);
    puts("motion regression passed (fractional travel, AI turning, signed push)");
    return 0;
}
