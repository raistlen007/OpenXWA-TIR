#!/usr/bin/env python3
"""Verify native turret gun/projectile transforms remain unchanged and
that only the lower seat uses its original pre-TrackIR cockpit orientation.
No new lower-seat input or projectile reversal is permitted.
Static contracts supplement (not replace) the in-game gun/laser test.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
def source(path):
    return (root / path).read_text(encoding="utf-8")

camera = source("src/xwa/flight/flight_view.c")
flight = source("src/xwa/flight/flight.c")
laser = source("src/xwa/flight/object/laser.c")
hud = source("src/xwa/flight/hud/hud.c")
snapshot_hud = source("src/xwa_runtime/snapshot/snapshot_hud.c")
trackir = source("src/xwa_runtime/input/trackir.c")
renderer = source("src/xwa_remaster/flight.c")

def check(ok, reason):
    if not ok:
        raise AssertionError(reason)

start = camera.index("int FVIEW_BuildCameraOrient(")
end = camera.index("void FVIEW_BuildCameraOrientNoTurret", start)
camera_body = camera[start:end]
gun_basis = camera_body.index("g_players[playerIdx].turretCamMat[0] =")
head_look = camera_body.index("FVIEW_ShadowRotateAxes(camShadow, camShadow[0]", gun_basis)
check(gun_basis < head_look,
      "gunner camera basis must be captured before the observer's look rotation")
check("XwaTurretView_ReverseCameraFacing" not in camera_body and
      "XwaTurretView_NegateQ15" not in camera_body,
      "lower turret has an extra camera-facing inversion")
check("FVIEW_transformaxes(g_curMatR0_X, g_curMatR0_Y, g_curMatR0_Z, extraPitchQ16)" in camera_body,
      "normal head-look pitch path was removed")
check("FVIEW_transformaxes(cameraAxisX, cameraAxisY, cameraAxisZ, extraYawQ16)" in camera_body,
      "normal head-look yaw path was removed")

check("XwaTurretAim_PitchInputForSeat" not in flight,
      "lower turret pitch was inverted before native gun articulation")
check("craft->turretAim.aimAngleA[seatIdx] + angleDelta" in flight and
      "craft->turretAim.aimAngleB[seatIdx] + angleDelta" in flight,
      "gun aim angles were not preserved")
for index, axis in enumerate("XYZ"):
    check(f"mobj->move{axis} = g_players[playerIdx].turretCamMat[{index}]" in laser,
          f"laser {axis} does not use the gunner's original turret basis")
    check(f"bore[{index}] * farDistance / 32768" in hud,
          f"reticle {axis} does not use same gunner's turret basis as projectile")
check("const int16_t* bore = g_players[g_localPlayer].turretCamMat;" in hud,
      "reticle no longer uses native gunner firing direction")
check("XwaTrackIR_GetTurretAimDirection" not in hud,
      "reticle depends on tracking DLL state rather than gun orientation")
check("XwaTrackIR_GetTurretAimDirection" not in snapshot_hud and
      "player->turretCamMat[axis]" in snapshot_hud,
      "modern HUD snapshot must derive gun direction from native turretCamMat")
check("XwaTrackIR_SetTurretAimDirection" not in camera and
      "s_turret_aim_world" not in trackir,
      "TrackIR still owns extra turret weapon-aim state")
check("XwaTurretMount_CockpitOrigin(" in renderer,
      "ship-fixed cockpit mount was lost")
check("cockpit->seat == 2" in renderer[renderer.index("static int fl_cockpit_model_matrix"):renderer.index("static const float fl_default_uvs")] and
      "flipped[1] = -flipped[1];" in renderer,
      "pre-TrackIR ventral cockpit flip was not restored")
check("snap->cockpit.seat == 2" in renderer and "e[1] = -e[1];" in renderer,
      "pre-TrackIR ventral cockpit flip was not restored for hyperspace")
check(camera.count("g_players[playerIdx].currentSeatIdx != 2") >= 2,
      "TrackIR is still being injected into lower-seat camera views")
check("g_players[g_localPlayer].currentSeatIdx != 2 && XwaTrackIR_CurrentPose" in hud,
      "classic HUD still applies lower-seat TrackIR")
check("player->currentSeatIdx != 2 && XwaTrackIR_CurrentPose" in snapshot_hud,
      "modern HUD still applies lower-seat TrackIR")
check("seatIdx == 1" in flight,
      "lower turret no longer uses its original aim response")

print("turret contracts pass: original lower mount and inputs; native projectile unchanged; upper TrackIR retained")
