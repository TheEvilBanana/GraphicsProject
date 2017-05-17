
// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 position		: SV_POSITION;
	//float4 color		: COLOR;        // RGBA color
	float3 normal       : NORMAL;       // Normal co-ordinates
	float2 uv           : TEXCOORD;     // UV co-ordinates
	float3 tangent		: TANGENT;
	float3 worldPos		: POSITION;
	float4 posForShadow	: POSITION1;
};

Texture2D textureSRV : register(t0);
Texture2D normalMapSRV : register(t1);
Texture2D ShadowMap : register(t2);

SamplerState basicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

struct DirectionalLight {
	float4 ambientColor;
	float4 diffuseColor;
	float3 direction;

};

cbuffer ExternalData : register(b0) {
	DirectionalLight dirLight1;
	DirectionalLight dirLight2;
	DirectionalLight dirLight3;
	float4 pointLightColor;
	float3 pointLightPosition;
	float3 cameraPosition;
	float4 pointLightColor1;
	float3 pointLightPosition1;
	float3 cameraPosition1;
	float3 spotLightDirection;
	float spotPower;
	float alphaV;
};

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{

	input.normal = normalize(input.normal);
input.tangent = normalize(input.tangent);

//N dot L for point light
float3 dirToPointLight = normalize(pointLightPosition - input.worldPos);
float lightAmountPL = saturate(dot(input.normal, dirToPointLight));

//N dot L for point light
float3 dirToPointLight1 = normalize(pointLightPosition1 - input.worldPos);
float lightAmountPL1 = saturate(dot(input.normal, dirToPointLight1));


//Spot Light calculation
float angleFromCenter = max(dot(-dirToPointLight, spotLightDirection), 0.0f);
float spotAmount = pow(angleFromCenter, spotPower);


//Specular highlight for point light
float3 toCamera = normalize(cameraPosition - input.worldPos);
float3 refl = reflect(-dirToPointLight, input.normal);
float specular = pow(saturate(dot(refl, toCamera)), 8);

float3 normalFromMap = normalMapSRV.Sample(basicSampler, input.uv).xyz * 2 - 1;

// Transform from tangent to world space
float3 N = input.normal;
float3 T = normalize(input.tangent - N * dot(input.tangent, N));
float3 B = cross(T, N);

float3x3 TBN = float3x3(T, B, N);
input.normal = normalize(mul(normalFromMap, TBN));

float lightAmount1 = saturate(dot(input.normal, -normalize(dirLight1.direction)));
float lightAmount2 = saturate(dot(input.normal, -normalize(dirLight2.direction)));
float lightAmount3 = saturate(dot(input.normal, -normalize(dirLight3.direction)));

float4 surfaceColor = textureSRV.Sample(basicSampler, input.uv);

float3 light1 = ((dirLight1.diffuseColor.rgb * lightAmount1 * surfaceColor.rgb) + (dirLight1.ambientColor.rgb * surfaceColor.rgb)) + specular + spotAmount;
float3 light2 = ((dirLight2.diffuseColor * lightAmount2 * surfaceColor) + (dirLight2.ambientColor * surfaceColor));
float3 light3 = ((dirLight3.diffuseColor * lightAmount3 * surfaceColor) + (dirLight3.ambientColor * surfaceColor));

float3 totalLight = light1 + light2 + light3;

//Shadow Mapping
float2 shadowUV = input.posForShadow.xy / input.posForShadow.w * 0.5f + 0.5f;
shadowUV.y = 1.0f - shadowUV.y;

float depthFromLight = input.posForShadow.z / input.posForShadow.w;

float shadowAmount = ShadowMap.SampleCmpLevelZero(
	ShadowSampler,
	shadowUV,
	depthFromLight
);

surfaceColor.a = alphaV;
return float4(totalLight * shadowAmount, surfaceColor.a);
// Just return the input color
// - This color (like most values passing through the rasterizer) is 
//   interpolated for each pixel between the corresponding vertices 
//   of the triangle we're rendering
//return float4(1.0f, 0.0f, 0.0f, 1.0f);
}