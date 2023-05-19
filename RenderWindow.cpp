#include "RenderWindow.h"
#include <cassert>
#include <chrono>

#define SAFE_RELEASE(p) \
if (p != nullptr) { \
	p->Release(); \
	p = nullptr;\
}

struct ExposureBuffer
{
	XMFLOAT4 exposure;
};

RenderWindow::RenderWindow() : 
	m_renderTargetView(nullptr)
	, m_renderTargetTexture(nullptr)
	, m_shaderResourceView(nullptr)
	, m_pRasterizerState(nullptr)
	, m_pAB_VS(nullptr)
	, m_pAB_PS(nullptr)
	, m_pDS_VS(nullptr)
	, m_pDS_PS(nullptr)
	, m_pU2_VS(nullptr)
	, m_pU2_PS(nullptr)
	, m_pSamplerState(nullptr)
	, m_pExposureBuffer(nullptr)
	, m_pDevice(nullptr)
	, m_pContext(nullptr)
	, m_downSamplingTexture(nullptr)
	, m_screenWidth(0)
	, m_screenHeight(0)
	, m_adpBrightness(1)
	, m_minPower2(0)
	, m_minPower2Value(0)
{
}

bool RenderWindow::Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height)
{
	CalculateMinPower2(width, height);

	D3D11_TEXTURE2D_DESC textureDesc;
	HRESULT result;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	m_pDevice = device;
	m_pContext = context;

	// Initialize the render target texture description.
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = 1;
	textureDesc.Height = 1;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_STAGING;
	textureDesc.BindFlags = 0;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;

	// Create the render target texture.
	result = m_pDevice->CreateTexture2D(&textureDesc, NULL, &m_renderTargetTexture);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC constantBufferDesc = {};
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(ExposureBuffer);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;

	HRESULT hr = m_pDevice->CreateBuffer(&constantBufferDesc, nullptr, &m_pExposureBuffer);

	D3D11_TEXTURE2D_DESC dsTextureDesc;
	ZeroMemory(&dsTextureDesc, sizeof(dsTextureDesc));
	dsTextureDesc.Width = m_minPower2Value;
	dsTextureDesc.Height = m_minPower2Value;
	dsTextureDesc.MipLevels = m_minPower2 + 1;
	dsTextureDesc.ArraySize = 1;
	dsTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
	dsTextureDesc.SampleDesc.Count = 1;
	dsTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	dsTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	dsTextureDesc.CPUAccessFlags = 0;
	dsTextureDesc.MiscFlags = 0;

	result = m_pDevice->CreateTexture2D(&dsTextureDesc, nullptr, &m_downSamplingTexture);
	if (FAILED(result))
	{
		return false;
	}

	// Setup the description of the shader resource view.
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	// Create rasterizer state
	if (SUCCEEDED(result))
	{
		D3D11_RASTERIZER_DESC rasterizerDesc;
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.ScissorEnable = FALSE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;

		result = m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pRasterizerState);
	}

	// Create texture samplers
	if (SUCCEEDED(result))
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = 1000;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.MaxAnisotropy = 16;

		result = m_pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState);
	}
	
	if (SUCCEEDED(result))
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;

		for (int i = 0, textureSize = m_minPower2Value; textureSize > 0; textureSize >>= 1, ++i)
		{
			srvDesc.Texture2D.MostDetailedMip = i;

			ID3D11ShaderResourceView* tmpSRV = nullptr;
			result = m_pDevice->CreateShaderResourceView(m_downSamplingTexture, &srvDesc, &tmpSRV);

			if (FAILED(result))
			{
				break;
			}

			m_downSamplingSRVs.push_back(tmpSRV);
		}
	}

	if (SUCCEEDED(result))
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		for (UINT i = 0, textureSize = m_minPower2Value; textureSize > 0; textureSize >>= 1, ++i)
		{
			rtvDesc.Texture2D.MipSlice = i;

			ID3D11RenderTargetView* tmpRTV = nullptr;
			result = m_pDevice->CreateRenderTargetView(m_downSamplingTexture, &rtvDesc, &tmpRTV);

			if (FAILED(result))
			{
				break;
			}

			m_downSamplingRTVs.push_back(tmpRTV);
		}
	}

	// Create vertex shader
	ShaderCompiler shaderCompiler;

	ID3DBlob * pBlob = NULL;
	m_pAB_VS = shaderCompiler.CreateVertexShader(m_pDevice, _T("ABShader.hlsl"), &pBlob);
	// Create pixel shader
	if (m_pAB_VS)
	{
		m_pAB_PS = shaderCompiler.CreatePixelShader(m_pDevice, _T("ABShader.hlsl"));
	}
	assert(m_pAB_VS != NULL && m_pAB_PS != NULL);
	if (m_pAB_VS == NULL || m_pAB_PS == NULL)
	{
		result = E_FAIL;
	}

	pBlob = nullptr;
	m_pDS_VS = shaderCompiler.CreateVertexShader(m_pDevice, _T("DSShader.hlsl"), &pBlob);
	// Create pixel shader
	if (m_pDS_VS)
	{
		m_pDS_PS = shaderCompiler.CreatePixelShader(m_pDevice, _T("DSShader.hlsl"));
	}
	assert(m_pDS_VS != NULL && m_pDS_PS != NULL);
	if (m_pDS_VS == NULL || m_pDS_PS == NULL)
	{
		result = E_FAIL;
	}

	pBlob = nullptr;
	m_pU2_VS = shaderCompiler.CreateVertexShader(m_pDevice, _T("U2Shader.hlsl"), &pBlob);
	// Create pixel shader
	if (m_pU2_VS)
	{
		m_pU2_PS = shaderCompiler.CreatePixelShader(m_pDevice, _T("U2Shader.hlsl"));
	}
	assert(m_pU2_VS != NULL && m_pU2_PS != NULL);
	if (m_pU2_VS == NULL || m_pU2_PS == NULL)
	{
		result = E_FAIL;
	}

	return true;
}

