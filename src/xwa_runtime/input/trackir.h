#ifndef XWA_RUNTIME_TRACKIR_H
#define XWA_RUNTIME_TRACKIR_H

#include <stdint.h>

/* Optional, Windows-only head pose. No NaturalPoint SDK files are included. */
typedef struct XwaTrackIRPose {
    int16_t yaw_q16;   /* negative = look left */
    int16_t pitch_q16; /* positive = look up */
    int16_t roll_q16;
    float left_cm;
    float up_cm;
    float back_cm;
} XwaTrackIRPose;

/* Returns zero if unavailable, unfocused, paused, or data is stale. */
int XwaTrackIR_Poll(XwaTrackIRPose* out);
void XwaTrackIR_Shutdown(void);

#endif
