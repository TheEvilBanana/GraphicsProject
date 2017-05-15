#include "Game.h"
#include "Vertex.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h" // For loading skyboxes (cube maps)
// For the DirectX Math library
using namespace DirectX;

#define max(a,b) (((a) > (b)) ? (a):(b))
#define min(a,b) (((a) < (b)) ? (a):(b))

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore( 
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{
	// Initialize fields
	vertexBuffer = 0;
	indexBuffer = 0;
	vertexShader = 0;
	pixelShader = 0;

	
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Release any (and all!) DirectX objects
	// we've made in the Game class
	// Delete our simple shader objects, which
	// will clean up their own internal DirectX stuff
	delete vertexShader;
	delete pixelShader;
	sphereSRV->Release();
	tileSRV->Release();
	normalTileSRV->Release();
	material2SRV->Release();
	normal2SRV->Release();
	material3SRV->Release();
	normal3SRV->Release();
	material4SRV->Release();
	normal4SRV->Release();
	material5SRV->Release();
	normal5SRV->Release();
	sampler1->Release();

	delete skyVertexShader;
	delete skyPixelShader;
	
	delete skyCubeMesh;
	delete skyCubeEntity;

	//Fade stuff clean up
	fadeBlendState->Release();
	fadeDepthState->Release();

	//Delete all the materials
	delete material1;
	delete material2;
	delete material3;
	delete material4;
	delete material5;

	for (auto& e : platformEntity) delete e;
	//for (auto& m : platformMesh) delete m;
	delete sphereEntity;
	delete sphereMesh;
	delete camera;
	delete platformMesh;

	//Post Process clean up
	delete ppVS;
	delete ppPS;
	delete brightPassPS;
	delete horzBlurPS;
	delete vertBlurPS;
	delete bloomPS;
	ppSRV->Release();
	ppSRV2->Release();
	bpSRV->Release();
	horBlurSRV->Release();
	verBlurSRV->Release();
	bpRTV->Release();
	horBlurRTV->Release();
	verBlurRTV->Release();
	ppRTV->Release();
	
	//Clean up sky stuff
	rasterStateSky->Release();
	depthStateSky->Release();
	skySRV1->Release();
	skySRV2->Release();
	
	// Clean up shadow map
	shadowDSV->Release();
	shadowSRV->Release();
	shadowRasterizer->Release();
	shadowSampler->Release();
	delete shadowVS;

	// Clean Blend Stuff
	blendState->Release();

	//Clean UI Stuff
	playButtonSprite->Release();
	quitButtonSprite->Release();
	scoreUISprite->Release();
	gameOverSprite->Release();

	//Clean up particle
	delete emitter;
	delete particleVS;
	delete particlePS;
	particleTexture->Release();
	particleBlendState->Release();
	particleDepthState->Release();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateMaterials();
	CreateParticles();
	CreateMatrices();
	CreateBasicGeometry();
	CreatePostProcessResources();
	CreateShadow();

	//UI stuff

	//Import Play Button Sprite
	spriteBatch.reset(new SpriteBatch(context));
	CreateWICTextureFromFile(device, L"Debug/TextureFiles/cyanplaypanel.png", 0, &playButtonSprite);
	CreateWICTextureFromFile(device, L"Debug/TextureFiles/cyanquitpanel.png", 0, &quitButtonSprite);
	CreateWICTextureFromFile(device, L"Debug/TextureFiles/scoreUIBg.png", 0, &scoreUISprite);
	CreateWICTextureFromFile(device, L"Debug/TextureFiles/gameOver.png", 0, &gameOverSprite);

	//Fade in stuff ************************

	D3D11_DEPTH_STENCIL_DESC fadeDesc = {};
	fadeDesc.DepthEnable = true;
	fadeDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Turns off depth writing
	fadeDesc.DepthFunc = D3D11_COMPARISON_LESS;
	device->CreateDepthStencilState(&fadeDesc, &fadeDepthState);

	// Blend for fade (alpha)
	D3D11_BLEND_DESC fadeIn = {};
	fadeIn.AlphaToCoverageEnable = false;
	fadeIn.IndependentBlendEnable = false;
	fadeIn.RenderTarget[0].BlendEnable = true;
	fadeIn.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	fadeIn.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	fadeIn.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	fadeIn.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	fadeIn.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	fadeIn.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	fadeIn.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&fadeIn, &fadeBlendState);
	//Finish fade stuff ************************
	
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files using
// my SimpleShader wrapper for DirectX shader manipulation.
// - SimpleShader provides helpful methods for sending
//   data to individual variables on the GPU
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = new SimpleVertexShader(device, context);
	if (!vertexShader->LoadShaderFile(L"Debug/VertexShader.cso"))
		vertexShader->LoadShaderFile(L"VertexShader.cso");		

	pixelShader = new SimplePixelShader(device, context);
	if(!pixelShader->LoadShaderFile(L"Debug/PixelShader.cso"))	
		pixelShader->LoadShaderFile(L"PixelShader.cso");

	shadowVS = new SimpleVertexShader(device, context);
	if (!shadowVS->LoadShaderFile(L"Debug/ShadowVS.cso"))
		shadowVS->LoadShaderFile(L"ShadowVS.cso");

	skyVertexShader = new SimpleVertexShader(device, context);
	if (!skyVertexShader->LoadShaderFile(L"Debug/SkyVertexShader.cso"))
		skyVertexShader->LoadShaderFile(L"SkyVertexShader.cso.cso");

	skyPixelShader = new SimplePixelShader(device, context);
	if (!skyPixelShader->LoadShaderFile(L"Debug/SkyPixelShader.cso"))
		skyPixelShader->LoadShaderFile(L"SkyPixelShader.cso");

	// Load particle shaders
	particleVS = new SimpleVertexShader(device, context);
	if (!particleVS->LoadShaderFile(L"Debug/ParticleVS.cso"))
		particleVS->LoadShaderFile(L"ParticleVS.cso");

	particlePS = new SimplePixelShader(device, context);
	if (!particlePS->LoadShaderFile(L"Debug/ParticlePS.cso"))
		particlePS->LoadShaderFile(L"ParticlePS.cso");

	//Postprocess stuff
	ppVS = new SimpleVertexShader(device, context);
	if (!ppVS->LoadShaderFile(L"Debug/PostProcessVS.cso"))
		ppVS->LoadShaderFile(L"PostProcessVS.cso");

	ppPS = new SimplePixelShader(device, context);
	if (!ppPS->LoadShaderFile(L"Debug/PostProcessPS.cso"))
		ppPS->LoadShaderFile(L"PostProcessPS.cso");

	brightPassPS = new SimplePixelShader(device, context);
	if (!brightPassPS->LoadShaderFile(L"Debug/BrightPassPS.cso"))
		brightPassPS->LoadShaderFile(L"BrightPassPS.cso");

	horzBlurPS = new SimplePixelShader(device, context);
	if (!horzBlurPS->LoadShaderFile(L"Debug/BlurHorizontalPS.cso"))
		horzBlurPS->LoadShaderFile(L"BlurHorizontalPS.cso");

	vertBlurPS = new SimplePixelShader(device, context);
	if (!vertBlurPS->LoadShaderFile(L"Debug/BlurVerticalPS.cso"))
		vertBlurPS->LoadShaderFile(L"BlurVerticalPS.cso");

	bloomPS = new SimplePixelShader(device, context);
	if (!bloomPS->LoadShaderFile(L"Debug/BloomFinalPS.cso"))
		bloomPS->LoadShaderFile(L"BloomFinalPS.cso");
}

