#pragma once
#include "stdafx.h"
#include "Shader.h"
struct BATTLE_INSTANCE_DATA
{
	XMFLOAT4X4 World; // HLSL float4x4 World 와 1:1 매칭
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
	// 인스턴스 데이터
	BATTLE_INSTANCE_DATA* m_pMappedInstanceData = nullptr;
	UINT                         m_nInstances = 0;
	ID3D12Resource* m_pd3dInstanceBuffer = nullptr;       // DEFAULT
	
	ID3D12Resource* m_pd3dInstanceUploadBuffer = nullptr; // UPLOAD
	D3D12_GPU_DESCRIPTOR_HANDLE  m_d3dInstanceSrvGpuHandle{};          // t18용 SRV 핸들
	void UpdateInstanceBuffer(ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateInstanceBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
		CScene* pOwnerScene);
	CGameObject* m_pMi24Model = nullptr;
	CGameObject* m_pModelMainRotor = nullptr;
	CGameObject* m_pModelTailRotor = nullptr;
	virtual void RenderReflection(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;
	
};

