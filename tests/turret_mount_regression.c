/* Pilot and upper gunner still use ship-fixed cockpit mounting; the lower
 * turret intentionally restores its original pre-TrackIR transform.
 * cc -std=c99 -Wall -Wextra -Werror -Isrc tests/turret_mount_regression.c -lm -o /tmp/xwa-turret-test
 * /tmp/xwa-turret-test
 */
#include "xwa_remaster/turret_mount_frame.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void check(int ok, const char* reason) {
    if (!ok) { fprintf(stderr, "turret mount: %s\n", reason); exit(1); }
}

/* The unchanged fixed-mount path for pilot and upper turret remains
 * independent of weapon rotation and head translation. */
int main(void) {
    const float ship_origin[3] = { 10000.0f, -2500.0f, 810.0f };
    const float pan[3] = { 64.0f, -16.0f, 32.0f };
    const float gun_aim_hardpoints[6][3] = {
        { 0.0f, 0.0f, -320.0f },
        { 200.0f, -120.0f, -280.0f },
        { -150.0f, 240.0f, -310.0f },
        { 210.0f, 140.0f, -180.0f },
        { -80.0f, -190.0f, -420.0f },
        { 400.0f, 50.0f, -30.0f }
    };
    const float head_offsets[5][3] = {
        { 0.0f, 0.0f, 0.0f },
        { 70.0f, 0.0f, 0.0f },
        { -45.0f, 120.0f, 10.0f },
        { 0.0f, 0.0f, -60.0f },
        { 35.0f, -15.0f, 85.0f }
    };
    for (int seat = 0; seat < 2; ++seat) {
        for (int gun = 0; gun < 6; ++gun) {
            for (int head = 0; head < 5; ++head) {
                const float* hardpoint = gun_aim_hardpoints[gun];
                const float* head_offset = head_offsets[head];
                float camera[3], cockpit[3];
                for (int i = 0; i < 3; ++i) {
                    camera[i] = ship_origin[i] + hardpoint[i] +
                                pan[i] * 0.0625f + head_offset[i];
                }
                XwaTurretMount_CockpitOrigin(camera, head_offset, hardpoint, pan, cockpit);
                for (int i = 0; i < 3; ++i) {
                    check(fabsf(cockpit[i] - ship_origin[i]) < 0.001f,
                          "cockpit origin changed under turret aim or TrackIR");
                }
            }
        }
    }
    /* Ventral turret is intentionally not covered by this helper: its
     * original cockpit transform is restored until in-game verification. */
    puts("turret mount regression passed (pilot/upper remain ship-fixed)");
    return 0;
}
