#pragma once
#include"stdafx.h"
#include"Shader.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI
class CUIShader : public CShader
{
public:
	CUIShader();
	virtual ~CUIShader();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader();
	virtual D3D12_SHADER_BYTECODE CreatePixelShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature) override;
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();

};

class CHealthBarShader : public CUIShader {
public:
	CHealthBarShader();
	virtual ~CHealthBarShader();

	virtual D3D12_SHADER_BYTECODE CreatePixelShader();
};

class CUITextureAtlasShader : public CUIShader
{
public:
	CUITextureAtlasShader();
	virtual ~CUITextureAtlasShader();

public:
	// VS, InputLayout, Blend, Depth, Rasterizer 상태는
	// 부모인 CUIShader의 것을 그대로 사용합니다.

	// 픽셀 셰이더만 PS_UI_TextureAtlas로 재정의합니다.
	virtual D3D12_SHADER_BYTECODE CreatePixelShader() override;
};