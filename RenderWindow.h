#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <vector>
#include "ShaderCompiler.h"

using std::vector;

using namespace DirectX;
class RenderWindow
{
	struct Vertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 tex;
	};
public:
	RenderWindow();
	bool Init(ID3D11Device*, ID3D11DeviceContext*, int, int);
	void Term();
	void SetRenderTarget(ID3D11DeviceContext* deviceContext, ID3D11DepthStencilView* depthStencilView);
	void ClearRenderTarget(ID3D11DeviceContext* deviceContext, ID3D11DepthStencilView* depthStencilView);
	ID3D11ShaderResourceView* GetShaderResourceView();

	float CalculateAverageBrightness(ID3D11ShaderResourceView* srv);
	void ToneMap(ID3D11ShaderResourceView* pSrcTextureSRV,
		ID3D11RenderTargetView* pDstTextureRTV,
		UINT renderTargetWidth,
		UINT renderTargetHeight,
		float deltaTime,
		FLOAT eyeAdaptationSpeed);

private:

	void Update(float avgBrightness, float deltaTime, float eyeAdaptationSpeed);

	void CalculateMinPower2(int a, int b);
	void EyeAdaptation(float avgBrightness, float time, float speed);

	ID3D11RenderTargetView* m_renderTargetView;
	ID3D11Texture2D* m_renderTargetTexture;
	ID3D11ShaderResourceView* m_shaderResourceView;
	ID3D11RasterizerState* m_pRasterizerState;

	ID3D11VertexShader* m_pAB_VS;
	ID3D11PixelShader* m_pAB_PS;

	ID3D11VertexShader* m_pDS_VS;
	ID3D11PixelShader* m_pDS_PS;

	ID3D11VertexShader* m_pU2_VS;
	ID3D11PixelShader* m_pU2_PS;

	ID3D11SamplerState* m_pSamplerState;

	ID3D11Buffer* m_pExposureBuffer;

	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;
	
	ID3D11Texture2D* m_downSamplingTexture;
	vector<ID3D11ShaderResourceView*> m_downSamplingSRVs;
	vector<ID3D11RenderTargetView*> m_downSamplingRTVs;

	int m_minPower2;
	int m_minPower2Value;

	int m_screenWidth, m_screenHeight;	

	float m_adpBrightness;

};