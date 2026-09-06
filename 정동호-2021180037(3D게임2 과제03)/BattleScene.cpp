#include "BattleScene.h"
#include "GameFramework.h"
extern CGameFramework gGameFramework;
CBattleScene::CBattleScene() {
	NextSceneNum = SelectStage;
}
CBattleScene::~CBattleScene() {}

bool CBattleScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		CGameObject* pPicked = PickObjectPointedByCursor((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), pCamera);
		if (pPicked)
		{
			OutputDebugStringA("BattleScene: Lock-on target picked!\n");
			m_pCurrentLockOnTarget = pPicked;
		}
		break;
	}
	}

	return false;
}

bool CBattleScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
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
		case VK_SPACE:
			if (m_pPlayer) {
				FirePlayerMissile();
			}
			break;
		default:
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_F8:
			m_nTerrainDebugMode = (m_nTerrainDebugMode + 1) % 2;
			return true;
		}
		break;
	default:
		break;
	}
	return(false);
}

void CBattleScene::BuildUIObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	float fWidth = (float)FRAME_BUFFER_WIDTH;
	float fHeight = (float)FRAME_BUFFER_HEIGHT;
	m_pUIShader = new CUIShader();
	m_pUIShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pUIShader->AddRef();

	CShader* pHealthBarShader = new CHealthBarShader();
	pHealthBarShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	CTexture* pUITexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pUITexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/UI/HPBar01.dds", RESOURCE_TEXTURE2D, 0);

	CreateShaderResourceView(pd3dDevice, pUITexture, 0, 3);

	m_pUIMaterial = new CMaterial();
	m_pUIMaterial->SetShader(m_pUIShader);
	m_pUIMaterial->SetTexture(pUITexture);
	m_pUIMaterial->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_pUIMaterial->AddRef();

	CMaterial* pHealthBarMaterial = new CMaterial();
	pHealthBarMaterial->SetShader(pHealthBarShader);
	pHealthBarMaterial->SetTexture(pUITexture);

	CGameObject* pHealthBarBG = new CGameObject(1, 1);
	pHealthBarBG->SetOwnerScene(this);
	CTexturedRectMesh* pBgMesh = new CTexturedRectMesh(pd3dDevice, pd3dCommandList, 300.0f, 30.0f, 0.0f, 0.0f, 0.0f, +1.0f);
	pHealthBarBG->SetMesh(0, pBgMesh);
	pHealthBarBG->m_bSkipFrustum = true;

	pHealthBarBG->SetMaterial(0, m_pUIMaterial);
	pHealthBarBG->SetScale(1.5f, 0.8f, 1.0f);
	pHealthBarBG->SetPosition(fWidth / 2.0f,  10.0f, 0.0f);
	pHealthBarBG->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_vpUIObjects.push_back(pHealthBarBG);

	CGameObject* pHealthBarFG = new CGameObject(1, 1);
	strcpy_s(pHealthBarFG->m_pstrFrameName, 64, "HealthBarFG");
	pHealthBarFG->SetOwnerScene(this);
	CTexturedRectMesh* pFgMesh = new CTexturedRectMesh(pd3dDevice, pd3dCommandList, 429.0f, 8.0f, 0.0f, 0.0f, 0.0f, +1.0f);
	pHealthBarFG->SetMesh(0, pFgMesh);
	pHealthBarFG->m_bSkipFrustum = true;
	pHealthBarFG->SetMaterial(0, pHealthBarMaterial);
	pHealthBarFG->SetPosition(fWidth / 2.0f+10.0f, 6.0f, 0.0f);
	pHealthBarFG->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_vpUIObjects.push_back(pHealthBarFG);

	m_pTextureAtlasShader = new CUITextureAtlasShader();
	m_pTextureAtlasShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pTextureAtlasShader->AddRef();

	CTexture* pNumberTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pNumberTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/UI/Font.dds", RESOURCE_TEXTURE2D, 0);
	CreateShaderResourceView(pd3dDevice, pNumberTexture, 0, 3);

	m_pScoreMaterial = new CMaterial();
	m_pScoreMaterial->SetShader(m_pTextureAtlasShader);
	m_pScoreMaterial->SetTexture(pNumberTexture);
	m_pScoreMaterial->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_pScoreMaterial->AddRef();

	float fDigitWidth = 32.0f;
	float fDigitHeight = 80.0f;

	float fStartX = fWidth - (fDigitWidth * 6) - 20.0f;
	float fStartY = 24.0f;

	m_vpScoreDigits.reserve(6);
	for (int i = 0; i < 6; ++i)
	{
		CGameObject* pDigit = new CGameObject(1, 1);
		pDigit->SetOwnerScene(this);
		CTexturedRectMesh* pDigitMesh = new CTexturedRectMesh(pd3dDevice, pd3dCommandList, fDigitWidth, fDigitHeight, 0.0f, 0.0f, 0.0f, +1.0f);
		pDigit->SetMesh(0, pDigitMesh);
		pDigit->m_bSkipFrustum = true;
		pDigit->SetMaterial(0, m_pScoreMaterial);
		pDigit->SetTextureTransformParams(XMFLOAT4(0.0f, 0.4f, 0.2f, 0.5f));
		pDigit->SetPosition(fStartX + (i * fDigitWidth) + (fDigitWidth * 0.5f), fStartY, 0.0f);
		pDigit->SetScale(1.0f, 1.7f, 1.0f);
		pDigit->CreateShaderVariables(pd3dDevice, pd3dCommandList);

		m_vpUIObjects.push_back(pDigit);
		m_vpScoreDigits.push_back(pDigit);
	}

	CShader* pCrossShader = m_pUIShader;
	CTexture* pCrossTex = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pCrossTex->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList,L"Image/UI/HiarCross.dds", RESOURCE_TEXTURE2D, 0);

	CreateShaderResourceView(pd3dDevice, pCrossTex, 0, 3);

	CMaterial* pCrossMat = new CMaterial();
	pCrossMat->SetShader(pCrossShader);
	pCrossMat->SetTexture(pCrossTex);
	pCrossMat->SetMaterialType(MATERIAL_ALBEDO_MAP);

	CGameObject* pCrossHair = new CGameObject(1, 1);
	pCrossHair->SetOwnerScene(this);
	pCrossHair->m_bSkipFrustum = true;

	float size = 40.0f;
	CTexturedRectMesh* pCrossMesh =new CTexturedRectMesh(pd3dDevice, pd3dCommandList,size, size, 0.0f, 0.0f, 0.0f, +1.0f);
	pCrossHair->SetMesh(0, pCrossMesh);
	pCrossHair->SetMaterial(0, pCrossMat);
	pCrossHair->SetPosition(fWidth * 0.5f, fHeight * 0.5f, 0.0f);
	pCrossHair->SetScale(10.0f, 10.0f, 10.0f);
	pCrossHair->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	strcpy_s(pCrossHair->m_pstrFrameName, 64, "CrossHair");
	m_vpUIObjects.push_back(pCrossHair);
}

void CBattleScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	m_pDescriptorHeap = new CDescriptorHeap();
	const int nCbvDescriptors = 200;
	const int nSrvDescriptors = 256;
	CreateCbvSrvDescriptorHeaps(pd3dDevice, nCbvDescriptors, nSrvDescriptors);

	BuildDefaultLightsAndMaterials();

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, this);
	m_pSkyBox->SetOwnerScene(this);
	m_pSkyBox->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_pSkyBox->SetLayer(EObjectLayer::StaticObject);

	XMFLOAT3 xmf3Scale(18.0f, 6.0f, 18.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.5f, 0.0f, 0.0f);
	m_pTerrain = new CHeightMapTerrain(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, _T("Terrain/HeightMap.raw"), 257, 257, 257, 257, xmf3Scale, xmf4Color, this);
	m_pTerrain->SetOwnerScene(this);
	m_pTerrain->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_pTerrain->SetLayer(EObjectLayer::StaticObject);

	m_pFlowingLava = new CFlowingLava(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, 257 * 4, 257, this);
	m_pFlowingLava->SetPosition(800.0f, 500.0f, 1000.0f);
	m_pFlowingLava->SetScale(4.0f, 1.0f, 10.0f);
	m_pFlowingLava->SetLayer(EObjectLayer::StaticObject);

	m_nShaders = 1;
	m_ppShaders = new CShader * [m_nShaders];

	CBattleShader* pObjectsShader = new CBattleShader();
	pObjectsShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	pObjectsShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, this);
	m_ppShaders[0] = pObjectsShader;

	m_pBillboardShader = new CBillboardShader();
	m_pBillboardShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pBillboardShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pTerrain, this);

	m_ppExplosionShaders = new CExplosionShader();
	m_ppExplosionShaders->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_ppExplosionShaders->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, this);

	UINT ncbElementBytes = ((sizeof(VS_CB_CAMERA_INFO) + 255) & ~255);
	m_pd3dcbMirrorParams = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	m_pd3dcbMirrorParams->Map(0, NULL, (void**)&m_pcbMappedMirrorParams);

	m_pMirrorShader = new CMirrorShader();
	m_pMirrorShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pMirror = new CMirrorObject(1, 1);
	m_pMirror->SetOwnerScene(this);
	m_pMirror->m_bSkipFrustum = true;
	CTexturedRectMesh* pMirrorMesh =
		new CTexturedRectMesh(pd3dDevice, pd3dCommandList,
			400.0f, 300.0f,
			0.0f, 0.0f, 0.0f, -1.0f);
	m_pMirror->SetMesh(0, pMirrorMesh);
	CMaterial* pMirrorMaterial = new CMaterial();
	pMirrorMaterial->SetShader(m_pMirrorShader);
	pMirrorMaterial->m_xmf4AlbedoColor = XMFLOAT4(0.7f, 0.7f, 0.75f, 0.4f);
	pMirrorMaterial->AddRef();
	m_pMirror->SetMaterial(0, pMirrorMaterial);
	m_pMirror->SetPosition(920.0f,700.0f, 1400.0);
	m_pMirror->SetScale(0.5f, 0.5f, 0.5f);
	m_pMirror->SetLayer(EObjectLayer::StaticObject);

	m_pMirror->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	XMFLOAT3 planeNormal = XMFLOAT3(0.0f, 0.0f, -1.0f);

	XMFLOAT3 mirrorPos = m_pMirror->GetPosition();

	XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&planeNormal));
	XMVECTOR p = XMLoadFloat3(&mirrorPos);

	float d = -XMVectorGetX(XMVector3Dot(n, p));

	XMFLOAT3 normalW;
	XMStoreFloat3(&normalW, n);
	m_pMirror->SetMirrorPlane(normalW, d);

	m_pUICamera = new CCamera();
	m_pUICamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	float fWidth = (float)FRAME_BUFFER_WIDTH;
	float fHeight = (float)FRAME_BUFFER_HEIGHT;
	m_pUICamera->SetViewport(0, 0, (int)fWidth, (int)fHeight, 0.0f, 1.0f);
	m_pUICamera->SetScissorRect(0, 0, (int)fWidth, (int)fHeight);

	XMMATRIX mtxView = XMMatrixIdentity();
	XMFLOAT4X4 xmf4x4View;
	XMStoreFloat4x4(&xmf4x4View, mtxView);
	m_pUICamera->SetViewMatrix(xmf4x4View);

	XMMATRIX mtxOrtho = XMMatrixOrthographicOffCenterLH(0.0f, fWidth, 0.0f, fHeight, -1.0f, 100.0f);
	XMFLOAT4X4 xmf4x4Ortho;
	XMStoreFloat4x4(&xmf4x4Ortho, mtxOrtho);

	m_pUICamera->SetProjectionMatrix(xmf4x4Ortho);

	BuildUIObjects(pd3dDevice, pd3dCommandList);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	const int MAX_PLAYER_MISSILES = 1;
	m_vPlayerMissiles.reserve(MAX_PLAYER_MISSILES);

	for (int i = 0; i < MAX_PLAYER_MISSILES; ++i)
	{
		CGameObject* m_pMissileModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList,
			m_pd3dGraphicsRootSignature, (char*)"Model/missile.bin", NULL, this);
		if (m_pMissileModel)
		{
			m_pMissileModel->m_bSkipFrustum = true;
			m_pMissileModel->SetScale(20.0f, 20.0f, 20.0f);
			m_pMissileModel->SetPosition(-90.0f, -50.0f, 0.0f);
			m_pMissileModel->IsSetLocalOOBBFromMesh = true;
			m_pMissileModel->SetLayer(EObjectLayer::PlayerBullet);
			m_pMissileModel->BuildBroadphaseSphere();
		}
		CMssileObject* pMissile = new CMssileObject(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature,this);
		pMissile->SetOwnerScene(this);
		if (m_pMissileModel)
		{
			pMissile->m_xmLocalOOBB = m_pMissileModel->m_xmLocalOOBB;
			pMissile->m_xmLocalSphere = m_pMissileModel->m_xmLocalSphere;
			pMissile->m_xmBigOOGG = m_pMissileModel->m_xmBigOOGG;
			pMissile->IsSetLocalOOBBFromMesh = m_pMissileModel->IsSetLocalOOBBFromMesh;

			if (m_pMissileModel->m_ppMeshes && m_pMissileModel->m_ppMeshes[0])
			{
				pMissile->SetMesh(0, m_pMissileModel->m_ppMeshes[0]);
			}
			if (m_pMissileModel->m_nMaterials > 0 && m_pMissileModel->m_ppMaterials[0])
				pMissile->SetMaterial(0, m_pMissileModel->m_ppMaterials[0]);
			if (m_pMissileModel->m_pChild)
			{
				pMissile->SetChild(m_pMissileModel);
			}
		}
		pMissile->m_bSkipFrustum = true;
		pMissile->IsSetLocalOOBBFromMesh = true;
		pMissile->SetLayer(EObjectLayer::PlayerBullet);
		pMissile->BuildBroadphaseSphere();
		pMissile->SetCollidable(false);
		pMissile->bRender = true;

		pMissile->CreateShaderVariables(pd3dDevice, pd3dCommandList);

		m_vPlayerMissiles.push_back(pMissile);
	}

	if (m_pPlayer)
	{
		m_pPlayer->BuildBroadphaseSphere();
	}

	BuildSpatialGrid();

	CreateShadowResources(pd3dDevice, pd3dCommandList);
	if (!m_pShadowMapShader)
	{
		m_pShadowMapShader = new CShadowMapShader();
		m_pShadowMapShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	}
}

