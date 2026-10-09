#ifndef XWA_FLIGHT_TURRET_VIEW_ORIENTATION_H
#define XWA_FLIGHT_TURRET_VIEW_ORIENTATION_H

#include <stdint.h>

/* The Otana's second/ventral turret's view looks into the gunner chair
 * when built directly from the legacy gun-mount camera. Face the OBSERVER
 * outwards with a fixed 180-degree local YAW (around seat up).
 *
 * Apply this to camera orientation only, AFTER gun mounting/aim and BEFORE
 * TrackIR head-look. Never transform the turret/cockpit mesh, seat pivot,
 * hardpoint, weapon aim, or camera position with it. */
static inline int XwaTurretView_ReverseCameraFacing(int seat, int mapCameraState) {
    return seat == 2 && mapCameraState == 0;
}

static inline int16_t XwaTurretView_NegateQ15(int16_t n) {
    return n == INT16_MIN ? INT16_MAX : (int16_t)-n;
}

/* Rows 0 and 2 are the camera right/forward axes; row 1 is UP.
 * A horizontal half-turn reverses right and forward, but never inverts up. */
static inline void XwaTurretView_ReverseRowsDouble(double rows[9]) {
    for (int col = 0; col < 3; ++col) {
        rows[col] = -rows[col];
        rows[6 + col] = -rows[6 + col];
    }
}

#endif