void Game::CreateMaterials() {
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/Cobble.tif", 0, &sphereSRV);
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/Testing_basecolor.png", 0, &tileSRV);
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/Testing_normal.png", 0, &normalTileSRV);

	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/material2.tiff", 0, &material2SRV);
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/normal2.tiff", 0, &normal2SRV);

	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/material3.tiff", 0, &material3SRV);
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/normal3.tiff", 0, &normal3SRV);

	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/material4.tiff", 0, &material4SRV);
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/normal4.tiff", 0, &normal4SRV);

	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/material5.tiff", 0, &material5SRV);
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/normal5.tiff", 0, &normal5SRV);

	CreateDDSTextureFromFile(device, L"Debug/TextureFiles/Stormy.dds", 0, &skySRV1);
	CreateDDSTextureFromFile(device, L"Debug/TextureFiles/Sunset.dds", 0, &skySRV2);

	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/particle.jpg", 0, &particleTexture);

	D3D11_SAMPLER_DESC sampleDesc = {};
	sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&sampleDesc, &sampler1);

	material1 = new Material(pixelShader, vertexShader, tileSRV, normalTileSRV, sampler1);
	material2 = new Material(pixelShader, vertexShader, material2SRV, normal2SRV, sampler1);
	material3 = new Material(pixelShader, vertexShader, material3SRV, normal3SRV, sampler1);
	material4 = new Material(pixelShader, vertexShader, material4SRV, normal4SRV, sampler1);
	material5 = new Material(pixelShader, vertexShader, material5SRV, normal5SRV, sampler1);

	// Set up the rasterize state
	D3D11_RASTERIZER_DESC rasterStateDesc = {};
	rasterStateDesc.FillMode = D3D11_FILL_SOLID;
	rasterStateDesc.CullMode = D3D11_CULL_FRONT;
	rasterStateDesc.DepthClipEnable = true;
	device->CreateRasterizerState(&rasterStateDesc, &rasterStateSky);

	D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
	depthStateDesc.DepthEnable = true;
	depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	device->CreateDepthStencilState(&depthStateDesc, &depthStateSky);

	
}

