#include "MainMenuScene.h"
#include <windowsx.h>

namespace
{
	constexpr LONG kMenuWidth = 1280;
	constexpr LONG kMenuHeight = 960;
	constexpr float kAtlasWidth = 544.0f;
	constexpr float kAtlasHeight = 112.0f;
}

CMainMenuScene::CMainMenuScene() {}
CMainMenuScene::~CMainMenuScene() { ReleaseObjects(); }

int CMainMenuScene::HitTestMenu(HWND hWnd, int x, int y) const
{
	RECT client = {};
	if (!::GetClientRect(hWnd, &client) || client.right <= 0 || client.bottom <= 0 ||
		x < 0 || y < 0 || x >= client.right || y >= client.bottom) return -1;

	// Compare exact scaled coordinates, avoiding rounded hits across an edge.
	const LONGLONG logicalX = static_cast<LONGLONG>(x) * kMenuWidth;
	const LONGLONG logicalY = static_cast<LONGLONG>(y) * kMenuHeight;
	for (int button = 0; button < 2; ++button)
	{
		const RECT& rect = m_rcButtons[button];
		if (logicalX >= static_cast<LONGLONG>(rect.left) * client.right &&
			logicalX < static_cast<LONGLONG>(rect.right) * client.right &&
			logicalY >= static_cast<LONGLONG>(rect.top) * client.bottom &&
			logicalY < static_cast<LONGLONG>(rect.bottom) * client.bottom) return button;
	}
	return -1;
}

void CMainMenuScene::UpdateHoverFromCursor()
{
	m_nHoveredButton = -1;
	if (IsFinshied) return;
	const HWND foreground = ::GetForegroundWindow();
	if (!::IsWindow(m_hMenuWindow))
	{
		DWORD processId = 0;
		const DWORD threadId = ::GetWindowThreadProcessId(foreground, &processId);
		if (processId == ::GetCurrentProcessId() && threadId == ::GetCurrentThreadId())
			m_hMenuWindow = foreground;
	}
	if (!m_hMenuWindow || foreground != m_hMenuWindow) return;

	POINT cursor = {};
	if (!::GetCursorPos(&cursor)) return;
	const HWND windowUnderCursor = ::WindowFromPoint(cursor);
	if (!windowUnderCursor || ::GetAncestor(windowUnderCursor, GA_ROOT) != m_hMenuWindow) return;
	if (::ScreenToClient(m_hMenuWindow, &cursor))
		m_nHoveredButton = HitTestMenu(m_hMenuWindow, cursor.x, cursor.y);
}

bool CMainMenuScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera)
{
	m_hMenuWindow = hWnd;
	if (nMessageID == WM_MOUSELEAVE || nMessageID == WM_CANCELMODE)
	{
		m_nHoveredButton = -1;
		return true;
	}
	if (nMessageID != WM_MOUSEMOVE && nMessageID != WM_LBUTTONDOWN) return false;
	if (IsFinshied) return true;
	m_nHoveredButton = HitTestMenu(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	if (nMessageID == WM_MOUSEMOVE) return true;
	if (m_nHoveredButton == 0)
	{
		NextSceneNum = BattleStage;
		IsFinshied = true;
		m_nHoveredButton = -1;
		return true;
	}
	if (m_nHoveredButton == 1)
	{
		m_nHoveredButton = -1;
		::PostQuitMessage(0);
		return true;
	}
	return false;
}

bool CMainMenuScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return false;
}

void CMainMenuScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	IsFinshied = false;
	m_hMenuWindow = nullptr;
	m_nHoveredButton = -1;
	try
	{
		m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
		m_bOwnRootSignature = true;
		m_pDescriptorHeap = new CDescriptorHeap();
		// Leave the existing player resource allowance available to the framework.
		CreateCbvSrvDescriptorHeaps(pd3dDevice, 200, 73);

		m_pUICamera = new CCamera();
		m_pUICamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		m_pUICamera->SetViewport(0, 0, kMenuWidth, kMenuHeight, 0.0f, 1.0f);
		m_pUICamera->SetScissorRect(0, 0, kMenuWidth, kMenuHeight);
		XMFLOAT4X4 projection;
		XMStoreFloat4x4(&projection, XMMatrixOrthographicOffCenterLH(
			0.0f, static_cast<float>(kMenuWidth), 0.0f, static_cast<float>(kMenuHeight), -1.0f, 100.0f));
		m_pUICamera->SetProjectionMatrix(projection);
		m_pUICamera->SetViewMatrix(Matrix4x4::Identity());

		m_pUIShader = new CUIShader();
		m_pUIShader->AddRef(); // The scene owns a reference separate from its materials.
		m_pUIShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

		auto createMaterial = [&](CMaterial*& material, const wchar_t* path)
		{
			material = new CMaterial();
			material->AddRef();
			material->SetShader(m_pUIShader);
			CTexture* texture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
			material->SetTexture(texture);
			material->SetMaterialType(MATERIAL_ALBEDO_MAP);
			texture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList,
				const_cast<wchar_t*>(path), RESOURCE_TEXTURE2D, 0);
			CreateShaderResourceView(pd3dDevice, texture, 0, 3);
		};
		createMaterial(m_pCoverMaterial, L"Image/UI/MenuCover.dds");
		createMaterial(m_pActionsMaterial, L"Image/UI/MenuActions.dds");

		// One backdrop and four prebuilt states; only three quads are drawn.
		m_nGameObjects = 5;
		m_ppGameObjects = new CGameObject * [m_nGameObjects]();
		auto createQuad = [&](int index, const RECT& rect, CMaterial* material, const XMFLOAT4& uv)
		{
			CGameObject* object = new CGameObject(1, 1);
			m_ppGameObjects[index] = object;
			object->SetOwnerScene(this);
			object->m_bSkipFrustum = true;
			object->SetMaterial(0, material);
			object->SetMesh(0, new CTexturedRectMesh(pd3dDevice, pd3dCommandList,
				static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top),
				0.0f, 0.0f, 0.0f, 0.0f, uv));
			object->SetPosition((rect.left + rect.right) * 0.5f,
				kMenuHeight - (rect.top + rect.bottom) * 0.5f, 0.0f);
			object->SetTextureTransformParams(XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
			object->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		};
		const RECT cover = { 0, 0, kMenuWidth, kMenuHeight };
		createQuad(0, cover, m_pCoverMaterial, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
		for (int button = 0; button < 2; ++button)
		{
			for (int state = 0; state < 2; ++state)
			{
				// V=0 is the top edge of the existing XY rectangle. Keep filtering
				// within this exact atlas cell, including at fullscreen scale.
				const XMFLOAT4 uv((state * 272.0f + 0.5f) / kAtlasWidth,
					(button * 56.0f + 0.5f) / kAtlasHeight, 271.0f / kAtlasWidth, 55.0f / kAtlasHeight);
				createQuad(1 + button * 2 + state, m_rcButtons[button], m_pActionsMaterial, uv);
			}
		}
	}
	catch (...)
	{
		ReleaseObjects();
		throw;
	}
}

void CMainMenuScene::ReleaseObjects()
{
	m_nHoveredButton = -1;
	m_hMenuWindow = nullptr;
	ReleaseUploadBuffers();
	if (m_ppGameObjects)
	{
		for (int i = 0; i < m_nGameObjects; ++i)
			if (m_ppGameObjects[i]) m_ppGameObjects[i]->Release();
		delete[] m_ppGameObjects;
		m_ppGameObjects = nullptr;
	}
	m_nGameObjects = 0;
	// CMaterial's destructor currently releases its texture, but not its shader.
	for (CMaterial** material : { &m_pCoverMaterial, &m_pActionsMaterial })
	{
		if (*material)
		{
			(*material)->SetShader(nullptr);
			(*material)->Release();
			*material = nullptr;
		}
	}
	if (m_pUIShader) { m_pUIShader->Release(); m_pUIShader = nullptr; }
	if (m_pUICamera)
	{
		m_pUICamera->ReleaseShaderVariables();
		delete m_pUICamera;
		m_pUICamera = nullptr;
	}
	if (m_pDescriptorHeap) { delete m_pDescriptorHeap; m_pDescriptorHeap = nullptr; }
	if (m_bOwnRootSignature && m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
	m_pd3dGraphicsRootSignature = nullptr;
	m_bOwnRootSignature = false;
}

void CMainMenuScene::ReleaseUploadBuffers()
{
	if (m_ppGameObjects)
		for (int i = 0; i < m_nGameObjects; ++i)
			if (m_ppGameObjects[i]) m_ppGameObjects[i]->ReleaseUploadBuffers();
	if (m_pCoverMaterial) m_pCoverMaterial->ReleaseUploadBuffers();
	if (m_pActionsMaterial) m_pActionsMaterial->ReleaseUploadBuffers();
}

bool CMainMenuScene::ProcessInput(UCHAR* pKeysBuffer) { return true; }

void CMainMenuScene::AnimateObjects(float fTimeElapsed)
{
	// Window leave/activation messages are not all forwarded by the framework.
	// Polling also refreshes hover after scene reentry without requiring a move.
	UpdateHoverFromCursor();
}

void CMainMenuScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!m_ppGameObjects || !m_pUICamera || !m_pUIShader || !m_pDescriptorHeap) return;
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);
	m_pUICamera->SetViewportsAndScissorRects(pd3dCommandList);
	m_pUICamera->UpdateShaderVariables(pd3dCommandList);
	m_pUIShader->Render(pd3dCommandList, m_pUICamera);
	m_ppGameObjects[0]->Render(pd3dCommandList, m_pUICamera);
	for (int button = 0; button < 2; ++button)
		m_ppGameObjects[1 + button * 2 + (m_nHoveredButton == button ? 1 : 0)]->Render(pd3dCommandList, m_pUICamera);
}
