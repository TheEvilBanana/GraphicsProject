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
	delete material1;
	delete skyVertexShader;
	delete skyPixelShader;
	//delete material2;
	delete skyCubeMesh;
	delete skyCubeEntity;

	for (auto& e : platformEntity) delete e;
	//for (auto& m : platformMesh) delete m;
	delete sphereEntity;
	delete sphereMesh;
	delete camera;
	delete platformMesh;

	sphereSRV->Release();
	tileSRV->Release();
	normalTileSRV->Release();
	sampler1->Release();

	rasterStateSky->Release();
	depthStateSky->Release();
	skySRV->Release();
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
	CreateMatrices();
	CreateBasicGeometry();

	//UI stuff

	//Import Play Button Sprite
	spriteBatch.reset(new SpriteBatch(context));
	CreateWICTextureFromFile(device, L"Debug/TextureFiles/cyanplaypanel.png", 0, &playButtonSprite);
	CreateWICTextureFromFile(device, L"Debug/TextureFiles/cyanquitpanel.png", 0, &quitButtonSprite);
	
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
}

void Game::CreateMaterials() {
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/Cobble.tif", 0, &sphereSRV);
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/Testing_basecolor.png", 0, &tileSRV);
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/Testing_normal.png", 0, &normalTileSRV);
	CreateDDSTextureFromFile(device, L"Debug/TextureFiles/SunnyCubeMap.dds", 0, &skySRV);

	D3D11_SAMPLER_DESC sampleDesc = {};
	sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&sampleDesc, &sampler1);

	material1 = new Material(pixelShader, vertexShader, tileSRV, normalTileSRV, sampler1);

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
	GameEntity* p2 = new GameEntity(platformMesh, material1);
	GameEntity* p3 = new GameEntity(platformMesh, material1);
	GameEntity* p4 = new GameEntity(platformMesh, material1);
	GameEntity* p5 = new GameEntity(platformMesh, material1);
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
	context->OMSetRenderTargets(1, &this->backBufferRTV, this->depthStencilView);
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
		platformEntity[0]->Move(0, 0, -deltaTime * 2);
		platformEntity[1]->Move(0, 0, -deltaTime * 2);
		platformEntity[2]->Move(0, 0, -deltaTime * 2);
		platformEntity[3]->Move(0, 0, -deltaTime * 2);
		platformEntity[4]->Move(0, 0, -deltaTime * 2);

		//Move Player
		if (GetAsyncKeyState('Z') & 0x8000)
		{
			sphereEntity->Move(-0.005, 0, 0);
		}
		if (GetAsyncKeyState('C') & 0x8000)
		{
			sphereEntity->Move(0.005, 0, 0);
		}

		if (sphereEntity->GetPosition().y < -1.6f)
		{

			if ((sphereEntity->GetPosition().x - .35f) < (platformEntity[platformCount % 5]->GetPosition().x + 0.5f) && (sphereEntity->GetPosition().x + .35f) >(platformEntity[platformCount % 5]->GetPosition().x - 0.5f))
			{
				speed = 10.0f;
				//printf("Collision!");
				platformCount++;
			}
		}

		speed = speed - (gravity * deltaTime);

		sphereEntity->Move(0, speed*deltaTime, 0);

		// Update the camera
		camera->Update(deltaTime);

		platformEntity[0]->UpdateWorldMatrix();
		platformEntity[1]->UpdateWorldMatrix();
		platformEntity[2]->UpdateWorldMatrix();
		platformEntity[3]->UpdateWorldMatrix();
		platformEntity[4]->UpdateWorldMatrix();
		sphereEntity->UpdateWorldMatrix();
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
	const float color[4] = {0.0f, 1.0f, 0.0f, 0.0f};

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

		for (int i = 0; i <= 4; i++) {
			renderer.SetVertexBuffer(platformEntity[i], vertexBuffer);
			renderer.SetIndexBuffer(platformEntity[i], indexBuffer);
			renderer.SetVertexShader(vertexShader, platformEntity[i], camera, shadowViewMatrix, shadowProjectionMatrix);
			renderer.SetPixelShader(pixelShader, platformEntity[i], camera, shadowSampler, shadowSRV);

			context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexed(platformEntity[i]->GetMesh()->GetIndexCount(), 0, 0);
		}

		pixelShader->SetShaderResourceView("ShadowMap", 0);
		/***************************************************/
		vertexBuffer = skyCubeEntity->GetMesh()->GetVertexBuffer();
		indexBuffer = skyCubeEntity->GetMesh()->GetIndexBuffer();

		context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		skyVertexShader->SetMatrix4x4("view", camera->GetView());
		skyVertexShader->SetMatrix4x4("projection", camera->GetProjection());
		skyVertexShader->CopyAllBufferData();
		skyVertexShader->SetShader();

		skyPixelShader->SetShaderResourceView("Sky", skySRV);
		skyPixelShader->CopyAllBufferData();
		skyPixelShader->SetShader();

		context->RSSetState(rasterStateSky);
		context->OMSetDepthStencilState(depthStateSky, 0);
		context->DrawIndexed(skyCubeEntity->GetMesh()->GetIndexCount(), 0, 0);

		// Reset the render states we've changed
		context->RSSetState(0);
		context->OMSetDepthStencilState(0, 0);
	}
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
	if (((x > playSpritePosition.x - 250) && (x < playSpritePosition.x + 250)) && ((y > playSpritePosition.y - 250) && (y < playSpritePosition.y + 250)))
	{
		if (buttonState & 0x0001)
		{
			mouseAtPlay = true;
		}
	}

	//Check if the quit button is clicked
	if (((x > quitSpritePosition.x - 250) && (x < quitSpritePosition.x + 250)) && ((y > quitSpritePosition.y - 250) && (y < quitSpritePosition.y + 250)))
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
		camera->Rotate(yDiff, xDiff);
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