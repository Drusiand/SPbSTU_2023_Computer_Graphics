#include "Renderer.h"

#include <stdio.h>
#include <tchar.h>
#include <assert.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "DDSTextureLoader11.h"

#include <chrono>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace DirectX;

static const UINT IndicesCount = 42;

struct TextureVertex
{
	XMVECTORF32 pos;
	XMFLOAT2 uv;
	XMFLOAT3 normal;
};

struct ModelBuffer
{
	XMMATRIX modelMatrix;
	XMMATRIX normalMatrix;
};

struct Light
{
	XMVECTORF32 pos;
	XMVECTORF32 color;
	XMVECTORF32 power; // mb another type?
};

struct SceneBuffer
{
	XMMATRIX VP;
	XMVECTORI32 lightParams; // x - lights count

	Light lights[4];
};

#define SAFE_RELEASE(p) \
if (p != NULL) { \
	p->Release(); \
	p = NULL;\
}

Renderer::Renderer()
	: m_pDevice(nullptr)
	, m_pContext(nullptr)
	, m_pSwapChain(nullptr)
	, m_pBackBufferRTV(nullptr)
	, m_pRenderRTV(nullptr)
	, m_pRenderTexture(nullptr)
	, m_pRenderSRV(nullptr)
	, m_pDepth(nullptr)
	, m_pDepthDSV(nullptr)
	, m_width(0)
	, m_height(0)
	, m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pVertexShader(nullptr)
	, m_pPixelShader(nullptr)
	, m_pInputLayout(nullptr)
	, m_pTexture(nullptr)
	, m_pTextureSRV(nullptr)
	, m_pSamplerState(nullptr)
	, m_pModelBuffer(nullptr)
	, m_pSceneBuffer(nullptr)
	, m_pRasterizerState(nullptr)
	, m_pShaderCompiler(nullptr)
	, m_usec(0)
	, m_currSec(0)
	, m_lon(0.0f)
	, m_lat(0.0f)
	, m_dist(10.0f)
	, m_xpos(0)
	, m_ypos(0)
	, m_pRenderWindow(nullptr)
	, m_lightPower(1)
	, m_elapsedSec(0)
{
}

bool Renderer::Init(HWND hWnd)
{
	HRESULT result;

	// Create a DirectX graphics interface factory.
	IDXGIFactory* pFactory = NULL;
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
	assert(SUCCEEDED(result));

	// Select hardware adapter
	IDXGIAdapter* pSelectedAdapter = NULL;
	if (SUCCEEDED(result))
	{
		IDXGIAdapter* pAdapter = NULL;
		UINT adapterIdx = 0;
		while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter)))
		{
			DXGI_ADAPTER_DESC desc;
			pAdapter->GetDesc(&desc);

			if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0)
			{
				pSelectedAdapter = pAdapter;
				break;
			}

			pAdapter->Release();

			adapterIdx++;
		}
	}
	assert(pSelectedAdapter != NULL);

	// Create DirectX 11 device
	D3D_FEATURE_LEVEL level;
	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
	if (SUCCEEDED(result))
	{
		result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
			D3D11_CREATE_DEVICE_DEBUG, levels, 1, D3D11_SDK_VERSION, &m_pDevice, &level, &m_pContext);
		assert(level == D3D_FEATURE_LEVEL_11_0);
		assert(SUCCEEDED(result));
	}

	// Create swapchain
	if (SUCCEEDED(result))
	{
		RECT rc;
		GetClientRect(hWnd, &rc);
		m_width = rc.right - rc.left;
		m_height = rc.bottom - rc.top;

		DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferDesc.Width = m_width;
		swapChainDesc.BufferDesc.Height = m_height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Windowed = true;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		result = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		assert(SUCCEEDED(result));
	}

	// Create render target views
	if (SUCCEEDED(result))
	{
		result = SetupBackBuffer();
	}

	// Create scene for render
	if (SUCCEEDED(result))
	{
		result = CreateScene();
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
		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 16;

		result = m_pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState);
	}

	m_pRenderWindow = new RenderWindow();
	m_pRenderWindow->Init(m_pDevice, m_pContext, m_width, m_height);

	SAFE_RELEASE(pSelectedAdapter);
	SAFE_RELEASE(pFactory);

	return SUCCEEDED(result);
}

