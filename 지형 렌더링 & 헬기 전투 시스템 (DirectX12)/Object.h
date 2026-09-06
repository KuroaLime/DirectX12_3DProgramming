//------------------------------------------------------- ----------------------
// File: Object.h
//-----------------------------------------------------------------------------

#pragma once

#include "Mesh.h"
#include "Camera.h"
#include <vector>
#include "Collision.h"
#define DIR_FORWARD					0x01
#define DIR_BACKWARD				0x02
#define DIR_LEFT					0x04
#define DIR_RIGHT					0x08
#define DIR_UP						0x10
#define DIR_DOWN					0x20

class CShader;
class CStandardShader;
class CScene;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define RESOURCE_TEXTURE2D			0x01
#define RESOURCE_TEXTURE2D_ARRAY	0x02	//[]
#define RESOURCE_TEXTURE2DARRAY		0x03
#define RESOURCE_TEXTURE_CUBE		0x04
#define RESOURCE_BUFFER				0x05
enum class EObjectLayer
{
	Default,
	Player,
	Enemy,
	PlayerBullet,
	EnemyBullet,
	StaticObject
};


class CGameObject;
struct MATERIAL
{
	XMFLOAT4				m_cAmbient;
	XMFLOAT4				m_cDiffuse;
	XMFLOAT4				m_cSpecular; //a = power
	XMFLOAT4				m_cEmissive;
};
class CTexture
{
public:
	CTexture(int nTextureResources, UINT nResourceType, int nSamplers, int nRootParameters);
	virtual ~CTexture();

private:
	int								m_nReferences = 0;

	UINT							m_nTextureType;

	int								m_nTextures = 0;
	_TCHAR							(*m_ppstrTextureNames)[64] = NULL;
	ID3D12Resource**				m_ppd3dTextures = NULL;
	ID3D12Resource**				m_ppd3dTextureUploadBuffers;

	UINT*							m_pnResourceTypes = NULL;

	DXGI_FORMAT*					m_pdxgiBufferFormats = NULL;
	int*							m_pnBufferElements = NULL;

	int								m_nRootParameters = 0;
	int*							m_pnRootParameterIndices = NULL;
	D3D12_GPU_DESCRIPTOR_HANDLE*	m_pd3dSrvGpuDescriptorHandles = NULL;

	int								m_nSamplers = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE*	m_pd3dSamplerGpuDescriptorHandles = NULL;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	void SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle);

	void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

	void LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, wchar_t* pszFileName, UINT nResourceType, UINT nIndex);
	void LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex);
	ID3D12Resource* CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue);

	int LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pParent, FILE* pInFile, UINT nIndex);
//	int LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pParent, FILE* pInFile, CShader* pShader, UINT nIndex);

	void SetRootParameterIndex(int nIndex, UINT nRootParameterIndex);
	void SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int nIndex) { return(m_pd3dSrvGpuDescriptorHandles[nIndex]); }

	int GetRootParameters() { return(m_nRootParameters); }
	int GetTextures() { return(m_nTextures); }
	_TCHAR* GetTextureName(int nIndex) { return(m_ppstrTextureNames[nIndex]); }
	ID3D12Resource* GetResource(int nIndex) { return(m_ppd3dTextures[nIndex]); }
	int GetRootParameter(int nIndex) { return(m_pnRootParameterIndices[nIndex]); }

	UINT GetTextureType() { return(m_nTextureType); }
	UINT GetTextureType(int nIndex) { return(m_pnResourceTypes[nIndex]); }
	DXGI_FORMAT GetBufferFormat(int nIndex) { return(m_pdxgiBufferFormats[nIndex]); }
	int GetBufferElements(int nIndex) { return(m_pnBufferElements[nIndex]); }

	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc(int nIndex);

	void ReleaseUploadBuffers();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

class CGameObject;

class CMaterial
{
public:
	CMaterial();
	virtual ~CMaterial();

private:
	int								m_nReferences = 0;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

public:
	CShader							*m_pShader = NULL;
	CTexture*						m_pTexture = NULL;

	XMFLOAT4						m_xmf4AlbedoColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4						m_xmf4EmissiveColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4SpecularColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4AmbientColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	void SetShader(CShader *pShader);
	void SetMaterialType(UINT nType) { m_nType |= nType; }
	void SetTexture(CTexture* pTexture);

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void ReleaseUploadBuffers();

public:
	UINT							m_nType = 0x00;

