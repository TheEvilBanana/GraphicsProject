#include "Renderer.h"



Renderer::Renderer(GameEntity* renderEntity, Camera* renderCamera) {
	this->gameEntity = renderEntity;
	this->camera = renderCamera;
}


Renderer::~Renderer() {
}

void Renderer::SetLights() {
	dirLight1.SetLightValues(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 0));
	dirLight2.SetLightValues(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 0));
}

ID3D11Buffer * Renderer::SetVertexBuffer() {
	vertexBufferRender = gameEntity->GetMesh()->GetVertexBuffer();
	return vertexBufferRender;
}

ID3D11Buffer * Renderer::SetIndexBuffer() {
	indexBufferRender = gameEntity->GetMesh()->GetIndexBuffer();
	return indexBufferRender;
}

SimpleVertexShader * Renderer::SetVertexShader() {
	vertexShaderRender = gameEntity->GetMaterial()->GetVertexShader();
	vertexShaderRender->SetMatrix4x4("world", *gameEntity->GetWorldMatrix());
	vertexShaderRender->SetMatrix4x4("view", camera->GetView());
	vertexShaderRender->SetMatrix4x4("projection", camera->GetProjection());

	vertexShaderRender->CopyAllBufferData();
	vertexShaderRender->SetShader();
	return vertexShaderRender;
}

SimplePixelShader * Renderer::SetPixelShader() {
	SetLights();
	pixelShaderRender = gameEntity->GetMaterial()->GetPixelShader();
	pixelShaderRender->SetData("dirLight1", &dirLight1, sizeof(DirectionalLight));
	pixelShaderRender->SetData("dirLight2", &dirLight2, sizeof(DirectionalLight));

	pixelShaderRender->SetShaderResourceView("textureSRV", gameEntity->GetMaterial()->GetMaterialSRV());
	pixelShaderRender->SetShaderResourceView("normalMapSRV", gameEntity->GetMaterial()->GetNormalSRV());
	pixelShaderRender->SetSamplerState("basicSampler", gameEntity->GetMaterial()->GetMaterialSampler());

	pixelShaderRender->CopyAllBufferData();
	pixelShaderRender->SetShader();
	return pixelShaderRender;
}


