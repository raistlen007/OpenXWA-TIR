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

## What to verify in-game

1. Enter the family room or concourse in the **modern** graphics mode (F5).
2. Check startup log for `Original frontend background dedithering: enabled`.
3. Check the resource log for the particular room, e.g.
   `2D file 'family/familyroom': source=original`.
4. Check `xwa.artwork` for the corresponding file/group atlas and the
   number of restored pixels. A value above zero means the renderer queued
   changed texture pixels for upload; zero means the conservative checkerboard
   detector found nothing to modify in that asset.
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
(>=512x320) frontend frames run through a conservative two-colour ordered
checkerboard detector. RGB is changed only when a repeated alternating
pattern is detected in an opaque 3x3 neighborhood. Original alpha is never
changed; the source art remains on disk, unmodified. At startup with the
toggle off, the new code has no effect on the atlas inputs.

No runtime shader, special presentation layer or additional texture lookup is
involved. Source decoding -> CPU filtering -> `Aeron_RuntimeAtlasBuild` ->
frontend texture lookup -> normal 2D draw is the tested route. The algorithm
does **not** yet attempt general error-diffusion reconstruction or color
debanding; if its pixel-change count is zero in the target artwork, a more
appropriate detector must be developed *before* claiming improvement.

To roll back, set `restore_original_artwork: false` and restart; or
switch back to `input-gameplay`, which does not contain this experiment.
