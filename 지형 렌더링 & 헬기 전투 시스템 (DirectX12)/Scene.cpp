//-----------------------------------------------------------------------------
// File: CScene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include"StaticObject.h"
#include <unordered_set>
#include <algorithm>
//CDescriptorHeap* CScene::m_pDescriptorHeap = NULL;

CDescriptorHeap::CDescriptorHeap()
{
	m_d3dSrvCPUDescriptorStartHandle.ptr = NULL;
	m_d3dSrvGPUDescriptorStartHandle.ptr = NULL;
}

CDescriptorHeap::~CDescriptorHeap()
{
	if (m_pd3dCbvSrvDescriptorHeap) m_pd3dCbvSrvDescriptorHeap->Release();
}

void CScene::CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = nConstantBufferViews + nShaderResourceViews; //CBVs + SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle;
}

void CScene::CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride)
{
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = pd3dConstantBuffers->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	for (int j = 0; j < nConstantBufferViews; j++)
	{
		d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress + (nStride * j);
		pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE CScene::CreateConstantBufferView(ID3D12Device* pd3dDevice, ID3D12Resource* pd3dConstantBuffer, UINT nStride)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	d3dCBVDesc.BufferLocation = pd3dConstantBuffer->GetGPUVirtualAddress();
	pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle;
	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	return(d3dCbvGPUDescriptorHandle);
}

D3D12_GPU_DESCRIPTOR_HANDLE CScene::CreateConstantBufferView(ID3D12Device* pd3dDevice, D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress, UINT nStride)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress;
	pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle;
	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	return(d3dCbvGPUDescriptorHandle);
}