void Renderer::Term()
{
	DestroyScene();

	SAFE_RELEASE(m_pDepthDSV);
	SAFE_RELEASE(m_pDepth);
	SAFE_RELEASE(m_pBackBufferRTV);
	SAFE_RELEASE(m_pSwapChain);
	//SAFE_RELEASE(m_pContext);
	//SAFE_RELEASE(m_pDevice);

	SAFE_RELEASE(m_pRenderRTV);

	SAFE_RELEASE(m_pRasterizerState);

	delete m_pShaderCompiler;
	m_pShaderCompiler = nullptr;

	SAFE_RELEASE(m_pRenderSRV);
	SAFE_RELEASE(m_pRenderTexture);

	m_pRenderWindow->Term();
	delete m_pRenderWindow;
	m_pRenderWindow = nullptr;
}

void Renderer::Resize(UINT width, UINT height)
{
	if (width != m_width || height != m_height)
	{
		SAFE_RELEASE(m_pDepthDSV);
		SAFE_RELEASE(m_pDepth);
		SAFE_RELEASE(m_pBackBufferRTV);

		HRESULT result = m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		if (SUCCEEDED(result))
		{
			m_width = width;
			m_height = height;

			result = SetupBackBuffer();
		}
		assert(SUCCEEDED(result));
	}
}

bool Renderer::Update()
{
	size_t usec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	if (m_usec == 0)
	{
		m_usec = usec; // Initial update
		m_currSec = usec;
	}

	m_elapsedSec = (usec - m_currSec) / 1000000.0f;
	m_currSec = usec;

	ModelBuffer cb;
	XMMATRIX m = XMMatrixRotationAxis({ 0,1,0 }, 0);

	cb.modelMatrix = XMMatrixTranspose(m);
	cb.normalMatrix = XMMatrixInverse(NULL, m);
	m_pContext->UpdateSubresource(m_pModelBuffer, 0, NULL, &cb, 0, 0);

	// Setup scene buffer
	SceneBuffer scb;

	static const float nearPlane = 0.001f;
	static const float farPlane = 100.0f;
	static const float fov = (float)M_PI * 2.0 / 3.0;

	XMMATRIX rot = XMMatrixRotationAxis({ 1, 0, 0 }, m_lat) * XMMatrixRotationAxis({ 0, 1, 0 }, m_lon);
	XMMATRIX shift = XMMatrixTranslation(m_xpos, 0, -m_dist);
	XMMATRIX view = XMMatrixInverse(nullptr, shift * rot);

	float width = nearPlane / tanf(fov / 2.0);
	float height = ((float)m_height / m_width) * width;
	scb.VP = XMMatrixTranspose(view * XMMatrixPerspectiveLH(width, height, nearPlane, farPlane));
	
	scb.lightParams.i[0] = 3;
	scb.lights[0].pos = XMVECTORF32{ 2.5f, 0.2f, -0.289f, 0.f };
	scb.lights[0].color = XMVECTORF32{ 0.75f, 0, 0, 0 };
	scb.lights[0].power = XMVECTORF32{ 1, 0, 0, 0 };

	scb.lights[1].pos = XMVECTORF32{ 2.5f, 0.2f, 0.289f, 0 };
	scb.lights[1].color = XMVECTORF32{ 0.75f, 0, 0, 0 };
	scb.lights[1].power = XMVECTORF32{ 1, 0, 0, 0 };

	scb.lights[2].pos = XMVECTORF32{ 2, 0.2f, 0, 0 };
	scb.lights[2].color = XMVECTORF32{ 0.75f, 0, 0, 0 };
	scb.lights[2].power = XMVECTORF32{ m_lightPower, 0, 0, 0 };


	m_pContext->UpdateSubresource(m_pSceneBuffer, 0, NULL, &scb, 0, 0);

	return true;
}