void CBattleScene::FirePlayerMissile()
{
	if (!m_pPlayer) return;

	CMssileObject* pMissile = nullptr;
	for (auto* m : m_vPlayerMissiles)
	{
		if (m && !m->IsActive())
		{

			pMissile = m;
			break;
		}
	}
	if (!pMissile) return;

	XMFLOAT3 playerPos = m_pPlayer->GetPosition();

	XMFLOAT3 right = m_pPlayer->GetRightVector();
	XMFLOAT3 up = m_pPlayer->GetUpVector();
	XMFLOAT3 look = m_pPlayer->GetLookVector();

	XMFLOAT3 localOffset = XMFLOAT3(0.0f, -2.0f, 8.0f);

	XMFLOAT3 worldOffset = XMFLOAT3(0, 0, 0);
	worldOffset = Vector3::Add(worldOffset, Vector3::ScalarProduct(right, localOffset.x, false));
	worldOffset = Vector3::Add(worldOffset, Vector3::ScalarProduct(up, localOffset.y, false));
	worldOffset = Vector3::Add(worldOffset, Vector3::ScalarProduct(look, localOffset.z, false));

	XMFLOAT3 firePos = Vector3::Add(playerPos, worldOffset);

	XMFLOAT3 fireDir = look;

	CGameObject* pTarget = nullptr;
	if (m_pCurrentLockOnTarget &&
		m_pCurrentLockOnTarget->GetLayer() == EObjectLayer::Enemy)
	{
		pTarget = m_pCurrentLockOnTarget;
	}

	pMissile->Fire(firePos, fireDir, pTarget);
	m_CollisionValue.InsertObjectToGridByAABB(pMissile);
}

