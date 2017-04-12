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
	SimpleVertexShader* SetVertexShader(DirectX::XMFLOAT4X4 shadowViewMatrix, DirectX::XMFLOAT4X4 shadowProjectionMatrix);
	SimplePixelShader* SetPixelShader(ID3D11SamplerState* shadowSampler, ID3D11ShaderResourceView* shadowSRV);

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

