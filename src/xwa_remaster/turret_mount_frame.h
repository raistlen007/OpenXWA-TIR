#ifndef XWA_REMASTER_TURRET_MOUNT_FRAME_H
#define XWA_REMASTER_TURRET_MOUNT_FRAME_H

#include <stdint.h>

/* The second (ventral/rear) gunner cockpit uses an inverted OPT orientation.
 * Its 180-degree flip must be fixed in the turret's untracked seat frame;
 * using the live camera matrix incorrectly rotates the *cockpit mesh*
 * whenever the observer moves their head with TrackIR.
 *
 * No model-specific offsets: select the pre-head seat mount frame only
 * where the cockpit-mesh inversion requires it. */
static inline const float* XwaTurretMount_FixedFrame(
    uint8_t seat, int base_valid, const float base_rows[9],
    const float live_camera_rows[9]) {
    return seat == 2 && base_valid && base_rows ? base_rows : live_camera_rows;
}

#endif
