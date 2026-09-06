#include "StaticObject.h"
#include"TopographyShader.h"

CHeightMapTerrain::CHeightMapTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color, CScene* OwnerScene) : CGameObject(0, 1)
{
	SetOwnerScene(OwnerScene);
	IsSetLocalOOBBFromMesh = false;
	m_nWidth = nWidth;
	m_nLength = nLength;

	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	m_xmf3Scale = xmf3Scale;

	m_pHeightMapImage = new CHeightMapImage(pFileName, nWidth, nLength, xmf3Scale);

	long cxBlocks = (m_nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (m_nLength - 1) / czQuadsPerBlock;

	m_nMeshes = cxBlocks * czBlocks;
	m_ppMeshes = new CMesh * [m_nMeshes];
	for (int i = 0; i < m_nMeshes; i++)	m_ppMeshes[i] = NULL;

	CHeightMapGridMesh* pHeightMapGridMesh = NULL;
	for (int z = 0, zStart = 0; z < czBlocks; z++)
	{
		for (int x = 0, xStart = 0; x < cxBlocks; x++)
		{
			xStart = x * (nBlockWidth - 1);
			zStart = z * (nBlockLength - 1);
			pHeightMapGridMesh = new CHeightMapGridMesh(pd3dDevice, pd3dCommandList, xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage);
			SetMesh(x + (z * cxBlocks), pHeightMapGridMesh);
		}
	}
	float fMinX = 0.0f;
	float fMaxX = (float)(m_nWidth - 1) * m_xmf3Scale.x;
	float fMinZ = 0.0f;
	float fMaxZ = (float)(m_nLength - 1) * m_xmf3Scale.z;

	BYTE* pPixels = m_pHeightMapImage->GetRawImagePixels();
	int nImageWidth = m_pHeightMapImage->GetRawImageWidth();
	int nImageLength = m_pHeightMapImage->GetRawImageLength();
	float fMinHeight = +FLT_MAX;
	float fMaxHeight = -FLT_MAX;

	for (int z = 0; z < nImageLength; z++)
	{
		for (int x = 0; x < nImageWidth; x++)
		{
			float fHeight = (float)pPixels[x + (z * nImageWidth)] * m_xmf3Scale.y;
			if (fHeight < fMinHeight) fMinHeight = fHeight;
			if (fHeight > fMaxHeight) fMaxHeight = fHeight;
		}
	}

	XMFLOAT3 xmf3Center = XMFLOAT3(
		fMinX + (fMaxX - fMinX) * 0.5f,
		fMinHeight + (fMaxHeight - fMinHeight) * 0.5f,
		fMinZ + (fMaxZ - fMinZ) * 0.5f
	);
	XMFLOAT3 xmf3Extents = XMFLOAT3(
		(fMaxX - fMinX) * 0.5f,
		(fMaxHeight - fMinHeight) * 0.5f,
		(fMaxZ - fMinZ) * 0.5f
	);

	m_xmLocalOOBB = BoundingOrientedBox(xmf3Center, xmf3Extents, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	CTexture* pTerrainTexture = new CTexture(3, RESOURCE_TEXTURE2D, 0, 1);

	pTerrainTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Terrain/Base_Texture.dds", RESOURCE_TEXTURE2D, 0);
	pTerrainTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Terrain/Detail_Texture_7.dds", RESOURCE_TEXTURE2D, 1);
	pTerrainTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Terrain/HeightMap(Alpha).dds", RESOURCE_TEXTURE2D, 2);
	OwnerScene->CreateShaderResourceViews(pd3dDevice, pTerrainTexture, 0, 11);

	CTerrainShader* pTerrainShader = new CTerrainShader();
	pTerrainShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	pTerrainShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CMaterial* pTerrainMaterial = new CMaterial();
	pTerrainMaterial->SetTexture(pTerrainTexture);
	pTerrainMaterial->SetShader(pTerrainShader);

	SetMaterial(0, pTerrainMaterial);
}

CHeightMapTerrain::~CHeightMapTerrain(void)
{
	if (m_pHeightMapImage) delete m_pHeightMapImage;
}
void CHeightMapTerrain::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	OnPrepareRender();

	if (m_pcbMappedGameObject && m_nMaterials > 0 && m_ppMaterials[0])
	{
		XMFLOAT4X4 xmf4x4World;
		XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
		m_pcbMappedGameObject->m_xmf4x4World = xmf4x4World;

		CMaterial* pMaterial = m_ppMaterials[0];
		m_pcbMappedGameObject->m_Material.m_cAmbient = pMaterial->m_xmf4AmbientColor;
		m_pcbMappedGameObject->m_Material.m_cDiffuse = pMaterial->m_xmf4AlbedoColor;
		m_pcbMappedGameObject->m_Material.m_cSpecular = pMaterial->m_xmf4SpecularColor;
		m_pcbMappedGameObject->m_Material.m_cEmissive = pMaterial->m_xmf4EmissiveColor;
		m_pcbMappedGameObject->m_nTexturesMask = pMaterial->m_nType;
	}

	if (m_d3dCbvGpuDescriptorHandle.ptr != NULL)
	{
		pd3dCommandList->SetGraphicsRootDescriptorTable(m_nCbvRootParameterIndex, m_d3dCbvGpuDescriptorHandle);
	}

	if ((m_nMaterials == 1) && (m_ppMaterials[0]))
	{
		int terrainPSO = nPipelineState;

		if (terrainPSO != 1 && m_pOwnerScene)
		{
			int mode = m_pOwnerScene->GetTerrainDebugMode();
			if (mode == 1) terrainPSO = 2;
			else if (mode == 2) terrainPSO = 3;
			else terrainPSO = 0;
		}
		if (m_ppMaterials[0]->m_pShader)
			m_ppMaterials[0]->m_pShader->Render(pd3dCommandList, pCamera, terrainPSO);

		m_ppMaterials[0]->UpdateShaderVariables(pd3dCommandList);
	}

	if (m_ppMeshes && pCamera)
	{
		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		XMMATRIX xmmtxWorld = XMLoadFloat4x4(&m_xmf4x4World);

		const BoundingFrustum& frustum = pCamera->GetFrustum();

		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i])
			{

				BoundingOrientedBox worldOOBB;

				m_ppMeshes[i]->m_xmOOBB.Transform(worldOOBB, xmmtxWorld);

				if (nPipelineState == 1 || frustum.Intersects(worldOOBB))
				{
					m_ppMeshes[i]->Render(pd3dCommandList, 0);
				}
			}
		}
	}

	if (m_pChild) m_pChild->Render(pd3dCommandList, pCamera, nPipelineState);
	if (m_pSibling) m_pSibling->Render(pd3dCommandList, pCamera, nPipelineState);
}

