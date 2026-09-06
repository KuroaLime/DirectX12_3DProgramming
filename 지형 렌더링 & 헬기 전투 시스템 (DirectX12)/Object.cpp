//-----------------------------------------------------------------------------
// File: CGameObject.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Object.h"
#include "Shader.h"
#include "Scene.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CTexture::CTexture(int nTextures, UINT nTextureType, int nSamplers, int nRootParameters)
{
	m_nTextureType = nTextureType;

	m_nTextures = nTextures;
	if (m_nTextures > 0)
	{
		m_ppd3dTextureUploadBuffers = new ID3D12Resource * [m_nTextures];
		m_ppd3dTextures = new ID3D12Resource * [m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_ppd3dTextureUploadBuffers[i] = m_ppd3dTextures[i] = NULL;

		m_ppstrTextureNames = new _TCHAR[m_nTextures][64];
		for (int i = 0; i < m_nTextures; i++) m_ppstrTextureNames[i][0] = '\0';

		m_pd3dSrvGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_pd3dSrvGpuDescriptorHandles[i].ptr = NULL;

		m_pnResourceTypes = new UINT[m_nTextures];
		m_pdxgiBufferFormats = new DXGI_FORMAT[m_nTextures];
		m_pnBufferElements = new int[m_nTextures];
	}
	m_nRootParameters = nRootParameters;
	if (nRootParameters > 0) m_pnRootParameterIndices = new int[nRootParameters];
	for (int i = 0; i < m_nRootParameters; i++) m_pnRootParameterIndices[i] = -1;

	m_nSamplers = nSamplers;
	if (m_nSamplers > 0) m_pd3dSamplerGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nSamplers];
}

CTexture::~CTexture()
{
	if (m_ppd3dTextures)
	{
		for (int i = 0; i < m_nTextures; i++) if (m_ppd3dTextures[i]) m_ppd3dTextures[i]->Release();
		delete[] m_ppd3dTextures;
	}

	if (m_ppstrTextureNames) delete[] m_ppstrTextureNames;

	if (m_pnResourceTypes) delete[] m_pnResourceTypes;
	if (m_pdxgiBufferFormats) delete[] m_pdxgiBufferFormats;
	if (m_pnBufferElements) delete[] m_pnBufferElements;

	if (m_pnRootParameterIndices) delete[] m_pnRootParameterIndices;
	if (m_pd3dSrvGpuDescriptorHandles) delete[] m_pd3dSrvGpuDescriptorHandles;

	if (m_pd3dSamplerGpuDescriptorHandles) delete[] m_pd3dSamplerGpuDescriptorHandles;
}

void CTexture::SetRootParameterIndex(int nIndex, UINT nRootParameterIndex)
{
	m_pnRootParameterIndices[nIndex] = nRootParameterIndex;
}

void CTexture::SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle)
{
	m_pd3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;
}

void CTexture::SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle)
{
	m_pd3dSamplerGpuDescriptorHandles[nIndex] = d3dSamplerGpuDescriptorHandle;
}

void CTexture::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_nRootParameters == m_nTextures)
	{
		for (int i = 0; i < m_nRootParameters; i++)
		{
			if (m_pd3dSrvGpuDescriptorHandles[i].ptr && (m_pnRootParameterIndices[i] != -1)) pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[i], m_pd3dSrvGpuDescriptorHandles[i]);
		}
	}
	else
	{
		if (m_pd3dSrvGpuDescriptorHandles[0].ptr) pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[0], m_pd3dSrvGpuDescriptorHandles[0]);
	}
}

void CTexture::UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex)
{
	pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[nParameterIndex], m_pd3dSrvGpuDescriptorHandles[nTextureIndex]);
}

void CTexture::ReleaseShaderVariables()
{
}

void CTexture::ReleaseUploadBuffers()
{
	if (m_ppd3dTextureUploadBuffers)
	{
		for (int i = 0; i < m_nTextures; i++) if (m_ppd3dTextureUploadBuffers[i]) m_ppd3dTextureUploadBuffers[i]->Release();
		delete[] m_ppd3dTextureUploadBuffers;
		m_ppd3dTextureUploadBuffers = NULL;
	}
}

void CTexture::LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, wchar_t* pszFileName, UINT nResourceType, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = nResourceType;
	m_ppd3dTextures[nIndex] = ::CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, pszFileName, &m_ppd3dTextureUploadBuffers[nIndex], D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
}

void CTexture::LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = RESOURCE_BUFFER;
	m_pdxgiBufferFormats[nIndex] = ndxgiFormat;
	m_pnBufferElements[nIndex] = nElements;
	m_ppd3dTextures[nIndex] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pData, nElements * nStride, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, &m_ppd3dTextureUploadBuffers[nIndex]);
}

ID3D12Resource* CTexture::CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue)
{
	m_pnResourceTypes[nIndex] = nResourceType;
	m_ppd3dTextures[nIndex] = ::CreateTexture2DResource(pd3dDevice, pd3dCommandList, nWidth, nHeight, nElements, nMipLevels, dxgiFormat, d3dResourceFlags, d3dResourceStates, pd3dClearValue);
	return(m_ppd3dTextures[nIndex]);
}

