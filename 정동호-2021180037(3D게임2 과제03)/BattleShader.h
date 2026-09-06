#pragma once
#include "stdafx.h"
#include "Shader.h"
struct BATTLE_INSTANCE_DATA
{
	XMFLOAT4X4 World;
};

class CBattleShader : public CObjectsShader
{
public:
	CBattleShader();
	~CBattleShader();

	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext = NULL, CScene* pOwnerScene = NULL) override;
	void AnimateObjects(float fTimeElapsed) override;
	void ReleaseObjects() override;

	void ReleaseUploadBuffers() override;

	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0) override;
	virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
	CGameObject* m_pMissileModel = nullptr;
	CGameObject* CloneGameObjectHierarchy(CGameObject* pSrc);
	void RenderShadowInstanced(ID3D12GraphicsCommandList* pd3dCommandList);
protected:

	BATTLE_INSTANCE_DATA* m_pMappedInstanceData = nullptr;
	UINT                         m_nInstances = 0;
	ID3D12Resource* m_pd3dInstanceBuffer = nullptr;

	ID3D12Resource* m_pd3dInstanceUploadBuffer = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE  m_d3dInstanceSrvGpuHandle{};
	void UpdateInstanceBuffer(ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateInstanceBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
		CScene* pOwnerScene);
	CGameObject* m_pMi24Model = nullptr;
	CGameObject* m_pModelMainRotor = nullptr;
	CGameObject* m_pModelTailRotor = nullptr;
	virtual void RenderReflection(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;

};
