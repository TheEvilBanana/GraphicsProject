#pragma once

#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"

class Renderer {
public:
	Renderer();
	~Renderer();

	void SetLights();
	/*ID3D11Buffer* SetVertexBuffer();
	ID3D11Buffer* SetIndexBuffer();
	SimpleVertexShader* SetVertexShader(DirectX::XMFLOAT4X4 shadowViewMatrix, DirectX::XMFLOAT4X4 shadowProjectionMatrix);
	SimplePixelShader* SetPixelShader(ID3D11SamplerState* shadowSampler, ID3D11ShaderResourceView* shadowSRV);*/

	void SetVertexBuffer(GameEntity* &gameEntity, ID3D11Buffer* &vertexBuffer);
	void SetIndexBuffer(GameEntity* &gameEntity, ID3D11Buffer* &indexBuffer);
	void SetVertexShader(SimpleVertexShader* &vertexShader, GameEntity* &gameEntity, Camera* &camera, XMFLOAT4X4 &shadowViewMatrix, XMFLOAT4X4 &shadowProjectionMatrix);
	void SetPixelShader(SimplePixelShader* &pixelShader, GameEntity* &gameEntity, Camera* &camera, ID3D11SamplerState* &shadowSampler, ID3D11ShaderResourceView* &shadowSRV);
private:
	GameEntity* gameEntity;
	Camera* camera;
	ID3D11Buffer *vertexBufferRender;
	ID3D11Buffer *indexBufferRender;
	SimpleVertexShader* vertexShaderRender;
	SimplePixelShader* pixelShaderRender;
	DirectionalLight dirLight1;
	DirectionalLight dirLight2;
	DirectionalLight dirLight3;
};