int CTexture::LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pParent, FILE* pInFile, UINT nIndex)
{
	char pstrTextureName[64] = { '\0' };

	BYTE nStrLength = 64;
	UINT nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
	nReads = (UINT)::fread(pstrTextureName, sizeof(char), nStrLength, pInFile);
	pstrTextureName[nStrLength] = '\0';

	bool bDuplicated = false;
	bool bLoaded = false;
	if (strcmp(pstrTextureName, "null"))
	{
		bLoaded = true;
		char pstrFilePath[64] = { '\0' };
		strcpy_s(pstrFilePath, 64, "Model/Textures/");

		bDuplicated = (pstrTextureName[0] == '@');
		strcpy_s(pstrFilePath + 15, 64 - 15, (bDuplicated) ? (pstrTextureName + 1) : pstrTextureName);
		strcpy_s(pstrFilePath + 15 + ((bDuplicated) ? (nStrLength - 1) : nStrLength), 64 - 15 - ((bDuplicated) ? (nStrLength - 1) : nStrLength), ".dds");

		size_t nConverted = 0;
		mbstowcs_s(&nConverted, m_ppstrTextureNames[nIndex], 64, pstrFilePath, _TRUNCATE);

#define _WITH_DISPLAY_TEXTURE_NAME

#ifdef _WITH_DISPLAY_TEXTURE_NAME
		static int nTextures = 0, nRepeatedTextures = 0;
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("Texture Name: %d %c %s\n"), (pstrTextureName[0] == '@') ? nRepeatedTextures++ : nTextures++, (pstrTextureName[0] == '@') ? '@' : ' ', m_ppstrTextureNames[nIndex]);
		OutputDebugString(pstrDebug);
#endif
		if (!bDuplicated)
		{
			LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, m_ppstrTextureNames[nIndex], RESOURCE_TEXTURE2D, nIndex);
			//CScene::CreateShaderResourceView(pd3dDevice, this, nIndex, 3);
		}
		else
		{
			if (pParent)
			{
				CGameObject* pRootGameObject = pParent;
				while (pRootGameObject)
				{
					if (!pRootGameObject->m_pParent) break;
					pRootGameObject = pRootGameObject->m_pParent;
				}
				D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle;
				int nParameterIndex = pRootGameObject->FindReplicatedTexture(m_ppstrTextureNames[nIndex], &d3dSrvGpuDescriptorHandle);
				if (nParameterIndex >= 0)
				{
					m_pd3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;
					m_pnRootParameterIndices[nIndex] = nParameterIndex;
				}
			}
		}
	}
	return(bLoaded);
}

D3D12_SHADER_RESOURCE_VIEW_DESC CTexture::GetShaderResourceViewDesc(int nIndex)
{
	ID3D12Resource* pShaderResource = GetResource(nIndex);
	D3D12_RESOURCE_DESC d3dResourceDesc = pShaderResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
	d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	int nTextureType = GetTextureType(nIndex);
	switch (nTextureType)
	{
	case RESOURCE_TEXTURE2D: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 1)
	case RESOURCE_TEXTURE2D_ARRAY: //[]
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		d3dShaderResourceViewDesc.Texture2D.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2D.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_TEXTURE2DARRAY: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize != 1)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		d3dShaderResourceViewDesc.Texture2DArray.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		d3dShaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
		break;
	case RESOURCE_TEXTURE_CUBE: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 6)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		d3dShaderResourceViewDesc.TextureCube.MipLevels = 1;
		d3dShaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_BUFFER: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		d3dShaderResourceViewDesc.Format = m_pdxgiBufferFormats[nIndex];
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dShaderResourceViewDesc.Buffer.FirstElement = 0;
		d3dShaderResourceViewDesc.Buffer.NumElements = m_pnBufferElements[nIndex];
		d3dShaderResourceViewDesc.Buffer.StructureByteStride = 0;
		d3dShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return(d3dShaderResourceViewDesc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CMaterial::CMaterial()
{
}

CMaterial::~CMaterial()
{
	if (m_pTexture) m_pTexture->Release();
	//if (m_pShader) m_pShader->Release();
}

void CMaterial::SetShader(CShader *pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;
	if (m_pShader) m_pShader->AddRef();
}

void CMaterial::SetTexture(CTexture* pTexture)
{
	if (m_pTexture) m_pTexture->Release();
	m_pTexture = pTexture;
	if (m_pTexture) m_pTexture->AddRef();
}

void CMaterial::ReleaseUploadBuffers()
{
	//if (m_pShader) m_pShader->ReleaseUploadBuffers();
	if (m_pTexture) m_pTexture->ReleaseUploadBuffers();
}

void CMaterial::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{


	if (m_pTexture) m_pTexture->UpdateShaderVariables(pd3dCommandList);
}

void CMaterial::ReleaseShaderVariables()
{
	if (m_pShader) m_pShader->ReleaseShaderVariables();
	if (m_pTexture) m_pTexture->ReleaseShaderVariables();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CGameObject::CGameObject()
{
	m_xmf4x4Transform = Matrix4x4::Identity();
	m_xmf4x4World = Matrix4x4::Identity();

	m_pd3dcbGameObject = NULL;
	m_pcbMappedGameObject = NULL;
	m_d3dCbvGpuDescriptorHandle.ptr = NULL;

	XMFLOAT3 zeroCenter(0.0f, 0.0f, 0.0f);
	XMFLOAT3 zeroExtents(0.0f, 0.0f, 0.0f);
	XMFLOAT4 identityRot(0.0f, 0.0f, 0.0f, 1.0f);

	m_xmLocalOOBB = BoundingOrientedBox(zeroCenter, zeroExtents, identityRot);
	m_xmWorldOOBB = m_xmLocalOOBB;
}

CGameObject::CGameObject(int nMeshes, int nMaterials) : CGameObject()
{
	m_nMeshes = nMeshes;
	m_ppMeshes = NULL;
	if (m_nMeshes > 0)
	{
		m_ppMeshes = new CMesh * [m_nMeshes];
		for (int i = 0; i < m_nMeshes; i++) {
			m_ppMeshes[i] = NULL;
		}
	}

	m_nMaterials = nMaterials;
	if (m_nMaterials > 0)
	{
		m_ppMaterials = new CMaterial * [m_nMaterials];
		for (int i = 0; i < m_nMaterials; i++) m_ppMaterials[i] = NULL;
	}
}

CGameObject::~CGameObject()
{
	if (m_pd3dcbGameObject)
	{
		m_pd3dcbGameObject->Unmap(0, NULL);
		m_pd3dcbGameObject->Release();
	}
	ReleaseShaderVariables();

	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->Release();
			m_ppMeshes[i] = NULL;
		}
		delete[] m_ppMeshes;
	}

	if (m_nMaterials > 0)
	{
		for (int i = 0; i < m_nMaterials; i++)
		{
			if (m_ppMaterials[i]) m_ppMaterials[i]->Release();
		}
	}
	if (m_ppMaterials) delete[] m_ppMaterials;
}

void CGameObject::AddRef() 
{ 
	m_nReferences++; 

	if (m_pSibling) m_pSibling->AddRef();
	if (m_pChild) m_pChild->AddRef();
}

void CGameObject::Release() 
{ 
	if (m_pSibling) m_pSibling->Release();
	if (m_pChild) m_pChild->Release();

	if (--m_nReferences <= 0) delete this; 
}

