#pragma once
#include"stdafx.h"
#include"Shader.h"

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

	virtual D3D12_SHADER_BYTECODE CreatePixelShader() override;
};
