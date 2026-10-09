#!/usr/bin/env python3
"""Verify Otana's classic gun/laser basis and upper-turret-equivalent TrackIR.
No new lower-seat weapon/input inversions. Geometry is tested by the
independent C regression; this checks cross-module integration contracts.
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
mount = source("src/xwa_remaster/turret_mount_frame.h")
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
      "OBJ_CorellianTransport2 = 58" in model_types and
      "XWA_SNAP_TYPE_FAMILY_TRANSPORT 65" in source("src/xwa_runtime/snapshot/snapshot.h"),
      "Otana was confused with the YT-1300 again")
check("XwaTrackIR_ClearPose();" in camera[camera.index("void FlightView_UpdatePlayerCamera("):],
      "head pose from previous frame might leak into next gunner frame")
# Native OPT guns rotate from the independent aim angles. Preserve
# this animation independently of TrackIR and the render camera.
check("g_curMeshType == MESH_RotaryBeamSystem" in native_mesh and
      "g_curRotAngle = mesh->rotAngle;" in native_mesh and
      "g_cockpitViewActive && g_players[g_localPlayer].currentSeatIdx" in native_cockpit,
      "original OPT turret animation reference missing")
check("Otana lower turret lacks required rotary metadata" in mesh_renderer and
      "anchor->object_type == XWA_SNAP_TYPE_FAMILY_TRANSPORT" in mesh_renderer,
      "missing Otana lower turret renderer-articulation diagnostic")
check("XWA_SNAP_MESH_ROTARY_GUN_TURRET" in mesh_renderer and
      "XWA_SNAP_MESH_ROTARY_BEAM" in mesh_renderer and
      "c->aim_angle_a" in mesh_renderer and "c->aim_angle_b" in mesh_renderer and
      "ship_mat3x4_mul(local_rotation, inherited, own)" in mesh_renderer,
      "remastered physical turret animation has lost its gun/beam axes")
# The classic seat-2 renderer uses the SAME 180-degree eye-pivot
# inversion for Otana and Millennium Falcon. The modern path must select
# it based on the native object type and NEVER on camera/TrackIR heading.
# Sabra (58) is deliberately not opted in by this focused change.
check("OBJ_FamilyTransport = 65" in model_types and
      "OBJ_MilleniumFalcon2 = 59" in model_types and
      "OBJ_CorellianTransport2 = 58" in model_types,
      "native craft IDs have changed")
check("object_type == OBJ_FamilyTransport ||" in mount and
      "object_type == OBJ_MilleniumFalcon2" in mount and
      "seat == 2" in mount,
      "classic ventral pivot must select Falcon and Otana lower seats only")
check("XwaTurretMount_UsesClassicPivot(cockpit->seat, anchor->object_type)" in renderer and
      "XwaTurretMount_UsesClassicPivot(snap->cockpit.seat, player_f->object_type)" in renderer and
      "XwaTurretMount_ApplyVentralFacing(basis);" in renderer and
      "XwaTurretMount_ApplyVentralFacing(bw);" in renderer,
      "Falcon lower-seat pivot missing from flight or hyperspace")
check("cockpit->seat == 2 && !XwaTurretMount_UsesClassicPivot(" in renderer and
      "(!player_f || !XwaTurretMount_UsesClassicPivot(" in renderer,
      "other craft must retain their existing classic-fallback rendering path")
check(renderer.count("XwaTurretMount_CockpitOrigin(") >= 2,
      "Otana cockpit mount origin recovery missing in flight/hyperspace")
check(renderer.count("XwaTurretMount_AnchorAtSeatPivot(") == 2 and
      "cockpit->hardpoint_local, position" in renderer and
      "snap->cockpit.hardpoint_local, pw" in renderer,
      "ventral model is no longer anchored about actual native gunner hardpoint")
check("basis[1 * 3 + axis] = -basis[1 * 3 + axis]" in mount and
      "basis[2 * 3 + axis] = -basis[2 * 3 + axis]" in mount and
      "basis[row * 3 + 1] = -basis[row * 3 + 1]" not in mount,
      "ventral model half-turn must negate LOCAL basis rows, not world columns")
check("ship_origin_inout[axis] += hardpoint_world[axis] - rotated_pivot" in mount,
      "ventral pivot compensation dropped; modern cockpit will displace")

check(renderer.count("flipped[1] = -flipped[1];") == 1 and
      renderer.count("e[1] = -e[1];") == 1,
      "unrelated ships' original lower gunner transforms were altered")
# Both gunner seats must share the SAME observer-only TrackIR path.
# The ventral model pivot is a renderer responsibility, not an input exception.
check("OBJ_FamilyTransport" not in camera and
      "OBJ_MilleniumFalcon2" not in camera and
      "g_players[playerIdx].currentSeatIdx > 0 &&" in camera and
      "tracking = XwaTrackIR_Poll(&trackir);" in camera,
      "Otana lower gunner is not using the upper-turret TrackIR camera path")
reticle_hud = hud[hud.index("int16_t effectiveLookYaw ="):
                  hud.index("const int16_t* bore = g_players[g_localPlayer].turretCamMat;") + 100]
check("OBJ_FamilyTransport" not in reticle_hud and
      "OBJ_MilleniumFalcon2" not in reticle_hud and
      "if (XwaTrackIR_CurrentPose(&head))" in reticle_hud and
      "if (g_players[g_localPlayer].currentSeatIdx > 0) {" in hud,
      "classic HUD still excludes Otana lower-seat look or weapon reticle")
check("otanaLowerSeat" not in snapshot_hud and
      "if (XwaTrackIR_CurrentPose(&head))" in snapshot_hud and
      "if (player->currentSeatIdx > 0) {" in snapshot_hud,
      "modern HUD still excludes Otana lower-seat TrackIR or gun vector")
check(camera_body.index("g_players[playerIdx].turretCamMat[0] =") <
      camera_body.index("FVIEW_transformaxes(g_curMatR0_X", gun_basis),
      "TrackIR must never alter the gunner's pre-head weapon basis")
# The gunner response must be shared by both turret seats on ALL crafts.
# Sabra = CorellianTransport2 (YT-1300, type 58); Falcon =
# MilleniumFalcon2 (59); Otana = FamilyTransport (YT-2000, 65).
# Inspect just the native turret input block, not unrelated per-craft
# flight configuration elsewhere in flight.c.
turret_begin = flight.index("if (seatIdx >= 0 && !swappedControls")
turret_end = flight.index("\nfinish:", turret_begin)
turret_input = flight[turret_begin:turret_end]
check("OBJ_CorellianTransport2 = 58" in model_types and
      "OBJ_MilleniumFalcon2 = 59" in model_types and
      "OBJ_FamilyTransport = 65" in model_types,
      "turret identity mapping changed")
check("OBJ_FamilyTransport" not in turret_input and
      "OBJ_CorellianTransport2" not in turret_input and
      "OBJ_MilleniumFalcon2" not in turret_input and
      "if (seatIdx == 1" not in turret_input,
      "turret sensitivity must not branch on Otana/Sabra/Falcon or upper/lower seat")
check(turret_input.count("XwaTurretAim_TimeScale(") == 1 and
      turret_input.count("XwaTurretAim_UpdateAccumulator(") == 2 and
      "g_players[playerIdx].smoothedInputPitch, integrationScale" in turret_input and
      "g_players[playerIdx].smoothedInputYaw, integrationScale" in turret_input,
      "both aim axes must use identical enhanced response for all turrets")
check("g_modelDefs[(uint16_t)modelIndex].turretAimLimitA[seatIdx]" in turret_input and
      "g_modelDefs[(uint16_t)modelIndex].turretAimLimitB[seatIdx]" in turret_input,
      "per-craft mechanical aim limits must remain unchanged")

print("turret contracts pass: Otana classic pivot; both turret seats share TrackIR; gun/laser aim remains native")
