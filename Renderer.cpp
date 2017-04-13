#include "Renderer.h"



Renderer::Renderer() {
	
}


Renderer::~Renderer() {
}

void Renderer::SetLights() {
	dirLight1.SetLightValues(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 0));
	dirLight2.SetLightValues(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 0));
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
	pixelShader->SetData("dirLight2", &dirLight2, sizeof(DirectionalLight));

	pixelShader->SetShaderResourceView("textureSRV", gameEntity->GetMaterial()->GetMaterialSRV());
	pixelShader->SetShaderResourceView("normalMapSRV", gameEntity->GetMaterial()->GetNormalSRV());
	pixelShader->SetSamplerState("basicSampler", gameEntity->GetMaterial()->GetMaterialSampler());

	pixelShader->SetSamplerState("ShadowSampler", shadowSampler);
	pixelShader->SetShaderResourceView("ShadowMap", shadowSRV);

	pixelShader->CopyAllBufferData();
	pixelShader->SetShader();
}

//SimpleVertexShader * Renderer::SetVertexShader(DirectX::XMFLOAT4X4 shadowViewMatrix, DirectX::XMFLOAT4X4 shadowProjectionMatrix) {
//	vertexShaderRender = gameEntity->GetMaterial()->GetVertexShader();
//	vertexShaderRender->SetMatrix4x4("world", *gameEntity->GetWorldMatrix());
//	vertexShaderRender->SetMatrix4x4("view", camera->GetView());
//	vertexShaderRender->SetMatrix4x4("projection", camera->GetProjection());
//	vertexShaderRender->SetMatrix4x4("shadowView", shadowViewMatrix);
//	vertexShaderRender->SetMatrix4x4("shadowProj", shadowProjectionMatrix);
//
//	vertexShaderRender->CopyAllBufferData();
//	vertexShaderRender->SetShader();
//	return vertexShaderRender;
//}
//
//SimplePixelShader * Renderer::SetPixelShader(ID3D11SamplerState* shadowSampler, ID3D11ShaderResourceView* shadowSRV) {
//	SetLights();
//	pixelShaderRender = gameEntity->GetMaterial()->GetPixelShader();
//	pixelShaderRender->SetData("dirLight1", &dirLight1, sizeof(DirectionalLight));
//	pixelShaderRender->SetData("dirLight2", &dirLight2, sizeof(DirectionalLight));
//
//	pixelShaderRender->SetShaderResourceView("textureSRV", gameEntity->GetMaterial()->GetMaterialSRV());
//	pixelShaderRender->SetShaderResourceView("normalMapSRV", gameEntity->GetMaterial()->GetNormalSRV());
//	pixelShaderRender->SetSamplerState("basicSampler", gameEntity->GetMaterial()->GetMaterialSampler());
//	
//	//Shadow things
//	pixelShaderRender->SetSamplerState("ShadowSampler", shadowSampler);
//	pixelShaderRender->SetShaderResourceView("ShadowMap", shadowSRV);
//
//	pixelShaderRender->CopyAllBufferData();
//	pixelShaderRender->SetShader();
//	return pixelShaderRender;
//}