void CGameObject::SetChild(CGameObject *pChild)
{
	if (m_pChild)
	{
		if (pChild) pChild->m_pSibling = m_pChild->m_pSibling;
		m_pChild->m_pSibling = pChild;
	}
	else
	{
		m_pChild = pChild;
	}
	if (pChild)
	{
		pChild->m_pParent = this;
	}
}

void CGameObject::SetMesh(int nIndex, CMesh* pMesh)
{
	if (m_ppMeshes)
	{
		if (m_ppMeshes[nIndex]) m_ppMeshes[nIndex]->Release();
		m_ppMeshes[nIndex] = pMesh;
		if (pMesh) pMesh->AddRef();

		if (IsSetLocalOOBBFromMesh && nIndex == 0 && pMesh) m_xmLocalOOBB = pMesh->m_xmOOBB;
	}
}

void CGameObject::SetShader(int nMaterial, CShader *pShader)
{
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->SetShader(pShader);
}

void CGameObject::SetMaterial(int nMaterial, CMaterial *pMaterial)
{
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->Release();
	m_ppMaterials[nMaterial] = pMaterial;
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->AddRef();
}

void CGameObject::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (m_pSibling) m_pSibling->Animate(fTimeElapsed, pxmf4x4Parent);
	if (m_pChild) m_pChild->Animate(fTimeElapsed, &m_xmf4x4World);
}

CGameObject *CGameObject::FindFrame(char *pstrFrameName)
{
	CGameObject *pFrameObject = NULL;
	if (!strncmp(m_pstrFrameName, pstrFrameName, max(strlen(m_pstrFrameName), strlen(pstrFrameName)))) return(this);

	if (m_pSibling) if (pFrameObject = m_pSibling->FindFrame(pstrFrameName)) return(pFrameObject);
	if (m_pChild) if (pFrameObject = m_pChild->FindFrame(pstrFrameName)) return(pFrameObject);

	return(NULL);
}
void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState) 
{
	Render(pd3dCommandList, pCamera, false, nPipelineState);
}
void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bSkipMaterialShader, int nPipelineState)
{
	bRender = true;
	if (nPipelineState != 1 && !m_bSkipFrustum && pCamera && (m_xmWorldOOBB.Extents.x > 0.0f)) {
		if (!pCamera->GetFrustum().Intersects(m_xmWorldOOBB))
			bRender = false;
	}
	
	if(bRender)
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

			m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0]._11 = m_xmf4TextureTransformParams.x;
			m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0]._12 = m_xmf4TextureTransformParams.y;
			m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0]._13 = m_xmf4TextureTransformParams.z;
			m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0]._14 = m_xmf4TextureTransformParams.w;
		}
		if (m_d3dCbvGpuDescriptorHandle.ptr != NULL)
		{
			pd3dCommandList->SetGraphicsRootDescriptorTable(m_nCbvRootParameterIndex, m_d3dCbvGpuDescriptorHandle);
		}
		XMMATRIX xmmtxParentWorld = XMLoadFloat4x4(&m_xmf4x4World);
		if (m_nMaterials > 1)
		{
			for (int i = 0; i < m_nMaterials; i++)
			{
				if (m_ppMaterials[i])
				{
					if (!bSkipMaterialShader && m_ppMaterials[i]->m_pShader) m_ppMaterials[i]->m_pShader->Render(pd3dCommandList, pCamera, nPipelineState);
					m_ppMaterials[i]->UpdateShaderVariables(pd3dCommandList);
				}

				if (m_nMeshes == 1)
				{
					if (m_ppMeshes[0]) m_ppMeshes[0]->Render(pd3dCommandList, i);
				}
			}
		}
		else
		{
			if ((m_nMaterials == 1) && (m_ppMaterials[0]))
			{
				if (!bSkipMaterialShader && m_ppMaterials[0]->m_pShader) m_ppMaterials[0]->m_pShader->Render(pd3dCommandList, pCamera, nPipelineState);
				m_ppMaterials[0]->UpdateShaderVariables(pd3dCommandList);
			}

			if (m_ppMeshes)
			{
				for (int i = 0; i < m_nMeshes; i++)
				{
					if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList, 0);
				}
			}
		}

		
		
	}
	if (m_pChild) m_pChild->Render(pd3dCommandList, pCamera, bSkipMaterialShader, nPipelineState);
	if (m_pSibling) m_pSibling->Render(pd3dCommandList, pCamera, bSkipMaterialShader,nPipelineState);
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	if (m_pcbMappedGameObject && m_nMaterials > 0 && m_ppMaterials[0])
	{
		XMFLOAT4X4 xmf4x4World;
		XMStoreFloat4x4(&xmf4x4World,
			XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
		m_pcbMappedGameObject->m_xmf4x4World = xmf4x4World;

		CMaterial* pMaterial = m_ppMaterials[0];

		m_pcbMappedGameObject->m_Material.m_cAmbient = pMaterial->m_xmf4AmbientColor;
		m_pcbMappedGameObject->m_Material.m_cDiffuse = pMaterial->m_xmf4AlbedoColor;
		m_pcbMappedGameObject->m_Material.m_cSpecular = pMaterial->m_xmf4SpecularColor;
		m_pcbMappedGameObject->m_Material.m_cEmissive = pMaterial->m_xmf4EmissiveColor;
		m_pcbMappedGameObject->m_nTexturesMask = pMaterial->m_nType;

		m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0]._11 = m_xmf4TextureTransformParams.x;
		m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0]._12 = m_xmf4TextureTransformParams.y;
		m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0]._13 = m_xmf4TextureTransformParams.z;
		m_pcbMappedGameObject->m_pxmf4x4TextureTransforms[0]._14 = m_xmf4TextureTransformParams.w;
	}

	if (m_d3dCbvGpuDescriptorHandle.ptr != NULL)
	{
		pd3dCommandList->SetGraphicsRootDescriptorTable(
			m_nCbvRootParameterIndex,      
			m_d3dCbvGpuDescriptorHandle);   
	}
}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World)
{
	if (pxmf4x4World) m_xmf4x4World = *pxmf4x4World;
	UpdateShaderVariables(pd3dCommandList);
}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, CMaterial *pMaterial)
{
	if (pMaterial) m_ppMaterials[0] = pMaterial;
	UpdateShaderVariables(pd3dCommandList);
}

void CGameObject::ReleaseShaderVariables()
{
}

