struct VSInput
{
    float2 pos : POSITION;
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
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}
