#include "MainMenuScene.h"
#include"StaticObject.h"
#include <windowsx.h>

CMainMenuScene::CMainMenuScene() {
	//NextSceneNum = BattleStage;
	//ChangeFinishedState();
}
CMainMenuScene::~CMainMenuScene() {}

bool CMainMenuScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera)
{
	switch (nMessageID)
	{
	case WM_RBUTTONDOWN:
		if (m_pStartButton==PickObjectPointedByCursor(LOWORD(lParam), HIWORD(lParam), m_pUICamera))
		{
			NextSceneNum = BattleStage;
			ChangeFinishedState(); 
			return true; 
		}
		else if (m_pExitButton == PickObjectPointedByCursor(LOWORD(lParam), HIWORD(lParam), m_pUICamera))
		{
			::PostQuitMessage(0);
			return true;
		}
		break;
	case WM_LBUTTONDOWN:
		if (m_pStartButton == PickObjectPointedByCursor(LOWORD(lParam), HIWORD(lParam), m_pUICamera))
		{
			NextSceneNum = BattleStage;
			ChangeFinishedState();
			return true;
		}
		else if (m_pExitButton == PickObjectPointedByCursor(LOWORD(lParam), HIWORD(lParam), m_pUICamera))
		{
			::PostQuitMessage(0);
			return true;
		}
		break;
	}
	return(false); 
}

bool CMainMenuScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{

	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'W': m_ppGameObjects[0]->MoveForward(+1.0f); break;
		case 'S': m_ppGameObjects[0]->MoveForward(-1.0f); break;
		case 'A': m_ppGameObjects[0]->MoveStrafe(-1.0f); break;
		case 'D': m_ppGameObjects[0]->MoveStrafe(+1.0f); break;
		case 'Q': m_ppGameObjects[0]->MoveUp(+1.0f); break;
		case 'R': m_ppGameObjects[0]->MoveUp(-1.0f); break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return(false);
}

void CMainMenuScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	

	m_pDescriptorHeap = new CDescriptorHeap();
	const int nCbvDescriptors = 200; 
	const int nSrvDescriptors = 17 + 50 + 1 + 1 + 3 + 1;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, nCbvDescriptors, nSrvDescriptors);

	BuildDefaultLightsAndMaterials();

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, this);
	m_pSkyBox->SetOwnerScene(this);
	m_pSkyBox->CreateShaderVariables(pd3dDevice, pd3dCommandList);


	


	m_pUICamera = new CCamera();
	m_pUICamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	float fWidth =  1280.0f;
	float fHeight = 960.0f;

	m_pUICamera->SetViewport(0, 0, (int)fWidth, (int)fHeight, 0.0f, 1.0f);
	m_pUICamera->SetScissorRect(0, 0, (int)fWidth, (int)fHeight);

	XMMATRIX xmmtxOrtho = XMMatrixOrthographicOffCenterLH(0.0f, fWidth, 0.0f, fHeight, -1.0f, 100.0f);
	XMFLOAT4X4 xmf4x4Ortho;
	XMStoreFloat4x4(&xmf4x4Ortho, xmmtxOrtho);

	m_pUICamera->SetProjectionMatrix(xmf4x4Ortho);

	XMMATRIX mtxView = XMMatrixIdentity();
	XMFLOAT4X4 xmf4x4View;
	XMStoreFloat4x4(&xmf4x4View, mtxView);
	m_pUICamera->SetViewMatrix(xmf4x4View);

	m_pUIShader = new CUIShader();
	m_pUIShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_nGameObjects = 2;
	m_ppGameObjects = new CGameObject * [m_nGameObjects];

	m_pStartButton = new CGameObject(1, 1);
	m_pStartButton->SetOwnerScene(this);
	m_pStartButton->m_bSkipFrustum = true;
	CTexture* pStartTex = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pStartTex->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/UI/StartButton.dds", RESOURCE_TEXTURE2D, 0);
	
	CreateShaderResourceView(pd3dDevice, pStartTex, 0, 3);

	CMaterial* pStartMat = new CMaterial();
	pStartMat->SetShader(m_pUIShader);
	pStartMat->SetTexture(pStartTex);
	pStartMat->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_pStartButton->SetMaterial(0, pStartMat);

	CTexturedRectMesh* pButtonMesh = new CTexturedRectMesh(pd3dDevice, pd3dCommandList, 200.0f, 50.0f, 0.0f);
	m_pStartButton->SetMesh(0, pButtonMesh);
	
	m_pStartButton->SetPosition(fWidth / 2.0f, fHeight / 2.0f, 0.0f);
	m_pStartButton->SetScale(1.0f, 5.0f, 1.0f);
	m_pStartButton->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_ppGameObjects[0] = m_pStartButton;

	m_pExitButton = new CGameObject(1, 1);
	m_pExitButton->SetOwnerScene(this);
	m_pExitButton->m_bSkipFrustum = true;
	CTexture* pExitTex = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pExitTex->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/UI/ExitButton.dds", RESOURCE_TEXTURE2D, 0);
	CreateShaderResourceView(pd3dDevice, pExitTex, 0, 3);

	CMaterial* pExitMat = new CMaterial();
	pExitMat->SetShader(m_pUIShader);
	pExitMat->SetTexture(pExitTex);
	pExitMat->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_pExitButton->SetMaterial(0, pExitMat);

	// [수정] CGameObject::SetMesh()가 내부적으로 AddRef()를 호출하므로 여기서 수동으로 또 부르면
	// 참조 카운트가 실제 소유자 수(2)보다 하나 많아져서(3) 두 버튼이 소멸해도 메시가 절대 삭제되지 않고 누수됨.
	m_pExitButton->SetMesh(0, pButtonMesh);

	m_pExitButton->SetPosition(fWidth / 2.0f, fHeight / 2.0f+ 100.0f, 0.0f);
	m_pExitButton->SetScale(1.0f, 5.0f, 1.0f);
	m_pExitButton->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_ppGameObjects[1] = m_pExitButton;
}

void CMainMenuScene::ReleaseObjects()
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

	if (m_pSkyBox) delete m_pSkyBox;

	if (m_ppGameObjects)
	{
		for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Release();
		delete[] m_ppGameObjects;
		m_ppGameObjects = nullptr; 
	}
	if (m_pUIShader) m_pUIShader->Release();
	if (m_pUICamera) delete m_pUICamera;
	if (m_pLights) delete[] m_pLights;
}
void CMainMenuScene::ReleaseUploadBuffers()
{
	if (m_pTerrain) m_pTerrain->ReleaseUploadBuffers();
	if (m_pSkyBox) m_pSkyBox->ReleaseUploadBuffers();

	for (int i = 0; i < m_nShaders; i++) m_ppShaders[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nGameObjects; i++) m_ppGameObjects[i]->ReleaseUploadBuffers();
}

bool CMainMenuScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return(false);
}

void CMainMenuScene::AnimateObjects(float fTimeElapsed)
{
	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->AnimateObjects(fTimeElapsed);

	if (m_pLights)
	{
		m_pLights[1].m_xmf3Position = m_pPlayer->GetPosition();
		m_pLights[1].m_xmf3Direction = m_pPlayer->GetLookVector();
	}
}

void CMainMenuScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress); //Lights
	if (m_pSkyBox) m_pSkyBox->Render(pd3dCommandList, pCamera);

	
	
	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->Render(pd3dCommandList, pCamera);

	if (m_pUICamera)
	{
		m_pUICamera->SetViewportsAndScissorRects(pd3dCommandList);
		m_pUICamera->UpdateShaderVariables(pd3dCommandList);
	}
	if (m_pUIShader)
	{
		m_pUIShader->Render(pd3dCommandList, m_pUICamera);
	}
	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Render(pd3dCommandList, pCamera);
}