void Renderer::MouseMove(int dx, int dy)
{
	m_lon += (float)dx / m_width * 5.0f;
	m_lat += (float)dy / m_height * 5.0f;

	if (m_lat <= -(float)M_PI / 2)
	{
		m_lat = -(float)M_PI / 2;
	}
	if (m_lat >= (float)M_PI / 2)
	{
		m_lat = (float)M_PI / 2;
	}
}

void Renderer::MouseWheel(int dz)
{
	m_dist += dz / 100.0f;
	if (m_dist < 1)
	{
		m_dist = 0;
	}
}

void Renderer::KeyArrowPress(float deltaX, float deltaY)
{
	m_xpos += deltaX;
	m_dist -= deltaY;
}

void Renderer::SwitchLightMode(float value)
{
	m_lightPower = value;
}

HRESULT Renderer::SetupBackBuffer()
{
	ID3D11Texture2D* pBackBuffer = NULL;
	HRESULT result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	assert(SUCCEEDED(result));
	if (SUCCEEDED(result))
	{
		result = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferRTV);
		assert(SUCCEEDED(result));

		SAFE_RELEASE(pBackBuffer);
	}

	D3D11_TEXTURE2D_DESC renderTextureDesc;
	ZeroMemory(&renderTextureDesc, sizeof(renderTextureDesc));
	renderTextureDesc.Width = m_width;
	renderTextureDesc.Height = m_height;
	renderTextureDesc.MipLevels = 1;
	renderTextureDesc.ArraySize = 1;
	renderTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	renderTextureDesc.SampleDesc.Count = 1;
	renderTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	renderTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	renderTextureDesc.CPUAccessFlags = 0;
	renderTextureDesc.MiscFlags = 0;

	result = m_pDevice->CreateTexture2D(&renderTextureDesc, nullptr, &m_pRenderTexture);


	if (SUCCEEDED(result))
	{
		result = m_pDevice->CreateRenderTargetView(m_pRenderTexture, nullptr, &m_pRenderRTV);
	}

	if (SUCCEEDED(result))
	{
		result = m_pDevice->CreateShaderResourceView(m_pRenderTexture, nullptr, &m_pRenderSRV);
	}

	if (SUCCEEDED(result))
	{
		D3D11_TEXTURE2D_DESC depthDesc = {};
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.ArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.Height = m_height;
		depthDesc.Width = m_width;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthDesc.CPUAccessFlags = 0;
		depthDesc.MiscFlags = 0;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.SampleDesc.Quality = 0;

		result = m_pDevice->CreateTexture2D(&depthDesc, NULL, &m_pDepth);
		if (SUCCEEDED(result))
		{
			result = m_pDevice->CreateDepthStencilView(m_pDepth, NULL, &m_pDepthDSV);
		}
	}

	return result;
}