	float							m_fGlossiness = 0.0f;
	float							m_fSmoothness = 0.0f;
	float							m_fSpecularHighlight = 0.0f;
	float							m_fMetallic = 0.0f;
	float							m_fGlossyReflection = 0.0f;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CGameObject
{
private:
	int								m_nReferences = 0;

public:
	void AddRef();
	void Release();

public:
	CGameObject();
	CGameObject(int nMeshes, int nMaterials);
	virtual ~CGameObject();

public:
	char							m_pstrFrameName[64];

	int								m_nMeshes = 0;
	CMesh**							m_ppMeshes = NULL;

	int								m_nMaterials = 0;
	CMaterial						**m_ppMaterials = NULL;

	XMFLOAT4X4						m_xmf4x4Transform;
	XMFLOAT4X4						m_xmf4x4World;

	CGameObject 					*m_pParent = NULL;
	CGameObject 					*m_pChild = NULL;
	CGameObject 					*m_pSibling = NULL;

	virtual void SetMesh(int nIndex, CMesh* pMesh);
	void SetShader(int nMaterial, CShader *pShader);
	void SetMaterial(int nMaterial, CMaterial *pMaterial);

	void SetChild(CGameObject *pChild);

	virtual void BuildMaterials(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList) { }

	virtual void PrepareAnimate() { }
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent=NULL);

	virtual void OnPrepareRender() { }
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bSkipMaterialShader, int nPipelineState = 0);

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World);
	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, CMaterial *pMaterial);

	virtual void ReleaseUploadBuffers();

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();

	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(XMFLOAT3 xmf3Position);
	virtual void SetScale(float x, float y, float z);

	void MoveStrafe(float fDistance = 1.0f);
	void MoveUp(float fDistance = 1.0f);
	void MoveForward(float fDistance = 1.0f);

	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f);
	void Rotate(XMFLOAT3 *pxmf3Axis, float fAngle);
	void Rotate(XMFLOAT4 *pxmf4Quaternion);

	CGameObject *GetParent() { return(m_pParent); }
	void UpdateTransform(XMFLOAT4X4 *pxmf4x4Parent=NULL);
	CGameObject *FindFrame(char *pstrFrameName);

	int FindReplicatedTexture(_TCHAR* pstrTextureName, D3D12_GPU_DESCRIPTOR_HANDLE* pd3dSrvGpuDescriptorHandle);

	UINT GetMeshType(UINT nIndex) { return((m_ppMeshes[nIndex]) ? m_ppMeshes[nIndex]->GetType() : 0x00); }
	struct CB_GAMEOBJECT_INFO
	{
		XMFLOAT4X4 m_xmf4x4World;      // 64 bytes
		MATERIAL   m_Material;         // 64 bytes -> 128
		UINT       m_nTexturesMask;    // 4 bytes
		XMFLOAT3 m_xmf3Padding;
		XMFLOAT4X4 m_pxmf4x4TextureTransforms[2]; // 128 bytes -> 272
	}; // Total = 256 bytes
	bool bRender = true;
protected:
	CScene* m_pOwnerScene = nullptr;
	ID3D12Resource* m_pd3dcbGameObject = NULL;
public:
	XMFLOAT4X4 GetWorldMatrix() const { return m_xmf4x4World; }
	int m_nCbvRootParameterIndex = 1;
	D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dCbvGpuDescriptorHandle;
	CB_GAMEOBJECT_INFO* m_pcbMappedGameObject = NULL;
	void LoadMaterialsFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CGameObject *pParent, FILE *pInFile, CShader* pShader);

	static CGameObject *LoadFrameHierarchyFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CGameObject *pParent, FILE *pInFile, CShader* pShader, CScene* pOwnerScene);
	static CGameObject *LoadGeometryFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, char *pstrFileName, CShader *pShader, CScene* pOwnerScene);

	static void PrintFrameInfo(CGameObject *pGameObject, CGameObject *pParent);

	void SetOwnerScene(CScene* s) { m_pOwnerScene = s; }
	void ResetOwnerScene() {
		m_pOwnerScene = nullptr;
		if (m_pChild)   m_pChild->ResetOwnerScene();
		if (m_pSibling) m_pSibling->ResetOwnerScene();
	}

	void SetLookAt(XMFLOAT3& xmf3Target, XMFLOAT3& xmf3Up);
	void GenerateRayForPicking(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection, XMMATRIX& xmmtxProjection);

	void UpdateBoundingBox();
	int PickOOBBByRayIntersection(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, float* pfHitDistance, XMMATRIX& xmmtxProjection);

	bool m_bSkipFrustum = false;
	BoundingOrientedBox m_xmLocalOOBB;
	//BoundingOrientedBox m_xmOOBB;
	BoundingOrientedBox m_xmWorldOOBB;
	BoundingSphere m_xmLocalSphere;
	BoundingSphere m_xmWorldSphere;
	BoundingOrientedBox m_xmBigOOGG;

	bool IsSetLocalOOBBFromMesh = true;


	/*Cellkey m_GridCell;
	bool m_bInGrid = false;*/
	std::vector<Cellkey> m_CurrentGridCells;

	void UpdateColliderFromTransform() {
		XMMATRIX xmWorld = XMLoadFloat4x4(&m_xmf4x4World);
		m_xmLocalOOBB.Transform(m_xmWorldOOBB, xmWorld);
	};

protected:
	bool         m_bCollidable = true;
	EObjectLayer m_eLayer = EObjectLayer::Default;