void CGameObject::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_nMeshes; i++)
	{
		if (m_ppMeshes[i]) m_ppMeshes[i]->ReleaseUploadBuffers();
	}

	for (int i = 0; i < m_nMaterials; i++)
	{
		if (m_ppMaterials[i]) m_ppMaterials[i]->ReleaseUploadBuffers();
	}

	if (m_pSibling) m_pSibling->ReleaseUploadBuffers();
	if (m_pChild) m_pChild->ReleaseUploadBuffers();
}

void CGameObject::UpdateTransform(XMFLOAT4X4 *pxmf4x4Parent)
{
	m_xmf4x4World = (pxmf4x4Parent) ? Matrix4x4::Multiply(m_xmf4x4Transform, *pxmf4x4Parent) : m_xmf4x4Transform;

	XMMATRIX xmWorld = XMLoadFloat4x4(&m_xmf4x4World);
	m_xmLocalOOBB.Transform(m_xmWorldOOBB, xmWorld);
	m_xmLocalSphere.Transform(m_xmWorldSphere, xmWorld);


	//UpdateBoundingBox();
	if (m_pSibling) m_pSibling->UpdateTransform(pxmf4x4Parent);
	if (m_pChild) m_pChild->UpdateTransform(&m_xmf4x4World);
}

void CGameObject::BuildBroadphaseSphere()
{
	if (m_pChild) m_pChild->BuildBroadphaseSphere();
	if (m_pSibling) m_pSibling->BuildBroadphaseSphere();

	if (m_pChild)
	{

		BoundingSphere childSphereInParentSpace;
		XMMATRIX xmmtxChildTransform = XMLoadFloat4x4(&m_pChild->m_xmf4x4Transform);
		m_pChild->m_xmLocalSphere.Transform(m_xmLocalSphere, xmmtxChildTransform);

		CGameObject* pChild = m_pChild->m_pSibling;
		while (pChild)
		{
			xmmtxChildTransform = XMLoadFloat4x4(&pChild->m_xmf4x4Transform);
			pChild->m_xmLocalSphere.Transform(childSphereInParentSpace, xmmtxChildTransform);

			BoundingSphere::CreateMerged(m_xmLocalSphere, m_xmLocalSphere, childSphereInParentSpace);

			pChild = pChild->m_pSibling;
		}
	}
	else
	{
		XMFLOAT3 arrCorners[8];
		m_xmLocalOOBB.GetCorners(arrCorners);

		BoundingSphere::CreateFromPoints(m_xmLocalSphere, 8, arrCorners, sizeof(XMFLOAT3));
	}

}
void CGameObject::CalculateCompositeOOBB()
{
	XMFLOAT3 minPos(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 maxPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	bool bHasBounds = false;

	if (IsSetLocalOOBBFromMesh)
	{
		if (m_ppMeshes && m_ppMeshes[0])
		{

			XMFLOAT3 arrCorners[8];
			m_ppMeshes[0]->m_xmOOBB.GetCorners(arrCorners);
			for (int i = 0; i < 8; ++i)
			{
				minPos.x = (std::min)(minPos.x, arrCorners[i].x);
				minPos.y = (std::min)(minPos.y, arrCorners[i].y);
				minPos.z = (std::min)(minPos.z, arrCorners[i].z);
				maxPos.x = (std::max)(maxPos.x, arrCorners[i].x);
				maxPos.y = (std::max)(maxPos.y, arrCorners[i].y);
				maxPos.z = (std::max)(maxPos.z, arrCorners[i].z);
			}
			bHasBounds = true;
		}
	}
	else if (m_xmLocalOOBB.Extents.x > 0 || m_xmLocalOOBB.Extents.y > 0 || m_xmLocalOOBB.Extents.z > 0)
	{
		XMFLOAT3 arrCorners[8];
		m_xmLocalOOBB.GetCorners(arrCorners);

		for (int i = 0; i < 8; ++i)
		{
			minPos.x = (std::min)(minPos.x, arrCorners[i].x);
			minPos.y = (std::min)(minPos.y, arrCorners[i].y);
			minPos.z = (std::min)(minPos.z, arrCorners[i].z);
			maxPos.x = (std::max)(maxPos.x, arrCorners[i].x);
			maxPos.y = (std::max)(maxPos.y, arrCorners[i].y);
			maxPos.z = (std::max)(maxPos.z, arrCorners[i].z);
		}
		bHasBounds = true;
	}
	CGameObject* pChild = m_pChild;
	while (pChild)
	{
		BoundingOrientedBox childOOBBInParentSpace;
		XMMATRIX xmmtxChildTransform = XMLoadFloat4x4(&pChild->m_xmf4x4Transform);
		pChild->m_xmLocalOOBB.Transform(childOOBBInParentSpace, xmmtxChildTransform);

		XMFLOAT3 arrChildCorners[8];
		childOOBBInParentSpace.GetCorners(arrChildCorners);

		for (int i = 0; i < 8; ++i)
		{
			minPos.x = (std::min)(minPos.x, arrChildCorners[i].x);
			minPos.y = (std::min)(minPos.y, arrChildCorners[i].y);
			minPos.z = (std::min)(minPos.z, arrChildCorners[i].z);
			maxPos.x = (std::max)(maxPos.x, arrChildCorners[i].x);
			maxPos.y = (std::max)(maxPos.y, arrChildCorners[i].y);
			maxPos.z = (std::max)(maxPos.z, arrChildCorners[i].z);
		}
		bHasBounds = true; 

		pChild = pChild->m_pSibling;
	}

	if (bHasBounds)
	{
		XMFLOAT3 center;
		center.x = (minPos.x + maxPos.x) * 0.5f;
		center.y = (minPos.y + maxPos.y) * 0.5f;
		center.z = (minPos.z + maxPos.z) * 0.5f;

		XMFLOAT3 extents;
		extents.x = (maxPos.x - minPos.x) * 0.5f;
		extents.y = (maxPos.y - minPos.y) * 0.5f;
		extents.z = (maxPos.z - minPos.z) * 0.5f;

		m_xmBigOOGG.Center = center;
		m_xmBigOOGG.Extents = extents;
		m_xmBigOOGG.Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); 
	}
	else
	{
		m_xmBigOOGG.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_xmBigOOGG.Extents = XMFLOAT3(0.0f, 0.0f, 0.0f); 
		m_xmBigOOGG.Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}
void CGameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4Transform._41 = x;
	m_xmf4x4Transform._42 = y;
	m_xmf4x4Transform._43 = z;

	 
	UpdateTransform(NULL);
}