HRESULT Renderer::CreateScene()
{
	// Textured cube
	static const TextureVertex Vertices[28] = {
		// Bottom face
		{{-0.5, -0.5,  0.5, 1}, {0,1}, {0,-1,0}},
		{{ 0.5, -0.5,  0.5, 1}, {1,1}, {0,-1,0}},
		{{ 0.5, -0.5, -0.5, 1}, {1,0}, {0,-1,0}},
		{{-0.5, -0.5, -0.5, 1}, {0,0}, {0,-1,0}},
		// Top face
		{{-0.5,  0.5, -0.5, 1}, {0,1}, {0,1,0}},
		{{ 0.5,  0.5, -0.5, 1}, {1,1}, {0,1,0}},
		{{ 0.5,  0.5,  0.5, 1}, {1,0}, {0,1,0}},
		{{-0.5,  0.5,  0.5, 1}, {0,0}, {0,1,0}},
		// Front face
		{{ 0.5, -0.5, -0.5, 1}, {0,1}, {1,0,0}},
		{{ 0.5, -0.5,  0.5, 1}, {1,1}, {1,0,0}},
		{{ 0.5,  0.5,  0.5, 1}, {1,0}, {1,0,0}},
		{{ 0.5,  0.5, -0.5, 1}, {0,0}, {1,0,0}},
		// Back face
		{{-0.5, -0.5,  0.5, 1}, {0,1}, {-1,0,0}},
		{{-0.5, -0.5, -0.5, 1}, {1,1}, {-1,0,0}},
		{{-0.5,  0.5, -0.5, 1}, {1,0}, {-1,0,0}},
		{{-0.5,  0.5,  0.5, 1}, {0,0}, {-1,0,0}},
		// Left face
		{{ 0.5, -0.5,  0.5, 1}, {0,1}, {0,0,1}},
		{{-0.5, -0.5,  0.5, 1}, {1,1}, {0,0,1}},
		{{-0.5,  0.5,  0.5, 1}, {1,0}, {0,0,1}},
		{{ 0.5,  0.5,  0.5, 1}, {0,0}, {0,0,1}},
		// Right face
		{{-0.5, -0.5, -0.5, 1}, {0,1}, {0,0,-1}},
		{{ 0.5, -0.5, -0.5, 1}, {1,1}, {0,0,-1}},
		{{ 0.5,  0.5, -0.5, 1}, {1,0}, {0,0,-1}},
		{{-0.5,  0.5, -0.5, 1}, {0,0}, {0,0,-1}},

		// Plane
		{{ 4.5, -0.5,-2,  1}, {0,1}, {0,1,0}},
		{{  4.5,  -0.5,2, 1}, {1,1}, {0,1,0}},
		{{ 0.75,  -0.5, 2,  1}, {1,0}, {0,1,0}},
		{{ 0.75, -0.5, -2, 1}, {0,0}, {0,1,0}},


	};
	static const UINT16 Indices[IndicesCount] = {
		0, 2, 1, 0, 3, 2,
		4, 6, 5, 4, 7, 6,
		8, 10, 9, 8, 11, 10,
		12, 14, 13, 12, 15, 14, 
		16, 18, 17, 16, 19, 18,
		20, 22, 21, 20, 23, 22,

		24, 27, 26, 24, 26, 25
	};

	// Create vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData = { 0 };
	vertexData.pSysMem = Vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT result = m_pDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &m_pVertexBuffer);
	assert(SUCCEEDED(result));

	// Create index buffer
	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC indexBufferDesc = { 0 };
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(Indices);
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexData = { 0 };
		indexData.pSysMem = Indices;
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		result = m_pDevice->CreateBuffer(&indexBufferDesc, &indexData, &m_pIndexBuffer);
		assert(SUCCEEDED(result));
	}

	// Create vertex shader
	ID3DBlob* pBlob = NULL;
	m_pVertexShader = m_pShaderCompiler->CreateVertexShader(m_pDevice, _T("ColorShader.hlsl"), &pBlob);
	// Create pixel shader
	if (m_pVertexShader)
	{
		m_pPixelShader = m_pShaderCompiler->CreatePixelShader(m_pDevice, _T("ColorShader.hlsl"));
	}
	assert(m_pVertexShader != NULL && m_pPixelShader != NULL);
	if (m_pVertexShader == NULL || m_pPixelShader == NULL)
	{
		result = E_FAIL;
	}

	// Create input layout
	if (SUCCEEDED(result))
	{
		D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[3] = {
			D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			D3D11_INPUT_ELEMENT_DESC{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMVECTORF32), D3D11_INPUT_PER_VERTEX_DATA, 0},
			D3D11_INPUT_ELEMENT_DESC{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMVECTORF32) + sizeof(XMFLOAT2), D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		result = m_pDevice->CreateInputLayout(inputLayoutDesc, 3, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &m_pInputLayout);
		assert(SUCCEEDED(result));
	}
	SAFE_RELEASE(pBlob);

	// Create model constant buffer
	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC cbDesc = { 0 };
		cbDesc.ByteWidth = sizeof(ModelBuffer);
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		result = m_pDevice->CreateBuffer(&cbDesc, NULL, &m_pModelBuffer);
	}

	// Create scene constant buffer
	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC cbDesc = { 0 };
		cbDesc.ByteWidth = sizeof(SceneBuffer);
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		result = m_pDevice->CreateBuffer(&cbDesc, NULL, &m_pSceneBuffer);
	}

	// Create rasterizer state
	if (SUCCEEDED(result))
	{
		D3D11_RASTERIZER_DESC rasterizerDesc;
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_BACK;
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

	// Create texture
	if (SUCCEEDED(result))
	{
		result = DirectX::CreateDDSTextureFromFile(m_pDevice, L"wood.dds", (ID3D11Resource**)&m_pTexture, &m_pTextureSRV);
	}

	return result;
}