void Game::CreateParticles()
{
	// Particle states ------------------------

	// A depth state for the particles
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Turns off depth writing
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	device->CreateDepthStencilState(&dsDesc, &particleDepthState);


	// Blend for particles (alpha)
	D3D11_BLEND_DESC blend = {};
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;
	blend.RenderTarget[0].BlendEnable = true;
	blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blend.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&blend, &particleBlendState);

	// Set up particles
	emitter = new Emitter(
		200,							// Max particles
		200,							// Particles per second
		1,								// Particle lifetime
		1.0f,							// Start size
		3.0f,							// End size
		XMFLOAT4(0, 1.0f, 0.1f, 0.5f),	// Start color
		XMFLOAT4(0, 1.0f, 0.1f, 0),		// End color
		XMFLOAT3(0, 1, 0),				// Start velocity
		XMFLOAT3(0, -2, 0),				// Start position
		XMFLOAT3(0, 0, 0),				// Start acceleration
		device,
		particleVS,
		particlePS,
		particleTexture);
}

// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
void Game::CreateMatrices()
{
	camera = new Camera(0, 0, -5);
	camera->UpdateProjectionMatrix((float) width / height);

	// Shadow view matrix (where the light is looking from)
	XMMATRIX shView = XMMatrixLookAtLH(
		XMVectorSet(0, 20, -20, 0), // Eye position
		XMVectorSet(0, 0, 0, 0),		// Look at pos
		XMVectorSet(0, 1, 0, 0));		// Up
	XMStoreFloat4x4(&shadowViewMatrix, XMMatrixTranspose(shView));

	// Shadow proj matrix
	XMMATRIX shProj = XMMatrixOrthographicLH(
		10.0f,		// Ortho width
		10.0f,		// Ortho height
		0.1f,		// Near plane
		100.0f);	// Far plane
	XMStoreFloat4x4(&shadowProjectionMatrix, XMMatrixTranspose(shProj));
}


// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	sphereMesh = new Mesh("Debug/Models/sphere.obj", device);
	platformMesh = new Mesh("Debug/Models/cube.obj", device);

	GameEntity* p1 = new GameEntity(platformMesh, material1);
	GameEntity* p2 = new GameEntity(platformMesh, material2);
	GameEntity* p3 = new GameEntity(platformMesh, material3);
	GameEntity* p4 = new GameEntity(platformMesh, material4);
	GameEntity* p5 = new GameEntity(platformMesh, material5);

	platformEntity.push_back(p1);
	platformEntity.push_back(p2);
	platformEntity.push_back(p3);
	platformEntity.push_back(p4);
	platformEntity.push_back(p5);

	platformEntity[0]->SetPosition(0, -2, 0);
	platformEntity[1]->SetPosition(0, -2, 2);
	platformEntity[2]->SetPosition(0, -2, 4);
	platformEntity[3]->SetPosition(0, -2, 6);
	platformEntity[4]->SetPosition(0, -2, 8);

	platformEntity[0]->SetScale(1, 0.3, 1);
	platformEntity[1]->SetScale(1, 0.3, 1);
	platformEntity[2]->SetScale(1, 0.3, 1);
	platformEntity[3]->SetScale(1, 0.3, 1);
	platformEntity[4]->SetScale(1, 0.3, 1);

	sphereEntity = new GameEntity(sphereMesh, material1);
	sphereEntity->SetPosition(0, -1.6f, 0);
	sphereEntity->SetScale(0.5f, 0.5f, 0.5f);
	//entities.push_back(sphere);

	skyCubeMesh = new Mesh("Debug/Models/cube.obj", device);
	skyCubeEntity = new GameEntity(skyCubeMesh, material1);
}