void CScene::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);

	int nTextures = pTexture->GetTextures();
	for (int i = 0; i < nTextures; i++)
	{
		ID3D12Resource* pShaderResource = pTexture->GetResource(i);
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(i, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
	int nRootParameters = pTexture->GetRootParameters();
	for (int i = 0; i < nRootParameters; i++) pTexture->SetRootParameterIndex(i, nRootParameterStartIndex + i);
}

void CScene::CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex, UINT nRootParameterStartIndex)
{
	ID3D12Resource* pShaderResource = pTexture->GetResource(nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle = pTexture->GetGpuDescriptorHandle(nIndex);
	if (pShaderResource && !d3dGpuDescriptorHandle.ptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(nIndex);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(nIndex, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetRootParameterIndex(nIndex, nRootParameterStartIndex + nIndex);
	}
}

void CScene::CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex)
{
	ID3D12Resource* pShaderResource = pTexture->GetResource(nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle = pTexture->GetGpuDescriptorHandle(nIndex);
	if (pShaderResource && !d3dGpuDescriptorHandle.ptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(nIndex);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(nIndex, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

CScene::CScene() : m_CollisionValue(200.0f)
{
	m_CollisionValue.CanLayersCollide = [this](EObjectLayer a, EObjectLayer b) {
		return this->CanLayersCollide(a, b);
		};

	m_CollisionValue.OnCollision = [this](CGameObject* A, CGameObject* B, const XMVECTOR& mtv) {
		this->OnCollision(A, B, mtv);
		};
}

CScene::~CScene()
{
}

void CScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 4;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.30f, 0.30f, 0.30f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = POINT_LIGHT;
	m_pLights[0].m_fRange = 1000.0f;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.1f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(30.0f, 30.0f, 30.0f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pLights[0].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.001f, 0.0001f);
	m_pLights[1].m_bEnable = true;
	m_pLights[1].m_nType = SPOT_LIGHT;
	m_pLights[1].m_fRange = 500.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(-50.0f, 20.0f, -5.0f);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[1].m_fFalloff = 8.0f;
	m_pLights[1].m_fPhi = (float)cos(XMConvertToRadians(40.0f));
	m_pLights[1].m_fTheta = (float)cos(XMConvertToRadians(20.0f));
	m_pLights[2].m_bEnable = true;
	m_pLights[2].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_pLights[2].m_xmf4Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 0.0f);
	m_pLights[2].m_xmf3Direction = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_pLights[3].m_bEnable = true;
	m_pLights[3].m_nType = SPOT_LIGHT;
	m_pLights[3].m_fRange = 600.0f;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.3f, 0.7f, 0.0f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(50.0f, 30.0f, 30.0f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(0.0f, 1.0f, 1.0f);
	m_pLights[3].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[3].m_fFalloff = 8.0f;
	m_pLights[3].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	m_pLights[3].m_fTheta = (float)cos(XMConvertToRadians(30.0f));
}

void CScene::BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CObjectsShader* pObjectsShader = new CObjectsShader();
	int nObjects = pObjectsShader->GetNumberOfObjects();

	m_pDescriptorHeap = new CDescriptorHeap();
	int nCbvDescriptors = 200; 
	int nSrvDescriptors = 17 + 50 + 1 + 1 + 3 +1; 
	CreateCbvSrvDescriptorHeaps(pd3dDevice, nCbvDescriptors, nSrvDescriptors);
	
	BuildDefaultLightsAndMaterials();

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, this);

	XMFLOAT3 xmf3Scale(18.0f, 6.0f, 18.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.5f, 0.0f, 0.0f);
	m_pTerrain = new CHeightMapTerrain(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, _T("Terrain/HeightMap.raw"), 257, 257, 257, 257, xmf3Scale, xmf4Color, this);

	m_nShaders = 1;
	m_ppShaders = new CShader*[m_nShaders];

	pObjectsShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	pObjectsShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, NULL,this);

	m_ppShaders[0] = pObjectsShader;

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::ReleaseObjects()
{
	if (m_pDescriptorHeap) delete m_pDescriptorHeap;

	ReleaseShaderVariables();

	if (m_ppShaders)
	{
		for (int i = 0; i < m_nShaders; i++)
		{
			m_ppShaders[i]->ReleaseShaderVariables();
			m_ppShaders[i]->ReleaseObjects();
			m_ppShaders[i]->Release();
		}
		delete[] m_ppShaders;
	}

	if (m_pTerrain) delete m_pTerrain;
	if (m_pSkyBox) delete m_pSkyBox;

	if (m_ppGameObjects)
	{
		for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Release();
		delete[] m_ppGameObjects;
	}

	if (m_pLights) delete[] m_pLights;
}

ID3D12RootSignature *CScene::CreateGraphicsRootSignature(ID3D12Device *pd3dDevice)
{
	ID3D12RootSignature *pd3dGraphicsRootSignature = NULL;

	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[12];

	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 1;
	pd3dDescriptorRanges[0].BaseShaderRegister = 6; //t6: gtxtAlbedoTexture
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[1].NumDescriptors = 1;
	pd3dDescriptorRanges[1].BaseShaderRegister = 7; //t7: gtxtSpecularTexture
	pd3dDescriptorRanges[1].RegisterSpace = 0;
	pd3dDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[2].NumDescriptors = 1;
	pd3dDescriptorRanges[2].BaseShaderRegister = 8; //t8: gtxtNormalTexture
	pd3dDescriptorRanges[2].RegisterSpace = 0;
	pd3dDescriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[3].NumDescriptors = 1;
	pd3dDescriptorRanges[3].BaseShaderRegister = 9; //t9: gtxtMetallicTexture
	pd3dDescriptorRanges[3].RegisterSpace = 0;
	pd3dDescriptorRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[4].NumDescriptors = 1;
	pd3dDescriptorRanges[4].BaseShaderRegister = 10; //t10: gtxtEmissionTexture
	pd3dDescriptorRanges[4].RegisterSpace = 0;
	pd3dDescriptorRanges[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[5].NumDescriptors = 1;
	pd3dDescriptorRanges[5].BaseShaderRegister = 11; //t11: gtxtDetailAlbedoTexture
	pd3dDescriptorRanges[5].RegisterSpace = 0;
	pd3dDescriptorRanges[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[6].NumDescriptors = 1;
	pd3dDescriptorRanges[6].BaseShaderRegister = 12; //t12: gtxtDetailNormalTexture
	pd3dDescriptorRanges[6].RegisterSpace = 0;
	pd3dDescriptorRanges[6].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[7].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[7].NumDescriptors = 1;
	pd3dDescriptorRanges[7].BaseShaderRegister = 13; //t13: gtxtSkyBoxTexture
	pd3dDescriptorRanges[7].RegisterSpace = 0;
	pd3dDescriptorRanges[7].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[8].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[8].NumDescriptors = 3;
	pd3dDescriptorRanges[8].BaseShaderRegister = 14; //t14~16: gtxtTerrainTexture
	pd3dDescriptorRanges[8].RegisterSpace = 0;
	pd3dDescriptorRanges[8].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[9].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[9].NumDescriptors = 3;
	pd3dDescriptorRanges[9].BaseShaderRegister = 17; //t14~16: gtxtTerrainTexture
	pd3dDescriptorRanges[9].RegisterSpace = 0;
	pd3dDescriptorRanges[9].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[10].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[10].NumDescriptors = 1;
	pd3dDescriptorRanges[10].BaseShaderRegister = 18; // t18
	pd3dDescriptorRanges[10].RegisterSpace = 0;
	pd3dDescriptorRanges[10].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[11].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[11].NumDescriptors = 1;
	pd3dDescriptorRanges[11].BaseShaderRegister = 20; // t18
	pd3dDescriptorRanges[11].RegisterSpace = 0;
	pd3dDescriptorRanges[11].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE pd3dCBVDescriptorRanges[1];
	pd3dCBVDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	pd3dCBVDescriptorRanges[0].NumDescriptors = 1;
	pd3dCBVDescriptorRanges[0].BaseShaderRegister = 2; // b2 (cbGameObjectInfo)
	pd3dCBVDescriptorRanges[0].RegisterSpace = 0;
	pd3dCBVDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	D3D12_ROOT_PARAMETER pd3dRootParameters[16];

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 1; //Camera
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[1].DescriptorTable.pDescriptorRanges = &(pd3dCBVDescriptorRanges[0]); //Player
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[2].Descriptor.ShaderRegister = 4; //Lights
	pd3dRootParameters[2].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[3].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[0]);
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[4].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[1]);
	pd3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[5].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[2]);
	pd3dRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[6].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[6].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[3]);
	pd3dRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[7].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[7].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[4]);
	pd3dRootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[8].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[8].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[5]);
	pd3dRootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[9].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[9].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[6]);
	pd3dRootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[10].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[10].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[7]);
	pd3dRootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[11].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[11].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[11].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[8]);
	pd3dRootParameters[11].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[12].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; //Texture Animate
	pd3dRootParameters[12].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[12].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[9]);
	pd3dRootParameters[12].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[13].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[13].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[13].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[10]);
	pd3dRootParameters[13].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[14].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[14].Descriptor.ShaderRegister = 14;   // b14
	pd3dRootParameters[14].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[14].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// -----------------------------------------
	// -----------------------------------------
	pd3dRootParameters[15].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[15].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[15].DescriptorTable.pDescriptorRanges = &pd3dDescriptorRanges[11];
	pd3dRootParameters[15].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC pd3dSamplerDescs[4];

	pd3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].MipLODBias = 0;
	pd3dSamplerDescs[0].MaxAnisotropy = 1;
	pd3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[0].MinLOD = 0;
	pd3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[0].ShaderRegister = 0;
	pd3dSamplerDescs[0].RegisterSpace = 0;
	pd3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].MipLODBias = 0;
	pd3dSamplerDescs[1].MaxAnisotropy = 1;
	pd3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[1].MinLOD = 0;
	pd3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[1].ShaderRegister = 1;
	pd3dSamplerDescs[1].RegisterSpace = 0;
	pd3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[2].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	pd3dSamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].MipLODBias = 0;
	pd3dSamplerDescs[2].MaxAnisotropy = 1;
	pd3dSamplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[2].MinLOD = 0;
	pd3dSamplerDescs[2].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[2].ShaderRegister = 2;
	pd3dSamplerDescs[2].RegisterSpace = 0;
	pd3dSamplerDescs[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[3] = pd3dSamplerDescs[0];
	pd3dSamplerDescs[3].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	pd3dSamplerDescs[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].MipLODBias = 0.0f;
	pd3dSamplerDescs[3].MaxAnisotropy = 1;
	pd3dSamplerDescs[3].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pd3dSamplerDescs[3].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	pd3dSamplerDescs[3].ShaderRegister = 3;
	pd3dSamplerDescs[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = _countof(pd3dSamplerDescs);
	d3dRootSignatureDesc.pStaticSamplers = pd3dSamplerDescs;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob *pd3dSignatureBlob = NULL;
	ID3DBlob *pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void **)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255);
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbLights->Map(0, NULL, (void **)&m_pcbMappedLights);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	::memcpy(m_pcbMappedLights->m_pLights, m_pLights, sizeof(LIGHT) * m_nLights);
	::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &m_xmf4GlobalAmbient, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedLights->m_nLights, &m_nLights, sizeof(int));
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbLights)
	{
		m_pd3dcbLights->Unmap(0, NULL);
		m_pd3dcbLights->Release();
	}

	if (m_pTerrain) m_pTerrain->ReleaseShaderVariables();
	if (m_pSkyBox) m_pSkyBox->ReleaseShaderVariables();
}

