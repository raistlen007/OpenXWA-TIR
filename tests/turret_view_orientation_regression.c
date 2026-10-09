/* Gunner seat 2 faces OUT of its cockpit. The corrective turn is a fixed
 * yaw about seat UP, applied to the view only, after turret aim and before
 * TrackIR. No cockpit mesh, pivot, hardpoint or gun-aim changes are allowed.
 *
 * cc -std=c99 -Wall -Wextra -Werror -Isrc \
 *   tests/turret_view_orientation_regression.c -lm -o /tmp/xwa-turret-view-test
 * /tmp/xwa-turret-view-test
 */
#include "xwa/flight/turret_view_orientation.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void demand(int ok, const char* msg) {
    if (!ok) {
        fprintf(stderr, "lower turret view: %s\n", msg);
        exit(1);
    }
}

static double dot(const double a[3], const double b[3]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

int main(void) {
    demand(!XwaTurretView_ReverseCameraFacing(0, 0), "pilot changed");
    demand(!XwaTurretView_ReverseCameraFacing(1, 0), "upper turret changed");
    demand(!XwaTurretView_ReverseCameraFacing(2, 1), "map camera changed");
    demand(XwaTurretView_ReverseCameraFacing(2, 0), "lower turret not corrected");

    /* Arbitrarily tilted, orthonormal camera frame; row 0 = right,
     * row 1 = up, row 2 = forward. A horizontal half turn reverses
     * RIGHT and FORWARD, but must leave UP unchanged (no upside-down seat).
     * This operation must be independent of gun aim and head roll. */
    const double rows[9] = {
        0.6, 0.0, -0.8,
        0.0, 1.0,  0.0,
        0.8, 0.0,  0.6
    };
    const double expected[9] = {
        -0.6, 0.0,  0.8,
         0.0, 1.0,  0.0,
        -0.8, 0.0, -0.6
    };
    double turned[9];
    for (int i = 0; i < 9; ++i) turned[i] = rows[i];
    XwaTurretView_ReverseRowsDouble(turned);
    for (int i = 0; i < 9; ++i)
        demand(fabs(turned[i] - expected[i]) < 1.e-12,
               "camera not yawed exactly 180 about its up axis");
    demand(fabs(dot(turned, turned+3)) < 1.e-12 &&
           fabs(dot(turned+3, turned+6)) < 1.e-12,
           "rotation corrupted orthogonality");
    demand(fabs(dot(turned+3, rows+3) - 1.0) < 1.e-12,
           "lower turret flipped upside down");
    demand(fabs(dot(turned+6, rows+6) + 1.0) < 1.e-12,
           "lower turret still facing into chair");
    XwaTurretView_ReverseRowsDouble(turned);
    for (int i = 0; i < 9; ++i)
        demand(fabs(turned[i] - rows[i]) < 1.e-12,
               "rotation is not a half-turn");

    demand(XwaTurretView_NegateQ15(0) == 0, "zero Q15 rotation changed");
    demand(XwaTurretView_NegateQ15(12345) == -12345, "Q15 positive sign");
    demand(XwaTurretView_NegateQ15(-12345) == 12345, "Q15 negative sign");
    demand(XwaTurretView_NegateQ15(INT16_MIN) == INT16_MAX,
           "Q15 minimum overflowed");

    puts("lower turret view passed (outward yaw, unchanged up, upper seat untouched)");
    return 0;
}
