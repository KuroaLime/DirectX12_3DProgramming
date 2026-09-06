#include "UIShader.h"

CUIShader::CUIShader() {

}
CUIShader::~CUIShader() {

}
D3D12_SHADER_BYTECODE CUIShader::CreateVertexShader() {
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSTextured", "vs_5_1", &m_pd3dVertexShaderBlob));
}
D3D12_SHADER_BYTECODE CUIShader::CreatePixelShader() {
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PS_UI_Textured", "ps_5_1", &m_pd3dPixelShaderBlob));
}

D3D12_INPUT_LAYOUT_DESC CUIShader::CreateInputLayout() {
	UINT nInputElementDescs = 2;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_BLEND_DESC CUIShader::CreateBlendState() {
	D3D12_BLEND_DESC d3dBlendDesc = CShader::CreateBlendState();

	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	return(d3dBlendDesc);
}
D3D12_DEPTH_STENCIL_DESC CUIShader::CreateDepthStencilState() {
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc = CShader::CreateDepthStencilState();

	d3dDepthStencilDesc.DepthEnable = FALSE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	return(d3dDepthStencilDesc);
}
D3D12_RASTERIZER_DESC CUIShader::CreateRasterizerState()
{

	D3D12_RASTERIZER_DESC d3dRasterizerDesc = CShader::CreateRasterizerState();

	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	return(d3dRasterizerDesc);
}

void CUIShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature) {
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	CShader::CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs)
		delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}

CHealthBarShader::CHealthBarShader() : CUIShader()
{
}

CHealthBarShader::~CHealthBarShader()
{
}

D3D12_SHADER_BYTECODE CHealthBarShader::CreatePixelShader()
{

	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PS_UI_HealthBar", "ps_5_1", &m_pd3dPixelShaderBlob));
}

CUITextureAtlasShader::CUITextureAtlasShader() : CUIShader()
{
}

CUITextureAtlasShader::~CUITextureAtlasShader()
{
}

D3D12_SHADER_BYTECODE CUITextureAtlasShader::CreatePixelShader()
{

	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PS_UI_TextureAtlas", "ps_5_1", &m_pd3dPixelShaderBlob));
}