void CScene::ReleaseUploadBuffers()
{
	if (m_pTerrain) m_pTerrain->ReleaseUploadBuffers();
	if (m_pSkyBox) m_pSkyBox->ReleaseUploadBuffers();

	for (int i = 0; i < m_nShaders; i++) m_ppShaders[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nGameObjects; i++) m_ppGameObjects[i]->ReleaseUploadBuffers();
}

CGameObject* CScene::PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera)
{
	XMFLOAT4X4 xmf4x4Projection = pCamera->GetProjectionMatrix();
	XMMATRIX xmmtxProjection = XMLoadFloat4x4(&xmf4x4Projection);
	XMMATRIX xmmtxView = XMLoadFloat4x4(&pCamera->GetViewMatrix());

	XMFLOAT3 xmf3PickPosition;

	if (fabs(xmf4x4Projection._44 - 1.0f) < 0.001f)
	{
		const D3D12_VIEWPORT& vp = pCamera->GetViewport();
		float fx = (float)xClient;
		float fy = (float)yClient;

		xmf3PickPosition.x = fx;
		xmf3PickPosition.y = (float)vp.Height - fy;
		xmf3PickPosition.z = 0.0f;
	}
	else
	{
		xmf3PickPosition.x = (((2.0f * xClient) / (float)pCamera->GetViewport().Width) - 1) / xmf4x4Projection._11;
		xmf3PickPosition.y = -(((2.0f * yClient) / (float)pCamera->GetViewport().Height) - 1) / xmf4x4Projection._22;
		xmf3PickPosition.z = 1.0f;
	}

	XMVECTOR xmvPickPosition = XMLoadFloat3(&xmf3PickPosition);
	CGameObject* pNearestObject = NULL;
	float fNearestHitDistance = FLT_MAX;

	for (int i = 0; i < m_nGameObjects; i++)
	{
		float fHitDistance = FLT_MAX;

		bool nIntersected = m_ppGameObjects[i]->PickOOBBByRayIntersection(xmvPickPosition, xmmtxView, &fHitDistance, xmmtxProjection);

		if ((nIntersected) && (fHitDistance < fNearestHitDistance))
		{
			fNearestHitDistance = fHitDistance;
			pNearestObject = m_ppGameObjects[i];
		}
	}
	for (int s = 0; s < m_nShaders; ++s)
	{
		CObjectsShader* pObjectsShader = dynamic_cast<CObjectsShader*>(m_ppShaders[s]);
		if (!pObjectsShader) continue;

		int nObjects = pObjectsShader->GetNumberOfObjects();
		for (int j = 0; j < nObjects; ++j)
		{
			CGameObject* pObj = pObjectsShader->GetObject(j);
			if (!pObj) continue;

			float fHitDistance = FLT_MAX;
			bool nIntersected = pObj->PickOOBBByRayIntersection(
				xmvPickPosition, xmmtxView, &fHitDistance, xmmtxProjection);

			if (nIntersected && (fHitDistance < fNearestHitDistance))
			{
				fNearestHitDistance = fHitDistance;
				pNearestObject = pObj;
			}
		}
	}
	return(pNearestObject);
}


bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera)
{
	return(false);
}

bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'W': m_pPlayer->MoveForward(+1.0f); break;
		case 'S': m_pPlayer->MoveForward(-1.0f); break;
		case 'A': m_pPlayer->MoveStrafe(-1.0f); break;
		case 'D': m_pPlayer->MoveStrafe(+1.0f); break;
		case 'Q': m_pPlayer->MoveUp(+1.0f); break;
		case 'R': m_pPlayer->MoveUp(-1.0f); break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return(false);
}

bool CScene::ProcessInput(UCHAR *pKeysBuffer)
{
	return(false);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	for (int i = 0; i < m_nGameObjects; i++)
		if (m_ppGameObjects[i]) m_ppGameObjects[i]->StoreOriginalPosition();

	for (int i = 0; i < m_nShaders; i++)
	{
		if (m_ppShaders[i])
		{
			CObjectsShader* pObjectsShader = static_cast<CObjectsShader*>(m_ppShaders[i]);

			int nObjects = pObjectsShader->GetNumberOfObjects();
			for (int j = 0; j < nObjects; j++)
			{
				CGameObject* pObj = pObjectsShader->GetObject(j);
				if (pObj) pObj->StoreOriginalPosition();
			}
		}
	}

	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Animate(fTimeElapsed, NULL);
	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->UpdateTransform(NULL);

	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->AnimateObjects(fTimeElapsed);
	const int nSolverIterations = 2;
	//for (int i = 0; i < nSolverIterations; ++i)
	//{
	//	m_CollisionValue.CheckCollision();

	//}
	if (m_pLights)
	{
		m_pLights[1].m_xmf3Position = m_pPlayer->GetPosition();
		m_pLights[1].m_xmf3Direction = m_pPlayer->GetLookVector();
	}
}

