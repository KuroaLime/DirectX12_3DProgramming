#pragma once
#include "stdafx.h"
#include "Scene.h"
#include "StaticObject.h"
#include "MainMenuShader.h"

class CMainMenuScene : public CScene
{
public:
	CMainMenuScene();
	~CMainMenuScene();
	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera) override;
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	void ReleaseObjects() override;
	void ReleaseUploadBuffers() override;
	bool ProcessInput(UCHAR* pKeysBuffer) override;
	bool UsesPlayerControls() const override { return false; }
	void AnimateObjects(float fTimeElapsed) override;
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) override;

private:
	int HitTestMenu(HWND hWnd, int x, int y) const;
	void UpdateHoverFromCursor();
	RECT m_rcButtons[2] = { { 76, 636, 348, 692 }, { 76, 704, 348, 760 } };
	HWND m_hMenuWindow = nullptr;
	int m_nHoveredButton = -1;
	CMaterial* m_pCoverMaterial = nullptr;
	CMaterial* m_pActionsMaterial = nullptr;
	bool m_bOwnRootSignature = false;
};
