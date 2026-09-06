#include "BattleShader.h"
#include "Scene.h"
#include <random>
CBattleShader::CBattleShader()
{
}

CBattleShader::~CBattleShader()
{
}
float GetRandomFloat(float min, float max)
{

	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(min, max);

	return dis(gen);
}
int GetRandomInt(int min, int max)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(min, max);

	return dis(gen);
}
D3D12_SHADER_BYTECODE CBattleShader::CreateVertexShader()
{
	return CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSStandardInstanced",
		"vs_5_1", &m_pd3dVertexShaderBlob);
}
void CBattleShader::CreateInstanceBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	CScene* pOwnerScene)
{
	if (!pOwnerScene) return;
	m_nInstances = m_nObjects;

	UINT bufferSize = sizeof(BATTLE_INSTANCE_DATA) * m_nInstances;

	m_pd3dInstanceBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList,
		NULL, bufferSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL);

	m_pd3dInstanceBuffer->Map(0, NULL, (void**)&m_pMappedInstanceData);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_nInstances;
	srvDesc.Buffer.StructureByteStride = sizeof(BATTLE_INSTANCE_DATA);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	CDescriptorHeap* pHeap = pOwnerScene->m_pDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pHeap->m_d3dSrvCPUDescriptorNextHandle;
	m_d3dInstanceSrvGpuHandle = pHeap->m_d3dSrvGPUDescriptorNextHandle;

	pd3dDevice->CreateShaderResourceView(m_pd3dInstanceBuffer, &srvDesc, cpuHandle);

	pHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	pHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
}
void CBattleShader::UpdateInstanceBuffer(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (!m_pMappedInstanceData) return;

	for (int i = 0; i < m_nObjects; ++i)
	{
		CGameObject* pObj = m_ppObjects[i];
		if (pObj)
		{
			XMFLOAT4X4 world = pObj->GetWorldMatrix();
			XMMATRIX xmWorld = XMLoadFloat4x4(&world);

			xmWorld = XMMatrixTranspose(xmWorld);
			XMStoreFloat4x4(&m_pMappedInstanceData[i].World, xmWorld);
		}
	}
}
CGameObject* CBattleShader::CloneGameObjectHierarchy(CGameObject* pSrc)
{
	if (!pSrc) return NULL;

	CGameObject* pDest = new CGameObject(pSrc->m_nMeshes, pSrc->m_nMaterials);

	pDest->m_xmf4x4Transform = pSrc->m_xmf4x4Transform;
	pDest->m_xmLocalOOBB = pSrc->m_xmLocalOOBB;
	pDest->m_xmLocalSphere = pSrc->m_xmLocalSphere;
	pDest->m_xmBigOOGG = pSrc->m_xmBigOOGG;
	pDest->IsSetLocalOOBBFromMesh = pSrc->IsSetLocalOOBBFromMesh;

	for (int i = 0; i < pSrc->m_nMeshes; ++i)
	{
		if (pSrc->m_ppMeshes[i])
		{
			pDest->SetMesh(i, pSrc->m_ppMeshes[i]);
		}
	}

	if (pSrc->m_pChild)
	{
		pDest->SetChild(CloneGameObjectHierarchy(pSrc->m_pChild));
	}
	if (pSrc->m_pSibling)
	{
		pDest->m_pSibling = CloneGameObjectHierarchy(pSrc->m_pSibling);
	}

	return pDest;
}

void RenderModelInstancedRecursive_Shadow(CGameObject* pModelNode, ID3D12GraphicsCommandList* pd3dCommandList, UINT nInstances)
{
	if (!pModelNode) return;

	pModelNode->UpdateShaderVariables(pd3dCommandList);

	for (int i = 0; i < pModelNode->m_nMeshes; ++i)
	{
		if (pModelNode->m_ppMeshes[i])
			pModelNode->m_ppMeshes[i]->RenderInstanced(pd3dCommandList, i, nInstances);
	}

	if (pModelNode->m_pSibling) RenderModelInstancedRecursive_Shadow(pModelNode->m_pSibling, pd3dCommandList, nInstances);
	if (pModelNode->m_pChild)   RenderModelInstancedRecursive_Shadow(pModelNode->m_pChild, pd3dCommandList, nInstances);
}

