/* Regression for Otana lower turret TrackIR cockpit mounting:
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

/* Same seat-2 basis flip as fl_cockpit_model_matrix: construct B*C^T,
 * invert its Y/Z columns, and map back to world basis with C. */
static void flip_basis(const float b[9], const float c[9], float out[9]) {
    float eye[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            eye[i*3+j] = b[i*3+0]*c[j*3+0] +
                         b[i*3+1]*c[j*3+1] + b[i*3+2]*c[j*3+2];
    for (int i = 0; i < 3; ++i) {
        eye[i*3+1] = -eye[i*3+1];
        eye[i*3+2] = -eye[i*3+2];
    }
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i*3+j] = eye[i*3+0]*c[0*3+j] +
                         eye[i*3+1]*c[1*3+j] + eye[i*3+2]*c[2*3+j];
}

int main(void) {
    const float base[9] = {1,0,0, 0,1,0, 0,0,1};
    const float head_yaw[9] = {0,0,1, 0,1,0, -1,0,0};
    const float head_roll[9] = {0,1,0, -1,0,0, 0,0,1};
    const float craft_basis[9] = {1,0,0, 0,1,0, 0,0,1};
    float model_base[9], model_yaw[9], model_roll[9], wrong[9];
    const float* mount = XwaTurretMount_FixedFrame(2, 1, base, base);
    flip_basis(craft_basis, mount, model_base);
    mount = XwaTurretMount_FixedFrame(2, 1, base, head_yaw);
    check(mount == base, "ventral turret uses head yaw instead of fixed mount");
    flip_basis(craft_basis, mount, model_yaw);
    mount = XwaTurretMount_FixedFrame(2, 1, base, head_roll);
    check(mount == base, "ventral turret uses head roll instead of fixed mount");
    flip_basis(craft_basis, mount, model_roll);
    for (int i = 0; i < 9; ++i) {
        check(fabsf(model_base[i]-model_yaw[i]) < 1.e-5f, "cockpit basis follows head yaw");
        check(fabsf(model_base[i]-model_roll[i]) < 1.e-5f, "cockpit basis follows head roll");
    }
    /* This catches the original regression; with a live head frame, the
     * lower cockpit actually rotates about a different axis. */
    flip_basis(craft_basis, head_yaw, wrong);
    check(fabsf(model_base[0]-wrong[0]) > 0.5f, "test missing head-following old behavior");
    check(XwaTurretMount_FixedFrame(1, 1, base, head_yaw) == head_yaw,
          "upper turret unnecessarily changes its original placement");
    check(XwaTurretMount_FixedFrame(0, 1, base, head_yaw) == head_yaw,
          "pilot cockpit unnecessarily changes its original placement");
    check(XwaTurretMount_FixedFrame(2, 0, base, head_yaw) == head_yaw,
          "untracked lower turret should retain legacy camera frame");
    puts("turret mount regression passed (ventral flip is independent of TrackIR head frame)");
    return 0;
}
