#pragma once

#include "DXCore.h"
#include "SimpleShader.h"
#include "Renderer.h"
#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"
#include <vector>
#include <DirectXMath.h>
#include "SpriteBatch.h"

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

	// Overridden mouse input helper methods
	void OnMouseDown (WPARAM buttonState, int x, int y);
	void OnMouseUp	 (WPARAM buttonState, int x, int y);
	void OnMouseMove (WPARAM buttonState, int x, int y);
	void OnMouseWheel(float wheelDelta,   int x, int y);
private:

	// Initialization helper methods - feel free to customize, combine, etc.
	void LoadShaders();
	void CreateMaterials();
	void CreateMatrices();
	void CreateBasicGeometry();

	void RenderShadowMap();

	// Buffers to hold actual geometry data
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	

	// Wrappers for DirectX shaders to provide simplified functionality
	SimpleVertexShader* vertexShader;
	SimplePixelShader* pixelShader;

	ID3D11ShaderResourceView* sphereSRV;
	ID3D11ShaderResourceView* tileSRV;
	ID3D11ShaderResourceView* normalTileSRV;
	
	ID3D11SamplerState* sampler1;

	//Sky
	ID3D11ShaderResourceView* skySRV1;
	ID3D11ShaderResourceView* skySRV2;
	SimpleVertexShader* skyVertexShader;
	SimplePixelShader* skyPixelShader;
	ID3D11RasterizerState* rasterStateSky;
	ID3D11DepthStencilState* depthStateSky;

	Mesh* skyCubeMesh;
	GameEntity* skyCubeEntity;

	float skyLerpValue = 0.0f;
	int counterLerp = 0;
	bool lerpState = true;
	// The matrices to go from model space to screen space
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;

	// Shadow stuff
	int shadowMapSize;
	ID3D11DepthStencilView* shadowDSV;
	ID3D11ShaderResourceView* shadowSRV;
	ID3D11SamplerState* shadowSampler;
	ID3D11RasterizerState* shadowRasterizer;
	SimpleVertexShader* shadowVS;
	DirectX::XMFLOAT4X4 shadowViewMatrix;
	DirectX::XMFLOAT4X4 shadowProjectionMatrix;

	//Blend Stuff
	ID3D11BlendState* blendState;
	
	// Keeps track of the old mouse position.  Useful for 
	// determining how far the mouse moved in a single frame.
	POINT prevMousePos;
	POINT currentMousePos;
	POINT difference;

	//std::vector<Mesh*> platformMesh;   // Mesh vector for platforms
	Mesh* sphereMesh;                  // Mesh for ball
	Mesh* platformMesh;				   // Mesh for platform
	std::vector<GameEntity*> platformEntity;  // Entity vector for platforms
	GameEntity* sphereEntity;                 // Entity for ball
	
	Renderer renderer;

	Camera* camera;

	Material *material1;

	float gravity = 20.0f;
	float speed = 10.0f;
	int platformCount = 1;


	//UI stuff
	std::unique_ptr<SpriteBatch> spriteBatch;
	ID3D11ShaderResourceView* playButtonSprite;
	ID3D11ShaderResourceView* quitButtonSprite;
	XMFLOAT2 playSpritePosition = XMFLOAT2(width / 2, height / 2);
	XMFLOAT2 quitSpritePosition = XMFLOAT2(width / 2, height / 2 + 200);
	bool mouseAtPlay = false;
	bool mouseAtQuit = false;


	//Game State Management
	enum  GameStateManager
	{
		MainMenu,
		GamePlay,
		Exit
	};
	GameStateManager gameState;
};

