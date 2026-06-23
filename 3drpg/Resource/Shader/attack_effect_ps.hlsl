float4 gParams : register(c0);
float4 gTint : register(c1);

float4 main(float4 color : COLOR0, float2 uv : TEXCOORD0) : COLOR0
{
    float wave = sin((uv.x * 18.0f) + (uv.y * 5.0f) + (gParams.x * 9.0f)) * 0.5f + 0.5f;
    float edge = pow(saturate(uv.y), 1.8f);
    float pulse = lerp(0.55f, 1.00f, wave);
    float3 rgb = saturate((color.rgb * gTint.rgb * pulse) + (gTint.rgb * edge * 0.08f));
    float alpha = color.a * gTint.a * saturate(0.60f + (wave * 0.40f));

    return float4(rgb, alpha);
}
