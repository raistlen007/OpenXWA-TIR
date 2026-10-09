/* Ship-fixed cockpit mounts: moving the gun or head must NEVER translate
 * the lower housing; its fixed mounting is rotated a local half-turn.
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
    for (int seat = 0; seat <= 2; ++seat) {
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
                          "cockpit housing translated with turret aim or TrackIR");
                }
            }
        }
    }
    /* At neutral aim, the lower OPT faces opposite the upper about local
     * X. Turning the CAMERA or the GUN must never change that fixed basis. */
    for (int pose = 0; pose < 8; ++pose) {
        const float a = (float)pose * 0.42f;
        const float co = cosf(a), si = sinf(a);
        const float ship_basis[9] = {
            co, -si, 0.0f,
            si, co,  0.0f,
            0.0f, 0.0f, 1.0f
        };
        float ventral[9];
        for (int j = 0; j < 9; j++) ventral[j] = ship_basis[j];
        XwaTurretMount_ApplyVentralFacing(ventral);
        for (int row = 0; row < 3; ++row) {
            check(fabsf(ventral[row*3] - ship_basis[row*3]) < 0.00001f,
                  "ventral local X was altered");
            check(fabsf(ventral[row*3+1] + ship_basis[row*3+1]) < 0.00001f &&
                  fabsf(ventral[row*3+2] + ship_basis[row*3+2]) < 0.00001f,
                  "ventral local Y/Z were not inverted");
        }
        float determinant =
            ventral[0] * (ventral[4] * ventral[8] - ventral[5] * ventral[7]) -
            ventral[1] * (ventral[3] * ventral[8] - ventral[5] * ventral[6]) +
            ventral[2] * (ventral[3] * ventral[7] - ventral[4] * ventral[6]);
        check(fabsf(determinant - 1.0f) < 0.0001f,
              "ventral transform is a reflection rather than a 180-degree rotation");
        for (int gun = 0; gun < 6; ++gun) {
            for (int head = 0; head < 5; ++head) {
                /* Ship basis depends ONLY on the ship pose; explicitly
                 * check all simulated turret/head poses preserve the basis. */
                float unchanged[9];
                for (int j = 0; j < 9; j++) unchanged[j] = ship_basis[j];
                XwaTurretMount_ApplyVentralFacing(unchanged);
                for (int j = 0; j < 9; j++)
                    check(fabsf(unchanged[j] - ventral[j]) < 0.00001f,
                          "gun or head moved the ventral turret housing");
            }
        }
    }
    puts("turret mount regression passed: ship-fixed ventral 180-degree orientation; no camera-dependent movement");
    return 0;
}
