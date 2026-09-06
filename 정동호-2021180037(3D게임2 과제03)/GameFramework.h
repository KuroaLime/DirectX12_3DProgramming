#pragma once

#define FRAME_BUFFER_WIDTH		1280
#define FRAME_BUFFER_HEIGHT		960

#include "Timer.h"
#include "Player.h"
#include "StageManager.h"
#include "Scene.h"
#include "MainMenuScene.h"

// 후처리(모션 블러) 모드. 이 값이 그대로 컴퓨트 셰이더의 gSampleCount 로 전달되어
// CS_MotionBlur 안에서 어떤 블러 알고리즘을 쓸지 고르는 분기 값이 된다.
//   F5 = BLUR_GAUSSIAN, F6 = BLUR_ZOOM, F7 = BLUR_OFF (GameFramework.cpp 의 키 처리 참고)
enum BLUR_MODE : int
{
	BLUR_OFF = 0,		// 블러 끄기 (강도를 0 으로 밀어 원본을 그대로 통과시킨다)
	BLUR_GAUSSIAN = 1,	// 가우시안 블러 (상하좌우 십자 방향 5탭 가중 평균)
	BLUR_ZOOM = 2		// 줌(방사형) 블러 (화면 중심에서 바깥으로 뻗는 방향으로만 샘플링)
};

// 응용 프로그램 전체를 총괄하는 클래스.
// 생성 순서 : OnCreate() -> 디바이스/스왑체인/힙/후처리 리소스 생성 -> BuildObjects()
// 실행 순서 : FrameAdvance() 를 매 프레임 반복 (입력 -> 애니메이션 -> 렌더링 -> Present)
// 종료 순서 : OnDestroy() -> ReleaseObjects() -> 각종 COM 리소스 Release
class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);	// 초기화 진입점
	void OnDestroy();									// 종료 및 리소스 해제

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void ChangeSwapChainState();						// 전체화면 <-> 창모드 전환 (F9)

    void BuildObjects();								// 스테이지/플레이어 등 게임 객체 생성
    void ReleaseObjects();

    void ProcessInput();								// [1단계] 키보드/마우스 입력 처리
    void AnimateObjects();								// [2단계] 객체 갱신(이동, 충돌, 애니메이션)
    void FrameAdvance();								// 한 프레임 전체 흐름 (1 -> 2 -> 렌더링)

	void WaitForGpuComplete();							// 펜스로 GPU 작업 완료까지 대기
	void MoveToNextFrame();								// 다음 후면버퍼 인덱스로 전환

	// 윈도우 메시지 -> 프레임워크 -> 스테이지 매니저 -> 현재 씬 순으로 입력이 전달된다.
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void ChangeScene();									// 메뉴 <-> 배틀 씬 교체
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

	ID3D12Resource* m_pd3dSceneColor = nullptr;
	ID3D12Resource* m_pd3dBlurColor = nullptr;

	ID3D12DescriptorHeap* m_pd3dPostProcessDescHeap = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSceneColorSrvGPUHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dBlurColorUavGPUHandle{};

	ID3D12RootSignature* m_pd3dMotionBlurRootSignature = nullptr;
	ID3D12PipelineState* m_pd3dMotionBlurPSO = nullptr;
	ID3D12Resource* m_pd3dcbMotionBlur = nullptr;

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

		return m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}

	float    gSampleCount;
	int m_nBlurMode = BLUR_GAUSSIAN;
};