CFlowingLava::CFlowingLava(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nWidth, int nLength, CScene* OwnerScene) : CGameObject(1, 1)
{
	SetOwnerScene(OwnerScene);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CTexture* pTexture = new CTexture(3, RESOURCE_TEXTURE2D, 1, 1);
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"ImageWave/lava2.dds", RESOURCE_TEXTURE2D, 0);
#ifdef _WITH_TEXTURE_DISTORTION_TRANSPARENT_TRANSFORM
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"ImageWave/lava1_transparent.dds", RESOURCE_TEXTURE2D, 1);
#else
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"ImageWave/lava1.dds", RESOURCE_TEXTURE2D, 1);
#endif
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"ImageWave/lava3.dds", RESOURCE_TEXTURE2D, 2);
	OwnerScene->CreateShaderResourceViews(pd3dDevice, pTexture, 0, 12);

	CMaterial* pMaterial = new CMaterial();
	pMaterial->SetTexture(pTexture);
	pMaterial->SetMaterialType(MATERIAL_ALBEDO_MAP);

	CGridMesh* pMesh = new CGridMesh(pd3dDevice, pd3dCommandList, 0, 0, nWidth, nLength, XMFLOAT3(1, 1, 1), XMFLOAT4(1, 1, 1, 1));
	SetMesh(0, pMesh);

	m_pxmf4x4TextureTransforms[0] = Matrix4x4::Identity();
	m_pxmf4x4TextureTransforms[1] = Matrix4x4::Identity();

	m_pxmf4x4TextureTransforms[0]._11 = 5.0f;
	m_pxmf4x4TextureTransforms[0]._22 = 1.0f;
	m_pxmf4x4TextureTransforms[1]._11 = 5.0f;
	m_pxmf4x4TextureTransforms[1]._22 = 1.0f;
	m_pxmf4x4TextureTransforms[1]._41 = 1.1f;

	m_pxmf4x4TextureTransforms[0]._44 = m_pxmf4x4TextureTransforms[1]._44 = 0.0f;

	CFlowingLavaShader* pFlowingLavaShader = new CFlowingLavaShader();
	pFlowingLavaShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	pFlowingLavaShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = OwnerScene->CreateConstantBufferView(pd3dDevice, m_pd3dcbGameObject, ncbElementBytes);

	SetMaterial(0, pMaterial);

	SetShader(0, pFlowingLavaShader);
}

