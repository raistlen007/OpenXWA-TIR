#ifndef XWA_REMASTER_TURRET_MOUNT_FRAME_H
#define XWA_REMASTER_TURRET_MOUNT_FRAME_H

/* Cockpit and turret housing are rigid parts of the ship. The original
 * X-Wing Alliance cockpit renderer uses the same craft model orientation
 * for both turret seats, and animates the guns separately via OPT rotary
 * nodes. The remaster's previous special case rotated the SECOND cockpit
 * 180 degrees in eye space. Because that eye frame follows gun aim (and
 * formerly TrackIR), the whole lower cockpit orbited and flipped whenever
 * the turret moved.
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

#endif
