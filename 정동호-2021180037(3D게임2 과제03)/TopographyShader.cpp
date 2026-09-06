#include "TopographyShader.h"

CTerrainShader::CTerrainShader()
{
}

CTerrainShader::~CTerrainShader()
{
}

D3D12_INPUT_LAYOUT_DESC CTerrainShader::CreateInputLayout()
{
	UINT nInputElementDescs = 4;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_SHADER_BYTECODE CTerrainShader::CreateVertexShader()
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSTerrain", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CTerrainShader::CreatePixelShader()
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSTerrain", "ps_5_1", &m_pd3dPixelShaderBlob));
}
D3D12_SHADER_BYTECODE CTerrainShader::CreateHullShader()
{
	return CShader::CompileShaderFromFile(L"Shaders.hlsl", "HSTerrain", "hs_5_1", &m_pd3dHullShaderBlob);
}

D3D12_SHADER_BYTECODE CTerrainShader::CreateDomainShader()
{
	return CShader::CompileShaderFromFile(L"Shaders.hlsl", "DSTerrain", "ds_5_1", &m_pd3dDomainShaderBlob);
}
void CTerrainShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	m_nPipelineStates = 3;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];
	m_ppd3dPipelineStates[0] = nullptr;
	m_ppd3dPipelineStates[1] = nullptr;

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc = CreateInputLayout();

	ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;

	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.HS = CreateHullShader();
	m_d3dPipelineStateDesc.DS = CreateDomainShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();

	if (!m_d3dPipelineStateDesc.VS.pShaderBytecode || !m_d3dPipelineStateDesc.HS.pShaderBytecode ||
		!m_d3dPipelineStateDesc.DS.pShaderBytecode || !m_d3dPipelineStateDesc.PS.pShaderBytecode)
	{
		OutputDebugStringA("ERROR: One or more Terrain Shaders failed to compile!\n");
		return;
	}

	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = d3dInputLayoutDesc;
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;

	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;

	m_d3dPipelineStateDesc.NumRenderTargets = 1;
	m_d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(
		&m_d3dPipelineStateDesc,
		__uuidof(ID3D12PipelineState),
		(void**)&m_ppd3dPipelineStates[0]);
	if (FAILED(hResult))
	{
		OutputDebugStringA("ERROR: Failed to create Terrain PSO!\n");

		return;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dReflectPSODesc = m_d3dPipelineStateDesc;

	d3dReflectPSODesc.DepthStencilState.DepthEnable = TRUE;
	d3dReflectPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dReflectPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	d3dReflectPSODesc.DepthStencilState.StencilEnable = TRUE;
	d3dReflectPSODesc.DepthStencilState.StencilReadMask = 0xFF;
	d3dReflectPSODesc.DepthStencilState.StencilWriteMask = 0x00;

	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.BackFace = d3dReflectPSODesc.DepthStencilState.FrontFace;

	d3dReflectPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	hResult = pd3dDevice->CreateGraphicsPipelineState(
		&d3dReflectPSODesc,
		__uuidof(ID3D12PipelineState),
		(void**)&m_ppd3dPipelineStates[1]);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dWireDesc = m_d3dPipelineStateDesc;
	d3dWireDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	d3dWireDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dWireDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[2]);
	if (FAILED(hr))
	{
		OutputDebugStringA("ERROR: Failed to create Terrain PSO[2] wireframe!\n");

		m_ppd3dPipelineStates[2] = m_ppd3dPipelineStates[0];
	}

	if (m_pd3dVertexShaderBlob)   m_pd3dVertexShaderBlob->Release();
	if (m_pd3dHullShaderBlob)     m_pd3dHullShaderBlob->Release();
	if (m_pd3dDomainShaderBlob)   m_pd3dDomainShaderBlob->Release();
	if (m_pd3dPixelShaderBlob)    m_pd3dPixelShaderBlob->Release();

	if (d3dInputLayoutDesc.pInputElementDescs)
		delete[] d3dInputLayoutDesc.pInputElementDescs;
}

CBillboardShader::CBillboardShader()
{
	m_pd3dGeometryShaderBlob = NULL;
}

CBillboardShader::~CBillboardShader()
{
}

D3D12_INPUT_LAYOUT_DESC CBillboardShader::CreateInputLayout()
{
	UINT nInputElementDescs = 3;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] =
	{
		"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
	};
	pd3dInputElementDescs[1] =
	{
		"CENTER", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		1, 0,
		D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1
	};
	pd3dInputElementDescs[2] =
	{
		"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT,
		1, 12,
		D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1
	};

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return d3dInputLayoutDesc;
}