public:
	bool IsCollidable() const { return m_bCollidable; }
	void SetCollidable(bool v) { m_bCollidable = v; }

	EObjectLayer GetLayer() const { return m_eLayer; }
	void SetLayer(EObjectLayer layer) { m_eLayer = layer; }

protected:
	XMFLOAT3 m_xmf3OriginalPosition;



public:
	const BoundingSphere& GetLocalSphere() const { return m_xmLocalSphere; }
	const BoundingSphere& GetWorldSphere() const { return m_xmWorldSphere; }
	void StoreOriginalPosition() {m_xmf3OriginalPosition = GetPosition(); }
	XMFLOAT3& GetOriginalPosition() { return(m_xmf3OriginalPosition); }

	bool IsCollisionLeaf() const { if (m_ppMeshes && m_nMeshes > 0) return true; if (m_xmLocalOOBB.Extents.x > 0.0f) return true; return false; }

	void BuildBroadphaseSphere();
	void CalculateCompositeOOBB();

protected:
	const float MaxHP = 1000.0f;
	float HP = 1000.0f;
public:
	float GetHealth() { return HP; };
	float GetMaxHealth() { return MaxHP; };
	void Damage(int Attack) { HP -= Attack; }
	XMFLOAT4 m_xmf4TextureTransformParams;
	void SetTextureTransformParams(XMFLOAT4 params) { m_xmf4TextureTransformParams = params; }
	bool m_bAlive = true;
	float m_fRespawnTimer = 0;
	float m_fRespawnDelay = 3.0f;

	bool m_bExplosionTriggered = false;
	void Die() {
		m_bExplosionTriggered = true;
		m_bAlive = false;
		m_fRespawnTimer = m_fRespawnDelay;
		bRender = false;
		SetScale(0, 0, 0);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSuperCobraObject : public CGameObject
{
public:
	CSuperCobraObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature);
	virtual ~CSuperCobraObject();

private:
	CGameObject					*m_pMainRotorFrame = NULL;
	CGameObject					*m_pTailRotorFrame = NULL;

public:
	virtual void PrepareAnimate();
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent = NULL);
};

class CGunshipObject : public CGameObject
{
public:
	CGunshipObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature);
	virtual ~CGunshipObject();

private:
	CGameObject					*m_pMainRotorFrame = NULL;
	CGameObject					*m_pTailRotorFrame = NULL;

public:
	virtual void PrepareAnimate();
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent = NULL);
};

class CMi24Object : public CGameObject
{
public:
	CMi24Object(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature);
	virtual ~CMi24Object();

private:
	CGameObject					*m_pMainRotorFrame = NULL;
	CGameObject					*m_pTailRotorFrame = NULL;

public:
	virtual void PrepareAnimate();
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent = NULL);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBox : public CGameObject
{
public:
	CSkyBox(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CScene* OwnerScene);
	virtual ~CSkyBox();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);
};


///////////////////////////////////////////////////////////////
class CMirrorObject : public CGameObject
{
public:
	XMFLOAT3 m_xmf3PlaneNormal;
	float    m_fPlaneD;

public:
	CMirrorObject(int nMeshes = 1, int nMaterials = 1)
		: CGameObject(nMeshes, nMaterials)
	{
		m_xmf3PlaneNormal = XMFLOAT3(0.0f, 0.0f, -1.0f);
		m_fPlaneD = 0.0f;
	}

	void SetMirrorPlane(const XMFLOAT3& normal, float d)
	{
		m_xmf3PlaneNormal = normal;
		m_fPlaneD = d;
	}
};


//////////////////////////////////////////////////////////
class CMssileObject : public CGameObject
{
public:
	CMssileObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CScene* pOwnerScene);
	virtual ~CMssileObject();
public:
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent = NULL) override;
	virtual void Render(ID3D12GraphicsCommandList* cmd, CCamera* pCamera, bool bSkipMaterial = false, int nPipelineState = 0) override;
	CGameObject* m_pTarget = NULL;
	void setTargetObject(CGameObject* PickObject) { m_pTarget = PickObject; }
	
	void Fire(const XMFLOAT3& startPos,const XMFLOAT3& dir,CGameObject* pTarget);
	
	bool IsActive() const { return m_bActive; }
	void Explode();

private:
	void OrientToDirection(const XMFLOAT3& dir);
	XMFLOAT3     m_xmf3Direction = XMFLOAT3(0, 0, 1);
	float        m_fSpeed = 300.0f;
	float        m_fLifeTime = 0.0f;
	const float        m_fMaxLifeTime = 5.0f;
	bool         m_bActive = false;
};

class CExplosionObject : public CGameObject
{
public:
	CExplosionObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual ~CExplosionObject();

	virtual void Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent = NULL) override;

	void Reset(XMFLOAT3 xmf3Position, float fScale, float fDuration = 0.5f);
	bool IsActive() const { return m_bActive; }
	bool  m_bActive = false;
private:
	float m_fElapsed = 0.0f;
	float m_fDuration = 0.5f;

	int   m_nSpriteCols = 7;
	int   m_nSpriteRows = 1;
};