void CGameObject::SetPosition(XMFLOAT3 xmf3Position)
{
	SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);
}

void CGameObject::SetScale(float x, float y, float z)
{
	XMMATRIX mtxScale = XMMatrixScaling(x, y, z);
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxScale, m_xmf4x4Transform);

	UpdateTransform(NULL);
}

XMFLOAT3 CGameObject::GetPosition()
{
	return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
}

XMFLOAT3 CGameObject::GetLook()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33)));
}

XMFLOAT3 CGameObject::GetUp()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23)));
}

XMFLOAT3 CGameObject::GetRight()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13)));
}

void CGameObject::MoveStrafe(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Right = GetRight();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Right, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::MoveUp(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Up = GetUp();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Up, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::MoveForward(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Look = GetLook();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Look, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);

	UpdateTransform(NULL);
}

void CGameObject::Rotate(XMFLOAT3 *pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);

	UpdateTransform(NULL);
}

void CGameObject::Rotate(XMFLOAT4 *pxmf4Quaternion)
{
	XMMATRIX mtxRotate = XMMatrixRotationQuaternion(XMLoadFloat4(pxmf4Quaternion));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);

	UpdateTransform(NULL);
}

//#define _WITH_DEBUG_FRAME_HIERARCHY

int CGameObject::FindReplicatedTexture(_TCHAR* pstrTextureName, D3D12_GPU_DESCRIPTOR_HANDLE* pd3dSrvGpuDescriptorHandle)
{
	int nParameterIndex = -1;

	for (int i = 0; i < m_nMaterials; i++)
	{
		if (m_ppMaterials[i] && m_ppMaterials[i]->m_pTexture)
		{
			int nTextures = m_ppMaterials[i]->m_pTexture->GetTextures();
			for (int j = 0; j < nTextures; j++)
			{
				if (!_tcsncmp(m_ppMaterials[i]->m_pTexture->GetTextureName(j), pstrTextureName, _tcslen(pstrTextureName)))
				{
					*pd3dSrvGpuDescriptorHandle = m_ppMaterials[i]->m_pTexture->GetGpuDescriptorHandle(j);
					nParameterIndex = m_ppMaterials[i]->m_pTexture->GetRootParameter(j);
					return(nParameterIndex);
				}
			}
		}
	}
	if (m_pSibling) if ((nParameterIndex = m_pSibling->FindReplicatedTexture(pstrTextureName, pd3dSrvGpuDescriptorHandle)) > 0) return(nParameterIndex);
	if (m_pChild) if ((nParameterIndex = m_pChild->FindReplicatedTexture(pstrTextureName, pd3dSrvGpuDescriptorHandle)) > 0) return(nParameterIndex);

	return(nParameterIndex);
}

void CGameObject::LoadMaterialsFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CGameObject *pParent, FILE *pInFile, CShader* pShader)
{
	char pstrToken[64] = { '\0' };

	int nMaterial = 0;
	BYTE nStrLength = 0;

	UINT nReads = (UINT)::fread(&m_nMaterials, sizeof(int), 1, pInFile);

	m_ppMaterials = new CMaterial*[m_nMaterials];
	for (int i = 0; i < m_nMaterials; i++) m_ppMaterials[i] = NULL;

	CMaterial *pMaterial = NULL;
	CTexture* pTexture = NULL;

	for ( ; ; )
	{
		nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
		nReads = (UINT)::fread(pstrToken, sizeof(char), nStrLength, pInFile); 
		pstrToken[nStrLength] = '\0';

		if (!strcmp(pstrToken, "<Material>:"))
		{
			nReads = (UINT)::fread(&nMaterial, sizeof(int), 1, pInFile);

			pMaterial = new CMaterial(); 
			pTexture = new CTexture(7, RESOURCE_TEXTURE2D, 0, 7); //0:Albedo, 1:Specular, 2:Metallic, 3:Normal, 4:Emission, 5:DetailAlbedo, 6:DetailNormal
			pMaterial->SetTexture(pTexture);
			if (!pShader) 
			{
				pShader = new CStandardShader();
				pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
				pMaterial->SetShader(pShader);
			}
			SetMaterial(nMaterial, pMaterial);

			UINT nMeshType = GetMeshType(0);
		}
		else if (!strcmp(pstrToken, "<AlbedoColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4AlbedoColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<EmissiveColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4EmissiveColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<SpecularColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4SpecularColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<Glossiness>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fGlossiness), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Smoothness>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fSmoothness), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Metallic>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fSpecularHighlight), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<SpecularHighlight>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fMetallic), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<GlossyReflection>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fGlossyReflection), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<AlbedoMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, 0)) {
				if (pTexture->GetGpuDescriptorHandle(0).ptr == 0)
				{
					if (m_pOwnerScene)
						m_pOwnerScene->CreateShaderResourceView(pd3dDevice, pTexture, 0, /*rootStartIndex*/ 3);
				}
				pMaterial->SetMaterialType(MATERIAL_ALBEDO_MAP);
			}
		}
		else if (!strcmp(pstrToken, "<SpecularMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, 1)) {
				if (pTexture->GetGpuDescriptorHandle(1).ptr == 0)
				{
					if (m_pOwnerScene)
						m_pOwnerScene->CreateShaderResourceView(pd3dDevice, pTexture, 1, /*rootStartIndex*/ 3);
				}
				pMaterial->SetMaterialType(MATERIAL_SPECULAR_MAP);
			}
		}
		else if (!strcmp(pstrToken, "<NormalMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, 2)) {
				if (pTexture->GetGpuDescriptorHandle(2).ptr == 0)
				{
					if (m_pOwnerScene)
						m_pOwnerScene->CreateShaderResourceView(pd3dDevice, pTexture, 2, /*rootStartIndex*/ 3);
				}
				pMaterial->SetMaterialType(MATERIAL_NORMAL_MAP);
			}
		}
		else if (!strcmp(pstrToken, "<MetallicMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, 3)) {
				if (pTexture->GetGpuDescriptorHandle(3).ptr == 0)
				{
					if (m_pOwnerScene)
						m_pOwnerScene->CreateShaderResourceView(pd3dDevice, pTexture, 3, /*rootStartIndex*/ 3);
				}
				pMaterial->SetMaterialType(MATERIAL_METALLIC_MAP);
			}
		}
		else if (!strcmp(pstrToken, "<EmissionMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, 4)) {
				if (pTexture->GetGpuDescriptorHandle(4).ptr == 0)
				{
					if (m_pOwnerScene)
						m_pOwnerScene->CreateShaderResourceView(pd3dDevice, pTexture, 4, /*rootStartIndex*/ 3);
				}
				pMaterial->SetMaterialType(MATERIAL_EMISSION_MAP);
			}
		}
		else if (!strcmp(pstrToken, "<DetailAlbedoMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, 5)) {
				if (pTexture->GetGpuDescriptorHandle(5).ptr == 0)
				{
					if (m_pOwnerScene)
						m_pOwnerScene->CreateShaderResourceView(pd3dDevice, pTexture, 5, /*rootStartIndex*/ 3);
				}
				pMaterial->SetMaterialType(MATERIAL_DETAIL_ALBEDO_MAP);
			}
		}
		else if (!strcmp(pstrToken, "<DetailNormalMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, 6)) {
				if (pTexture->GetGpuDescriptorHandle(6).ptr == 0)
				{
					if (m_pOwnerScene)
						m_pOwnerScene->CreateShaderResourceView(pd3dDevice, pTexture, 6, /*rootStartIndex*/ 3);
				}
				pMaterial->SetMaterialType(MATERIAL_DETAIL_NORMAL_MAP);
			}
		}
		else if (!strcmp(pstrToken, "</Materials>"))
		{
			break;
		}
	}
}
void CGameObject::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pd3dcbGameObject) return;
	UINT ncbElementBytes = (sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255;
	m_pd3dcbGameObject = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbGameObject->Map(0, NULL, (void**)&m_pcbMappedGameObject);
	if (m_pOwnerScene)
	{
		m_d3dCbvGpuDescriptorHandle = m_pOwnerScene->CreateConstantBufferView(pd3dDevice, m_pd3dcbGameObject->GetGPUVirtualAddress(), ncbElementBytes);
	}
	else
	{
		m_d3dCbvGpuDescriptorHandle.ptr = NULL;
	}
	if (m_pSibling) m_pSibling->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	if (m_pChild) m_pChild->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}