D3D12_RASTERIZER_DESC CBillboardShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc = CShader::CreateRasterizerState();
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	return(d3dRasterizerDesc);
}

D3D12_SHADER_BYTECODE CBillboardShader::CreateGeometryShader()
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "GS_Billboard", "gs_5_1", &m_pd3dGeometryShaderBlob));
}
D3D12_SHADER_BYTECODE CBillboardShader::CreateVertexShader()
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VS_GS_Billboard", "vs_5_1", &m_pd3dVertexShaderBlob));
}
D3D12_SHADER_BYTECODE CBillboardShader::CreatePixelShader()
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PS_GS_Billboard", "ps_5_1", &m_pd3dPixelShaderBlob));
}
void CBillboardShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	m_nPipelineStates = 2;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	m_ppd3dPipelineStates[0] = NULL;
	m_ppd3dPipelineStates[1] = NULL;

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc = CreateInputLayout();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineStateDesc;
	::ZeroMemory(&d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	d3dPipelineStateDesc.VS = CreateVertexShader();
	d3dPipelineStateDesc.GS = CreateGeometryShader();
	d3dPipelineStateDesc.PS = CreatePixelShader();
	d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	d3dPipelineStateDesc.BlendState = CreateBlendState();
	d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	d3dPipelineStateDesc.InputLayout = d3dInputLayoutDesc;
	d3dPipelineStateDesc.SampleMask = UINT_MAX;

	d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	d3dPipelineStateDesc.NumRenderTargets = 1;
	d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dPipelineStateDesc.SampleDesc.Count = 1;
	d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dReflectPSODesc = d3dPipelineStateDesc;

	d3dReflectPSODesc.DepthStencilState.DepthEnable = TRUE;
	d3dReflectPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dReflectPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	d3dReflectPSODesc.DepthStencilState.StencilEnable = TRUE;
	d3dReflectPSODesc.DepthStencilState.StencilReadMask = 0xFF;
	d3dReflectPSODesc.DepthStencilState.StencilWriteMask = 0x00;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.BackFace = d3dReflectPSODesc.DepthStencilState.FrontFace;

	d3dReflectPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	hResult = pd3dDevice->CreateGraphicsPipelineState(&d3dReflectPSODesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[1]);
	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dGeometryShaderBlob) m_pd3dGeometryShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (d3dPipelineStateDesc.InputLayout.pInputElementDescs)
		delete[] d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}
void CBillboardShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CHeightMapTerrain* pTerrain, CScene* pOwnerScene)
{

	m_ppTextures[0] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_ppTextures[0]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, (wchar_t*)L"Image/Grass01.dds", RESOURCE_TEXTURE2D, 0);
	m_ppTextures[1] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_ppTextures[1]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, (wchar_t*)L"Image/Grass02.dds", RESOURCE_TEXTURE2D, 0);
	m_ppMaterials[0] = new CMaterial();
	m_ppMaterials[0]->SetShader(this);
	m_ppMaterials[0]->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_ppMaterials[0]->SetTexture(m_ppTextures[0]);
	m_ppMaterials[1] = new CMaterial();
	m_ppMaterials[1]->SetShader(this);
	m_ppMaterials[1]->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_ppMaterials[1]->SetTexture(m_ppTextures[1]);

	m_ppTextures[2] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_ppTextures[2]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, (wchar_t*)L"Image/Flower01.dds", RESOURCE_TEXTURE2D, 0);
	m_ppTextures[3] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_ppTextures[3]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, (wchar_t*)L"Image/Flower02.dds", RESOURCE_TEXTURE2D, 0);

	m_ppMaterials[2] = new CMaterial();
	m_ppMaterials[2]->SetShader(this);
	m_ppMaterials[2]->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_ppMaterials[2]->SetTexture(m_ppTextures[2]);
	m_ppMaterials[3] = new CMaterial();
	m_ppMaterials[3]->SetShader(this);
	m_ppMaterials[3]->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_ppMaterials[3]->SetTexture(m_ppTextures[3]);

	m_ppTextures[4] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_ppTextures[4]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, (wchar_t*)L"Image/Tree01.dds", RESOURCE_TEXTURE2D, 0);
	m_ppTextures[5] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_ppTextures[5]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, (wchar_t*)L"Image/Tree02.dds", RESOURCE_TEXTURE2D, 0);
	m_ppTextures[6] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_ppTextures[6]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, (wchar_t*)L"Image/Tree03.dds", RESOURCE_TEXTURE2D, 0);

	m_ppMaterials[4] = new CMaterial();
	m_ppMaterials[4]->SetShader(this);
	m_ppMaterials[4]->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_ppMaterials[4]->SetTexture(m_ppTextures[4]);
	m_ppMaterials[5] = new CMaterial();
	m_ppMaterials[5]->SetShader(this);
	m_ppMaterials[5]->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_ppMaterials[5]->SetTexture(m_ppTextures[5]);
	m_ppMaterials[6] = new CMaterial();
	m_ppMaterials[6]->SetShader(this);
	m_ppMaterials[6]->SetMaterialType(MATERIAL_ALBEDO_MAP);
	m_ppMaterials[6]->SetTexture(m_ppTextures[6]);

	for (int i = 0; i < BillBoardType; i++)
		pOwnerScene->CreateShaderResourceView(pd3dDevice, m_ppTextures[i], 0, 3);

	std::vector<BILLBOARD_INSTANCE> vInstancesGrass[2];
	std::vector<BILLBOARD_INSTANCE> vInstancesFlower[2];
	std::vector<BILLBOARD_INSTANCE> vInstancesTree[3];
	CRawFormatImage* pRawFormatImage = new CRawFormatImage(L"Image/ObjectsMap.raw", 257, 257, true);
	XMFLOAT3 xmf3Scale = pTerrain->GetScale();
	XMFLOAT2 xmf2Size[4] = { XMFLOAT2(8.0f, 8.0f),XMFLOAT2(8.0f, 16.0f),XMFLOAT2(24.0f, 36.0f),XMFLOAT2(16.0f, 46.0f) };
	float fyOffset[7] = { {8.0f * 0.5f},{6.0f * 0.5f},{16.0f * 0.5f} ,{16.0f * 0.5f} ,{33.0f * 0.5f} ,{33.0f * 0.5f} ,{40.0f * 0.5f} };
	for (int z = 2; z <= 254; z++)
	{
		for (int x = 2; x <= 254; x++)
		{
			BYTE nPixel = pRawFormatImage->GetRawImagePixel(x, z);
			float fx = NULL;
			float fz = NULL;
			float fy = NULL;
			BILLBOARD_INSTANCE inst;
			switch (nPixel) {
			case 102:
				fx = float(x) * xmf3Scale.x;
				fz = float(z) * xmf3Scale.z;
				fy = pTerrain->GetHeight(fx, fz) + fyOffset[0];

				inst.m_xmf3Center = XMFLOAT3(fx, fy, fz);
				inst.m_xmf2Size = xmf2Size[0];
				vInstancesGrass[0].push_back(inst);
				break;
			case 128:
				fx = float(x) * xmf3Scale.x;
				fz = float(z) * xmf3Scale.z;
				fy = pTerrain->GetHeight(fx, fz) + fyOffset[1];

				inst.m_xmf3Center = XMFLOAT3(fx, fy, fz);
				inst.m_xmf2Size = xmf2Size[0];
				vInstancesGrass[1].push_back(inst);
				break;
			case 153:
				fx = float(x) * xmf3Scale.x;
				fz = float(z) * xmf3Scale.z;
				fy = pTerrain->GetHeight(fx, fz) + fyOffset[2];

				inst.m_xmf3Center = XMFLOAT3(fx, fy, fz);
				inst.m_xmf2Size = xmf2Size[1];
				vInstancesFlower[0].push_back(inst);
				break;
			case 179:
				fx = float(x) * xmf3Scale.x;
				fz = float(z) * xmf3Scale.z;
				fy = pTerrain->GetHeight(fx, fz) + fyOffset[3];

				inst.m_xmf3Center = XMFLOAT3(fx, fy, fz);
				inst.m_xmf2Size = xmf2Size[1];
				vInstancesFlower[1].push_back(inst);
				break;
			case 204:
				fx = float(x) * xmf3Scale.x;
				fz = float(z) * xmf3Scale.z;
				fy = pTerrain->GetHeight(fx, fz) + fyOffset[4];

				inst.m_xmf3Center = XMFLOAT3(fx, fy, fz);
				inst.m_xmf2Size = xmf2Size[2];
				vInstancesTree[0].push_back(inst);
				break;
			case 225:
				fx = float(x) * xmf3Scale.x;
				fz = float(z) * xmf3Scale.z;
				fy = pTerrain->GetHeight(fx, fz) + fyOffset[5];

				inst.m_xmf3Center = XMFLOAT3(fx, fy, fz);
				inst.m_xmf2Size = xmf2Size[2];
				vInstancesTree[1].push_back(inst);
				break;
			case 255:
				fx = float(x) * xmf3Scale.x;
				fz = float(z) * xmf3Scale.z;
				fy = pTerrain->GetHeight(fx, fz) + fyOffset[6];

				inst.m_xmf3Center = XMFLOAT3(fx, fy, fz);
				inst.m_xmf2Size = xmf2Size[3];
				vInstancesTree[2].push_back(inst);
			default:
				break;
			}
		}
	}
	delete pRawFormatImage;

	BILLBOARD_VERTEX vertices[1];
	vertices[0].m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);

	for (int j = 0; j < BillBoardType; j++) {
		if (j < 2)
		{
			m_nInstances[j] = static_cast<UINT>(vInstancesGrass[j].size());
			m_pd3dInstanceBuffer[j] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, vInstancesGrass[j].data(), sizeof(BILLBOARD_INSTANCE) * m_nInstances[j], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dInstanceUploadBuffer[j]);
		}
		else if (j < 4)
		{
			m_nInstances[j] = static_cast<UINT>(vInstancesFlower[j - 2].size());
			m_pd3dInstanceBuffer[j] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, vInstancesFlower[j - 2].data(), sizeof(BILLBOARD_INSTANCE) * m_nInstances[j], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dInstanceUploadBuffer[j]);
		}
		else
		{
			m_nInstances[j] = static_cast<UINT>(vInstancesTree[j - 4].size());
			m_pd3dInstanceBuffer[j] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, vInstancesTree[j - 4].data(), sizeof(BILLBOARD_INSTANCE) * m_nInstances[j], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dInstanceUploadBuffer[j]);
		}
		if (m_nInstances == 0) return;

		m_pd3dVertexBuffer[j] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, vertices, sizeof(BILLBOARD_VERTEX) * 1, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer[j]);

		vbViews[j][0].BufferLocation = m_pd3dVertexBuffer[j]->GetGPUVirtualAddress();
		vbViews[j][0].SizeInBytes = sizeof(BILLBOARD_VERTEX) * 1;
		vbViews[j][0].StrideInBytes = sizeof(BILLBOARD_VERTEX);

		vbViews[j][1].BufferLocation = m_pd3dInstanceBuffer[j]->GetGPUVirtualAddress();
		vbViews[j][1].SizeInBytes = sizeof(BILLBOARD_INSTANCE) * m_nInstances[j];
		vbViews[j][1].StrideInBytes = sizeof(BILLBOARD_INSTANCE);

	}
}