void Game::CreatePostProcessResources()
{
	// Create post process resources -----------------------------------------
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D* ppTexture;
	device->CreateTexture2D(&textureDesc, 0, &ppTexture);

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	device->CreateRenderTargetView(ppTexture, &rtvDesc, &ppRTV);

	//device->CreateRenderTargetView(ppTexture, &rtvDesc, &horBlurRTV);

	// Create the Shader Resource View
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	device->CreateShaderResourceView(ppTexture, &srvDesc, &ppSRV);
	device->CreateShaderResourceView(ppTexture, &srvDesc, &ppSRV2);

	// We don't need the texture reference itself no mo'
	ppTexture->Release();

	ID3D11Texture2D* bpTexture;
	device->CreateTexture2D(&textureDesc, 0, &bpTexture);
	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC bprtvDesc = {};
	bprtvDesc.Format = textureDesc.Format;
	bprtvDesc.Texture2D.MipSlice = 0;
	bprtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	device->CreateRenderTargetView(bpTexture, &bprtvDesc, &bpRTV);

	D3D11_SHADER_RESOURCE_VIEW_DESC bpsrvDesc = {};
	bpsrvDesc.Format = textureDesc.Format;
	bpsrvDesc.Texture2D.MipLevels = 1;
	bpsrvDesc.Texture2D.MostDetailedMip = 0;
	bpsrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	device->CreateShaderResourceView(bpTexture, &bpsrvDesc, &bpSRV);

	bpTexture->Release();

	ID3D11Texture2D* hbTexture;
	device->CreateTexture2D(&textureDesc, 0, &hbTexture);

	D3D11_RENDER_TARGET_VIEW_DESC hbrtvDesc = {};
	hbrtvDesc.Format = textureDesc.Format;
	hbrtvDesc.Texture2D.MipSlice = 0;
	hbrtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	device->CreateRenderTargetView(hbTexture, &hbrtvDesc, &horBlurRTV);

	D3D11_SHADER_RESOURCE_VIEW_DESC hbsrvDesc = {};
	hbsrvDesc.Format = textureDesc.Format;
	hbsrvDesc.Texture2D.MipLevels = 1;
	hbsrvDesc.Texture2D.MostDetailedMip = 0;
	hbsrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	device->CreateShaderResourceView(hbTexture, &hbsrvDesc, &horBlurSRV);

	hbTexture->Release();

	ID3D11Texture2D* vbTexture;
	device->CreateTexture2D(&textureDesc, 0, &vbTexture);

	D3D11_RENDER_TARGET_VIEW_DESC vbrtvDesc = {};
	vbrtvDesc.Format = textureDesc.Format;
	vbrtvDesc.Texture2D.MipSlice = 0;
	vbrtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	device->CreateRenderTargetView(vbTexture, &vbrtvDesc, &verBlurRTV);

	D3D11_SHADER_RESOURCE_VIEW_DESC vbsrvDesc = {};
	vbsrvDesc.Format = textureDesc.Format;
	vbsrvDesc.Texture2D.MipLevels = 1;
	vbsrvDesc.Texture2D.MostDetailedMip = 0;
	vbsrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	device->CreateShaderResourceView(vbTexture, &vbsrvDesc, &verBlurSRV);

	vbTexture->Release();
}

void Game::CreateShadow()
{
	// Create shadow requirements ------------------------------------------
	shadowMapSize = 2048;


	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapSize;
	shadowDesc.Height = shadowMapSize;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	ID3D11Texture2D* shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, &shadowTexture);

	// Create the depth/stencil
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(shadowTexture, &shadowDSDesc, &shadowDSV);

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTexture, &srvDesc, &shadowSRV);

	// Release the texture reference since we don't need it
	shadowTexture->Release();

	// Create the special "comparison" sampler state for shadows
	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; // Could be anisotropic
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f;
	shadowSampDesc.BorderColor[1] = 1.0f;
	shadowSampDesc.BorderColor[2] = 1.0f;
	shadowSampDesc.BorderColor[3] = 1.0f;
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	// Create a rasterizer state
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Multiplied by (smallest possible value > 0 in depth buffer)
	shadowRastDesc.DepthBiasClamp = 0.0f;
	shadowRastDesc.SlopeScaledDepthBias = 1.0f;
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	//Blending State
	//Set up the Blend Desc
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//Settign the Blend Desc to FALSE [will be be used in 'OMBlendState' in Draw functiom]
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	device->CreateBlendState(&blendDesc, &blendState);
}

