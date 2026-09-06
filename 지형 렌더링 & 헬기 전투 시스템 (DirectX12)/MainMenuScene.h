#pragma once
#include "stdafx.h"
#include "Scene.h"
#include"StaticObject.h"
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
	void AnimateObjects(float fTimeElapsed) override;
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) override;
	CCamera* m_pMenuCamera = nullptr;
	float m_fRotateSpeed = 10.0f;
protected:
	CGameObject* m_pStartButton = NULL;
	CGameObject* m_pExitButton = NULL;

};

