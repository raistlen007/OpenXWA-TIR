#!/usr/bin/env python3
"""Guard that frontend ABP fonts use pre-upload coverage reconstruction.

Shaders had no observable effect in menu screenshots. This guards the CPU
path actually supplying pixels to the original frontend atlas loader.
"""
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
assets=(root/"src/xwa_remaster/assets.c").read_text()
cmake=(root/"CMakeLists.txt").read_text()
config=(root/"resources/remaster/config.yaml").read_text()
aa=(root/"src/xwa_remaster/frontend_font_aa.c").read_text()

assert re.search(r"^\s*prefer_original_2d:\s*false",config,re.M)
assert not list((root/"resources/remaster/fonts").glob("*.fnt")) if (root/"resources/remaster/fonts").exists() else True
assert "frontend_font_aa.c" in cmake
assert "frontend_font_smooth.comp.hlsl" not in cmake
assert "XwaFrontendFontAA_Upscale" in assets
assert "XWA_ORIGINAL_FONT_SCALE, 0, 1," in assets
assert "XWA_ORIGINAL_FONT_SCALE, 1, 0," in assets
assert "assets_smooth_frontend_font" not in assets
assert "CPU high-quality AA atlas prepared" in assets
assert "weighted_alpha" in aa and "pixel[3]" in aa
print("Frontend atlas is anti-aliased before GPU upload; flight fonts keep their original sampling.")
