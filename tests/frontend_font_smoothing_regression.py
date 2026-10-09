#!/usr/bin/env python3
"""Source-level guard for frontend-only GPU font coverage filtering.

This checks the exact regression where the default baked font path silently
skipped the filter. GPU output quality still requires an in-game screenshot.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
assets = (root / "src/xwa_remaster/assets.c").read_text()
shader = (root / "shaders/frontend_font_smooth.comp.hlsl").read_text()
config = (root / "resources/remaster/config.yaml").read_text()
cmake = (root / "CMakeLists.txt").read_text()

assert re.search(r"^\s*prefer_original_2d:\s*false", config, re.MULTILINE), (
    "Test must cover the shipped default: baked/remastered fonts first"
)
frontend_loader = assets.split("static AssetLoadStatus assets_load_frontend_font(", 1)[1]
frontend_loader = frontend_loader.split("static int assets_prepare_frontend_fonts(", 1)[0]
assert "AeronFontAtlas_Load(&slot->atlas, cmd, basename)" in frontend_loader
assert "assets_load_original_frontend_font(a, cmd, slot, font_size)" in frontend_loader
success = frontend_loader.split("if (status == ASSET_LOAD_SUCCESS) {", 1)[1]
success = success.split("Aeron_LogInfo(", 1)[0]
assert "assets_smooth_frontend_font(a, cmd, slot);" in success, (
    "Both successful font sources must submit the smoothing pass"
)
assert "if (source == ASSET_SOURCE_ORIGINAL)" not in success, (
    "Do not gate smoothing on original fonts: shipped defaults are baked"
)
assert "frontend_font_smooth.comp.hlsl" in cmake
assert "src.rgb / src.a" in shader and "float4(ink * alpha, alpha)" in shader, (
    "Filtered alpha and RGB must remain premultiplied together"
)
flight_loader = assets.split("static AssetLoadStatus assets_load_flight_font(", 1)[1]
flight_loader = flight_loader.split("int XwaRemasterAssets_PrepareFlightFonts(", 1)[0]
assert "assets_smooth_frontend_font(" not in flight_loader, (
    "Do not alter cockpit or in-flight font atlases"
)
assert "GPU coverage smoothing submitted" in assets, (
    "Keep diagnostic logging for validation on actual hardware"
)
print("Frontend font smoothing: both menu font paths covered; flight fonts excluded; PMA intact.")
