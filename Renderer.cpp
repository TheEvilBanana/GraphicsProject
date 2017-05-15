#include "Renderer.h"



Renderer::Renderer() {
	
}


Renderer::~Renderer() {
}

void Renderer::SetLights() {
	dirLight1.SetLightValues(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 0));
	//dirLight2.SetLightValues(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 0));
}

void Renderer::SetVertexBuffer(GameEntity* &gameEntity, ID3D11Buffer* &vertexBuffer) {
	vertexBuffer = gameEntity->GetMesh()->GetVertexBuffer();
}

void Renderer::SetIndexBuffer(GameEntity* &gameEntity, ID3D11Buffer* &indexBuffer) {
	indexBuffer = gameEntity->GetMesh()->GetIndexBuffer();
}

void Renderer::SetVertexShader(SimpleVertexShader* &vertexShader, GameEntity* &gameEntity, Camera* &camera, XMFLOAT4X4 &shadowViewMatrix, XMFLOAT4X4 &shadowProjectionMatrix) {
	vertexShader = gameEntity->GetMaterial()->GetVertexShader();
	vertexShader->SetMatrix4x4("world", *gameEntity->GetWorldMatrix());
	vertexShader->SetMatrix4x4("view", camera->GetView());
	vertexShader->SetMatrix4x4("projection", camera->GetProjection());

	vertexShader->SetMatrix4x4("shadowView", shadowViewMatrix);
	vertexShader->SetMatrix4x4("shadowProj", shadowProjectionMatrix);
	
	vertexShader->CopyAllBufferData();
	vertexShader->SetShader();
}

void Renderer::SetPixelShader(SimplePixelShader* &pixelShader, GameEntity* &gameEntity, Camera* &camera, ID3D11SamplerState* &shadowSampler, ID3D11ShaderResourceView* &shadowSRV) {
	SetLights();
	pixelShader = gameEntity->GetMaterial()->GetPixelShader();
	pixelShader->SetData("dirLight1", &dirLight1, sizeof(DirectionalLight));
	//pixelShader->SetData("dirLight2", &dirLight2, sizeof(DirectionalLight));
	pixelShader->SetFloat3("pointLightPosition", XMFLOAT3(2, 2, 0));
	pixelShader->SetFloat4("pointLightColor", XMFLOAT4(0.1, 0.1f, 1, 1));
	pixelShader->SetFloat3("cameraPosition", XMFLOAT3(0, 0, -5));

	pixelShader->SetFloat3("spotLightDirection", XMFLOAT3(0, 3, 3));
	pixelShader->SetFloat("spotPower", 0.5);

	pixelShader->SetShaderResourceView("textureSRV", gameEntity->GetMaterial()->GetMaterialSRV());
	pixelShader->SetShaderResourceView("normalMapSRV", gameEntity->GetMaterial()->GetNormalSRV());
	pixelShader->SetSamplerState("basicSampler", gameEntity->GetMaterial()->GetMaterialSampler());

	pixelShader->SetSamplerState("ShadowSampler", shadowSampler);
	pixelShader->SetShaderResourceView("ShadowMap", shadowSRV);

	pixelShader->CopyAllBufferData();
	pixelShader->SetShader();
}