CGameObject *CGameObject::LoadFrameHierarchyFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CGameObject *pParent, FILE *pInFile, CShader* pShader, CScene* pOwnerScene)
{
	char pstrToken[64] = { '\0' };

	BYTE nStrLength = 0;
	UINT nReads = 0;

	int nFrame = 0, nTextures = 0;

	CGameObject *pGameObject = NULL;

	for ( ; ; )
	{
		nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
		nReads = (UINT)::fread(pstrToken, sizeof(char), nStrLength, pInFile);
		pstrToken[nStrLength] = '\0';

		if (!strcmp(pstrToken, "<Frame>:"))
		{
			pGameObject = new CGameObject(1, 1);
			if (pOwnerScene) pGameObject->SetOwnerScene(pOwnerScene);

			nReads = (UINT)::fread(&nFrame, sizeof(int), 1, pInFile);
			nReads = (UINT)::fread(&nTextures, sizeof(int), 1, pInFile);

			nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
			nReads = (UINT)::fread(pGameObject->m_pstrFrameName, sizeof(char), nStrLength, pInFile);
			pGameObject->m_pstrFrameName[nStrLength] = '\0';
		}
		else if (!strcmp(pstrToken, "<Transform>:"))
		{
			XMFLOAT3 xmf3Position, xmf3Rotation, xmf3Scale;
			XMFLOAT4 xmf4Rotation;
			nReads = (UINT)::fread(&xmf3Position, sizeof(float), 3, pInFile);
			nReads = (UINT)::fread(&xmf3Rotation, sizeof(float), 3, pInFile); //Euler Angle
			nReads = (UINT)::fread(&xmf3Scale, sizeof(float), 3, pInFile);
			nReads = (UINT)::fread(&xmf4Rotation, sizeof(float), 4, pInFile); //Quaternion
		}
		else if (!strcmp(pstrToken, "<TransformMatrix>:"))
		{
			nReads = (UINT)::fread(&pGameObject->m_xmf4x4Transform, sizeof(float), 16, pInFile);
		}
		else if (!strcmp(pstrToken, "<Mesh>:"))
		{
			CStandardMesh *pMesh = new CStandardMesh(pd3dDevice, pd3dCommandList);
			pMesh->LoadMeshFromFile(pd3dDevice, pd3dCommandList, pInFile);
			pGameObject->SetMesh(0, pMesh);
		}
		else if (!strcmp(pstrToken, "<Materials>:"))
		{
			pGameObject->LoadMaterialsFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<Children>:"))
		{
			int nChilds = 0;
			nReads = (UINT)::fread(&nChilds, sizeof(int), 1, pInFile);
			if (nChilds > 0)
			{
				for (int i = 0; i < nChilds; i++)
				{
					CGameObject *pChild = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pGameObject, pInFile, pShader, pOwnerScene);
					if (pChild) pGameObject->SetChild(pChild);
#ifdef _WITH_DEBUG_FRAME_HIERARCHY
					TCHAR pstrDebug[256] = { 0 };
					_stprintf_s(pstrDebug, 256, _T("(Frame: %p) (Parent: %p)\n"), pChild, pGameObject);
					OutputDebugString(pstrDebug);
#endif
				}
			}
		}
		else if (!strcmp(pstrToken, "</Frame>"))
		{

			if (pGameObject) {
				pGameObject->CalculateCompositeOOBB();
				pGameObject->BuildBroadphaseSphere();
			}
			break;
		}
	}
	return(pGameObject);
}

void CGameObject::PrintFrameInfo(CGameObject *pGameObject, CGameObject *pParent)
{
	TCHAR pstrDebug[256] = { 0 };

	_stprintf_s(pstrDebug, 256, _T("(Frame: %p) (Parent: %p)\n"), pGameObject, pParent);
	OutputDebugString(pstrDebug);

	if (pGameObject->m_pSibling) CGameObject::PrintFrameInfo(pGameObject->m_pSibling, pParent);
	if (pGameObject->m_pChild) CGameObject::PrintFrameInfo(pGameObject->m_pChild, pGameObject);
}

