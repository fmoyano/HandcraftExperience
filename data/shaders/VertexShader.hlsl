struct VS_INPUT
{
    float4 position : POSITION;
    float2 texcoord: TEXCOORD;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4  position : SV_POSITION;
    float2 texcoord: TEXCOORD;
    float4  color : COLOR;
};

cbuffer Matrix_Buffer
{
    matrix world_matrix;
    matrix view_matrix;
    matrix projection_matrix;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(projection_matrix, mul(view_matrix, mul(world_matrix, input.position)));
    output.texcoord = input.texcoord;
    output.color = input.color;
	return output;
}