void RenderWindow::Term()
{
	SAFE_RELEASE(m_shaderResourceView);
	SAFE_RELEASE(m_renderTargetView);
	SAFE_RELEASE(m_renderTargetTexture);
	SAFE_RELEASE(m_pRasterizerState);

	SAFE_RELEASE(m_pAB_VS);
	SAFE_RELEASE(m_pAB_PS);
	SAFE_RELEASE(m_pDS_VS);
	SAFE_RELEASE(m_pDS_PS);
	SAFE_RELEASE(m_pU2_VS);
	SAFE_RELEASE(m_pU2_PS);

	SAFE_RELEASE(m_pSamplerState);

	SAFE_RELEASE(m_pExposureBuffer);

	SAFE_RELEASE(m_pContext);
	SAFE_RELEASE(m_pDevice);

	SAFE_RELEASE(m_downSamplingTexture);

	for (auto srv : m_downSamplingSRVs)
		SAFE_RELEASE(srv);

	for (auto rtv : m_downSamplingRTVs)
		SAFE_RELEASE(rtv);

}

void RenderWindow::SetRenderTarget(ID3D11DeviceContext* deviceContext, ID3D11DepthStencilView* depthStencilView)
{
	deviceContext->OMSetRenderTargets(1, &m_renderTargetView, depthStencilView);
}

