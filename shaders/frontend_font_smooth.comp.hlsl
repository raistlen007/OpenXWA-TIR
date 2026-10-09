/* Menu/briefing glyph atlas AA — never used for in-flight HUD text.
 *
 * Original font bitmaps are scaled 4x with nearest-neighbour before upload.
 * That preserves their proportions but also produces stairstep edges.
 * A narrow alpha-only kernel softens those steps once at atlas load time;
 * existing textured-glyph rendering, metrics, colors and clips are intact.
 *
 * Use alpha only: filtering premultiplied RGB would create dark fringes.
 * Original frontend fonts encode white RGB with coverage in alpha.
 */
Texture2D<float4> glyph_atlas : register(t0, space0);
SamplerState glyph_sampler : register(s0, space0);
RWTexture2D<float4> smoothed_atlas : register(u0, space1);

cbuffer FontSmoothParams : register(b0, space2) {
    uint2 atlas_size;
    float strength;
    float _pad;
};

float coverage(uint2 pos, float2 offset) {
    const float2 uv = (float2(pos) + 0.5f + offset) / float2(atlas_size);
    return glyph_atlas.SampleLevel(glyph_sampler, uv, 0.0f).a;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    if (any(id.xy >= atlas_size)) return;
    const uint2 pos = id.xy;
    const float2 uv = (float2(pos) + 0.5f) / float2(atlas_size);
    const float4 src = glyph_atlas.SampleLevel(glyph_sampler, uv, 0.0f);
    /* Source atlas has a fourfold nearest-neighbour expansion. Two
     * atlas texels is half an original bitmap pixel. This low-radius
     * kernel smooths the diagonals without spreading across glyphs. */
    const float n0 = coverage(pos, float2(-2.0f, 0.0f));
    const float n1 = coverage(pos, float2( 2.0f, 0.0f));
    const float n2 = coverage(pos, float2(0.0f, -2.0f));
    const float n3 = coverage(pos, float2(0.0f,  2.0f));
    const float neighbour = 0.25f * (n0 + n1 + n2 + n3);
    const float alpha = saturate(lerp(src.a, 0.55f * src.a + 0.45f * neighbour, strength));
    smoothed_atlas[pos] = float4(src.rgb, alpha);
}
