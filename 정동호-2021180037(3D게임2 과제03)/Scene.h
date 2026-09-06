#pragma once

#include "Shader.h"
#include "Player.h"
#include "Object.h"
#include"Camera.h"
#include"Collision.h"
#include"UIShader.h"

#include<unordered_map>

#define MAX_LIGHTS			16

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

#define MainMenuStage 0x00
#define SelectStage 0x01
#define BattleStage 0x02
#define SHADOW_MAP_SIZE 2048

class CHeightMapTerrain;
class CSkyBox;
class CTexture;
class CGameObject;
struct LIGHT
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction;
	float 					m_fTheta;
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi;
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct LIGHTS
{
	LIGHT					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};
struct CB_SHADOW_INFO
{
	XMFLOAT4X4 m_xmf4x4LightViewProj;
	XMFLOAT4X4 m_xmf4x4ShadowTransform;
};

class CDescriptorHeap
{
public:
	CDescriptorHeap();
	~CDescriptorHeap();

	ID3D12DescriptorHeap* m_pd3dCbvSrvDescriptorHeap = NULL;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dCbvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dCbvGPUDescriptorStartHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorStartHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dCbvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dCbvGPUDescriptorNextHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorNextHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_d3dSrvGPUDescriptorStartHandle); }
};

class CScene
{
public:
    CScene();
    ~CScene();

	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void BuildDefaultLightsAndMaterials();
	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseObjects();

	virtual ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	virtual bool ProcessInput(UCHAR *pKeysBuffer);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL);

	virtual void ReleaseUploadBuffers();

	CPlayer								*m_pPlayer = NULL;

public:
	ID3D12RootSignature					*m_pd3dGraphicsRootSignature = NULL;

	int									m_nGameObjects = 0;
	CGameObject							**m_ppGameObjects = NULL;

	int									m_nShaders = 0;
	CShader								**m_ppShaders = NULL;

	CSkyBox								*m_pSkyBox = NULL;
	CHeightMapTerrain*					m_pTerrain = NULL;

	LIGHT								*m_pLights = NULL;
	int									m_nLights = 0;

	XMFLOAT4							m_xmf4GlobalAmbient;

	ID3D12Resource						*m_pd3dcbLights = NULL;
	LIGHTS								*m_pcbMappedLights = NULL;

public:
	CDescriptorHeap*				m_pDescriptorHeap = NULL;

	virtual void CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews);
	virtual void CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride);
	virtual D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(ID3D12Device* pd3dDevice, ID3D12Resource* pd3dConstantBuffer, UINT nStride);
	virtual D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(ID3D12Device* pd3dDevice, D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress, UINT nStride);
	virtual void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);
	virtual void CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex, UINT nRootParameterStartIndex);
	virtual void CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorNextHandle() { return(m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle); }
protected:
	bool IsFinshied = FALSE;
	int NextSceneNum = NULL;
public:
	int GetSceneNum() { return NextSceneNum; }
	int GetSceneState() { return IsFinshied; }
	void ChangeFinishedState() { if (IsFinshied)IsFinshied = FALSE; else IsFinshied = TRUE; }
	CGameObject* PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera);

	CCollisionAlogrithm m_CollisionValue;

	bool CanLayersCollide(EObjectLayer a, EObjectLayer b);
	void OnCollision(CGameObject* a, CGameObject* b, const XMVECTOR& mtv);
	virtual void BuildSpatialGrid();

	void SetPlayer(CPlayer* pPlayer)
	{
		m_pPlayer = pPlayer;
	}

	CPlayer* GetPlayer() const
	{
		return m_pPlayer;
	}

	CShader* m_pUIShader = NULL;
	CCamera* m_pUICamera = NULL;
	std::vector<CGameObject*> m_vpUIObjects;
	CMaterial* m_pUIMaterial;
	virtual void BuildUIObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) {};

	virtual void RenderOverlay(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) {}

protected:
	CShadowMapShader* m_pShadowMapShader = nullptr;

	ID3D12Resource* m_pd3dShadowMap = NULL;
	ID3D12DescriptorHeap* m_pd3dShadowDsvHeap = NULL;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dShadowDsvCPUHandle{};

	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dShadowSrvGPUHandle{};

	ID3D12Resource* m_pd3dcbShadowInfo = NULL;
	CB_SHADOW_INFO* m_pcbMappedShadowInfo = NULL;

	D3D12_VIEWPORT             m_d3dShadowViewport{};
	D3D12_RECT                 m_d3dShadowScissorRect{};

	D3D12_RESOURCE_STATES        m_ShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
public:

	void CreateShadowResources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateShadowShaderVariables(const XMFLOAT3& xmf3LightDir, const XMFLOAT3& xmf3FocusPos);
	virtual void RenderShadowMap(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShadowResources();

	ID3D12Resource* GetShadowMapResource() const { return m_pd3dShadowMap; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetShadowDsv() const { return m_d3dShadowDsvCPUHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetShadowSrvGpuHandle() const { return m_d3dShadowSrvGPUHandle; }
	ID3D12Resource* GetShadowCB() const { return m_pd3dcbShadowInfo; }
protected:
	int  m_nTerrainDebugMode = 0;
public:
	int  GetTerrainDebugMode() const { return m_nTerrainDebugMode; }
	void SetTerrainDebugMode(int m) { m_nTerrainDebugMode = m; }
};
