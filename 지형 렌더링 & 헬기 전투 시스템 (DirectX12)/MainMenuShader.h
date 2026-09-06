#pragma once
#include "stdafx.h"
#include "Shader.h"
#include"UIShader.h"

class CMainMenuShader : public CObjectsShader
{
public:
	CMainMenuShader();
	~CMainMenuShader();

	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext = NULL, CScene* pOwnerScene = NULL) override;
	void AnimateObjects(float fTimeElapsed) override;
	void ReleaseObjects() override;

	void ReleaseUploadBuffers() override;

	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0) override;

};