void CBillboardShader::ReleaseObjects()
{
	for (int i = 0; i < BillBoardType; i++)
	{
		if (m_pd3dVertexBuffer[i]) { m_pd3dVertexBuffer[i]->Release();   m_pd3dVertexBuffer[i] = NULL; }
		if (m_pd3dInstanceBuffer[i]) { m_pd3dInstanceBuffer[i]->Release(); m_pd3dInstanceBuffer[i] = NULL; }
	}

	for (int i = 0; i < BillBoardType; i++) {
		m_ppMaterials[i]->Release();
		m_ppMaterials[i] = NULL;
		m_ppTextures[i] = NULL;
	}
}

void CBillboardShader::ReleaseUploadBuffers()
{
	for (int i = 0; i < BillBoardType; i++)
	{
		if (m_pd3dVertexUploadBuffer[i]) { m_pd3dVertexUploadBuffer[i]->Release();   m_pd3dVertexUploadBuffer[i] = NULL; }
		if (m_pd3dInstanceUploadBuffer[i]) { m_pd3dInstanceUploadBuffer[i]->Release(); m_pd3dInstanceUploadBuffer[i] = NULL; }
	}
	for (int i = 0; i < BillBoardType; i++) {
		m_ppMaterials[i]->ReleaseUploadBuffers();
	}

}

void CBillboardShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	if (!m_nInstances) return;

	CShader::OnPrepareRender(pd3dCommandList, nPipelineState);

	for (int j = 0; j < BillBoardType; j++) {

		m_ppMaterials[j]->UpdateShaderVariables(pd3dCommandList);

		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		pd3dCommandList->IASetVertexBuffers(0, 2, vbViews[j]);

		if (m_nInstances[j] > 0)
		{
			pd3dCommandList->DrawInstanced(1, m_nInstances[j], 0, 0);
		}
	}
}

CFlowingLavaShader::CFlowingLavaShader()
{
}

CFlowingLavaShader::~CFlowingLavaShader()
{
}
D3D12_SHADER_BYTECODE CFlowingLavaShader::CreateVertexShader()
{
	return CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSTextureTransform", "vs_5_1", &m_pd3dVertexShaderBlob);

}

D3D12_SHADER_BYTECODE CFlowingLavaShader::CreatePixelShader()
{
#ifdef _WITH_TEXTURE_TRANSFORM
	return CShader::CompileShaderFromFile(
		L"Shaders.hlsl",
		"PSTextureTransform",
		"ps_5_1",
		&m_pd3dPixelShaderBlob
	);
#endif

#ifdef _WITH_TEXTURE_DISTORTION_TRANSFORM
	return CShader::CompileShaderFromFile(
		L"Shaders.hlsl",
		"PSTextureTransformDistortion",
		"ps_5_1",
		&m_pd3dPixelShaderBlob
	);
#endif

#ifdef _WITH_TEXTURE_DISTORTION_TRANSPARENT_TRANSFORM
	return CShader::CompileShaderFromFile(
		L"Shaders.hlsl",
		"PSTextureTransformDistortionTransparent",
		"ps_5_1",
		&m_pd3dPixelShaderBlob
	);
#endif

}
D3D12_INPUT_LAYOUT_DESC CFlowingLavaShader::CreateInputLayout()
{
	UINT nInputElementDescs = 3;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_RASTERIZER_DESC CFlowingLavaShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

#ifdef _WITH_TEXTURE_DISTORTION_TRANSPARENT_TRANSFORM
D3D12_BLEND_DESC CFlowingLavaShader::CreateBlendState()
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

D3D12_DEPTH_STENCIL_DESC CFlowingLavaShader::CreateDepthStencilState()
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = TRUE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}
#endif
void CFlowingLavaShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	m_nPipelineStates = 2;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];
	m_ppd3dPipelineStates[0] = nullptr;
	m_ppd3dPipelineStates[1] = nullptr;

	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = 1;
	m_d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (FAILED(hResult))
	{
		OutputDebugString(L"!!!!!!!!!! CFlowingLavaShader::CreateGraphicsPipelineState FAILED !!!!!!!!!!\n");
	}
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dReflectPSODesc = m_d3dPipelineStateDesc;

	d3dReflectPSODesc.DepthStencilState.DepthEnable = FALSE;
	d3dReflectPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dReflectPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3dReflectPSODesc.DepthStencilState.StencilEnable = TRUE;
	d3dReflectPSODesc.DepthStencilState.StencilReadMask = 0xFF;
	d3dReflectPSODesc.DepthStencilState.StencilWriteMask = 0x00;

	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dReflectPSODesc.DepthStencilState.BackFace = d3dReflectPSODesc.DepthStencilState.FrontFace;

	d3dReflectPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	d3dReflectPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;

	hResult = pd3dDevice->CreateGraphicsPipelineState(&d3dReflectPSODesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[1]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs)
		delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}

CExplosionShader::CExplosionShader() {}
CExplosionShader::~CExplosionShader() {}

void CExplosionShader::TriggerExplosion(const XMFLOAT3& position, float fScale, float fDuration)
{

	for (auto& exp : m_Explosions)
	{
		if (!exp.bActive)
		{
			exp.bActive = true;
			exp.fElapsed = 0.0f;
			exp.fDuration = fDuration;
			exp.vCenter = position;
			exp.fScale = fScale;
			exp.nSpriteCols = 7;
			exp.nSpriteRows = 1;

			float UScale = 1.0f / 7.0f;
			float VScale = 1.0f;
			exp.uv = XMFLOAT4(0.0f, 0.0f, UScale, VScale);
			return;
		}
	}
}

void CExplosionShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext, CScene* pOwnerScene){

	m_Explosions.resize(MAX_EXPLOSIONS);
	m_InstanceDataCPU.resize(MAX_EXPLOSIONS);

	struct QUAD_VERTEX
	{
		XMFLOAT2 vLocalPos;
		XMFLOAT2 vTex;
	};

	QUAD_VERTEX quadVertices[4] =
	{
		{ XMFLOAT2(-0.5f,  0.5f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT2(0.5f,  0.5f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT2(0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT2(-0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f) },
	};

	UINT indices[6] = { 0,1,2, 0,2,3 };

	m_pd3dQuadVertexBuffer = ::CreateBufferResource(
		pd3dDevice, pd3dCommandList,
		quadVertices, sizeof(quadVertices),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		&m_pd3dQuadVertexUploadBuffer);

	m_d3dQuadVBView.BufferLocation = m_pd3dQuadVertexBuffer->GetGPUVirtualAddress();
	m_d3dQuadVBView.StrideInBytes = sizeof(QUAD_VERTEX);
	m_d3dQuadVBView.SizeInBytes = sizeof(quadVertices);

	m_pd3dQuadIndexBuffer = ::CreateBufferResource(
		pd3dDevice, pd3dCommandList,
		indices, sizeof(indices),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_INDEX_BUFFER,
		&m_pd3dQuadIndexUploadBuffer);

	m_d3dQuadIBView.BufferLocation = m_pd3dQuadIndexBuffer->GetGPUVirtualAddress();
	m_d3dQuadIBView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dQuadIBView.SizeInBytes = sizeof(indices);

	UINT bufferSize = sizeof(EXPLOSION_INSTANCE) * MAX_EXPLOSIONS;

	m_pd3dInstanceBuffer = ::CreateBufferResource(
		pd3dDevice, nullptr,
		nullptr, bufferSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ);

	m_d3dInstanceVBView.BufferLocation = m_pd3dInstanceBuffer->GetGPUVirtualAddress();
	m_d3dInstanceVBView.StrideInBytes = sizeof(EXPLOSION_INSTANCE);
	m_d3dInstanceVBView.SizeInBytes = bufferSize;

	D3D12_RANGE readRange = { 0, 0 };
	m_pd3dInstanceBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pMappedInstanceBuffer));

	m_pExplosionTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_pExplosionTexture->LoadTextureFromDDSFile(
		pd3dDevice, pd3dCommandList,
		L"Image/Explosion.dds",
		RESOURCE_TEXTURE2D, 0);

	if (!m_pExplosionTexture->GetResource(0))
	{
		OutputDebugStringA("ERROR: Failed to load Explosion.dds (resource is nullptr)\n");
	}

	if (pOwnerScene)
	{
		pOwnerScene->CreateShaderResourceView(pd3dDevice, m_pExplosionTexture, 0, 3);
	}

	m_pExplosionTexture->SetRootParameterIndex(0, 3);
}
void CExplosionShader::AnimateObjects(float fTimeElapsed){
	m_nActiveInstances = 0;

	for (auto& exp : m_Explosions)
	{
		if (!exp.bActive) continue;

		exp.fElapsed += fTimeElapsed;
		if (exp.fElapsed >= exp.fDuration)
		{
			exp.bActive = false;
			continue;
		}

		const int totalFrames = exp.nSpriteCols * exp.nSpriteRows;
		float progress = exp.fElapsed / exp.fDuration;
		int frame = min((int)(progress * totalFrames), totalFrames - 1);

		float UScale = 1.0f / (float)exp.nSpriteCols;
		float VScale = 1.0f / (float)exp.nSpriteRows;

		int col = frame % exp.nSpriteCols;
		int row = frame / exp.nSpriteCols;

		float UOffset = col * UScale;
		float VOffset = row * VScale;

		exp.uv = XMFLOAT4(UOffset, VOffset, UScale, VScale);

		if (m_nActiveInstances < MAX_EXPLOSIONS)
		{
			EXPLOSION_INSTANCE& inst = m_InstanceDataCPU[m_nActiveInstances];
			inst.vCenter = exp.vCenter;
			inst.fSize = exp.fScale;
			inst.uv = exp.uv;
			++m_nActiveInstances;
		}
	}

	if (m_nActiveInstances > 0 && m_pMappedInstanceBuffer)
	{
		memcpy(
			m_pMappedInstanceBuffer,
			m_InstanceDataCPU.data(),
			sizeof(EXPLOSION_INSTANCE) * m_nActiveInstances);
	}
}
void CExplosionShader::ReleaseObjects() {
	if (m_pd3dQuadVertexBuffer)
	{
		m_pd3dQuadVertexBuffer->Release();
		m_pd3dQuadVertexBuffer = nullptr;
	}
	if (m_pd3dQuadIndexBuffer)
	{
		m_pd3dQuadIndexBuffer->Release();
		m_pd3dQuadIndexBuffer = nullptr;
	}
	if (m_pd3dInstanceBuffer)
	{
		m_pd3dInstanceBuffer->Unmap(0, nullptr);
		m_pd3dInstanceBuffer->Release();
		m_pd3dInstanceBuffer = nullptr;
		m_pMappedInstanceBuffer = nullptr;
	}

	m_Explosions.clear();
	m_InstanceDataCPU.clear();

	if (m_pExplosionTexture)
	{
		m_pExplosionTexture->Release();
		m_pExplosionTexture = nullptr;
	}
}
void CExplosionShader::ReleaseUploadBuffers() {
	if (m_pd3dQuadVertexUploadBuffer)
	{
		m_pd3dQuadVertexUploadBuffer->Release();
		m_pd3dQuadVertexUploadBuffer = nullptr;
	}
	if (m_pd3dQuadIndexUploadBuffer)
	{
		m_pd3dQuadIndexUploadBuffer->Release();
		m_pd3dQuadIndexUploadBuffer = nullptr;
	}
}

void CExplosionShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState) {
	if (m_nActiveInstances == 0) return;

	CShader::OnPrepareRender(pd3dCommandList, nPipelineState);

	D3D12_VERTEX_BUFFER_VIEW vbViews[2] = { m_d3dQuadVBView, m_d3dInstanceVBView };
	pd3dCommandList->IASetVertexBuffers(0, 2, vbViews);
	pd3dCommandList->IASetIndexBuffer(&m_d3dQuadIBView);
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (m_pExplosionTexture)
	{
		m_pExplosionTexture->UpdateShaderVariables(pd3dCommandList);
	}
	pd3dCommandList->DrawIndexedInstanced(6,m_nActiveInstances,0, 0, 0);
}