CGameObject *CGameObject::LoadGeometryFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, char *pstrFileName, CShader* pShader, CScene* pOwnerScene)
{
	FILE *pInFile = NULL;
	::fopen_s(&pInFile, pstrFileName, "rb");
	::rewind(pInFile);

	CGameObject *pGameObject = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, NULL, pInFile, pShader, pOwnerScene);
	if (pGameObject)
		pGameObject->CreateShaderVariables(pd3dDevice, pd3dCommandList);
#ifdef _WITH_DEBUG_FRAME_HIERARCHY

	CGameObject::PrintFrameInfo(pGameObject, NULL);
#endif

	return(pGameObject);
}

void CGameObject::SetLookAt(XMFLOAT3& xmf3Target, XMFLOAT3& xmf3Up)
{
	XMFLOAT3 xmf3Position(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43);
	XMFLOAT4X4 mtxLookAt = Matrix4x4::LookAtLH(xmf3Position, xmf3Target, xmf3Up);
	m_xmf4x4World._11 = mtxLookAt._11; m_xmf4x4World._12 = mtxLookAt._21; m_xmf4x4World._13 = mtxLookAt._31;
	m_xmf4x4World._21 = mtxLookAt._12; m_xmf4x4World._22 = mtxLookAt._22; m_xmf4x4World._23 = mtxLookAt._32;
	m_xmf4x4World._31 = mtxLookAt._13; m_xmf4x4World._32 = mtxLookAt._23; m_xmf4x4World._33 = mtxLookAt._33;
}
void CGameObject::UpdateBoundingBox()
{
	if (m_ppMeshes)
	{
		for(int i =0 ; i<m_nMeshes;i++)
		{
			m_xmLocalOOBB.Transform(m_xmWorldOOBB, XMLoadFloat4x4(&m_xmf4x4World));
			m_ppMeshes[i]->m_xmOOBB.Transform(m_ppMeshes[i]->m_xmOOBB, XMLoadFloat4x4(&m_xmf4x4World));
			XMStoreFloat4(&m_ppMeshes[i]->m_xmOOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&m_ppMeshes[i]->m_xmOOBB.Orientation)));
		}
	}
}
void CGameObject::GenerateRayForPicking(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection, XMMATRIX& xmmtxProjection)
{
	XMMATRIX xmmtxToModel = XMMatrixInverse(NULL, XMLoadFloat4x4(&m_xmf4x4World) * xmmtxView);
	XMFLOAT4X4 xmf4x4Proj;
	XMStoreFloat4x4(&xmf4x4Proj, xmmtxProjection);


	if (fabs(xmf4x4Proj._44 - 1.0f) < 0.001f)
	{
		XMVECTOR xmvRayOrigin_View = xmvPickPosition;
		XMVECTOR xmvRayDirection_View = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

		xmvPickRayOrigin = XMVector3TransformCoord(xmvRayOrigin_View, xmmtxToModel);
		xmvPickRayDirection = XMVector3TransformNormal(xmvRayDirection_View, xmmtxToModel);
		xmvPickRayDirection = XMVector3Normalize(xmvPickRayDirection);
	}
	else
	{
		XMFLOAT3 xmf3CameraOrigin(0.0f, 0.0f, 0.0f);
		xmvPickRayOrigin = XMVector3TransformCoord(XMLoadFloat3(&xmf3CameraOrigin), xmmtxToModel);
		xmvPickRayDirection = XMVector3TransformCoord(xmvPickPosition, xmmtxToModel);
		xmvPickRayDirection = XMVector3Normalize(xmvPickRayDirection-xmvPickRayOrigin);
	}
}
int CGameObject::PickOOBBByRayIntersection(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, float* pfHitDistance, XMMATRIX& xmmtxProjection)
{

	BOOL nIntersected = false;

	if (m_ppMeshes && m_ppMeshes[0])
	{
		XMVECTOR xmvPickRayOrigin, xmvPickRayDirection;
		GenerateRayForPicking(xmvPickPosition, xmmtxView, xmvPickRayOrigin, xmvPickRayDirection, xmmtxProjection);
		nIntersected = m_ppMeshes[0]->CheckRayIntersectionOOBB(xmvPickRayOrigin, xmvPickRayDirection, pfHitDistance);
		
	}
	return(nIntersected);
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CSkyBox::CSkyBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CScene* OwnerScene) : CGameObject(1, 1)
{
	m_bSkipFrustum = true;

	CSkyBoxMesh *pSkyBoxMesh = new CSkyBoxMesh(pd3dDevice, pd3dCommandList, 20.0f, 20.0f, 20.0f);
	SetMesh(0, pSkyBoxMesh);

	CTexture *pSkyBoxTexture = new CTexture(1, RESOURCE_TEXTURE_CUBE, 0, 1);
	pSkyBoxTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"SkyBox/SkyBox_0.dds", RESOURCE_TEXTURE_CUBE, 0);
	OwnerScene->CreateShaderResourceViews(pd3dDevice, pSkyBoxTexture, 0, 10);

	CSkyBoxShader *pSkyBoxShader = new CSkyBoxShader();
	pSkyBoxShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	pSkyBoxShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CMaterial *pSkyBoxMaterial = new CMaterial();
	pSkyBoxMaterial->SetTexture(pSkyBoxTexture);
	pSkyBoxMaterial->SetShader(pSkyBoxShader);

	SetMaterial(0, pSkyBoxMaterial);
}

CSkyBox::~CSkyBox()
{
}

void CSkyBox::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState) 
{
	XMFLOAT3 xmf3CameraPos = pCamera->GetPosition();
	SetPosition(xmf3CameraPos.x, xmf3CameraPos.y, xmf3CameraPos.z);
	CGameObject::Render(pd3dCommandList, pCamera, nPipelineState); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
CSuperCobraObject::CSuperCobraObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature) : CGameObject(1, 0)
{
	m_nCbvRootParameterIndex = 1;

}

CSuperCobraObject::~CSuperCobraObject()
{
}

void CSuperCobraObject::PrepareAnimate()
{
	m_pMainRotorFrame = FindFrame("MainRotor");
	m_pTailRotorFrame = FindFrame("TailRotor");
}

