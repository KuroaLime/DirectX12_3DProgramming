#pragma once

#define FRAME_BUFFER_WIDTH		1280
#define FRAME_BUFFER_HEIGHT		960

#include "Timer.h"
#include "Player.h"
#include "StageManager.h"
#include "Scene.h"
#include "MainMenuScene.h"

enum BLUR_MODE : int
{
	BLUR_OFF = 0,
	BLUR_GAUSSIAN = 1,
	BLUR_BOX = 2
};
class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void ChangeSwapChainState();

    void BuildObjects();
    void ReleaseObjects();

    void ProcessInput();
    void AnimateObjects();
    void FrameAdvance();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void ChangeScene();
private:
	HINSTANCE					m_hInstance;
	HWND						m_hWnd; 

	int							m_nWndClientWidth;
	int							m_nWndClientHeight;
        
	IDXGIFactory4				*m_pdxgiFactory = NULL;
	IDXGISwapChain3				*m_pdxgiSwapChain = NULL;
	ID3D12Device				*m_pd3dDevice = NULL;

	bool						m_bMsaa4xEnable = false;
	UINT						m_nMsaa4xQualityLevels = 0;

	static const UINT			m_nSwapChainBuffers = 2;
	UINT						m_nSwapChainBufferIndex;

	ID3D12Resource				*m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	ID3D12DescriptorHeap		*m_pd3dRtvDescriptorHeap = NULL;

	ID3D12Resource				*m_pd3dDepthStencilBuffer = NULL;
	ID3D12DescriptorHeap		*m_pd3dDsvDescriptorHeap = NULL;

	ID3D12CommandAllocator		*m_pd3dCommandAllocator = NULL;
	ID3D12CommandQueue			*m_pd3dCommandQueue = NULL;
	ID3D12GraphicsCommandList	*m_pd3dCommandList = NULL;

	ID3D12Fence					*m_pd3dFence = NULL;
	UINT64						m_nFenceValues[m_nSwapChainBuffers];
	HANDLE						m_hFenceEvent;

#if defined(_DEBUG)
	ID3D12Debug					*m_pd3dDebugController;
#endif

	CGameTimer					m_GameTimer;

	CStageManager*				m_pStageManager = NULL;
	CScene						*m_pScene = NULL;
	CPlayer						*m_pPlayer = NULL;
	CCamera						*m_pCamera = NULL;

	POINT						m_ptOldCursorPos;

	_TCHAR						m_pszFrameRate[70];


	void CreatePostProcessResources();
	void CreateMotionBlurComputePipeline();
protected:
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSceneColorRtvCPUHandle{};
	// 모션블러용 리소스
	ID3D12Resource* m_pd3dSceneColor = nullptr;   // 장면을 먼저 그릴 RT
	ID3D12Resource* m_pd3dBlurColor = nullptr;   // Compute 결과 저장용

	ID3D12DescriptorHeap* m_pd3dPostProcessDescHeap = nullptr; // SRV/UAV용

	// SRV/UAV 핸들
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSceneColorSrvGPUHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dBlurColorUavGPUHandle{};

	// Compute용
	ID3D12RootSignature* m_pd3dMotionBlurRootSignature = nullptr;
	ID3D12PipelineState* m_pd3dMotionBlurPSO = nullptr;
	ID3D12Resource* m_pd3dcbMotionBlur = nullptr; // cbMotionBlur

	struct MOTION_BLUR_CB
	{
		XMFLOAT2 gScreenSize;
		float    gBlurStrength;
		float    gSampleCount;
	}* m_pMappedMotionBlurCB = nullptr;


public:
	D3D12_CPU_DESCRIPTOR_HANDLE GetSceneColorRtvHandle() const { return m_d3dSceneColorRtvCPUHandle; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetMainDsvHandle() const
	{
			// 메인 깊이 버퍼 DSV는 DSV 힙 시작 핸들
		return m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}

	float    gSampleCount;
	int m_nBlurMode = BLUR_GAUSSIAN;
};