void CScene::BuildSpatialGrid()
{
	m_CollisionValue.ClearGrid();

	for (int i = 0; i < m_nGameObjects; ++i)
	{
		if (!m_ppGameObjects[i]) continue;

		m_CollisionValue.InsertObjectToGridByAABB(m_ppGameObjects[i]);
	}
	for (int i = 0; i < m_nShaders; ++i)
	{
		if (!m_ppShaders[i]) continue;
		CObjectsShader* pObjectsShader = static_cast<CObjectsShader*>(m_ppShaders[i]);
		if (!pObjectsShader) continue;

		int pShaderObjects = pObjectsShader->GetNumberOfObjects();
		for (int index=0; index <pShaderObjects;index++)
			if(pObjectsShader->GetObject(index))
				m_CollisionValue.InsertObjectToGridByAABB(pObjectsShader->GetObject(index));
	}
	if (m_pPlayer)
	{
		m_CollisionValue.InsertObjectToGridByAABB(m_pPlayer);
	}
}
bool CScene::CanLayersCollide(EObjectLayer a, EObjectLayer b)
{
	
	if (a == b && a != EObjectLayer::Enemy)
		return false;
	


	if (a == EObjectLayer::Enemy && b == EObjectLayer::Enemy)
		return true;

	if ((a == EObjectLayer::Player && b == EObjectLayer::Enemy) ||
		(a == EObjectLayer::Enemy && b == EObjectLayer::Player))
		return true;

	if ((a == EObjectLayer::Player && b == EObjectLayer::EnemyBullet) ||
		(a == EObjectLayer::EnemyBullet && b == EObjectLayer::Player) ||
		(a == EObjectLayer::Enemy && b == EObjectLayer::PlayerBullet) ||
		(a == EObjectLayer::PlayerBullet && b == EObjectLayer::Enemy))
		return true;

	if ((a == EObjectLayer::StaticObject && (b == EObjectLayer::Player || b == EObjectLayer::Enemy)) ||
		(b == EObjectLayer::StaticObject && (a == EObjectLayer::Player || a == EObjectLayer::Enemy)))
		return true;

	return false;

}

