/* Classic-vs-modern ventral cockpit transform PARITY, not string matching.
 * Independent oracle: the original renderer applies a 180-degree
 * LOCAL-X rotation about the gunner's current MODEL-SPACE hardpoint.
 *
 * cc -std=c99 -Wall -Wextra -Werror -Isrc tests/turret_mount_regression.c \
 *    -lm -o /tmp/xwa-turret-test && /tmp/xwa-turret-test
 */
#include "xwa_remaster/turret_mount_frame.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int truth, const char *why) {
    if (!truth) {
        fprintf(stderr, "ventral cockpit parity FAIL: %s\n", why);
        exit(1);
    }
}

static int close_to(float a, float b) {
    return fabsf(a - b) < 0.015f;
}

/* Original OPT axes are ROWS; native model->world multiplies the rows
 * by local model coordinates. A tilted ship makes ROW/COLUMN confusion
 * observable (axis-aligned tests alone cannot detect it). */
static void ship_axes(int pose, float out[9]) {
    const float yaw = 0.31f + (float)pose * 0.37f;
    const float pitch = -0.42f + (float)pose * 0.13f;
    const float cy = cosf(yaw), sy = sinf(yaw);
    const float cp = cosf(pitch), sp = sinf(pitch);
    const float axes[9] = {
        cy*cp, -sy, cy*sp,
        sy*cp,  cy, sy*sp,
          -sp, 0.0f, cp
    };
    memcpy(out, axes, sizeof axes);
}

static void model_to_world(const float basis[9],
                           const float position[3],
                           const float vertex[3], float out[3]) {
    for (int j = 0; j < 3; ++j) {
        out[j] = position[j] +
                 basis[0 * 3 + j] * vertex[0] +
                 basis[1 * 3 + j] * vertex[1] +
                 basis[2 * 3 + j] * vertex[2];
    }
}

int main(void) {
    const float ship_origin[3] = {10000.0f, -2500.0f, 810.0f};
    const float pan[3] = {64.0f, -16.0f, 32.0f};
    const float head_displacements[][3] = {
        {0.0f, 0.0f, 0.0f}, {70.0f, 0.0f, 0.0f},
        {-45.0f, 120.0f, 10.0f}, {35.0f, -15.0f, 85.0f}
    };
    /* Aim-dependent seat hardpoints, in ORIGINAL model-local coordinates. */
    const float seat_pivots[][3] = {
        {0.0f, 0.0f, -320.0f}, {200.0f, -120.0f, -280.0f},
        {-150.0f, 240.0f, -310.0f}, {210.0f, 140.0f, -180.0f},
        {-80.0f, -190.0f, -420.0f}, {400.0f, 50.0f, -30.0f}
    };
    const float vertices[][3] = {
        {0.0f, 0.0f, 0.0f}, {155.0f, -40.0f, 230.0f},
        {-200.0f, 230.0f, -115.0f}, {92.0f, -300.0f, 75.0f}
    };
    const float classic_half_turn[3] = {1.0f, -1.0f, -1.0f};
    int comparisons = 0;

    for (int pose = 0; pose < 10; ++pose) {
        float basis[9], ventral[9];
        ship_axes(pose, basis);
        memcpy(ventral, basis, sizeof ventral);
        XwaTurretMount_ApplyVentralFacing(ventral);

        /* The original local X row MUST survive and local Y/Z ROWS
         * must be flipped; the broken implementation flipped columns. */
        for (int row = 0; row < 3; ++row) {
            for (int axis = 0; axis < 3; ++axis) {
                const float expected = basis[row * 3 + axis] * classic_half_turn[row];
                check(close_to(ventral[row * 3 + axis], expected),
                      "ventral half-turn is not in model-local coordinates");
            }
        }
        const float det =
            ventral[0] * (ventral[4]*ventral[8] - ventral[5]*ventral[7]) -
            ventral[1] * (ventral[3]*ventral[8] - ventral[5]*ventral[6]) +
            ventral[2] * (ventral[3]*ventral[7] - ventral[4]*ventral[6]);
        check(close_to(det, 1.0f), "ventral rotation is not proper");

        for (unsigned gun = 0; gun < sizeof seat_pivots / sizeof seat_pivots[0]; ++gun) {
            const float *pivot = seat_pivots[gun];
            float world_pivot[3], native_seat_world[3];
            model_to_world(basis, (const float[3]){0.0f,0.0f,0.0f},
                           pivot, world_pivot);
            for (int axis = 0; axis < 3; ++axis)
                native_seat_world[axis] = ship_origin[axis] + world_pivot[axis];

            for (unsigned head = 0; head < sizeof head_displacements / sizeof head_displacements[0]; ++head) {
                const float *head_offset = head_displacements[head];
                float camera[3], recovered[3], world_translation[3], new_pivot_world[3];
                for (int axis = 0; axis < 3; ++axis) {
                    camera[axis] = native_seat_world[axis] +
                                   pan[axis] * 0.0625f + head_offset[axis];
                }
                XwaTurretMount_CockpitOrigin(camera, head_offset,
                                             world_pivot, pan, recovered);
                for (int axis = 0; axis < 3; ++axis)
                    check(close_to(recovered[axis], ship_origin[axis]),
                          "native ship origin recovery changed with aim or head");

                memcpy(world_translation, recovered, sizeof world_translation);
                XwaTurretMount_AnchorAtSeatPivot(
                    ventral, world_pivot, pivot, world_translation);

                /* Camera must sit at the ORIGINAL gunner hardpoint,
                 * regardless of lower-cockpit mesh rotation. */
                model_to_world(ventral, world_translation, pivot, new_pivot_world);
                for (int axis = 0; axis < 3; ++axis) {
                    check(close_to(new_pivot_world[axis], native_seat_world[axis]),
                          "cockpit eye has moved off the gunner pivot");
                    check(close_to(
                            camera[axis] - new_pivot_world[axis],
                            pan[axis] * 0.0625f + head_offset[axis]),
                          "TrackIR translation moved the gun/cockpit instead of the observer");
                }

                for (unsigned v = 0; v < sizeof vertices / sizeof vertices[0]; ++v) {
                    const float *vertex = vertices[v];
                    float modern[3], classic[3];
                    model_to_world(ventral, world_translation, vertex, modern);

                    /* Independent CLASSIC ORACLE:
                     * camera_eye + original_ship_axes * R * (vertex - eye_model).
                     * This is RenderScene_DrawObjectModel's pivot translate/
                     * rotate/translate sequence, not the modern helper. */
                    for (int axis = 0; axis < 3; ++axis) {
                        classic[axis] = native_seat_world[axis];
                        for (int row = 0; row < 3; ++row)
                            classic[axis] += basis[row * 3 + axis] *
                                classic_half_turn[row] * (vertex[row] - pivot[row]);
                        check(close_to(modern[axis], classic[axis]),
                              "modern turret geometry differs from classic pivot rotation");
                        comparisons++;
                    }
                }
            }
        }
    }
    printf("PASS: classic/modern Otana ventral transform agrees at %d world coordinates; upper/pilot untouched\n",
           comparisons);
    return 0;
}