D3D12_INPUT_LAYOUT_DESC CExplosionShader::CreateInputLayout()
{
	UINT nInputElementDescs = 5;
	D3D12_INPUT_ELEMENT_DESC* pDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pDescs[0] = {"LOCALPOS", 0, DXGI_FORMAT_R32G32_FLOAT,0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
	pDescs[1] = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,0, 8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
	pDescs[2] = {"INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT,1, 0,D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1};
	pDescs[3] = {"INSTANCESIZE", 0, DXGI_FORMAT_R32_FLOAT,1, 12,D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1};
	pDescs[4] = {"INSTANCEUV", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,1, 16,D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1};

	D3D12_INPUT_LAYOUT_DESC layoutDesc = {};
	layoutDesc.pInputElementDescs = pDescs;
	layoutDesc.NumElements = nInputElementDescs;
	return layoutDesc;
}
D3D12_RASTERIZER_DESC CExplosionShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return d3dRasterizerDesc;
}
D3D12_SHADER_BYTECODE CExplosionShader::CreateVertexShader()
{

	return CShader::CompileShaderFromFile(L"Shaders.hlsl","VS_Explosion_Instanced","vs_5_1",&m_pd3dVertexShaderBlob);
}

D3D12_SHADER_BYTECODE CExplosionShader::CreatePixelShader()
{
	return CShader::CompileShaderFromFile(L"Shaders.hlsl","PS_Explosion_Instanced","ps_5_1",&m_pd3dPixelShaderBlob);
}

D3D12_BLEND_DESC CExplosionShader::CreateBlendState()
{
	D3D12_BLEND_DESC desc = CShader::CreateBlendState();
	desc.RenderTarget[0].BlendEnable = TRUE;
	desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	return desc;
}

void CExplosionShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();

	m_d3dPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	m_d3dPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	m_d3dPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = 1;
	m_d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(
		&m_d3dPipelineStateDesc,
		__uuidof(ID3D12PipelineState),
		(void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob)   m_pd3dVertexShaderBlob->Release();
	if (m_pd3dGeometryShaderBlob) m_pd3dGeometryShaderBlob->Release();
	if (m_pd3dPixelShaderBlob)    m_pd3dPixelShaderBlob->Release();
	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs)
		delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}
