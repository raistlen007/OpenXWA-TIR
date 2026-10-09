#ifndef XWA_FLIGHT_OBJECT_MOTION_MATH_H
#define XWA_FLIGHT_OBJECT_MOTION_MATH_H

#include <stdint.h>

/* The original scalar rounds (elapsedTicks * speedUnitsPerSecond) down
 * every simulation update. At 236 updates/sec low speeds can lose all
 * forward movement; boarding approaches then crawl or stall.
 *
 * Instead use the absolute simulation tick's position in the 236-tick
 * second as a deterministic fractional accumulator. It needs no extra
 * per-object state, survives save/replay, and is unchanged by render FPS.
 * Constant speed integrates to exactly the former per-second distance,
 * regardless of 1-, 4-, or 8-tick update granularity. */
static inline uint16_t XwaObject_ScaledTickRate(uint32_t gameTimeTicks,
                                                uint16_t elapsedTicks,
                                                uint32_t ratePerSecond) {
    const uint32_t phase = gameTimeTicks % 236u;
    const uint64_t start = (uint64_t)phase * ratePerSecond;
    const uint64_t end = ((uint64_t)phase + elapsedTicks) * ratePerSecond;
    return (uint16_t)((end / 236u) - (start / 236u));
}

static inline uint16_t XwaObject_ScaledForwardMove(uint32_t gameTimeTicks,
                                                    uint16_t elapsedTicks,
                                                    uint16_t speed) {
    const uint32_t unitsPerSecond = (4660u * (uint32_t)speed + 128u) >> 8;
    return XwaObject_ScaledTickRate(gameTimeTicks, elapsedTicks, unitsPerSecond);
}

/* The fallback for a sub-tick push must preserve the residual's sign,
 * not the rounded (possibly zero) half-magnitude clamp's sign. */
static inline int XwaObject_MinimumSignedPush(int residual, int scaledStep) {
    return scaledStep ? scaledStep : (residual < 0 ? -1 : 1);
}

#endif
