#include "stdafx.h"
#include "GameFramework.h"

CGameFramework::CGameFramework()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; i++) m_ppd3dSwapChainBackBuffers[i] = NULL;
	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	gnRtvDescriptorIncrementSize = 0;
	gnDsvDescriptorIncrementSize = 0;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pStageManager = NULL;
	m_pScene = NULL;
	m_pPlayer = NULL;

	m_bMsaa4xEnable = false;
	m_nMsaa4xQualityLevels = 0;

	_tcscpy_s(m_pszFrameRate, _T("LabProject ("));

}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();
	CreateDepthStencilView();
	CreatePostProcessResources();
	CreateMotionBlurComputePipeline();
	CoInitialize(NULL);

	BuildObjects();

	return(true);
}

void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1 **)&m_pdxgiSwapChain);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain **)&m_pdxgiSwapChain);
#endif
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT nDXGIFactoryFlags = 0;
#if defined(_DEBUG)
	ID3D12Debug *pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void **)&pd3dDebugController);
	if (pd3dDebugController)
	{
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void **)&m_pdxgiFactory);

	IDXGIAdapter1 *pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice))) break;
	}

	if (!pd3dAdapter)
	{
		m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void **)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice);
	}

	::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	::gnDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void **)&m_pd3dFence);
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	HRESULT hResult;

	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void **)&m_pd3dCommandQueue);

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&m_pd3dCommandAllocator);

	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&m_pd3dCommandList);
	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));

	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers + 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(
		&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)&m_pd3dRtvDescriptorHeap);

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(
		&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)&m_pd3dDsvDescriptorHeap);
}

void CGameFramework::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void **)&m_ppd3dSwapChainBackBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i], NULL, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += gnRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void **)&m_pd3dDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, &d3dDepthStencilViewDesc, d3dDsvCPUDescriptorHandle);
}
void CGameFramework::CreatePostProcessResources()
{
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = m_nWndClientWidth;
	texDesc.Height = m_nWndClientHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = texDesc.Format;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.125f;
	clearValue.Color[2] = 0.3f;
	clearValue.Color[3] = 1.0f;

	HRESULT hr = m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&m_pd3dSceneColor));

	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	hr = m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&m_pd3dBlurColor));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
		m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	rtvHandle.ptr += (SIZE_T)m_nSwapChainBuffers * (SIZE_T)gnRtvDescriptorIncrementSize;
	m_pd3dDevice->CreateRenderTargetView(m_pd3dSceneColor, nullptr, rtvHandle);
	m_d3dSceneColorRtvCPUHandle = rtvHandle;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	hr = m_pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pd3dPostProcessDescHeap));

	UINT srvUavStride = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = m_pd3dPostProcessDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = m_pd3dPostProcessDescHeap->GetGPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	m_pd3dDevice->CreateShaderResourceView(m_pd3dSceneColor, &srvDesc, cpuStart);
	m_d3dSceneColorSrvGPUHandle = gpuStart;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuUav = cpuStart;
	cpuUav.ptr += srvUavStride;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuUav = gpuStart;
	gpuUav.ptr += srvUavStride;

	m_pd3dDevice->CreateUnorderedAccessView(m_pd3dBlurColor, nullptr, &uavDesc, cpuUav);
	m_d3dBlurColorUavGPUHandle = gpuUav;
}
void CGameFramework::CreateMotionBlurComputePipeline()
{
	D3D12_ROOT_PARAMETER rootParams[3] = {};

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].Descriptor.RegisterSpace = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE rangeSRV = {};
	rangeSRV.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	rangeSRV.NumDescriptors = 1;
	rangeSRV.BaseShaderRegister = 0;
	rangeSRV.RegisterSpace = 0;
	rangeSRV.OffsetInDescriptorsFromTableStart = 0;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].DescriptorTable.pDescriptorRanges = &rangeSRV;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE rangeUAV = {};
	rangeUAV.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	rangeUAV.NumDescriptors = 1;
	rangeUAV.BaseShaderRegister = 0;
	rangeUAV.RegisterSpace = 0;
	rangeUAV.OffsetInDescriptorsFromTableStart = 0;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &rangeUAV;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = _countof(rootParams);
	rootSigDesc.pParameters = rootParams;

	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &samplerDesc;

	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ID3DBlob* pSerialized = nullptr;
	ID3DBlob* pError = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSerialized, &pError);
	hr = m_pd3dDevice->CreateRootSignature(0, pSerialized->GetBufferPointer(), pSerialized->GetBufferSize(),
		IID_PPV_ARGS(&m_pd3dMotionBlurRootSignature));
	if (pSerialized) pSerialized->Release();
	if (pError) pError->Release();

	ID3DBlob* pCS = nullptr;
	ID3DBlob* pErrorCS = nullptr;
	UINT flags = 0;