void Renderer::DestroyScene()
{
	SAFE_RELEASE(m_pSamplerState);

	SAFE_RELEASE(m_pTextureSRV);
	SAFE_RELEASE(m_pTexture);

	SAFE_RELEASE(m_pRasterizerState);
	SAFE_RELEASE(m_pModelBuffer);
	SAFE_RELEASE(m_pSceneBuffer);

	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexShader);

	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pVertexBuffer);
}

bool Renderer::Render()
{
	RenderToBackBuffer();

	m_pRenderWindow->ToneMap(m_pRenderSRV, m_pBackBufferRTV, m_width, m_height, m_elapsedSec, 0.75f);

	HRESULT result = m_pSwapChain->Present(1, 0);
	assert(SUCCEEDED(result));

	return SUCCEEDED(result);
}

void Renderer::RenderScene()
{
	ID3D11Buffer* vertexBuffers[] = {m_pVertexBuffer};
	UINT stride = sizeof(TextureVertex);
	UINT offset = 0;

	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	m_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	m_pContext->IASetInputLayout(m_pInputLayout);

	m_pContext->VSSetShader(m_pVertexShader, NULL, 0);
	m_pContext->PSSetShader(m_pPixelShader, NULL, 0);

	ID3D11Buffer* constBuffers[] = { m_pSceneBuffer };
	m_pContext->VSSetConstantBuffers(1, 1, constBuffers);
	m_pContext->PSSetConstantBuffers(1, 1, constBuffers);

	m_pContext->RSSetState(m_pRasterizerState);
	m_pContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11ShaderResourceView* textures[] = {m_pTextureSRV};
	m_pContext->PSSetShaderResources(0, 1, textures);

	ID3D11SamplerState* samplers[] = {m_pSamplerState};
	m_pContext->PSSetSamplers(0, 1, samplers);

	m_pContext->VSSetConstantBuffers(0, 1, { &m_pModelBuffer });

	m_pContext->DrawIndexed(IndicesCount, 0, 0);
}

void Renderer::RenderToTexture()
{
	m_pRenderWindow->SetRenderTarget(m_pContext, m_pDepthDSV);
	m_pRenderWindow->ClearRenderTarget(m_pContext, m_pDepthDSV);
	RenderScene();
}

void Renderer::RenderToBackBuffer()
{
	m_pContext->ClearState();

	m_pContext->OMSetRenderTargets(1, &m_pRenderRTV, m_pDepthDSV);

	const FLOAT BackColor[4] = { 0.25f, 0.25f, 0.25f, 1.0f };
	m_pContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);
	m_pContext->ClearRenderTargetView(m_pRenderRTV, BackColor);
	m_pContext->ClearDepthStencilView(m_pDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	D3D11_VIEWPORT viewport{ 0, 0, (float)m_width, (float)m_height, 0.0f, 1.0f };
	m_pContext->RSSetViewports(1, &viewport);
	D3D11_RECT rect{ 0, 0, (LONG)m_width, (LONG)m_height };
	m_pContext->RSSetScissorRects(1, &rect);

	RenderScene();
}