void CBattleScene::ReleaseObjects()
{
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
		m_ppShaders = NULL;
	}

	if (m_pTerrain) delete m_pTerrain;
	if (m_pSkyBox) delete m_pSkyBox;

	if (m_ppExplosionShaders) m_ppExplosionShaders->ReleaseObjects();

	if (m_pFlowingLava) delete m_pFlowingLava;
	if (m_pLights) delete[] m_pLights;

	for (auto* pMissile : m_vPlayerMissiles)
	{
		if (pMissile) pMissile->Release();
	}
	m_vPlayerMissiles.clear();

	for (auto& pUIObject : m_vpUIObjects)
	{
		if (pUIObject) pUIObject->Release();
	}
	m_vpUIObjects.clear();
	m_vpScoreDigits.clear();

	if (m_pUIMaterial) m_pUIMaterial->Release();
	if (m_pScoreMaterial) m_pScoreMaterial->Release();

	if (m_pUIShader) m_pUIShader->Release();
	if (m_pTextureAtlasShader) m_pTextureAtlasShader->Release();

	if (m_pBillboardShader)
	{
		m_pBillboardShader->ReleaseShaderVariables();
		m_pBillboardShader->ReleaseObjects();
		m_pBillboardShader->Release();
		m_pBillboardShader = NULL;
	}

	if (m_pUICamera)
	{
		delete m_pUICamera;
		m_pUICamera = NULL;
	}

	if (m_pd3dcbMirrorParams)
	{
		m_pd3dcbMirrorParams->Unmap(0, NULL);
		m_pd3dcbMirrorParams->Release();
		m_pd3dcbMirrorParams = NULL;
	}

	if (m_pMirror) m_pMirror->Release();
	if (m_pMirrorShader)
	{
		m_pMirrorShader->ReleaseShaderVariables();
		m_pMirrorShader->Release();
	}

	if (m_pDescriptorHeap)
	{
		delete m_pDescriptorHeap;
		m_pDescriptorHeap = NULL;
	}
	ReleaseShadowResources();
	if (m_pShadowMapShader) { m_pShadowMapShader->Release(); m_pShadowMapShader = nullptr; }
}