#if defined(_DEBUG)
	flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	hr = D3DCompileFromFile(L"Shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"CS_MotionBlur", "cs_5_1", flags, 0, &pCS, &pErrorCS);

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pd3dMotionBlurRootSignature;
	psoDesc.CS.BytecodeLength = pCS->GetBufferSize();
	psoDesc.CS.pShaderBytecode = pCS->GetBufferPointer();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	hr = m_pd3dDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pd3dMotionBlurPSO));
	if (pCS) pCS->Release();

	UINT cbSize = (sizeof(MOTION_BLUR_CB) + 255) & ~255;
	hr = m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(cbSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pd3dcbMotionBlur));

	m_pd3dcbMotionBlur->Map(0, nullptr, (void**)&m_pMappedMotionBlurCB);
}

void CGameFramework::ChangeSwapChainState()
{
	WaitForGpuComplete();

	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

	DXGI_MODE_DESC dxgiTargetParameters;
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiTargetParameters.Width = m_nWndClientWidth;
	dxgiTargetParameters.Height = m_nWndClientHeight;
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth, m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam, CCamera* pCamera)
{

	switch (nMessageID)
	{
		case WM_LBUTTONDOWN:
			if (m_pStageManager) m_pStageManager->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam, pCamera);
			::SetCapture(hWnd);
			::GetCursorPos(&m_ptOldCursorPos);

			break;
		case WM_LBUTTONUP:
			::ReleaseCapture();
			if (m_pStageManager) m_pStageManager->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam, pCamera);

			break;
		case WM_MOUSEMOVE:
			break;
		default:
			if (m_pStageManager) m_pStageManager->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam,  pCamera);
			break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_F1:
				case VK_F2:
				case VK_F3:
					m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());
					break;
				case VK_F9:
					ChangeSwapChainState();
					break;
				case VK_F5:
					m_nBlurMode = BLUR_GAUSSIAN;
					break;
				case VK_F6:
					m_nBlurMode = BLUR_BOX;
					break;
				case VK_F7:
					m_nBlurMode = BLUR_OFF;
					break;
				default:
					if (m_pStageManager) m_pStageManager->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
					break;
			}
			break;
		case WM_KEYDOWN:
			if (m_pStageManager) m_pStageManager->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
			break;
		default:

			break;
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE)
				m_GameTimer.Stop();
			else
				m_GameTimer.Start();
			break;
		}
		case WM_SIZE:
			break;
		case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
			OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam,  m_pCamera);
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
			OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
			break;
	}
	return(0);
}

void CGameFramework::OnDestroy()
{
    ReleaseObjects();

	::CloseHandle(m_hFenceEvent);

	if (m_pd3dcbMotionBlur)
	{
		if (m_pMappedMotionBlurCB) m_pd3dcbMotionBlur->Unmap(0, NULL);
		m_pd3dcbMotionBlur->Release();
		m_pd3dcbMotionBlur = nullptr;
		m_pMappedMotionBlurCB = nullptr;
	}
	if (m_pd3dMotionBlurPSO) { m_pd3dMotionBlurPSO->Release(); m_pd3dMotionBlurPSO = nullptr; }
	if (m_pd3dMotionBlurRootSignature) { m_pd3dMotionBlurRootSignature->Release(); m_pd3dMotionBlurRootSignature = nullptr; }
	if (m_pd3dPostProcessDescHeap) { m_pd3dPostProcessDescHeap->Release(); m_pd3dPostProcessDescHeap = nullptr; }
	if (m_pd3dBlurColor) { m_pd3dBlurColor->Release(); m_pd3dBlurColor = nullptr; }
	if (m_pd3dSceneColor) { m_pd3dSceneColor->Release(); m_pd3dSceneColor = nullptr; }

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();

	if (m_pd3dCommandAllocator) m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	if (m_pd3dFence) m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
    if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();

#if defined(_DEBUG)
	IDXGIDebug1	*pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void **)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif

	::CoUninitialize();
}

