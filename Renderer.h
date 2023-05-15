#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include "ShaderCompiler.h"

class Renderer
{
public:
	Renderer();

	bool Init(HWND hWnd);
	void Term();

	void Resize(UINT width, UINT height);

	bool Update();
	bool Render();

	void MouseMove(int dx, int dy);
	void MouseWheel(int dz);

	void KeyArrowPress(float deltaX, float deltaY);

	void SwitchLightMode(float value);

private:
	HRESULT SetupBackBuffer();

	HRESULT CreateScene();
	void DestroyScene();
	void RenderScene();
	
	void RenderToTexture();
	void RenderToBackBuffer();

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;

	IDXGISwapChain* m_pSwapChain;
	ID3D11RenderTargetView* m_pBackBufferRTV;

	ID3D11Texture2D* m_pDepth;
	ID3D11DepthStencilView* m_pDepthDSV;

	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	ID3D11VertexShader* m_pVertexShader;
	ID3D11PixelShader* m_pPixelShader;
	ID3D11InputLayout* m_pInputLayout;

	ID3D11Resource* m_pTexture;
	ID3D11ShaderResourceView* m_pTextureSRV;

	ID3D11SamplerState* m_pSamplerState;

	ID3D11Buffer* m_pModelBuffer;
	ID3D11Buffer* m_pSceneBuffer;

	ID3D11RasterizerState* m_pRasterizerState;

	ShaderCompiler* m_pShaderCompiler;

	UINT m_width;
	UINT m_height;

	size_t m_usec;
	size_t m_currSec;
	float m_elapsedSec;

	float m_lon;
	float m_lat;
	float m_dist;

	float m_xpos;
	float m_ypos;

	float m_lightPower;

};