void CSuperCobraObject::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (m_pMainRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pMainRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pMainRotorFrame->m_xmf4x4Transform);
	}
	if (m_pTailRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pTailRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTailRotorFrame->m_xmf4x4Transform);
	}

	CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
CGunshipObject::CGunshipObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature) : CGameObject(0, 0)
{
	m_nCbvRootParameterIndex = 1;

}

CGunshipObject::~CGunshipObject()
{
}

void CGunshipObject::PrepareAnimate()
{
	m_pMainRotorFrame = FindFrame("Rotor");
	m_pTailRotorFrame = FindFrame("Back_Rotor");
}

void CGunshipObject::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (m_pMainRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pMainRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pMainRotorFrame->m_xmf4x4Transform);
	}
	if (m_pTailRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pTailRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTailRotorFrame->m_xmf4x4Transform);
	}

	CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
CMi24Object::CMi24Object(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature) : CGameObject(0, 0)
{
	m_nCbvRootParameterIndex = 1;

}

CMi24Object::~CMi24Object()
{
}

void CMi24Object::PrepareAnimate()
{
	m_pMainRotorFrame = FindFrame("Top_Rotor");
	m_pTailRotorFrame = FindFrame("Tail_Rotor");
}

void CMi24Object::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (m_pMainRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pMainRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pMainRotorFrame->m_xmf4x4Transform);
	}
	if (m_pTailRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pTailRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTailRotorFrame->m_xmf4x4Transform);
	}

	CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);
}


CMssileObject::CMssileObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CScene* pOwnerScene) : CGameObject(1, 1)
{
	m_nCbvRootParameterIndex = 1;


	m_bActive = false;
	m_bSkipFrustum = false;
	IsSetLocalOOBBFromMesh = true;
	SetLayer(EObjectLayer::PlayerBullet);
	SetCollidable(false);
	bRender = false;

	//pOwnerScene->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CMssileObject::~CMssileObject()
{
}
void CMssileObject::OrientToDirection(const XMFLOAT3& dir)
{
	XMVECTOR vDir = XMLoadFloat3(&dir);
	XMVECTOR vUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	if (fabsf(XMVectorGetY(vDir)) > 0.99f) vUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	XMMATRIX mtxLocalCorrection = XMMatrixRotationX(XMConvertToRadians(90.0f));

	XMMATRIX mtxLookTo = XMMatrixInverse(nullptr, XMMatrixLookToLH(XMVectorZero(), vDir, vUp));
	XMMATRIX mtxOrient = XMMatrixMultiply(mtxLocalCorrection, mtxLookTo);

	XMFLOAT3 xmf3Position = GetPosition();
	XMStoreFloat4x4(&m_xmf4x4Transform, mtxOrient);
	m_xmf4x4Transform._41 = xmf3Position.x;
	m_xmf4x4Transform._42 = xmf3Position.y;
	m_xmf4x4Transform._43 = xmf3Position.z;

	UpdateTransform(nullptr);
}

void CMssileObject::Fire(const XMFLOAT3& startPos, const XMFLOAT3& dir, CGameObject* pTarget)
{

	XMFLOAT3 normDir = dir;
	normDir = Vector3::Normalize(normDir);

	m_xmf3Direction = normDir;
	m_pTarget = pTarget;
	m_fLifeTime = 0.0f;
	m_bActive = true;

	SetPosition(startPos);

	bRender = true;
	SetCollidable(true);

	OrientToDirection(m_xmf3Direction);
}
void CMssileObject::Explode() 
{
	m_bActive = false;
	SetCollidable(false);
	bRender = false;

}
void CMssileObject::Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent) {
	if (!m_bActive) return;

	m_fLifeTime += fTimeElapsed;
	if (m_fLifeTime >= m_fMaxLifeTime)
	{
		Explode();
		return;
	}

	XMFLOAT3 pos = GetPosition();

	if (m_pTarget && m_pTarget->GetHealth() > 0.0f)
	{
		XMFLOAT3 targetPos = m_pTarget->GetPosition();
		XMFLOAT3 toTarget = Vector3::Subtract(targetPos, pos);

		if (Vector3::Length(toTarget) < 5.0f)
		{
			Explode();
			return;
		}

		XMFLOAT3 newDir = Vector3::Normalize(toTarget);
		m_xmf3Direction = newDir;
	}
	XMFLOAT3 shift = Vector3::ScalarProduct(m_xmf3Direction, m_fSpeed * fTimeElapsed, false);
	pos = Vector3::Add(pos, shift);
	SetPosition(pos.x, pos.y, pos.z);
	OrientToDirection(m_xmf3Direction);

	//CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);
}
void CMssileObject::Render(ID3D12GraphicsCommandList* cmd, CCamera* pCamera, bool bSkipMaterial, int nPipelineState)
{

	if (m_nMaterials > 0 && m_ppMaterials[0] && m_ppMaterials[0]->m_pShader)
	{
		OutputDebugStringA("Missile Render: shader bound for missile\n");
	}
	if (m_nMeshes > 0 && m_ppMeshes[0])
	{
		OutputDebugStringA("Missile Render: mesh is StandardMesh?\n");
	}
	CGameObject::Render(cmd, pCamera, bSkipMaterial, nPipelineState);
}
///////////////
CExplosionObject::CExplosionObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
	: CGameObject(1,1)
{
	m_bActive = false; 
	m_fElapsed = 0.0f;
}

CExplosionObject::~CExplosionObject() {}

void CExplosionObject::Reset(XMFLOAT3 xmf3Position, float fScale, float fDuration)
{
	m_bActive = true;
	m_fElapsed = 0.0f;
	m_fDuration = fDuration;
	m_bSkipFrustum = true;
	SetPosition(xmf3Position);
	SetScale(fScale, fScale, fScale); 
}

void CExplosionObject::Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent)
{
	if (!m_bActive) return;
	m_fElapsed += fTimeElapsed;
	if (m_fElapsed >= m_fDuration)
	{
		m_bActive = false;
		return;
	}
	const int totalFrames = 7; 
	float progress = m_fElapsed / m_fDuration;
	int frame = min((int)(progress * totalFrames), totalFrames - 1);
	float UScale = 1.0f / 7.0f;
	float VScale = 1.0f;    
	float UOffset = frame * UScale;
	float VOffset = 0.0f;
	SetTextureTransformParams(XMFLOAT4(UOffset, VOffset, UScale, VScale));
}
