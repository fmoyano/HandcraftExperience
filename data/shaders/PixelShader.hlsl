struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texcoord: TEXCOORD;
    float4 color : COLOR;
};

cbuffer PS_Extra_Render_Info
{
    int use_texture;
};

Texture2D tex;
SamplerState ss;

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 final_color = input.color;
    
    if (use_texture == 1)
    {
        final_color *= tex.Sample(ss, input.texcoord);
    }

    return final_color;
}