#ifndef XWA_REMASTER_TURRET_MOUNT_FRAME_H
#define XWA_REMASTER_TURRET_MOUNT_FRAME_H

#include "xwa/assets/object_type.h"

/* The classic renderer applies the same ventral (second gunner seat)
 * local 180-degree model rotation on both the YT-2000 (Otana) and
 * Millennium Falcon. The remaster MUST use the native pivot formulation
 * for these models, never the camera-relative legacy reconstruction.
 * Keep other craft on their existing paths until parity is established. */
static inline int XwaTurretMount_UsesClassicPivot(int seat, unsigned int object_type) {
    return seat == 2 &&
           (object_type == OBJ_FamilyTransport ||
            object_type == OBJ_MilleniumFalcon2);
}

/* Convert the snapshot's eye position back into the underlying craft origin.
 * hardpoint_world is the current aim-dependent native gunner eye hardpoint,
 * camera_pan and head_world belong exclusively to the observer. */
static inline void XwaTurretMount_CockpitOrigin(
    const float camera_local[3], const float head_world[3],
    const float hardpoint_world[3], const float camera_pan[3],
    float out[3]) {
    for (int i = 0; i < 3; ++i) {
        out[i] = camera_local[i] - head_world[i] - hardpoint_world[i] -
                 camera_pan[i] * 0.0625f;
    }
}

/* The ORIGINAL renderer's ventral-seat operation is a 180-degree turn
 * around the gunner eye pivot in MODEL SPACE, not a turn of the ship in
 * world space. fl_model_matrix transposes these basis rows to construct
 * the model-to-world matrix: negate LOCAL axes (ROWS 1 and 2), NEVER
 * the world-coordinate columns. R = diag(1,-1,-1), det(R)=+1.
 * Camera / TrackIR rotations must not enter this basis. */
static inline void XwaTurretMount_ApplyVentralFacing(float basis[9]) {
    for (int axis = 0; axis < 3; ++axis) {
        basis[1 * 3 + axis] = -basis[1 * 3 + axis];
        basis[2 * 3 + axis] = -basis[2 * 3 + axis];
    }
}

/* Reproduce the classic RenderScene_DrawObjectModel ventral pivot:
 *   T = ship_origin + hardpoint_world - flipped_model_to_world * hardpoint_local
 * The native renderer rotates the cockpit ABOUT mesh.pos (the gunner's
 * current hardpoint), translating the model as well as reorienting it.
 * Without this translation the entire cockpit is displaced by roughly
 * twice its ventral mounting offset; this was visible in modern mode
 * while F5/classic rendered the same seat correctly.
 *
 * The recovered ship origin is input/output; only the model placement is
 * corrected. This does not move the camera, guns, aiming or projectiles.
 */
static inline void XwaTurretMount_AnchorAtSeatPivot(
    const float flipped_basis[9], const float hardpoint_world[3],
    const float hardpoint_local[3], float ship_origin_inout[3]) {
    for (int axis = 0; axis < 3; ++axis) {
        const float rotated_pivot =
            flipped_basis[0 * 3 + axis] * hardpoint_local[0] +
            flipped_basis[1 * 3 + axis] * hardpoint_local[1] +
            flipped_basis[2 * 3 + axis] * hardpoint_local[2];
        ship_origin_inout[axis] += hardpoint_world[axis] - rotated_pivot;
    }
}

#endif
