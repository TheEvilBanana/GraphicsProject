struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Textures and such
Texture2D AllPassTex		: register(t0);
Texture2D OgTex        : register(t1);
SamplerState Sampler	: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	

	float4 tex1 = AllPassTex.Sample(Sampler, input.uv);
	float4 tex2 = OgTex.Sample(Sampler, input.uv);
	float4 finalTex = tex1 + tex2;
	return finalTex;
}