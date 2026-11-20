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
	
private:
    CMaterial* CreateHudMaterial(ID3D12Device* device, ID3D12GraphicsCommandList* commands,
        const wchar_t* texturePath, CShader* shader);
    CGameObject* CreateHudQuad(ID3D12Device* device, ID3D12GraphicsCommandList* commands,
        const char* name, CMaterial* material, float width, float height, float centerX, float centerYFromTop,
        const XMFLOAT4& meshUv = XMFLOAT4(0, 0, 1, 1));
    static void SetHudNumber(CGameObject* const* digits, size_t count, int value, bool leadingZeros);
    CGameObject* GetLiveLockTarget() const;
    void BuildFlightDisplay(ID3D12Device* device, ID3D12GraphicsCommandList* commands);
    void UpdateFlightDisplay();
    void BuildTargetDisplay(ID3D12Device* device, ID3D12GraphicsCommandList* commands);
    void UpdateTargetDisplay(CCamera* camera);
    void UpdateBattleHud(float elapsed);

    struct RadarContact {
        CGameObject* enemy;
        CGameObject* marker;
    };
    std::vector<RadarContact> m_radarContacts;
    CGameObject* m_pRadarSweep = nullptr;
    CGameObject* m_pHullFill = nullptr;
    CGameObject* m_hullDigits[3] = {};
    CGameObject* m_scoreDigits[6] = {};

    CGameObject* m_pHeadingTape = nullptr;
    CGameObject* m_ppHeadingDigits[3] = {};
    CGameObject* m_ppSpeedDigits[3] = {};
    CGameObject* m_ppHeightDigits[4] = {};

    CGameObject* m_pAimReticle = nullptr;
    CGameObject* m_pTargetCorners[4] = {};
    CGameObject* m_pTargetLockCue = nullptr;
    CGameObject* m_pTargetDisplayTracked = nullptr;
    float m_fTargetAcquiredAt = 0.0f;
    float m_fHudTime = 0.0f;
};