CFlowingLava::~CFlowingLava()
{
}

void CFlowingLava::Animate(float fTimeElapsed)
{
	m_pxmf4x4TextureTransforms[1]._31 += fTimeElapsed * 0.06f;
}

void CFlowingLava::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	OnPrepareRender();

	if (m_pcbMappedGameObject && m_nMaterials > 0 && m_ppMaterials[0])
	{
		XMFLOAT4X4 xmf4x4World;
		XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
		m_pcbMappedGameObject->m_xmf4x4World = xmf4x4World;

		CMaterial* pMaterial = m_ppMaterials[0];
		m_pcbMappedGameObject->m_Material.m_cAmbient = pMaterial->m_xmf4AmbientColor;
		m_pcbMappedGameObject->m_Material.m_cDiffuse = pMaterial->m_xmf4AlbedoColor;
		m_pcbMappedGameObject->m_Material.m_cSpecular = pMaterial->m_xmf4SpecularColor;
		m_pcbMappedGameObject->m_Material.m_cEmissive = pMaterial->m_xmf4EmissiveColor;
		m_pcbMappedGameObject->m_nTexturesMask = pMaterial->m_nType;

		XMMATRIX tex0 = XMLoadFloat4x4(&m_pxmf4x4TextureTransforms[0]);
		XMStoreFloat4x4(
			&m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0],
			XMMatrixTranspose(tex0)
		);

		XMMATRIX tex1 = XMLoadFloat4x4(&m_pxmf4x4TextureTransforms[1]);
		XMStoreFloat4x4(
			&m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[1],
			XMMatrixTranspose(tex1)
		);
	}

	if (m_d3dCbvGpuDescriptorHandle.ptr != NULL)
	{
		pd3dCommandList->SetGraphicsRootDescriptorTable(m_nCbvRootParameterIndex, m_d3dCbvGpuDescriptorHandle);
	}

	if ((m_nMaterials == 1) && (m_ppMaterials[0]))
	{
		if (m_ppMaterials[0]->m_pShader)
			m_ppMaterials[0]->m_pShader->Render(pd3dCommandList, pCamera, nPipelineState);

		m_ppMaterials[0]->UpdateShaderVariables(pd3dCommandList);
	}

	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList, 0);
		}
	}

	if (m_pChild) m_pChild->Render(pd3dCommandList, pCamera, nPipelineState);
	if (m_pSibling) m_pSibling->Render(pd3dCommandList, pCamera, nPipelineState);
}
