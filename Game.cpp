#include "Game.h"
#include "Vertex.h"
#include "WICTextureLoader.h"
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
	//delete material2;

	for (auto& e : platformEntity) delete e;
	//for (auto& m : platformMesh) delete m;
	delete sphereEntity;
	delete sphereMesh;
	delete camera;
	delete platformMesh;

	sphereSRV->Release();
	sampler1->Release();
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
	CreateMatrices();
	CreateBasicGeometry();
	
	dirLight1.SetLightValues(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 0));
	dirLight2.SetLightValues(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 0));

	//CreateWICTextureFromFile(device, context, L"Debug/Flames.jpg", 0, &flamesSRV);

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
	
	CreateWICTextureFromFile(device, context, L"Debug/TextureFiles/Cobble.tif", 0, &sphereSRV);

	D3D11_SAMPLER_DESC sampleDesc = {};
	sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&sampleDesc, &sampler1);

	material1 = new Material(pixelShader, vertexShader, sphereSRV, sampler1);
	
}



// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
void Game::CreateMatrices()
{
	camera = new Camera(0, 0, -5);
	camera->UpdateProjectionMatrix((float) width / height);

}


// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	sphereMesh = new Mesh("Debug/Models/sphere.obj", device);
	//meshes.push_back(sphereMesh);

	platformMesh = new Mesh("Debug/Models/cube.obj", device);

	GameEntity* p1 = new GameEntity(platformMesh, material1);
	GameEntity* p2 = new GameEntity(platformMesh, material1);
	platformEntity.push_back(p1);
	platformEntity.push_back(p2);

	platformEntity[0]->SetPosition(0, -2, 0);
	platformEntity[1]->SetPosition(0, -2, 3);

	sphereEntity = new GameEntity(sphereMesh, material1);
	//entities.push_back(sphere);

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
	float sinTime = (sin(totalTime * 2) + 2.0f) / 10.0f;
	float xposition = rand() % 3;
	
	//Reset platforms
	if (platformEntity[0]->GetPosition().z < -6)
	{
		platformEntity[0]->SetPosition(xposition, -2, 0);
	}
	if (platformEntity[1]->GetPosition().z < -6)
	{
		platformEntity[1]->SetPosition(xposition, -2, 0);
	}

	//Move platforms
	platformEntity[0]->Move(0, 0, -deltaTime);
	platformEntity[1]->Move(0, 0, -deltaTime);

	//Move Player
	if (GetAsyncKeyState('Z') & 0x8000)
	{
		sphereEntity->Move(-0.005, 0, 0);
	}
	if (GetAsyncKeyState('C') & 0x8000)
	{
		sphereEntity->Move(0.005, 0, 0);
	}

	// Update the camera
	camera->Update(deltaTime);

	platformEntity[0]->UpdateWorldMatrix();
	platformEntity[1]->UpdateWorldMatrix();
	sphereEntity->UpdateWorldMatrix();

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
	const float color[4] = {0.4f, 0.6f, 0.75f, 0.0f};

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV, color);
	context->ClearDepthStencilView(
		depthStencilView, 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);
	
	vertexBuffer = sphereEntity->GetMesh()->GetVertexBuffer();
	indexBuffer = sphereEntity->GetMesh()->GetIndexBuffer();
	
	ID3D11Buffer* vertexBufferPlatform1 = platformEntity[0]->GetMesh()->GetVertexBuffer();
	ID3D11Buffer* indexBufferPlatform1 = platformEntity[0]->GetMesh()->GetIndexBuffer();
	ID3D11Buffer* vertexBufferPlatform2 = platformEntity[1]->GetMesh()->GetVertexBuffer();
	ID3D11Buffer* indexBufferPlatform2 = platformEntity[1]->GetMesh()->GetIndexBuffer();

	// Set buffers in the input assembler
	//  - Do this ONCE PER OBJECT you're drawing, since each object might
	//    have different geometry.
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	
	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	vertexShader->SetMatrix4x4("world", *sphereEntity->GetWorldMatrix());
	vertexShader->SetMatrix4x4("view", camera->GetView());
	vertexShader->SetMatrix4x4("projection", camera->GetProjection());

	vertexShader->CopyAllBufferData();
	vertexShader->SetShader();
	
	pixelShader->SetData("light1", &dirLight1, sizeof(DirectionalLight));
	pixelShader->SetData("light2", &dirLight2, sizeof(DirectionalLight));

	pixelShader->SetShaderResourceView("textureSRV", sphereSRV);
	pixelShader->SetSamplerState("basicSampler", sampler1);

	pixelShader->CopyAllBufferData();
	pixelShader->SetShader();

	// Finally do the actual drawing
	context->DrawIndexed(sphereEntity->GetMesh()->GetIndexCount(), 0, 0);

	/**********************************************************************************************/
	
	context->IASetVertexBuffers(0, 1, &vertexBufferPlatform1, &stride, &offset);
	context->IASetIndexBuffer(indexBufferPlatform1, DXGI_FORMAT_R32_UINT, 0);

	vertexShader->SetMatrix4x4("world", *platformEntity[0]->GetWorldMatrix());

	vertexShader->CopyAllBufferData();
	vertexShader->SetShader();

	pixelShader->SetShaderResourceView("textureSRV", sphereSRV);
	pixelShader->SetSamplerState("basicSampler", sampler1);

	pixelShader->CopyAllBufferData();
	pixelShader->SetShader();

	context->DrawIndexed(platformEntity[0]->GetMesh()->GetIndexCount(), 0, 0);

	context->IASetVertexBuffers(0, 1, &vertexBufferPlatform2, &stride, &offset);
	context->IASetIndexBuffer(indexBufferPlatform2, DXGI_FORMAT_R32_UINT, 0);

	vertexShader->SetMatrix4x4("world", *platformEntity[1]->GetWorldMatrix());

	vertexShader->CopyAllBufferData();
	vertexShader->SetShader();

	pixelShader->SetShaderResourceView("textureSRV", sphereSRV);
	pixelShader->SetSamplerState("basicSampler", sampler1);

	pixelShader->CopyAllBufferData();
	pixelShader->SetShader();

	context->DrawIndexed(platformEntity[1]->GetMesh()->GetIndexCount(), 0, 0);

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