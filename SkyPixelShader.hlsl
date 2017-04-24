// External texture-related data
TextureCube Sky1			: register(t0);
TextureCube Sky2			: register(t1);
SamplerState Sampler	: register(s0);

cbuffer externalData : register(b0)
{
	float lerpValue;
};

// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float3 uvworld			: TEXCOORD;
};

// Entry point for this pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
	float4 sky1Tex = Sky1.Sample(Sampler, input.uvworld);
	float4 sky2Tex = Sky2.Sample(Sampler, input.uvworld);
	return lerp(sky1Tex, sky2Tex, lerpValue);
}