void CBattleScene::ReleaseUploadBuffers()
{
	if (m_pBillboardShader) m_pBillboardShader->ReleaseUploadBuffers();

	if (m_pTerrain) m_pTerrain->ReleaseUploadBuffers();
	if (m_pSkyBox) m_pSkyBox->ReleaseUploadBuffers();

	for (int i = 0; i < m_nShaders; i++) m_ppShaders[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nGameObjects; i++) m_ppGameObjects[i]->ReleaseUploadBuffers();
	if (m_pFlowingLava) m_pFlowingLava->ReleaseUploadBuffers();
	for (auto& pUIObject : m_vpUIObjects)
	{
		if (pUIObject) pUIObject->ReleaseUploadBuffers();
	}

}
bool CBattleScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return(false);
}
void DebugPrintObjectCells(CGameObject* obj, const char* name)
{

}
void CBattleScene::AnimateObjects(float fTimeElapsed)
{

	if (m_pLights)
	{
		m_pLights[1].m_xmf3Position = m_pPlayer->GetPosition();
		m_pLights[1].m_xmf3Direction = m_pPlayer->GetLookVector();
	}
	if (m_pFlowingLava) m_pFlowingLava->Animate(fTimeElapsed);
	if (m_pPlayer)
	{

		m_CollisionValue.UpdateDynamicObjectInGrid(m_pPlayer);
	}
	if (m_pLights && m_pPlayer)
	{

		m_fSunAngle += fTimeElapsed * 0.1f;
		XMFLOAT3 xmf3SunDir(
			cosf(m_fSunAngle),
			-0.6f - 0.3f * sinf(m_fSunAngle * 0.5f),
			sinf(m_fSunAngle)
		);
		m_pLights[2].m_xmf3Direction = xmf3SunDir;

		UpdateShadowShaderVariables(m_pLights[2].m_xmf3Direction, m_pPlayer->GetPosition());
	}
	for (int i = 0; i < m_nShaders; ++i)
	{
		if (!m_ppShaders[i]) continue;
		m_ppShaders[i]->AnimateObjects(fTimeElapsed);
		CObjectsShader* pObjectsShader = static_cast<CObjectsShader*>(m_ppShaders[i]);
		int pShaderObjects = pObjectsShader->GetNumberOfObjects();

		for (int index = 0; index < pShaderObjects; index++) {
			CGameObject* obj = pObjectsShader->GetObject(index);
			if (obj)
			{
				if (obj->m_bExplosionTriggered) {
					obj->m_bExplosionTriggered = false;
					XMFLOAT3 pos = obj->GetPosition();
					m_ppExplosionShaders->TriggerExplosion(pos);
				}
				m_CollisionValue.UpdateDynamicObjectInGrid(obj);
			}
		}
	}
	for (auto* pMissile : m_vPlayerMissiles)
	{
		if (!pMissile) continue;
		if (!pMissile->bRender) continue;

		pMissile->Animate(fTimeElapsed);
		m_CollisionValue.UpdateDynamicObjectInGrid(pMissile);
	}
	m_CollisionValue.CheckCollision();

	m_fScoreTimer += fTimeElapsed;

	if (m_fScoreTimer >= 0.1f)
	{
		m_fScoreTimer = 0.0f;

		m_nScore++;

		if (m_nScore > 999999)
			m_nScore = 0;
	}

	int nScore = m_nScore;
	const float fUWidth = 0.1f;
	const float fVHeight = 1.0f;

	for (int i = 5; i >= 0; --i)
	{
		if (m_vpScoreDigits.empty()) break;

		CGameObject* pDigitObject = m_vpScoreDigits[i];
		int digit = nScore % 10;
		nScore /= 10;

		float fUOffset = digit * fUWidth;
		float fVOffset = 0.0f;

		pDigitObject->SetTextureTransformParams(
			XMFLOAT4(fUOffset, fVOffset, fUWidth, fVHeight)
		);
		pDigitObject->bRender = true;
	}

	for (auto& pUIObject : m_vpUIObjects)
	{
		if (!pUIObject) continue;

		if (m_pPlayer && strcmp(pUIObject->m_pstrFrameName, "HealthBarFG") == 0)
		{
			float fHealthPercent = static_cast<float>(m_pPlayer->GetHealth()) / m_pPlayer->GetMaxHealth();
			if (fHealthPercent < 0.0f) fHealthPercent = 0.0f;
			if (fHealthPercent > 1.0f) fHealthPercent = 1.0f;
			pUIObject->SetTextureTransformParams(XMFLOAT4(fHealthPercent, 0.0f, 0.0f, 0.0f));
		}
		pUIObject->Animate(fTimeElapsed, NULL);
		pUIObject->UpdateTransform(NULL);
	}

	if (m_ppExplosionShaders) m_ppExplosionShaders->AnimateObjects(fTimeElapsed);
}
void MakeReflectedView(CCamera* pSrcCamera,const XMFLOAT3& planeNormal,float planeD,XMFLOAT4X4& outView)
{
	XMVECTOR vPlane = XMVectorSet(planeNormal.x, planeNormal.y, planeNormal.z, planeD);

	XMMATRIX R = XMMatrixReflect(vPlane);

	XMMATRIX V_Orig = XMLoadFloat4x4(&pSrcCamera->GetViewMatrix());

	XMMATRIX CamWorld_Orig = XMMatrixInverse(NULL, V_Orig);

	XMMATRIX CamWorld_Reflected = XMMatrixMultiply(CamWorld_Orig, R);

	XMMATRIX V_Reflected = XMMatrixInverse(NULL, CamWorld_Reflected);

	XMStoreFloat4x4(&outView, V_Reflected);
}

