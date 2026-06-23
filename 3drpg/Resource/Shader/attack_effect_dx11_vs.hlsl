cbuffer SceneBuffer : register(b0)
{
    row_major float4x4 gViewProj;
    float4 gParams;
    float4 gTint;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(float4(input.pos, 1.0f), gViewProj);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}
