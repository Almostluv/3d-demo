cbuffer HealthBarBuffer : register(b0)
{
    float4 gTopColor;
    float4 gBottomColor;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 baseColor = lerp(gBottomColor.rgb, gTopColor.rgb, saturate(1.0f - input.uv.y));
    float highlight = smoothstep(0.18f, 0.0f, abs(input.uv.y - 0.28f)) * 0.18f;
    float edgeShade = smoothstep(0.78f, 1.0f, input.uv.y) * 0.25f;
    float3 color = saturate(baseColor + highlight - edgeShade);
    return float4(color * input.color.rgb, input.color.a);
}
