#pragma once

#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"

class Renderer {
public:
	Renderer(GameEntity* renderEntity, Camera* renderCamera);
	~Renderer();

	void SetLights();
	ID3D11Buffer* SetVertexBuffer();
	ID3D11Buffer* SetIndexBuffer();
	SimpleVertexShader* SetVertexShader();
	SimplePixelShader* SetPixelShader();

private:
	GameEntity* gameEntity;
	Camera* camera;
	ID3D11Buffer *vertexBufferRender;
	ID3D11Buffer *indexBufferRender;
	SimpleVertexShader* vertexShaderRender;
	SimplePixelShader* pixelShaderRender;
	DirectionalLight dirLight1;
	DirectionalLight dirLight2;
};

