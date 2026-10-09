/* Gunner response tests, both high-rate and 8-tick classic cadence.
 * cc -std=c99 -Wall -Wextra -Werror -Isrc tests/turret_aim_regression.c -lm -o /tmp/xwa-turret-aim-test
 * /tmp/xwa-turret-aim-test
 */
#include "xwa/flight/turret_aim_response.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void require(int condition, const char* reason) {
    if (!condition) { fprintf(stderr, "turret aim: %s\n", reason); exit(1); }
}

/* Simulate total aim angle while the operator holds an axis at constant
 * input and then releases. A high-rate path should approach the same
 * steady angular velocity, without multiplying by render FPS. */
static double aim_for_ticks(int frameTicks, int totalTicks, double input,
                            double* lastAccum) {
    double a = *lastAccum, angle = 0.0;
    for (int t = 0; t < totalTicks; t += frameTicks) {
        double scale = XwaTurretAim_TimeScale((uint16_t)frameTicks, frameTicks < 8);
        a = XwaTurretAim_UpdateAccumulator(a, input, scale);
        angle += a * scale;
    }
    *lastAccum = a;
    return angle;
}

int main(void) {
    double previous = 0.0;
    /* The gunner input is now the original signed yaw/pitch input for
     * BOTH seats. No lower-seat-only pitch reversal is permitted. */
    require(XwaTurretAim_UpdateAccumulator(0.0, 100.0, 1.0) ==
            -XwaTurretAim_UpdateAccumulator(0.0, -100.0, 1.0),
            "gunner positive/negative pitch response must be symmetric");
    require(XwaTurretAim_UpdateAccumulator(0.0, 100.0, 1.0) == 150.0,
            "new gunner gains must respond promptly to the first full-rate step");
    for (int tick = 1; tick <= 120; ++tick) {
        const double scale = XwaTurretAim_TimeScale(1, 1);
        previous = XwaTurretAim_UpdateAccumulator(previous, 100.0, scale);
    }
    require(fabs(previous - 300.0) < 1.0,
            "high-rate turret must reach threefold input rate without vanishing");
    double slow = 0.0, fast = 0.0;
    /* At steady-state, the high-rate and compatibility update must move the
     * turret by roughly the same angle over equivalent simulation time. */
    slow = 300.0; fast = 300.0;
    const double slow_angle = aim_for_ticks(8, 800, 100.0, &slow);
    const double fast_angle = aim_for_ticks(1, 800, 100.0, &fast);
    require(fabs(slow_angle - fast_angle) < 0.01,
            "turn rate must not depend on simulation tick size at steady state");
    require(fabs(slow_angle - 30000.0) < 0.01,
            "turret should advance at normalized 3x input angular rate");
    const double neutral = XwaTurretAim_UpdateAccumulator(300.0, 0.0, 1.0);
    require(neutral == 150.0, "release must damp turret momentum promptly");
    require(XwaTurretAim_UpdateAccumulator(200.0, -50.0, 1.0) < 200.0,
            "reversing stick must immediately start reversing turret response");
    puts("turret response regression passed (faster aim, damping, tick-rate parity)");
    return 0;
}