void CScene::OnCollision(CGameObject* a, CGameObject* b, const XMVECTOR& mtv)
{
	
	if (!a || !b) return;
	if (!a->IsCollidable() || !b->IsCollidable()) return;

	XMVECTOR vMtv = mtv;
	float depth = XMVectorGetX(XMVector3Length(vMtv));

	if (depth < 0.0001f) return;

	XMVECTOR vNormal = XMVector3Normalize(vMtv);

	XMFLOAT3 posA_f3 = a->GetPosition();
	XMFLOAT3 posB_f3 = b->GetPosition();
	XMVECTOR posA = XMLoadFloat3(&posA_f3);
	XMVECTOR posB = XMLoadFloat3(&posB_f3);

	XMVECTOR centerDiff = XMVectorSubtract(posA, posB);
	if (XMVectorGetX(XMVector3Dot(centerDiff, vNormal)) < 0.0f)
	{
		vNormal = XMVectorNegate(vNormal);
		vMtv = XMVectorNegate(vMtv);
	}

	EObjectLayer layerA = a->GetLayer();
	EObjectLayer layerB = b->GetLayer();

	XMVECTOR corrA = XMVectorZero();
	XMVECTOR corrB = XMVectorZero();

	if (layerA == EObjectLayer::StaticObject)
	{
		corrB = XMVectorNegate(vMtv);
	}
	else if (layerB == EObjectLayer::StaticObject)
	{
		corrA = vMtv;
	}
	else
	{
		XMVECTOR half = XMVectorScale(vMtv, 0.5f);
		corrA = half;
		corrB = XMVectorNegate(half);
	}

	if (!XMVector3Equal(corrA, XMVectorZero()))
	{
		posA = XMVectorAdd(posA, corrA);
		XMStoreFloat3(&posA_f3, posA);
		a->SetPosition(posA_f3);

		if (a->GetLayer() == EObjectLayer::Player || a->GetLayer() == EObjectLayer::Enemy)
		{
			CPlayer* pDynamicA = dynamic_cast<CPlayer*>(a);
			if (pDynamicA)
			{
				XMFLOAT3 velF3 = pDynamicA->GetVelocity();
				XMVECTOR vel = XMLoadFloat3(&velF3);

				float dot = XMVectorGetX(XMVector3Dot(vel, vNormal));
				if (dot < 0.0f)
				{
					XMVECTOR slideVel = XMVectorSubtract(vel, XMVectorScale(vNormal, dot));
					XMFLOAT3 newVel;
					XMStoreFloat3(&newVel, slideVel);
					pDynamicA->SetVelocity(newVel);
				}
			}
		}
	}

	if (!XMVector3Equal(corrB, XMVectorZero()))
	{
		posB = XMVectorAdd(posB, corrB);
		XMStoreFloat3(&posB_f3, posB);
		b->SetPosition(posB_f3);

		XMVECTOR vNormalB = XMVectorNegate(vNormal);
		if (b->GetLayer() == EObjectLayer::Player || b->GetLayer() == EObjectLayer::Enemy)
		{
			CPlayer* pDynamicB = dynamic_cast<CPlayer*>(b);
			if (pDynamicB)
			{
				XMFLOAT3 velF3 = pDynamicB->GetVelocity();
				XMVECTOR vel = XMLoadFloat3(&velF3);
				float dot = XMVectorGetX(XMVector3Dot(vel, vNormalB));
				if (dot < 0.0f)
				{
					XMVECTOR slideVel = XMVectorSubtract(vel, XMVectorScale(vNormalB, dot));
					XMFLOAT3 newVel;
					XMStoreFloat3(&newVel, slideVel);
					pDynamicB->SetVelocity(newVel);
				}
			}
		}
	}
	if (a->GetLayer() == EObjectLayer::PlayerBullet &&
		b->GetLayer() == EObjectLayer::Enemy)
	{
		b->Damage(1000);
	}
	else if (b->GetLayer() == EObjectLayer::PlayerBullet &&
		a->GetLayer() == EObjectLayer::Enemy)
	{
		// [수정] 여기서는 a가 적이므로 a에게 데미지를 줘야 하는데 b(미사일 자신)에게 주고 있었음.
		// 그 결과 이 순서로 페어가 들어오는 절반의 경우 적이 데미지를 전혀 못 받았음.
		a->Damage(1000);
	}
	else if (b->GetLayer() == EObjectLayer::Player &&
		a->GetLayer() == EObjectLayer::Enemy ||
		a->GetLayer() == EObjectLayer::Player &&
		b->GetLayer() == EObjectLayer::Enemy) {

		a->Damage(10);
		b->Damage(10);
	}
	if (a->GetLayer() == EObjectLayer::Enemy && a->GetHealth() <= 0) {
		a->Die();

	}
	else if (b->GetLayer() == EObjectLayer::Enemy && b->GetHealth() <= 0) {
		b->Die();
	}

	// [수정] 미사일(PlayerBullet)이 무언가와 충돌하면 데미지만 주고 자기 자신은 계속 날아가던 문제.
	// 적/지형 등 무엇을 맞히든 그 자리에서 폭발 처리해서 미사일을 비활성화(사라지게) 시킨다.
	if (a->GetLayer() == EObjectLayer::PlayerBullet)
	{
		CMssileObject* pMissile = dynamic_cast<CMssileObject*>(a);
		if (pMissile) pMissile->Explode();
	}
	if (b->GetLayer() == EObjectLayer::PlayerBullet)
	{
		CMssileObject* pMissile = dynamic_cast<CMssileObject*>(b);
		if (pMissile) pMissile->Explode();
	}
}
bool IntersectOOBB(CGameObject* a, CGameObject* b)
{
	return a->m_xmWorldOOBB.Intersects(b->m_xmWorldOOBB);
}



