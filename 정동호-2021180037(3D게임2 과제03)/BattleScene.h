#pragma once
#include "stdafx.h"
#include "Scene.h"
#include "TopographyShader.h"
#include"StaticObject.h"
#include "BattleShader.h"

class CBattleScene : public CScene
{
public:
	CBattleScene();
	~CBattleScene();

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	void ReleaseObjects() override;
	void ReleaseUploadBuffers() override;

	bool ProcessInput(UCHAR* pKeysBuffer) override;
	void AnimateObjects(float fTimeElapsed) override;
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) override;
	void RenderOverlay(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) override;

	CBillboardShader* m_pBillboardShader = NULL;

	CFlowingLava* m_pFlowingLava = NULL;
	void BuildUIObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	CShader* m_pTextureAtlasShader = NULL;
	CMaterial* m_pScoreMaterial = NULL;
	std::vector<CGameObject*> m_vpScoreDigits;
	int m_nScore = 0;

	CMirrorObject* m_pMirror = nullptr;
	CMirrorShader* m_pMirrorShader = nullptr;

	ID3D12Resource* m_pd3dcbMirrorParams = NULL;
	VS_CB_CAMERA_INFO* m_pcbMappedMirrorParams = NULL;

	std::vector<CMssileObject*> m_vPlayerMissiles;

	CGameObject* m_pCurrentLockOnTarget = nullptr;

	int m_nLastMouseX = 0;
	int m_nLastMouseY = 0;

	void FirePlayerMissile();

	float m_fScoreTimer = 0.0f;
	float m_fSunAngle = 0.6f;

	CExplosionShader* m_ppExplosionShaders = NULL;
	virtual void RenderShadowMap(ID3D12GraphicsCommandList* pd3dCommandList) override;

};