void RenderWindow::ClearRenderTarget(ID3D11DeviceContext* deviceContext, ID3D11DepthStencilView* depthStencilView)
{
	const FLOAT backgroundColor[4] = { 0.12f, 0.52f, 0.25f, 1.0f };
	deviceContext->ClearRenderTargetView(m_renderTargetView, backgroundColor);

	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

ID3D11ShaderResourceView* RenderWindow::GetShaderResourceView()
{
	return m_shaderResourceView;
}

float RenderWindow::CalculateAverageBrightness(ID3D11ShaderResourceView* srv)
{
	UINT textureSize = m_minPower2Value;

	m_pContext->ClearState();

	m_pContext->OMSetRenderTargets(1, &m_downSamplingRTVs[0], nullptr);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftY = 0;
	viewport.TopLeftX = 0;
	viewport.Width = (FLOAT)textureSize;
	viewport.Height = (FLOAT)textureSize;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	D3D11_RECT rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.right = textureSize;
	rect.bottom = textureSize;

	m_pContext->RSSetViewports(1, &viewport);
	m_pContext->RSSetScissorRects(1, &rect);
	m_pContext->RSSetState(m_pRasterizerState);

	m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_pContext->IASetInputLayout(nullptr);

	m_pContext->VSSetShader(m_pAB_VS, nullptr, 0);
	m_pContext->PSSetShader(m_pAB_PS, nullptr, 0);

	m_pContext->PSSetSamplers(0, 1, &m_pSamplerState);
	m_pContext->PSSetShaderResources(0, 1, &srv);

	m_pContext->Draw(4, 0);

	m_pContext->VSSetShader(m_pDS_VS, nullptr, 0);
	m_pContext->PSSetShader(m_pDS_PS, nullptr, 0);

	int i = 0;
	for (int n = textureSize >> 1; n > 0; n >>= 1, i++)
	{
		viewport.Width = (FLOAT)n;
		viewport.Height = (FLOAT)n;

		rect.right = n;
		rect.bottom = n;

		m_pContext->OMSetRenderTargets(1, &m_downSamplingRTVs[(size_t)i + 1], nullptr);

		m_pContext->RSSetViewports(1, &viewport);
		m_pContext->RSSetScissorRects(1, &rect);

		m_pContext->PSSetShaderResources(0, 1, &m_downSamplingSRVs[i]);

		m_pContext->Draw(4, 0);
	}

	m_pContext->CopySubresourceRegion(m_renderTargetTexture, 0, 0, 0, 0, m_downSamplingTexture, i, nullptr);

	D3D11_MAPPED_SUBRESOURCE resourceDesc = {};	
	auto hr = m_pContext->Map(m_renderTargetTexture, 0, D3D11_MAP_READ, 0, &resourceDesc);
	if (FAILED(hr))
		return 0;
	FLOAT averageBrightness = *reinterpret_cast<FLOAT*>(resourceDesc.pData);
	m_pContext->Unmap(m_renderTargetTexture, 0);

	return expf(averageBrightness) - 1.0f;
}

void RenderWindow::ToneMap(
	ID3D11ShaderResourceView* pSrcTextureSRV,
	ID3D11RenderTargetView* pDstTextureRTV,
	UINT renderTargetWidth,
	UINT renderTargetHeight,
	float deltaTime,
	FLOAT eyeAdaptationSpeed)
{
	Update(CalculateAverageBrightness(pSrcTextureSRV), deltaTime, eyeAdaptationSpeed);

	m_pContext->ClearState();

	m_pContext->OMSetRenderTargets(1, &pDstTextureRTV, nullptr);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftY = 0;
	viewport.TopLeftX = 0;
	viewport.Width = (FLOAT)renderTargetWidth;
	viewport.Height = (FLOAT)renderTargetHeight;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	D3D11_RECT rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.right = renderTargetWidth;
	rect.bottom = renderTargetHeight;

	m_pContext->RSSetViewports(1, &viewport);
	m_pContext->RSSetScissorRects(1, &rect);
	m_pContext->RSSetState(m_pRasterizerState);

	m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_pContext->IASetInputLayout(nullptr);

	m_pContext->VSSetShader(m_pU2_VS, nullptr, 0);
	m_pContext->PSSetShader(m_pU2_PS, nullptr, 0);

	m_pContext->PSSetSamplers(0, 1, &m_pSamplerState);
	m_pContext->PSSetShaderResources(0, 1, &pSrcTextureSRV);
	m_pContext->PSSetConstantBuffers(0, 1, &m_pExposureBuffer);

	m_pContext->Draw(4, 0);
}

void RenderWindow::Update(float avgBrightness, float deltaTime, float eyeAdaptationSpeed)
{
	
	EyeAdaptation(avgBrightness, deltaTime, eyeAdaptationSpeed);
	static ExposureBuffer exposureBuffer = {};
	exposureBuffer.exposure = { (1.03f - (2.0f / (2 + log10f(10 * m_adpBrightness + 1)))) / avgBrightness, 0.0f, 0.0f, 0.0f};

	m_pContext->UpdateSubresource(m_pExposureBuffer, 0, nullptr, &exposureBuffer, 0, 0);
}

void RenderWindow::CalculateMinPower2(int a, int b)
{
	int min = a < b ? a : b;
	m_minPower2 = static_cast<int>(ceil(log2(min)) - 1);
	m_minPower2Value = (int)pow(2, m_minPower2);
}

void RenderWindow::EyeAdaptation(float avgBrightness, float time, float speed)
{
	m_adpBrightness = m_adpBrightness + (avgBrightness - m_adpBrightness) * (1 - expf(-time / speed));
}