void CScene::CreateShadowResources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = SHADOW_MAP_SIZE;
	texDesc.Height = SHADOW_MAP_SIZE;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	HRESULT hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_pd3dShadowMap)
	);
	m_ShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = pd3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pd3dShadowDsvHeap));
	m_d3dShadowDsvCPUHandle = m_pd3dShadowDsvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	pd3dDevice->CreateDepthStencilView(m_pd3dShadowMap, &dsvDesc, m_d3dShadowDsvCPUHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle;

	pd3dDevice->CreateShaderResourceView(m_pd3dShadowMap, &srvDesc, cpuHandle);

	UINT inc = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += inc;
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += inc;

	m_d3dShadowSrvGPUHandle = gpuHandle;

	UINT ncbElementBytes = ((sizeof(CB_SHADOW_INFO) + 255) & ~255);
	m_pd3dcbShadowInfo = ::CreateBufferResource(
		pd3dDevice,
		pd3dCommandList,
		NULL,
		ncbElementBytes,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		NULL
	);
	m_pd3dcbShadowInfo->Map(0, NULL, (void**)&m_pcbMappedShadowInfo);

	// 5) Shadow viewport/scissor
	m_d3dShadowViewport = { 0.0f, 0.0f, (float)SHADOW_MAP_SIZE, (float)SHADOW_MAP_SIZE, 0.0f, 1.0f };
	m_d3dShadowScissorRect = { 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };
}

