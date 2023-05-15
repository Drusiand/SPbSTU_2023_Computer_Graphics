#pragma once
#include <d3d11.h>
#include <stdio.h>
#include <tchar.h>
#include <d3dcompiler.h>
#include <cassert>

class ShaderCompiler
{
public:
	ShaderCompiler();

	~ShaderCompiler();

	ID3D11VertexShader* CreateVertexShader(ID3D11Device* m_pDevice, LPCTSTR shaderSource, ID3DBlob** ppBlob);

	ID3D11PixelShader* CreatePixelShader(ID3D11Device* m_pDevice, LPCTSTR shaderSource);
};

