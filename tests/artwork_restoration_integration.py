"""Source-level guard against an artwork filter detached from the displayed 2D path.

This is *not* a substitute for the in-game before/after screenshot check.
It ties room registration, snapshot draw, original decode, atlas upload and
sprite submission to the same frontend rendering route.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(path):
    return (root / path).read_text(encoding="utf-8")

family = read("src/xwa/frontend/family_transport_room.c")
assets = read("src/xwa_remaster/assets.c")
renderer = read("src/xwa_remaster/frontend.c")
config = read("src/xwa_app/host_config.c")
app = read("src/xwa_app/main.c")
defaults = read("resources/remaster/config.yaml")

assert 'familyroom.bmp' in family and '"background"' in family
assert 'FrontImage_DrawSpriteOpaque("background"' in family
assert 'XwaRemasterOriginal2d_LoadFrontend(' in assets
assert 'assets_restore_large_artwork(&frames)' in assets
assert 'assets_build_runtime_atlas(&slot->original_atlas, cmd' in assets
assert 'Aeron_RuntimeAtlasBuild(atlas, cmd, frames' in assets
assert 'assets_runtime_frame(&slot->original_atlas, frame, out)' in assets
assert 'XwaRemasterAssets_FrontendSprite(g.assets' in renderer
assert 'AeronDrawList_AddSprite(list, &s)' in renderer
assert 'XwaRemasterAssets_Create(remaster_root, options->prefer_original_2d,' in read(
    "src/xwa_remaster/xwa_remaster.c")
assert '"assets.restore_original_artwork"' in config
assert 'host_config.restore_original_artwork' in app
assert 'restore_original_artwork: false' in defaults
print("Artwork restore frontend-path integration assertions passed")