void CScene::UpdateShadowShaderVariables(const XMFLOAT3& xmf3LightDir, const XMFLOAT3& xmf3FocusPos)
{
	XMVECTOR vLightDir = XMVector3Normalize(XMLoadFloat3(&xmf3LightDir));
	XMVECTOR vTarget = XMLoadFloat3(&xmf3FocusPos);

	XMVECTOR vLightPos = vTarget - vLightDir * 200.0f;

	XMMATRIX mtxLightView = XMMatrixLookAtLH(
		vLightPos,
		vTarget,
		XMVectorSet(0, 1, 0, 0)
	);
	float shadowArea = 3000.0f;
	XMMATRIX mtxLightProj = XMMatrixOrthographicLH(shadowArea, shadowArea, 1.0f, 6000.0f);

	XMMATRIX mtxLightViewProj = mtxLightView * mtxLightProj;

	XMStoreFloat4x4(&m_pcbMappedShadowInfo->m_xmf4x4LightViewProj,
		XMMatrixTranspose(mtxLightViewProj));
}
void CScene::RenderShadowMap(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (!m_pd3dShadowMap || !m_pd3dShadowDsvHeap || !m_pShadowMapShader) return;

	if (m_ShadowMapState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pd3dShadowMap,
			m_ShadowMapState,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);
		pd3dCommandList->ResourceBarrier(1, &barrier);
		m_ShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	pd3dCommandList->RSSetViewports(1, &m_d3dShadowViewport);
	pd3dCommandList->RSSetScissorRects(1, &m_d3dShadowScissorRect);

	pd3dCommandList->OMSetRenderTargets(0, nullptr, FALSE, &m_d3dShadowDsvCPUHandle);

	// (4) Clear
	pd3dCommandList->ClearDepthStencilView(m_d3dShadowDsvCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// (5) RootSignature/DescriptorHeap
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	m_pShadowMapShader->OnPrepareRender(pd3dCommandList, 0);

	pd3dCommandList->SetGraphicsRootConstantBufferView(14, m_pd3dcbShadowInfo->GetGPUVirtualAddress());


	if (m_pPlayer)
	{
		m_pPlayer->CGameObject::Render(pd3dCommandList, nullptr, true, 1);
	}
	for (int i = 0; i < m_nGameObjects; i++)
	{
		if (m_ppGameObjects[i])
			m_ppGameObjects[i]->Render(pd3dCommandList, nullptr, true, 1);
	}

	for (int i = 0; i < m_nShaders; i++)
	{
		CObjectsShader* pObjShader = dynamic_cast<CObjectsShader*>(m_ppShaders[i]);
		if (!pObjShader) continue;

		int nObjects = pObjShader->GetNumberOfObjects();
		for (int j = 0; j < nObjects; j++)
		{
			CGameObject* pObj = pObjShader->GetObject(j);
			if (pObj) pObj->Render(pd3dCommandList, nullptr, true, 1);
		}
	}

	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pd3dShadowMap,
			m_ShadowMapState,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		pd3dCommandList->ResourceBarrier(1, &barrier);
		m_ShadowMapState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
}
void CScene::ReleaseShadowResources()
{
	if (m_pd3dcbShadowInfo)
	{
		m_pd3dcbShadowInfo->Unmap(0, NULL);
		m_pd3dcbShadowInfo->Release();
		m_pd3dcbShadowInfo = NULL;
		m_pcbMappedShadowInfo = NULL;
	}

	if (m_pd3dShadowDsvHeap)
	{
		m_pd3dShadowDsvHeap->Release();
		m_pd3dShadowDsvHeap = NULL;
	}

	if (m_pd3dShadowMap)
	{
		m_pd3dShadowMap->Release();
		m_pd3dShadowMap = NULL;
	}
}


//////////////////////
void CScene::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress); //Lights

	if (m_pSkyBox) m_pSkyBox->Render(pd3dCommandList, pCamera);
	if (m_pTerrain) m_pTerrain->Render(pd3dCommandList, pCamera);

	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Render(pd3dCommandList, pCamera);
	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->Render(pd3dCommandList, pCamera);
}

