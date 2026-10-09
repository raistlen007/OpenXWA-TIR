#ifndef XWA_FLIGHT_TURRET_AIM_RESPONSE_H
#define XWA_FLIGHT_TURRET_AIM_RESPONSE_H

#include <stdint.h>

/* Modern gunner response. Both mouse and physical joysticks already feed
 * the same smoothed flight-input axes; scale the resulting turret rate
 * without changing the pilot's controls or the turret's mechanical stops.
 *
 * Gain 3.0: threefold higher steady aim rate than the first modern
 * normalization (which was still reported too slow compared with piloting).
 * Response 0.5: settle approximately twice as quickly as the previous
 * 0.25-pole damping. A full 8-tick compatibility update retains the same
 * result regardless of graphics FPS or high-rate flight stepping. */
#define XWA_TURRET_AIM_RATE_GAIN 3.0
#define XWA_TURRET_AIM_RESPONSE 0.5

static inline double XwaTurretAim_TimeScale(uint16_t elapsedTicks, int highRate) {
    return highRate ? (double)elapsedTicks / 8.0 : 1.0;
}

/* Build #130 corrected the ventral seat's viewing direction with a fixed
 * horizontal half-turn. Relative to that outward-facing view, its old
 * pitch sign is reversed. Apply the correction to GUN AIM input only:
 * not to the TrackIR head pose, yaw, pilot, or the upper gunner seat.
 * seatIdx here is zero-based (upper = 0, lower = 1). */
static inline double XwaTurretAim_PitchInputForSeat(int seatIdx, double pitchInput) {
    return seatIdx == 1 ? -pitchInput : pitchInput;
}

static inline double XwaTurretAim_UpdateAccumulator(
    double previous, double smoothedInput, double timeScale) {
    double blend = XWA_TURRET_AIM_RESPONSE * timeScale;
    if (blend > 1.0) blend = 1.0;
    if (blend < 0.0) blend = 0.0;
    return previous + (smoothedInput * XWA_TURRET_AIM_RATE_GAIN - previous) * blend;
}

#endif
