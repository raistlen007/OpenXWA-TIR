#ifndef XWA_REMASTER_TURRET_MOUNT_FRAME_H
#define XWA_REMASTER_TURRET_MOUNT_FRAME_H

/* Cockpit and turret housing are rigid parts of the ship. Gun components
 * animate independently via OPT rotary nodes. The original ventral seating
 * used a 180-degree inversion in the VIEW basis; using that moving basis in
 * the remaster causes the entire housing to orbit whenever the guns turn.
 * Apply the same lower-facing orientation in the static SHIP basis instead.
 *
 * The camera is at ship origin + seat hardpoint + pan + head displacement.
 * Subtract those eye-only offsets directly in WORLD coordinates to recover
 * the invariant craft origin. NEVER rotate the cockpit through the eye
 * frame: no yaw/pitch/roll from either TrackIR or turret aim belongs here.
 */
static inline void XwaTurretMount_CockpitOrigin(
    const float camera_local[3], const float head_world[3],
    const float hardpoint_world[3], const float camera_pan[3],
    float out[3]) {
    for (int i = 0; i < 3; ++i) {
        out[i] = camera_local[i] - head_world[i] - hardpoint_world[i] -
                 camera_pan[i] * 0.0625f;
    }
}

/* Original ventral seat faces 180 degrees opposite the dorsal one.
 * Rotate the turret OPT in its fixed ship-local frame, NEVER the camera
 * frame (which follows turret yaw/pitch). X is the fixed local side axis.
 * This is a proper half-turn with determinant +1, not a reflection. */
static inline void XwaTurretMount_ApplyVentralFacing(float basis[9]) {
    for (int row = 0; row < 3; ++row) {
        basis[row * 3 + 1] = -basis[row * 3 + 1];
        basis[row * 3 + 2] = -basis[row * 3 + 2];
    }
}

#endif