void Game::RenderShadowMap()
{
	// Initial setup: No RTV (remember to clear shadow map)
	context->OMSetRenderTargets(0, 0, shadowDSV);
	context->ClearDepthStencilView(shadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
	context->RSSetState(shadowRasterizer);

	// Set up an appropriate shadow view port
	D3D11_VIEWPORT shadowVP = {};
	shadowVP.TopLeftX = 0;
	shadowVP.TopLeftY = 0;
	shadowVP.Width = (float)shadowMapSize;
	shadowVP.Height = (float)shadowMapSize;
	shadowVP.MinDepth = 0.0f;
	shadowVP.MaxDepth = 1.0f;
	context->RSSetViewports(1, &shadowVP);

	// Set up shaders for making the shadow map
	shadowVS->SetShader();
	shadowVS->SetMatrix4x4("view", shadowViewMatrix);
	shadowVS->SetMatrix4x4("projection", shadowProjectionMatrix);

	// Turn off pixel shader
	context->PSSetShader(0, 0, 0);

	// Actually draw the entities
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	//for (unsigned int i = 0; i < entities.size(); i++)
	{

		// Grab the data from the first entity's mesh
		GameEntity* ge = sphereEntity;
		ID3D11Buffer* vb = ge->GetMesh()->GetVertexBuffer();
		ID3D11Buffer* ib = ge->GetMesh()->GetIndexBuffer();

		// Set buffers in the input assembler
		context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

		shadowVS->SetMatrix4x4("world", *ge->GetWorldMatrix());
		shadowVS->CopyAllBufferData();

		// Finally do the actual drawing
		context->DrawIndexed(ge->GetMesh()->GetIndexCount(), 0, 0);
	}

	// Revert to original targets and states
	//context->OMSetRenderTargets(1, &this->backBufferRTV, this->depthStencilView);
	context->OMSetRenderTargets(1, &this->ppRTV, this->depthStencilView);
	shadowVP.Width = (float)this->width;
	shadowVP.Height = (float)this->height;
	context->RSSetViewports(1, &shadowVP);
	context->RSSetState(0);
}


// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Update the projection matrix assuming the
	// camera exists
	if (camera)
		camera->UpdateProjectionMatrix((float) width / height);
}


// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	if (mouseAtPlay)
	{

		bool currentTab = (GetAsyncKeyState('P') & 0x8000) != 0;
		if (currentTab && !prevTab)
			paused = !paused;
		prevTab = currentTab;


		if (paused)
		{
			timeScale = 0;
		}
		else
		{
			//speed = currentSpeed;
			if (score <= 5)
			{
				timeScale = deltaTime;

			}
			if (score > 5)
			{
				timeScale = deltaTime * 1.2;
			}
			if (score > 10)
			{
				timeScale = deltaTime * 1.4;
			}
			if (score > 20)
			{
				timeScale = deltaTime * 1.6;
			}
			if (score > 40)
			{
				timeScale = deltaTime * 1.8;
			}
			if (score > 80)
			{
				timeScale = deltaTime * 2.0;
			}
		}
		

		gameState = GamePlay;
		float sinTime = (sin(totalTime * 2) + 2.0f) / 10.0f;
		float xposition = rand() % 3;

		//Reset platforms
		if (platformEntity[0]->GetPosition().z < -2)
		{
			platformEntity[0]->SetPosition(xposition, -2, 8);
		}
		if (platformEntity[1]->GetPosition().z < -2)
		{
			platformEntity[1]->SetPosition(xposition, -2, 8);
		}
		if (platformEntity[2]->GetPosition().z < -2)
		{
			platformEntity[2]->SetPosition(xposition, -2, 8);
		}
		if (platformEntity[3]->GetPosition().z < -2)
		{
			platformEntity[3]->SetPosition(xposition, -2, 8);
		}
		if (platformEntity[4]->GetPosition().z < -2)
		{
			platformEntity[4]->SetPosition(xposition, -2, 8);
		}

		//Move platforms
		platformEntity[0]->Move(0, 0, -timeScale * 2.01);
		platformEntity[1]->Move(0, 0, -timeScale * 2.01);
		platformEntity[2]->Move(0, 0, -timeScale * 2.01);
		platformEntity[3]->Move(0, 0, -timeScale * 2.01);
		platformEntity[4]->Move(0, 0, -timeScale * 2.01);

		//Move Player
		if (GetAsyncKeyState('A') & 0x8000)
		{
			sphereEntity->Move(-timeScale*2, 0, 0);
		}
		if (GetAsyncKeyState('D') & 0x8000)
		{
			sphereEntity->Move(timeScale*2, 0, 0);
		}

		if (sphereEntity->GetPosition().y < -1.6f)
		{

			if ((sphereEntity->GetPosition().x - .35f) < (platformEntity[platformCount % 5]->GetPosition().x + 0.5f) && (sphereEntity->GetPosition().x + .35f) >(platformEntity[platformCount % 5]->GetPosition().x - 0.5f))
			{
				
				speed = constSpeed;
				score++;
				printf("%d", score);
				emitter->SetEmitterPosition(sphereEntity->GetPosition());
				emitter->SpawnParticle();
				platformCount++;
				
			}
			else
			{
				gameState = GameOver;
			}
		}

		speed = speed - (gravity * timeScale);

		sphereEntity->Move(0, speed*timeScale, 0);

		emitter->Update(deltaTime);
		// Update the camera
		camera->Update(deltaTime);

		platformEntity[0]->UpdateWorldMatrix();
		platformEntity[1]->UpdateWorldMatrix();
		platformEntity[2]->UpdateWorldMatrix();
		platformEntity[3]->UpdateWorldMatrix();
		platformEntity[4]->UpdateWorldMatrix();
		sphereEntity->UpdateWorldMatrix();

		// Changing alpha value for skybox lerp
		counterLerp++;                     // Using counter for tracking change
		if (counterLerp % 100 == 0) {
			if (lerpState == true) {         // Lerp state used for tracking whether increment or decrement alpha value
				skyLerpValue += 0.01f;
			}
			if (lerpState == false) {
				skyLerpValue -= 0.01f;
			}
		}

		if (counterLerp % 10000 == 0) {               // changeing lerstate logic
			lerpState = false;
			if (counterLerp % 20000 == 0) {
				lerpState = true;
				counterLerp = 0;
			}
		}
	}
	else
	{
		gameState = MainMenu;
	}

	if (mouseAtQuit)
	{
		gameState = Exit;
	}
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();
	
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	

	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = {1.0f, 1.0f, 0.0f, 0.0f};

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV, color);
	context->ClearDepthStencilView(
		depthStencilView, 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);
	
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	switch (gameState)
	{
	case MainMenu:
		spriteBatch->Begin();

		spriteBatch->Draw(playButtonSprite, playSpritePosition);
		spriteBatch->Draw(quitButtonSprite, quitSpritePosition);

		spriteBatch->End();
		break;
	
	case GamePlay:
	{
		// Post process initial setup =================
		// Start rendering somewhere else!
		context->ClearRenderTargetView(ppRTV, color);
		context->OMSetRenderTargets(1, &ppRTV, depthStencilView);

		//SkyBox
		vertexBuffer = skyCubeEntity->GetMesh()->GetVertexBuffer();
		indexBuffer = skyCubeEntity->GetMesh()->GetIndexBuffer();

		context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		skyVertexShader->SetMatrix4x4("view", camera->GetView());
		skyVertexShader->SetMatrix4x4("projection", camera->GetProjection());
		skyVertexShader->CopyAllBufferData();
		skyVertexShader->SetShader();

		skyPixelShader->SetShaderResourceView("Sky1", skySRV1);
		skyPixelShader->SetShaderResourceView("Sky2", skySRV2);
		skyPixelShader->SetData("lerpValue", &skyLerpValue, sizeof(skyLerpValue));
		skyPixelShader->CopyAllBufferData();
		skyPixelShader->SetShader();

		context->RSSetState(rasterStateSky);
		context->OMSetDepthStencilState(depthStateSky, 0);
		context->DrawIndexed(skyCubeEntity->GetMesh()->GetIndexCount(), 0, 0);

		// Reset the render states we've changed
		context->RSSetState(0);
		context->OMSetDepthStencilState(0, 0);

		/***************************************************************************/
		float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // Set blend factor[inconsequential, since not using]
		context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF); // Setting the blend state
		RenderShadowMap();
		renderer.SetVertexBuffer(sphereEntity, vertexBuffer);
		renderer.SetIndexBuffer(sphereEntity, indexBuffer);
		renderer.SetVertexShader(vertexShader, sphereEntity, camera, shadowViewMatrix, shadowProjectionMatrix);
		renderer.SetPixelShader(pixelShader, sphereEntity, camera, shadowSampler, shadowSRV);

		context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed(sphereEntity->GetMesh()->GetIndexCount(), 0, 0);

		/*********************************************************************************************/
		float dist;
		float maxDist;

		for (int i = 0; i <= 4; i++) {

			context->OMSetBlendState(fadeBlendState, 0, 0xffffffff);  // Alpha blending

			dist = 8.1f - platformEntity[i]->GetPosition().z;
			maxDist = dist * 0.125;

			if (dist <= 0) {
				pixelShader->SetFloat("alphaV", 0.0f);
			} else if (dist > 8.0f) {
				pixelShader->SetFloat("alphaV", 1.0f);
			} else {
				pixelShader->SetFloat("alphaV", maxDist);
			}

			renderer.SetVertexBuffer(platformEntity[i], vertexBuffer);
			renderer.SetIndexBuffer(platformEntity[i], indexBuffer);
			renderer.SetVertexShader(vertexShader, platformEntity[i], camera, shadowViewMatrix, shadowProjectionMatrix);
			renderer.SetPixelShader(pixelShader, platformEntity[i], camera, shadowSampler, shadowSRV);

			context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexed(platformEntity[i]->GetMesh()->GetIndexCount(), 0, 0);

			context->OMSetBlendState(0, 0, 0xffffffff);
		}

		pixelShader->SetShaderResourceView("ShadowMap", 0);
		/***************************************************/
		//Score UI
		spriteBatch->Begin();
		spriteBatch->Draw(scoreUISprite, XMFLOAT2(width / 2 - 600, height / 2 - 350));
		spriteBatch->End();


		/******************************************************/
		
		// Particle states
		float blend[4] = {1,1,1,1};
		context->OMSetBlendState(particleBlendState, blend, 0xffffffff);  // Additive blending
		context->OMSetDepthStencilState(particleDepthState, 0);			// No depth WRITING

																		// Draw the emitter
		emitter->Draw(context, camera);

		// Reset to default states for next frame
		context->OMSetBlendState(0, blend, 0xffffffff);
		context->OMSetDepthStencilState(0, 0);

		// Actually do post processing ========================
		//Bright Pass
		context->ClearRenderTargetView(bpRTV, color);
		context->OMSetRenderTargets(1, &bpRTV, 0);

		ppVS->SetShader();

		brightPassPS->SetShaderResourceView("BrightPassTex", ppSRV);
		brightPassPS->SetSamplerState("Sampler", sampler1);
		brightPassPS->CopyAllBufferData();
		brightPassPS->SetShader();
		ID3D11Buffer* nothing = 0;
		context->IASetVertexBuffers(0, 1, &nothing, &stride, &offset);
		context->IASetIndexBuffer(0, DXGI_FORMAT_R32_UINT, 0);
		context->Draw(3, 0);
		brightPassPS->SetShaderResourceView("BrightPassTex", 0);

		//Horizontal Blur Pass
		context->ClearRenderTargetView(horBlurRTV, color);
		context->OMSetRenderTargets(1, &horBlurRTV, 0);

		ppVS->SetShader();

		horzBlurPS->SetInt("blurAmount", 3);
		horzBlurPS->SetFloat("pixelWidth", 1.0f / (width / 2));
		horzBlurPS->SetShaderResourceView("HorzBlurTex", bpSRV);
		horzBlurPS->SetSamplerState("Sampler", sampler1);
		horzBlurPS->CopyAllBufferData();
		horzBlurPS->SetShader();
		ID3D11Buffer* nothing1 = 0;
		context->IASetVertexBuffers(0, 1, &nothing1, &stride, &offset);
		context->IASetIndexBuffer(0, DXGI_FORMAT_R32_UINT, 0);
		context->Draw(3, 0);
		horzBlurPS->SetShaderResourceView("HorzBlurTex", 0);

		//Vertical Blur Pass
		context->ClearRenderTargetView(verBlurRTV, color);
		context->OMSetRenderTargets(1, &verBlurRTV, 0);

		ppVS->SetShader();

		vertBlurPS->SetInt("blurAmount", 3);
		vertBlurPS->SetFloat("pixelHeight", 1.0f / (height / 2));
		vertBlurPS->SetShaderResourceView("VertBlurTex", horBlurSRV);
		vertBlurPS->SetSamplerState("Sampler", sampler1);
		vertBlurPS->CopyAllBufferData();
		vertBlurPS->SetShader();
		ID3D11Buffer* nothing2 = 0;
		context->IASetVertexBuffers(0, 1, &nothing2, &stride, &offset);
		context->IASetIndexBuffer(0, DXGI_FORMAT_R32_UINT, 0);
		context->Draw(3, 0);
		vertBlurPS->SetShaderResourceView("VertBlurTex", 0);

		// Get the shaders ready for post processing
		// Back to the back buffer
		context->OMSetRenderTargets(1, &backBufferRTV, 0);

		ppVS->SetShader();

		bloomPS->SetShaderResourceView("AllPassTex", verBlurSRV);
		bloomPS->SetShaderResourceView("OgTex", ppSRV2);
		bloomPS->SetSamplerState("Sampler", sampler1);
		bloomPS->CopyAllBufferData();
		bloomPS->SetShader();

		ID3D11Buffer* nothing3 = 0;
		context->IASetVertexBuffers(0, 1, &nothing3, &stride, &offset);
		context->IASetIndexBuffer(0, DXGI_FORMAT_R32_UINT, 0);

		// Actually draw exactly 3 vertices
		context->Draw(3, 0);

		bloomPS->SetShaderResourceView("AllPassTex", 0);
		bloomPS->SetShaderResourceView("OgTex", 0);
	}
		break;
	case GameOver:
		spriteBatch->Begin();
		spriteBatch->Draw(gameOverSprite, XMFLOAT2(width / 2, height / 2));
		spriteBatch->End();
		break;
	case Exit:
		Quit();
		break;
	
	default:
		break;
	}


	
