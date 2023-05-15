#include "ShaderCompiler.h"

#define SAFE_RELEASE(p) \
if (p != NULL) { \
	p->Release(); \
	p = NULL;\
}

ShaderCompiler::ShaderCompiler()
{
}

ShaderCompiler::~ShaderCompiler()
{
}

ID3D11VertexShader* ShaderCompiler::CreateVertexShader(ID3D11Device* m_pDevice, LPCTSTR shaderSource, ID3DBlob** ppBlob)
{
	ID3D11VertexShader* pVertexShader = NULL;

	FILE* pFile = NULL;

	_tfopen_s(&pFile, shaderSource, _T("rb"));
	if (pFile != NULL)
	{
		fseek(pFile, 0, SEEK_END);
		int size = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		char* pSourceCode = (char*)malloc((size_t)size + 1);
		fread(pSourceCode, size, 1, pFile);
		pSourceCode[size] = 0;

		fclose(pFile);

		ID3DBlob* pError = NULL;
		HRESULT result = D3DCompile(pSourceCode, size, "", NULL, NULL, "VS", "vs_5_0", 0, 0, ppBlob, &pError);
		if (!SUCCEEDED(result))
		{
			const char* pMsg = (const char*)pError->GetBufferPointer();
			OutputDebugStringA(pMsg);
		}
		else
		{
			result = m_pDevice->CreateVertexShader((*ppBlob)->GetBufferPointer(), (*ppBlob)->GetBufferSize(), NULL, &pVertexShader);
			assert(SUCCEEDED(result));
		}

		SAFE_RELEASE(pError);
	}

	return pVertexShader;
}

ID3D11PixelShader* ShaderCompiler::CreatePixelShader(ID3D11Device* m_pDevice, LPCTSTR shaderSource)
{
	ID3D11PixelShader* pPixelShader = NULL;

	FILE* pFile = NULL;

	_tfopen_s(&pFile, shaderSource, _T("rb"));
	if (pFile != NULL)
	{
		fseek(pFile, 0, SEEK_END);
		int size = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		char* pSourceCode = (char*)malloc((size_t)size + 1);
		fread(pSourceCode, size, 1, pFile);
		pSourceCode[size] = 0;

		fclose(pFile);

		ID3DBlob* pBlob = NULL;
		ID3DBlob* pError = NULL;
		HRESULT result = D3DCompile(pSourceCode, size, "", NULL, NULL, "PS", "ps_5_0", 0, 0, &pBlob, &pError);
		if (!SUCCEEDED(result))
		{
			const char* pMsg = (const char*)pError->GetBufferPointer();
			OutputDebugStringA(pMsg);
		}
		else
		{
			result = m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &pPixelShader);
			assert(SUCCEEDED(result));
		}

		SAFE_RELEASE(pError);
		SAFE_RELEASE(pBlob);
	}

	return pPixelShader;
}
