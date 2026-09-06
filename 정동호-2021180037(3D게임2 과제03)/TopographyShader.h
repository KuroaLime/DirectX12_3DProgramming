#pragma once
#include "stdafx.h"
#include "Shader.h"
#include"Object.h"
#include "StaticObject.h"
#include<vector>

#define BillBoardType 7

class CTerrainShader : public CShader
{
public:
	CTerrainShader();
	virtual ~CTerrainShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader();
	virtual D3D12_SHADER_BYTECODE CreatePixelShader();
	virtual D3D12_SHADER_BYTECODE CreateHullShader();
	virtual D3D12_SHADER_BYTECODE CreateDomainShader();
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
protected:
	ID3DBlob* m_pd3dTerrainDebugPixelShaderBlob = NULL;
};

struct BILLBOARD_VERTEX
{
	XMFLOAT3 m_xmf3Position;
};

struct BILLBOARD_INSTANCE
{
	XMFLOAT3 m_xmf3Center;
	XMFLOAT2 m_xmf2Size;
};

class CBillboardShader : public CShader
{
public:
	CBillboardShader();
	virtual ~CBillboardShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader();

	virtual D3D12_SHADER_BYTECODE CreateGeometryShader();
	virtual D3D12_SHADER_BYTECODE CreatePixelShader();
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);

	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CHeightMapTerrain* pTerrain, CScene* pOwnerScene);

	virtual void ReleaseObjects();
	virtual void ReleaseUploadBuffers();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);

protected:

	ID3DBlob* m_pd3dGeometryShaderBlob = NULL;

	CTexture* m_ppTextures[BillBoardType] = {};
	CMaterial* m_ppMaterials[BillBoardType] = {};

	ID3D12Resource* m_pd3dVertexBuffer[BillBoardType] = {};
	ID3D12Resource* m_pd3dVertexUploadBuffer[BillBoardType] = {};
	ID3D12Resource* m_pd3dInstanceBuffer[BillBoardType] = {};
	ID3D12Resource* m_pd3dInstanceUploadBuffer[BillBoardType] = {};

	D3D12_VERTEX_BUFFER_VIEW		m_d3dVertexBufferView;
	D3D12_VERTEX_BUFFER_VIEW m_d3dInstanceBufferView;

	UINT							m_nInstances[BillBoardType] = { 0 };
	D3D12_VERTEX_BUFFER_VIEW vbViews[BillBoardType][2];
};

class CFlowingLavaShader : public CShader
{
public:
	CFlowingLavaShader();
	virtual ~CFlowingLavaShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
#ifdef _WITH_TEXTURE_DISTORTION_TRANSPARENT_TRANSFORM
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
#endif

	virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader() override;
};

class CExplosionShader : public CShader
{
public:
	CExplosionShader();
	virtual ~CExplosionShader();

	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext = NULL, CScene* pOwnerScene = NULL) override;
	void AnimateObjects(float fTimeElapsed) override;
	void ReleaseObjects() override;
	void ReleaseUploadBuffers() override;
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0) override;

	ID3DBlob* m_pd3dGeometryShaderBlob = NULL;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader();
	virtual D3D12_SHADER_BYTECODE CreatePixelShader();
	virtual D3D12_BLEND_DESC CreateBlendState();
	D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);

	void TriggerExplosion(const XMFLOAT3& position, float fScale = 20.0f, float fDuration = 0.5f);

	struct EXPLOSION_CPU {
		bool    bActive = false;
		float   fElapsed = 0.0f;
		float   fDuration = 0.5f;
		XMFLOAT3 vCenter = XMFLOAT3(0, 0, 0);
		float   fScale = 50.0f;
		int   nSpriteCols = 7;
		int   nSpriteRows = 1;
		XMFLOAT4 uv = XMFLOAT4(0, 0, 1, 1);
	};
	struct EXPLOSION_INSTANCE {
		XMFLOAT3 vCenter;
		float fSize;
		XMFLOAT4 uv;
	};
	ID3D12Resource* m_pd3dQuadVertexUploadBuffer = nullptr;
	ID3D12Resource* m_pd3dQuadIndexUploadBuffer = nullptr;
private:
	const int MAX_EXPLOSIONS = 20;
	std::vector<EXPLOSION_CPU> m_Explosions;
	std::vector<EXPLOSION_INSTANCE> m_InstanceDataCPU;

	ID3D12Resource* m_pd3dQuadVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_d3dQuadVBView{};
	ID3D12Resource* m_pd3dQuadIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_d3dQuadIBView{};

	ID3D12Resource* m_pd3dInstanceBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_d3dInstanceVBView{};
	EXPLOSION_INSTANCE* m_pMappedInstanceBuffer = nullptr;

	UINT m_nActiveInstances = 0;

	CTexture* m_pExplosionTexture = nullptr;
};