/*************************************************************************/
	swapChain->Present(0, 0);
	
}


#pragma region Mouse Input

// --------------------------------------------------------
// Helper method for mouse clicking.  We get this information
// from the OS-level messages anyway, so these helpers have
// been created to provide basic mouse input if you want it.
// --------------------------------------------------------
void Game::OnMouseDown(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;

	//Check if the play button is clicked
	if (((x > playSpritePosition.x) && (x < playSpritePosition.x+300)) && ((y > playSpritePosition.y) && (y < playSpritePosition.y + 120)))
	{
		if (buttonState & 0x0001)
		{
			mouseAtPlay = true;
		}
	}

	//Check if the quit button is clicked
	if (((x > quitSpritePosition.x) && (x < quitSpritePosition.x + 250)) && ((y > quitSpritePosition.y) && (y < quitSpritePosition.y + 120)))
	{
		if (buttonState & 0x0001)
		{
			mouseAtQuit = true;
		}
	}

	// Caputure the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hWnd);
}

// --------------------------------------------------------
// Helper method for mouse release
// --------------------------------------------------------
void Game::OnMouseUp(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}

// --------------------------------------------------------
// Helper method for mouse movement.  We only get this message
// if the mouse is currently over the window, or if we're 
// currently capturing the mouse.
// --------------------------------------------------------
void Game::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// Check left mouse button
	if (buttonState & 0x0001) {
		float xDiff = (x - prevMousePos.x) * 0.005f;
		float yDiff = (y - prevMousePos.y) * 0.005f;
		//camera->Rotate(yDiff, xDiff);
	}

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;
}

// --------------------------------------------------------
// Helper method for mouse wheel scrolling.  
// WheelDelta may be positive or negative, depending 
// on the direction of the scroll
// --------------------------------------------------------
void Game::OnMouseWheel(float wheelDelta, int x, int y)
{
	// Add any custom code here...
}
#pragma endregion