void CBattleShader::RenderShadowInstanced(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_nInstances == 0 || !m_pMi24Model) return;

	UpdateInstanceBuffer(pd3dCommandList);

	pd3dCommandList->SetGraphicsRootDescriptorTable(13, m_d3dInstanceSrvGpuHandle);

	RenderModelInstancedRecursive_Shadow(m_pMi24Model, pd3dCommandList, m_nInstances);
}
void CBattleShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext, CScene* pOwnerScene)
{
	m_nObjects = 10;
	m_ppObjects = new CGameObject * [m_nObjects];

	m_pMi24Model = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Mi24.bin", this, pOwnerScene);
	m_pMi24Model->IsSetLocalOOBBFromMesh = true;
	m_pMi24Model->SetLayer(EObjectLayer::Enemy);
	if (m_pMi24Model)
	{
		m_pMi24Model->BuildBroadphaseSphere();

		m_pModelMainRotor = m_pMi24Model->FindFrame("Top_Rotor");
		m_pModelTailRotor = m_pMi24Model->FindFrame("Tail_Rotor");
	}
	float fMapMinX = 100.0f, fMapMaxX = 1900.0f;
	float fMapMinZ = 100.0f, fMapMaxZ = 1900.0f;
	for (int i = 0; i < m_nObjects; i++)
	{
		CMi24Object* pObject = new CMi24Object(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);

		float x = Random(fMapMinX, fMapMaxX);
		float z = Random(fMapMinZ, fMapMaxZ);
		float y = Random(800.0f, 1000.0f);
		if (pOwnerScene) pObject->SetOwnerScene(pOwnerScene);

		pObject->SetPosition(x, y, z);
		pObject->SetScale(4.0f, 4.0f, 4.0f);
		pObject->Rotate(0.0f, 90.0f, 0.0f);
		pObject->SetLayer(EObjectLayer::Enemy);
		pObject->IsSetLocalOOBBFromMesh = true;

		if (m_pMi24Model) {
			pObject->m_xmLocalOOBB = m_pMi24Model->m_xmLocalOOBB;
			pObject->m_xmLocalSphere = m_pMi24Model->m_xmLocalSphere;
			pObject->m_xmBigOOGG = m_pMi24Model->m_xmBigOOGG;

			if (m_pMi24Model->m_ppMeshes && m_pMi24Model->m_ppMeshes[0])
			{
				pObject->SetMesh(0, m_pMi24Model->m_ppMeshes[0]);
			}
			if (m_pMi24Model->m_pChild)
			{
				pObject->SetChild(CloneGameObjectHierarchy(m_pMi24Model->m_pChild));
			}
		}
		pObject->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		pObject->PrepareAnimate();
		m_ppObjects[i] = pObject;
	}

	CreateInstanceBuffer(pd3dDevice, pd3dCommandList, pOwnerScene);
}

void CBattleShader::ReleaseObjects()
{
	if (m_ppObjects)
	{
		for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) m_ppObjects[j]->Release();
		delete[] m_ppObjects;
	}
}
void CBattleShader::RenderReflection(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	Render(pd3dCommandList, pCamera, 1);
}
void CBattleShader::AnimateObjects(float fTimeElapsed)
{
	if (m_pModelMainRotor)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pModelMainRotor->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pModelMainRotor->m_xmf4x4Transform);
	}
	if (m_pModelTailRotor)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pModelTailRotor->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pModelTailRotor->m_xmf4x4Transform);
	}
	if (m_pMi24Model)
	{
		m_pMi24Model->UpdateTransform(NULL);
	}
	for (int i = 0; i < m_nObjects; ++i)
	{
		if (!m_ppObjects[i]) continue;
		m_ppObjects[i]->Animate(fTimeElapsed);
		m_ppObjects[i]->UpdateTransform(NULL);
	}
}

void CBattleShader::ReleaseUploadBuffers()
{
	for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) m_ppObjects[j]->ReleaseUploadBuffers();
}

void RenderModelInstancedRecursive(CGameObject* pModelNode, ID3D12GraphicsCommandList* pd3dCommandList, UINT nInstances)
{
	if (!pModelNode) return;

	if (pModelNode->m_nMaterials > 0 && pModelNode->m_ppMaterials[0])
	{
		pModelNode->m_ppMaterials[0]->UpdateShaderVariables(pd3dCommandList);
		pModelNode->UpdateShaderVariables(pd3dCommandList);
	}
	for (int i = 0; i < pModelNode->m_nMeshes; ++i)
	{
		if (pModelNode->m_ppMeshes[i])
		{
			pModelNode->m_ppMeshes[i]->RenderInstanced(pd3dCommandList, i, nInstances);
		}
	}

	if (pModelNode->m_pSibling)
		RenderModelInstancedRecursive(pModelNode->m_pSibling, pd3dCommandList, nInstances);
	if (pModelNode->m_pChild)
		RenderModelInstancedRecursive(pModelNode->m_pChild, pd3dCommandList, nInstances);
}
void CBattleShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	if (m_nInstances == 0 || !m_pMi24Model) return;
	UpdateInstanceBuffer(pd3dCommandList);
	CShader::Render(pd3dCommandList, pCamera, nPipelineState);
	pd3dCommandList->SetGraphicsRootDescriptorTable(13, m_d3dInstanceSrvGpuHandle);
	RenderModelInstancedRecursive(m_pMi24Model, pd3dCommandList, m_nInstances);
}
