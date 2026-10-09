#!/usr/bin/env python3
"""Verify native turret gun/projectile transforms remain unchanged and
that only Otana lower gunner uses the TrackIR-disabled ship-fixed cockpit.
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
mesh_renderer = source("src/xwa_remaster/ship.c")
model_types = source("src/xwa/assets/object_type.h")
native_mesh = source("src/xwa/render/render_scene_model.c")
native_cockpit = source("src/xwa/render/render_scene_core.c")

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
# Use the actual YT-2000 game object ID. These MUST be different ships.
check("OBJ_FamilyTransport = 65" in model_types and
      "OBJ_CorellianTransport2 = 58" in model_types,
      "Otana was confused with the YT-1300 again")
check("XwaTrackIR_ClearPose();" in camera[camera.index("void FlightView_UpdatePlayerCamera("):],
      "Otana TrackIR bypass might retain the previous seat's head pose")
# Native OPT guns rotate from the independent aim angles. Preserve
# this animation independently of TrackIR and the render camera.
check("g_curMeshType == MESH_RotaryBeamSystem" in native_mesh and
      "g_curRotAngle = mesh->rotAngle;" in native_mesh and
      "g_cockpitViewActive && g_players[g_localPlayer].currentSeatIdx" in native_cockpit,
      "original OPT turret animation reference missing")
check("XWA_SNAP_MESH_ROTARY_GUN_TURRET" in mesh_renderer and
      "XWA_SNAP_MESH_ROTARY_BEAM" in mesh_renderer and
      "c->aim_angle_a" in mesh_renderer and "c->aim_angle_b" in mesh_renderer and
      "ship_mat3x4_mul(local_rotation, inherited, own)" in mesh_renderer,
      "remastered physical turret animation has lost its gun/beam axes")
# Otana = OBJ_FamilyTransport, object type 65 in assets/object_type.h.
# The old view-space transform is present ONLY for other ships, guarded by
# object-type discrimination, while Otana's lower shell uses ship-only data.
check("OBJ_FamilyTransport" in renderer and
      "anchor->object_type != OBJ_FamilyTransport" in renderer and
      "anchor->object_type == OBJ_FamilyTransport" in renderer and
      "XwaTurretMount_ApplyVentralFacing(basis);" in renderer,
      "Otana-only stable lower cockpit path missing")
check("player_f->object_type != OBJ_FamilyTransport" in renderer and
      "player_f->object_type == OBJ_FamilyTransport" in renderer and
      "XwaTurretMount_ApplyVentralFacing(bw);" in renderer,
      "Otana-only stable hyperspace cockpit path missing")
check(renderer.count("XwaTurretMount_CockpitOrigin(") >= 2,
      "Otana cockpit mount not ship-fixed in flight and hyperspace")
check(renderer.count("flipped[1] = -flipped[1];") == 1 and
      renderer.count("e[1] = -e[1];") == 1,
      "unrelated ships' original lower gunner transforms were altered")
check(camera.count("OBJ_FamilyTransport") >= 2 and
      "currentSeatIdx == 2" in camera,
      "TrackIR is not specifically excluded from Otana lower-seat camera views")
check("g_objectTable[g_players[g_localPlayer].objectIndex].objectType == OBJ_FamilyTransport" in hud,
      "classic HUD does not distinguish Otana lower-seat TrackIR")
check("!otanaLowerSeat && XwaTrackIR_CurrentPose" in snapshot_hud,
      "modern HUD still applies Otana lower-seat TrackIR")
check("seatIdx == 1 &&" in flight and "OBJ_FamilyTransport" in flight,
      "Otana lower turret no longer uses its original aim response")

print("turret contracts pass: Otana lower only; guns/projectiles native; ship-fixed shell; TrackIR off on Otana lower")
