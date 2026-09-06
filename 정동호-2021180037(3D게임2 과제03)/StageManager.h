#pragma once
#include "Shader.h"
#include "Player.h"
#include "Scene.h"
#include "MainMenuScene.h"
#include "BattleScene.h"
#include <stack>

#define MainMenuStage 0x00
#define SelectStage 0x01
#define BattleStage 0x02

class CStageManager
{
public:
    CStageManager();
    ~CStageManager();
private:
    CScene* m_pScene = NULL;
    CPlayer* m_pPlayer = NULL;
    bool m_bChangeSceneBool = FALSE;
    int m_bChangeSceneNum = NULL;
public:

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
    void SetScene(CScene* curScene) { m_pScene = curScene; }
	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	ID3D12RootSignature* GetGraphicsRootSignature() { return m_pScene->GetGraphicsRootSignature(); }

    void AnimateObjects(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
    void ReleaseObjects();
    void ReleaseUploadBuffers();

	bool ProcessInput(UCHAR* pKeysBuffer);
    CScene* GetScene() { return m_pScene; };
    void SetPlayer(CPlayer* curPlayer) { m_pPlayer = curPlayer; m_pScene->m_pPlayer= curPlayer;}
    CCamera* GetCamera();
    bool GetNextSceneBool();

    ID3D12RootSignature* CreateRepresentativeGraphicsRootSignature(ID3D12Device* pd3dDevice);
    ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;

    int GetSceneNum() { return m_pScene->GetSceneNum(); }
    bool GetSceneState() { return m_pScene->GetSceneState(); }
    bool GetManagerApproveChange() { return m_bChangeSceneBool; }
    void ChangeScene();

    bool m_bPopSceneApprove = FALSE;
    void ChangePopSceneApprove(){m_bPopSceneApprove= (m_bPopSceneApprove == FALSE) ? TRUE : FALSE;}
    std::stack<int> m_sceneHistory;
};
