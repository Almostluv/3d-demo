cbuffer SceneBuffer : register(b0)
{
    row_major float4x4 gViewProj;
    float4 gParams;
    float4 gTint;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float wave = sin((input.uv.x * 18.0f) + (input.uv.y * 5.0f) + (gParams.x * 9.0f)) * 0.5f + 0.5f;
    float edge = pow(saturate(input.uv.y), 1.8f);
    float pulse = lerp(0.55f, 1.0f, wave);
    float3 rgb = saturate((input.color.rgb * gTint.rgb * pulse) + (gTint.rgb * edge * 0.08f));
    float alpha = input.color.a * gTint.a * saturate(0.60f + (wave * 0.40f));
    return float4(rgb, alpha);
}
