/* Frontend/briefing font atlas smoothing; no flight font uses this shader.
 * Both the default baked PNG atlases and the runtime-decoded original atlases
 * arrive at the GPU in premultiplied-alpha form. A coverage filter MUST update
 * RGB with alpha, otherwise newly transparent edge texels create dark fringes.
 * Font metrics, colors, and the frontend geometry stay untouched.
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

    /* Two texels are half an original 4x-upscaled bitmap pixel. Keep
     * the softening localized rather than blurring whole menu surfaces. */
    const float n0 = coverage(pos, float2(-2.0f, 0.0f));
    const float n1 = coverage(pos, float2( 2.0f, 0.0f));
    const float n2 = coverage(pos, float2(0.0f, -2.0f));
    const float n3 = coverage(pos, float2(0.0f,  2.0f));
    const float neighbour = 0.25f * (n0 + n1 + n2 + n3);
    const float alpha = saturate(lerp(src.a, 0.55f * src.a + 0.45f * neighbour, strength));

    /* The glyph atlases are white ink encoded as PMA. Recover ink color
     * for partially covered texels; fully transparent texels are also
     * white ink, so newly reconstructed edge coverage remains white.
     * Multiply by the filtered coverage BEFORE writing PMA output. */
    const float3 ink = src.a > (1.0f / 255.0f)
        ? saturate(src.rgb / src.a) : float3(1.0f, 1.0f, 1.0f);
    smoothed_atlas[pos] = float4(ink * alpha, alpha);
}
