//cbuffer Data : register(b0)
//{
//	float pixelWidth;
//	float pixelHeight;
//	int blurAmount;
//}


// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Textures and such
Texture2D BrightPassTex		: register(t0);
//Texture2D Pixel2        : register(t1);
SamplerState Sampler	: register(s0);

// Entry point for this pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
	// Total color before dividing
	//float4 totalColor = float4(0,0,0,0);
	//float4 color = float4(0, 0, 0, 0);
	//uint numSamples = 0;

	float2 uv = input.uv;
	//float4 totalColor = Pixels.Sample(Sampler, uv1);
	float4 color = BrightPassTex.Sample(Sampler, uv);
	float lum = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));  // Pixel luminance

	if (lum < 0.95f) {
		color = float4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	return color;
}