XMMATRIX MakeObliqueProjectionMatrix(XMMATRIX projectionMatrix, XMVECTOR viewSpaceClipPlane)
{
	viewSpaceClipPlane = XMPlaneNormalize(viewSpaceClipPlane);

	XMMATRIX P_Inv = XMMatrixInverse(NULL, projectionMatrix);

	XMVECTOR clipSpacePlane = XMPlaneTransform(viewSpaceClipPlane, XMMatrixTranspose(P_Inv));

	XMVECTOR vSign = XMVectorSet(
		(XMVectorGetX(clipSpacePlane) > 0.0f) ? 1.0f : -1.0f,
		(XMVectorGetY(clipSpacePlane) > 0.0f) ? 1.0f : -1.0f,
		1.0f,
		1.0f
	);
	XMVECTOR q;
	q.m128_f32[0] = (vSign.m128_f32[0] + projectionMatrix.r[2].m128_f32[0]) / projectionMatrix.r[0].m128_f32[0];
	q.m128_f32[1] = (vSign.m128_f32[1] + projectionMatrix.r[2].m128_f32[1]) / projectionMatrix.r[1].m128_f32[1];
	q.m128_f32[2] = -1.0f;
	q.m128_f32[3] = (1.0f + projectionMatrix.r[2].m128_f32[2]) / projectionMatrix.r[3].m128_f32[2];

	XMVECTOR c = XMVectorSet(2.0f, 2.0f, 2.0f, 2.0f) / XMVector4Dot(clipSpacePlane, q);

	XMMATRIX P_Oblique = projectionMatrix;
	P_Oblique.r[0].m128_f32[2] = XMVectorGetX(clipSpacePlane) * XMVectorGetX(c);
	P_Oblique.r[1].m128_f32[2] = XMVectorGetY(clipSpacePlane) * XMVectorGetY(c);
	P_Oblique.r[2].m128_f32[2] = XMVectorGetZ(clipSpacePlane) * XMVectorGetZ(c) + 1.0f;
	P_Oblique.r[3].m128_f32[2] = XMVectorGetW(clipSpacePlane) * XMVectorGetW(c);

	return P_Oblique;
}
void CBattleScene::RenderShadowMap(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (!m_pd3dShadowMap || !m_pd3dShadowDsvHeap || !m_pShadowMapShader) return;
	if (m_ShadowMapState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pd3dShadowMap,m_ShadowMapState,D3D12_RESOURCE_STATE_DEPTH_WRITE);
		pd3dCommandList->ResourceBarrier(1, &barrier);
		m_ShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	pd3dCommandList->RSSetViewports(1, &m_d3dShadowViewport);
	pd3dCommandList->RSSetScissorRects(1, &m_d3dShadowScissorRect);

	pd3dCommandList->OMSetRenderTargets(0, nullptr, FALSE, &m_d3dShadowDsvCPUHandle);

	pd3dCommandList->ClearDepthStencilView(m_d3dShadowDsvCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
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
		CBattleShader* pBattleShader = dynamic_cast<CBattleShader*>(m_ppShaders[i]);
		if (pBattleShader)
		{
			pBattleShader->RenderShadowInstanced(pd3dCommandList);
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
void CBattleScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{

	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress);

	pd3dCommandList->SetGraphicsRootConstantBufferView(14, m_pd3dcbShadowInfo->GetGPUVirtualAddress());
	pd3dCommandList->SetGraphicsRootDescriptorTable(15, m_d3dShadowSrvGPUHandle);

	if (m_pSkyBox)   m_pSkyBox->Render(pd3dCommandList, pCamera);
	if (m_pFlowingLava)
		m_pFlowingLava->Render(pd3dCommandList, pCamera);
	if (m_pTerrain)  m_pTerrain->Render(pd3dCommandList, pCamera);

	for (int i = 0; i < m_nShaders; ++i)
		if (m_ppShaders[i])
			m_ppShaders[i]->Render(pd3dCommandList, pCamera);

	if (m_pBillboardShader)
		m_pBillboardShader->Render(pd3dCommandList, pCamera);

	if(m_ppExplosionShaders) m_ppExplosionShaders->Render(pd3dCommandList, pCamera);

	if (m_pPlayer)
		m_pPlayer->Render(pd3dCommandList, pCamera);
	for (auto* pMissile : m_vPlayerMissiles)
	{
		if (!pMissile) continue;
		if (!pMissile->bRender) continue;

		pMissile->Render(pd3dCommandList, pCamera);
	}
	if (m_pMirror)
	{
		XMFLOAT3 mirrorPos = m_pMirror->GetPosition();

	}
	pd3dCommandList->OMSetStencilRef(1);

	if (m_pMirror && m_pMirrorShader)
	{
		pCamera->UpdateShaderVariables(pd3dCommandList);

		m_pMirror->UpdateTransform(NULL);
		m_pMirror->OnPrepareRender();
		m_pMirror->UpdateShaderVariables(pd3dCommandList);
		m_pMirrorShader->Render(pd3dCommandList, pCamera, CMirrorShader::MIRROR_MASK_PSO);

		if (m_pMirror->m_ppMeshes && m_pMirror->m_ppMeshes[0])
			m_pMirror->m_ppMeshes[0]->Render(pd3dCommandList, 0);
	}

	if (m_pMirror && m_pMirrorShader)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = gGameFramework.GetMainDsvHandle();
		pd3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
	pd3dCommandList->OMSetStencilRef(1);
	if (m_pMirror && m_pMirrorShader)
	{
		XMFLOAT4X4 xmf4x4OrigView = pCamera->GetViewMatrix();
		XMFLOAT4X4 xmf4x4OrigProj = pCamera->GetProjectionMatrix();
		XMFLOAT3 xmf3OrigPos = pCamera->GetPosition();
		XMFLOAT4X4 xmf4x4ReflectedView;
		MakeReflectedView(pCamera, m_pMirror->m_xmf3PlaneNormal, m_pMirror->m_fPlaneD, xmf4x4ReflectedView);

		XMMATRIX V_Reflected_Inv = XMMatrixInverse(NULL, XMLoadFloat4x4(&xmf4x4ReflectedView));
		XMFLOAT3 xmf3ReflectedPos;
		xmf3ReflectedPos.x = XMVectorGetX(V_Reflected_Inv.r[3]);
		xmf3ReflectedPos.y = XMVectorGetY(V_Reflected_Inv.r[3]);
		xmf3ReflectedPos.z = XMVectorGetZ(V_Reflected_Inv.r[3]);

		pCamera->SetViewMatrix(xmf4x4ReflectedView);
		pCamera->SetPosition(xmf3ReflectedPos);

		XMVECTOR vMirrorPlaneWorld = XMLoadFloat4(&XMFLOAT4(
			m_pMirror->m_xmf3PlaneNormal.x,
			m_pMirror->m_xmf3PlaneNormal.y,
			m_pMirror->m_xmf3PlaneNormal.z,
			m_pMirror->m_fPlaneD
		));

		XMMATRIX V_Reflected = XMLoadFloat4x4(&xmf4x4ReflectedView);
		XMMATRIX P_Original = XMLoadFloat4x4(&xmf4x4OrigProj);

		XMMATRIX V_Reflected_InvT = XMMatrixTranspose(XMMatrixInverse(NULL, V_Reflected));
		XMVECTOR vMirrorPlaneView = XMPlaneTransform(vMirrorPlaneWorld, V_Reflected_InvT);
		(void)vMirrorPlaneView;

		pCamera->SetProjectionMatrix(xmf4x4OrigProj);
		pCamera->UpdateFrustum();

		VS_CB_CAMERA_INFO mirrorData;
		XMStoreFloat4x4(&mirrorData.m_xmf4x4View, XMMatrixTranspose(XMLoadFloat4x4(&xmf4x4ReflectedView)));
		XMStoreFloat4x4(&mirrorData.m_xmf4x4Projection, XMMatrixTranspose(XMLoadFloat4x4(&xmf4x4OrigProj)));
		mirrorData.m_xmf3Position = pCamera->GetPosition();
		::memcpy(m_pcbMappedMirrorParams, &mirrorData, sizeof(VS_CB_CAMERA_INFO));
		D3D12_GPU_VIRTUAL_ADDRESS cbvMirrorAddress = m_pd3dcbMirrorParams->GetGPUVirtualAddress();
		pd3dCommandList->SetGraphicsRootConstantBufferView(0, cbvMirrorAddress);

		if (m_pSkyBox) m_pSkyBox->Render(pd3dCommandList, pCamera, 1);
		if (m_pTerrain) m_pTerrain->Render(pd3dCommandList, pCamera, 1);
		if (m_pBillboardShader) m_pBillboardShader->Render(pd3dCommandList, pCamera, 1);
		for (int i = 0; i < m_nShaders; ++i)
		{
			if (!m_ppShaders[i]) continue;
			if (auto pObjectsShader = dynamic_cast<CObjectsShader*>(m_ppShaders[i]))
			{
				pObjectsShader->RenderReflection(pd3dCommandList, pCamera);
			}
		}
		if (m_pFlowingLava) m_pFlowingLava->Render(pd3dCommandList, pCamera, 1);
		for (auto* pMissile : m_vPlayerMissiles)
		{
			if (!pMissile) continue;
			if (!pMissile->bRender) continue;

			pMissile->Render(pd3dCommandList, pCamera,1);
		}
		if (m_pPlayer) m_pPlayer->Render(pd3dCommandList, pCamera, 1);
		pCamera->SetPosition(xmf3OrigPos);
		pCamera->SetViewMatrix(xmf4x4OrigView);
		pCamera->SetProjectionMatrix(xmf4x4OrigProj);
		pCamera->UpdateFrustum();
		pCamera->UpdateShaderVariables(pd3dCommandList);

	}
	if (m_pMirror && m_pMirrorShader)
	{

		m_pMirror->UpdateTransform(NULL);
		m_pMirror->OnPrepareRender();
		m_pMirror->UpdateShaderVariables(pd3dCommandList);

		m_pMirrorShader->Render(pd3dCommandList, pCamera, CMirrorShader::MIRROR_GLASS_PSO);

		if (m_pMirror->m_nMaterials > 0 && m_pMirror->m_ppMaterials[0])
			m_pMirror->UpdateShaderVariable(pd3dCommandList, m_pMirror->m_ppMaterials[0]);

		if (m_pMirror->m_ppMeshes && m_pMirror->m_ppMeshes[0])
			m_pMirror->m_ppMeshes[0]->Render(pd3dCommandList, 0);

	}
	pd3dCommandList->OMSetStencilRef(0);

}

void CBattleScene::RenderOverlay(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!pd3dCommandList) return;

	if (m_pd3dGraphicsRootSignature)
		pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);

	pd3dCommandList->SetDescriptorHeaps(1, &m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	if (m_pPlayer)
	{
		pCamera->SetViewportsAndScissorRects(pd3dCommandList);
		pCamera->UpdateShaderVariables(pd3dCommandList);

		if (pCamera->GetMode() == THIRD_PERSON_CAMERA)
		{

			m_pPlayer->Render(
				pd3dCommandList,
				pCamera,
				PLAYER_PSO_OVERLAY);
		}
		else
		{

			m_pPlayer->Render(pd3dCommandList, pCamera);
		}
	}

	if (m_pUICamera && !m_vpUIObjects.empty())
	{
		m_pUICamera->SetViewportsAndScissorRects(pd3dCommandList);
		m_pUICamera->UpdateShaderVariables(pd3dCommandList);

		for (auto& pUIObject : m_vpUIObjects)
		{
			if (pUIObject && strcmp(pUIObject->m_pstrFrameName, "CrossHair") == 0)
			{
				if (!pCamera || pCamera->GetMode() == THIRD_PERSON_CAMERA)
					continue;
			}

			if (pUIObject)
				pUIObject->Render(pd3dCommandList, m_pUICamera);
		}
	}
}
