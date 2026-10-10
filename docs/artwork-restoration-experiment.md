# Experimental artwork dedithering

Branch: `experiment/artwork-dedither`, based on `input-gameplay`. No original game files are edited.

## Enable/disable

Disabled by default, independent of the classic/modern renderer switch. In the user's existing `config.yaml` add:

```yaml
assets:
  prefer_original_2d: true
  restore_original_artwork: true
```

Restart the game to load the altered GPU atlases. For a fair before/after, leave
`prefer_original_2d: true` for **both** test runs and change only
`restore_original_artwork` to `false`/`true` between runs.
Removing the restoration key also disables it if the shipped default remains false.

`prefer_original_2d` ensures the original decoded background is used even if an
authored replacement KTX2 happens to exist. The filter deliberately does *not*
modify HD replacement artwork, text/fonts, small UI sprites or flight textures.

## Verification on the actual original game artwork (2026-10-10)

The original `FRONTRES/FAMILY/FAMILYROOM.CBM` supplied separately by the
tester (not checked into GitHub) was decoded using the same indexed/RLE
structure as `Xwa2d_DecodeCbm`: one 640x480 RGBA frame, 252 distinct decoded
RGBA colours, with transparent pixels retained.

The revised C filter was run against that complete image with
AddressSanitizer/UndefinedBehaviorSanitizer enabled. It changed
262,939 / 307,200 pixels (85.59%), while preserving every alpha byte.
Average absolute RGB difference was 2.14/255 (maximum 14/255 per channel).
Strong edges (>=60/255 original neighbor contrast) retained ~99.6% of
their aggregate contrast strength. The large wall panels visibly
lost palette-era colour stippling. Fine surface texture on the robot
was also smoothed, and the final aesthetic decision still needs the
game user's side-by-side comparison.

The asset is a **CBM, not a BMP**. Original game frontend registrations
name `familyroom.bmp` but the remaster's reader tries the corresponding
`.cbm` first. Do not assume BMP data is used.

This confirms that the revised algorithm genuinely changes the real
source pixels, unlike the first algorithm. It does not prove the
Windows GPU output until the resulting build has been run in the game.

## What to verify in-game

1. Enter the family room or concourse in the **modern** graphics mode (F5).
2. Check startup log for `Original frontend background dedithering: enabled`.
3. Check the resource log for the particular room, e.g.
   `2D file 'family/familyroom': source=original`.
4. Check `xwa.artwork` for the corresponding file/group atlas and the
   number of restored pixels. A value above zero means the renderer queued
   changed texture pixels for upload; zero means the edge-aware reconstruction found nothing to modify
   in that asset (or no eligible opaque pixels were present).
5. Confirm `frontend assets committed` appears after GPU submission. Capture the
   same room with the filter disabled, then enabled. Look at *identical*
   regions at 100% zoom and compare dithering, edges, details and brightness.
6. Confirm classic F5 rendering is unchanged, as it uses the unmodified
   original framebuffer.

Static path assertions and the standalone RGBA regression run in the build
pipeline, but actual display and visual quality still **require human in-game
comparison**, since the original copyrighted assets are not in this GitHub
repository. Do not claim visual success from the unit test alone.

## Implementation and rollback

At each room-asset load, after decoding the original CBM/BMP/DAT pixels and
before constructing the existing RGBA runtime atlas, sufficiently large
(>=512x320) frontend frames run through a 5x5 local, color-distance weighted
reconstruction. Both 2x2 and multilevel ordered dithering are handled. A
color-similarity threshold excludes dissimilar neighbouring surfaces; an
edge check protects strong lines and object boundaries. Fine texture within
the similarity threshold can still be softened, so visual inspection of
real assets is mandatory. Original alpha is never
changed; the source art remains on disk, unmodified. At startup with the
toggle off, the new code has no effect on the atlas inputs.

No runtime shader, special presentation layer or additional texture lookup is
involved. Source decoding -> CPU filtering -> `Aeron_RuntimeAtlasBuild` ->
frontend texture lookup -> normal 2D draw is the tested route. The algorithm
does **not** yet perform palette reconstruction or dedicated color debanding.
In the first iteration, an exact 3x3 checkerboard detector compiled and passed
synthetic unit tests, yet the user's real screenshots were visually identical.
We must test against real game artwork and require visible A/B improvement
before declaring success.

To roll back, set `restore_original_artwork: false` and restart; or
switch back to `input-gameplay`, which does not contain this experiment.