void CGameFramework::BuildObjects()
{
	float a = XMConvertToDegrees(atan(0.5));

	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	m_pStageManager = new CStageManager();
	if (m_pStageManager) m_pStageManager->BuildObjects(m_pd3dDevice, m_pd3dCommandList);

	m_pScene = m_pStageManager->GetScene();

	CAirplanePlayer *pAirplanePlayer = new CAirplanePlayer(m_pd3dDevice, m_pd3dCommandList, m_pStageManager->GetGraphicsRootSignature(), m_pScene);
	pAirplanePlayer->SetPosition(XMFLOAT3(920.0f, 745.0f, 1270.0));
	m_pStageManager->SetPlayer(pAirplanePlayer);
	m_pPlayer = pAirplanePlayer;
	m_pCamera = m_pPlayer->GetCamera();

	m_pd3dCommandList->Close();
	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pStageManager) m_pStageManager->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

	m_GameTimer.Reset();
}

void CGameFramework::ReleaseObjects()
{
	if (m_pPlayer) m_pPlayer->Release();

	if (m_pStageManager) m_pStageManager->ReleaseObjects();
	if (m_pStageManager) delete m_pStageManager;
}

void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;

	if (GetKeyboardState(pKeysBuffer) && m_pStageManager) bProcessedByScene = m_pStageManager->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene)
	{
		m_pPlayer->StoreOriginalPosition();
		DWORD dwDirection = 0;

		if (pKeysBuffer[VK_UP] & 0xF0) dwDirection |= DIR_FORWARD;
		if (pKeysBuffer[VK_DOWN] & 0xF0) dwDirection |= DIR_BACKWARD;
		if (pKeysBuffer[VK_LEFT] & 0xF0) dwDirection |= DIR_LEFT;
		if (pKeysBuffer[VK_RIGHT] & 0xF0) dwDirection |= DIR_RIGHT;
		if (pKeysBuffer[VK_SHIFT] & 0xF0) dwDirection |= DIR_UP;
		if (pKeysBuffer[VK_CONTROL] & 0xF0) dwDirection |= DIR_DOWN;

		float cxDelta = 0.0f, cyDelta = 0.0f;
		POINT ptCursorPos;
		if (GetCapture() == m_hWnd)
		{
			SetCursor(NULL);
			GetCursorPos(&ptCursorPos);
			cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
			cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
			SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
		}

		if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
		{
			if (cxDelta || cyDelta)
			{
				if (pKeysBuffer[VK_RBUTTON] & 0xF0)
					m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta);
				else
					m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
			}
			m_pPlayer->SetMoveInput(dwDirection != 0);
			if (dwDirection)
			{

				m_pPlayer->Move(dwDirection, 2.25f, true);
			}
		}
	}
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
}
void CGameFramework::ChangeScene()
{
	WaitForGpuComplete();

	if (m_pStageManager) m_pStageManager->ReleaseObjects();
	if (m_pPlayer) {
		delete m_pPlayer;
		m_pPlayer = NULL;
		m_pCamera = NULL;
	}

	m_pStageManager->ChangeScene();
	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	m_pScene = m_pStageManager->GetScene();

	if (m_pStageManager) m_pStageManager->BuildObjects(m_pd3dDevice, m_pd3dCommandList);

	CAirplanePlayer* pAirplanePlayer = new CAirplanePlayer(m_pd3dDevice, m_pd3dCommandList, m_pStageManager->GetGraphicsRootSignature(), m_pScene);
	pAirplanePlayer->SetPosition(XMFLOAT3(920.0f, 745.0f, 1270.0));
	m_pStageManager->SetPlayer(pAirplanePlayer);
	m_pPlayer = pAirplanePlayer;
	m_pCamera = m_pPlayer->GetCamera();
	m_pStageManager->GetScene()->SetPlayer(m_pPlayer);

	hResult = m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pStageManager) m_pStageManager->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();
}
void CGameFramework::AnimateObjects()
{
	float fTimeElapsed = m_GameTimer.GetTimeElapsed();

	if (m_pStageManager) m_pStageManager->AnimateObjects(fTimeElapsed);

	m_pPlayer->Animate(fTimeElapsed, NULL);
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::FrameAdvance()
{
	m_GameTimer.Tick(0.0f);

	ProcessInput();
	AnimateObjects();

	if (m_pStageManager && m_pStageManager->GetManagerApproveChange())
	{
		ChangeScene();
	}

	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	D3D12_RESOURCE_BARRIER barriers[3] = {};

	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Transition.pResource = m_pd3dSceneColor;
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[1].Transition.pResource = m_pd3dBlurColor;
	barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[2].Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
	barriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[2].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_pd3dCommandList->ResourceBarrier(3, barriers);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pScene = m_pStageManager->GetScene();

	if (m_pScene)
		m_pScene->RenderShadowMap(m_pd3dCommandList);
	m_pd3dCommandList->ClearRenderTargetView(m_d3dSceneColorRtvCPUHandle, pfClearColor, 0, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle =
		m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	m_pd3dCommandList->ClearDepthStencilView(
		d3dDsvCPUDescriptorHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, NULL);

	m_pd3dCommandList->OMSetRenderTargets(
		1,
		&m_d3dSceneColorRtvCPUHandle,
		TRUE,
		&d3dDsvCPUDescriptorHandle);

	if (m_pStageManager) m_pStageManager->Render(m_pd3dCommandList, m_pCamera);

#ifdef _WITH_PLAYER_TOP
	m_pd3dCommandList->ClearDepthStencilView(
		d3dDsvCPUDescriptorHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, NULL);
#endif

	D3D12_RESOURCE_BARRIER sceneToSrv = {};
	sceneToSrv.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	sceneToSrv.Transition.pResource = m_pd3dSceneColor;
	sceneToSrv.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	sceneToSrv.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	sceneToSrv.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &sceneToSrv);

	ID3D12DescriptorHeap* ppHeaps[] = { m_pd3dPostProcessDescHeap };
	m_pd3dCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_pd3dCommandList->SetComputeRootSignature(m_pd3dMotionBlurRootSignature);
	m_pd3dCommandList->SetPipelineState(m_pd3dMotionBlurPSO);

	float speed = 0.0f;
	if (m_pPlayer) speed = m_pPlayer->GetSpeed();

	float speedThreshold = 8.0f;
	float maxSpeed = 80.0f;
	float blurStrength = 0.0f;

	if (speed > speedThreshold)
	{
		blurStrength = (speed - speedThreshold) / (maxSpeed - speedThreshold);
		if (blurStrength > 3.0f) blurStrength = 3.0f;
		if (blurStrength < 0.0f) blurStrength = 0.0f;
	}

	m_pMappedMotionBlurCB->gScreenSize = XMFLOAT2((float)m_nWndClientWidth, (float)m_nWndClientHeight);
	m_pMappedMotionBlurCB->gBlurStrength = (m_nBlurMode == BLUR_OFF) ? 0.0f : blurStrength;
	m_pMappedMotionBlurCB->gSampleCount = (float)m_nBlurMode;

	D3D12_GPU_VIRTUAL_ADDRESS cbAddr = m_pd3dcbMotionBlur->GetGPUVirtualAddress();
	m_pd3dCommandList->SetComputeRootConstantBufferView(0, cbAddr);
	m_pd3dCommandList->SetComputeRootDescriptorTable(1, m_d3dSceneColorSrvGPUHandle);
	m_pd3dCommandList->SetComputeRootDescriptorTable(2, m_d3dBlurColorUavGPUHandle);

	UINT groupX = (m_nWndClientWidth + 7) / 8;
	UINT groupY = (m_nWndClientHeight + 7) / 8;
	m_pd3dCommandList->Dispatch(groupX, groupY, 1);

	D3D12_RESOURCE_BARRIER blurToCopy = {};
	blurToCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	blurToCopy.Transition.pResource = m_pd3dBlurColor;
	blurToCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	blurToCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	blurToCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &blurToCopy);

	m_pd3dCommandList->CopyResource(
		m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex],
		m_pd3dBlurColor);

	{

		D3D12_RESOURCE_BARRIER bbToRT = {};
		bbToRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		bbToRT.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
		bbToRT.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		bbToRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		bbToRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_pd3dCommandList->ResourceBarrier(1, &bbToRT);

		UINT rtvInc = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE bbRtv = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		bbRtv.ptr += (SIZE_T)m_nSwapChainBufferIndex * rtvInc;

		m_pd3dCommandList->OMSetRenderTargets(1, &bbRtv, TRUE, &d3dDsvCPUDescriptorHandle);

		if (m_pScene)
			m_pScene->RenderOverlay(m_pd3dCommandList, m_pCamera);

		D3D12_RESOURCE_BARRIER rtToPresent = {};
		rtToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rtToPresent.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
		rtToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rtToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rtToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_pd3dCommandList->ResourceBarrier(1, &rtToPresent);
	}

	hResult = m_pd3dCommandList->Close();

	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

#ifdef _WITH_PRESENT_PARAMETERS
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters = {};
	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
	m_pdxgiSwapChain->Present(1, 0);
#else
	m_pdxgiSwapChain->Present(0, 0);
#endif
#endif

	MoveToNextFrame();

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
	size_t nLength = _tcslen(m_pszFrameRate);
	XMFLOAT3 xmf3Position = m_pPlayer->GetPosition();
	_stprintf_s(m_pszFrameRate + nLength, 70 - nLength,
		_T("(%4f, %4f, %4f)"), xmf3Position.x, xmf3Position.y, xmf3Position.z);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}
