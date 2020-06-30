//--------------------------------------------------------------------------------------
// File: AsyncRender.cpp
//
// This sample shows a simple example of the Microsoft Direct3D's High-Level 
// Shader Language (HLSL) using the Effect interface. 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "Common.h"
#include "AsyncRender.h"

AsyncRender::AsyncRender()
	:m_pSecondaryDevice(NULL),
	m_pSecondaryImmediateContext(NULL),
	m_pPrimaryDevice(NULL),
	m_pPrimaryImmediateContext(NULL),
	m_bMultiDevices(FALSE),
	m_pDialogResourceManager(NULL),
	m_pD3DSettingsDlg(NULL),
	m_pSampleUI(NULL),
	m_pHUD(NULL),
	m_pTxtHelper(NULL),
	m_pTempBuffer(NULL),
	m_pSecondaryUIBuffer(NULL),
	m_pSecondaryUIBufferRTV(NULL),
	m_pSecondaryUIDepthStencilBuffer(NULL),
	m_pSecondaryUIDepthStencilView(NULL),
	m_pStagingBuffer(NULL)
{

}

AsyncRender::~AsyncRender()
{
}
//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
IDXGIAdapter* AsyncRender::QuerySecondaryAdapter(ID3D11Device* pPrimaryDevice, bool bWARP)
{
	IDXGIDevice1 * pPrimaryDXGIDevice = NULL;
	IDXGIAdapter * pPrimaryDXGIAdapter = NULL;
	IDXGIAdapter * pSecondaryDXGIAdapter = NULL;
	IDXGIAdapter * pDXGIAdapter = NULL;
	IDXGIFactory1 * pFactory = NULL;
	DXGI_ADAPTER_DESC adapterDesc;
	DXGI_ADAPTER_DESC primaryAdapterDesc;

	if( FAILED(pPrimaryDevice->QueryInterface(__uuidof(IDXGIDevice1), (void **)&pPrimaryDXGIDevice)) ||
		FAILED(pPrimaryDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pPrimaryDXGIAdapter)) )
		return NULL;

	pPrimaryDXGIAdapter->GetDesc(&primaryAdapterDesc);

	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory)))
		return NULL;

	for (UINT i = 0; pFactory->EnumAdapters(i, &pDXGIAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		pDXGIAdapter->GetDesc(&adapterDesc);
		if((adapterDesc.AdapterLuid.HighPart == primaryAdapterDesc.AdapterLuid.HighPart)&&
			(adapterDesc.AdapterLuid.LowPart == primaryAdapterDesc.AdapterLuid.LowPart))
			continue;
		if ( !bWARP && adapterDesc.VendorId == 0x8086 ) 
		{
			pSecondaryDXGIAdapter = pDXGIAdapter;
			break;
		}
		else if( bWARP && adapterDesc.VendorId == 0x1414 )
		{
			pSecondaryDXGIAdapter = pDXGIAdapter;
			break;
		}
	}

	SAFE_RELEASE(pPrimaryDXGIAdapter);
	SAFE_RELEASE(pPrimaryDXGIDevice);
	SAFE_RELEASE(pFactory);

	return pSecondaryDXGIAdapter;
}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
HRESULT	AsyncRender::OnCreateDevice(ASYNC_RESOURCE_DESC* pDesc, IDXGIAdapter* pSecondaryAapter)
{
	HRESULT hr;
	
	// init member variables
	m_Width = pDesc->Width;
	m_Height = pDesc->Height;
	m_Format = pDesc->Format;
	m_pDialogResourceManager = pDesc->DialogResourceManager;
	m_pD3DSettingsDlg = pDesc->D3DSettingsDlg;
	m_pHUD = pDesc->HUD;
	m_pSampleUI = pDesc->SampleUI;

	m_pPrimaryDevice = DXUTGetD3D11Device(); // pd3dDevice;
	m_pPrimaryImmediateContext = DXUTGetD3D11DeviceContext();//pd3dImmediateContext;

	// Define resources---------------------------------------------
	D3D11_TEXTURE2D_DESC UIBufferDesc = {
		(int)m_Width,							// UINT Width;
		(int)m_Height,							// UINT Height;
        1,                                      // UINT MipLevels;
        1,                                      // UINT ArraySize;
		m_Format,								// DXGI_FORMAT Format;
        { 1, 0, },                              // DXGI_SAMPLE_DESC SampleDesc;
        D3D11_USAGE_DEFAULT,                    // D3D11_USAGE Usage;
        D3D11_BIND_SHADER_RESOURCE 
            | D3D11_BIND_RENDER_TARGET,         // UINT BindFlags;
        0,                                      // UINT CPUAccessFlags;
        0,                                      // UINT MiscFlags;
    };

    D3D11_RENDER_TARGET_VIEW_DESC UIBufferRTVDesc = {
        m_Format,								// DXGI_FORMAT Format;
        D3D11_RTV_DIMENSION_TEXTURE2D,          // D3D11_DSV_DIMENSION ViewDimension;
        0,                                      // UINT ReadOnlyUsage;
        {0, },                                  // D3D11_TEX2D_RTV Texture2D;
    };

    D3D11_SHADER_RESOURCE_VIEW_DESC UIBufferSRVDesc = {
        m_Format,								// DXGI_FORMAT Format;
        D3D11_SRV_DIMENSION_TEXTURE2D,          // D3D11_SRV_DIMENSION ViewDimension;
        {0, 1, },                               // D3D11_TEX2D_SRV Texture2D;
    };

    D3D11_TEXTURE2D_DESC UIDepthStencilBufferDesc = {
        ( int) m_Width,   // UINT Width;
        ( int) m_Height,   // UINT Height;
        1,                                      // UINT MipLevels;
        1,                                      // UINT ArraySize;
        DXGI_FORMAT_R32_TYPELESS,               // DXGI_FORMAT Format;
        { 1, 0, },                              // DXGI_SAMPLE_DESC SampleDesc;
        D3D11_USAGE_DEFAULT,                    // D3D11_USAGE Usage; 
        D3D11_BIND_DEPTH_STENCIL,				// UINT BindFlags;
        0,                                      // UINT CPUAccessFlags;
        0,                                      // UINT MiscFlags;
    };

	D3D11_DEPTH_STENCIL_VIEW_DESC UIDepthStencilViewDesc = {
		DXGI_FORMAT_D32_FLOAT,                  // DXGI_FORMAT Format;
		D3D11_DSV_DIMENSION_TEXTURE2D,          // D3D11_DSV_DIMENSION ViewDimension;
		0,                                      // UINT ReadOnlyUsage;
		{ 0, },                                 // D3D11_TEX2D_RTV Texture2D;
	};

	// Create resources---------------------------------------------
	if(pSecondaryAapter)
	{
		m_bMultiDevices = true;

		UINT createDeviceFlags = 0;
	#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		UINT numFeatureLevels = ARRAYSIZE(featureLevels);
		V_RETURN( D3D11CreateDevice(pSecondaryAapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &m_pSecondaryDevice, NULL, &m_pSecondaryImmediateContext) );
	}
	else
	{
		m_bMultiDevices = false;
		m_pSecondaryDevice = m_pPrimaryDevice;
		m_pSecondaryImmediateContext = m_pPrimaryImmediateContext;		
	}

	V_RETURN( m_pDialogResourceManager->OnD3D11CreateDevice( m_pSecondaryDevice, m_pSecondaryImmediateContext ) );
	V_RETURN( m_pD3DSettingsDlg->OnD3D11CreateDevice( m_pSecondaryDevice ) );
	m_pTxtHelper = new CDXUTTextHelper(m_pSecondaryDevice, m_pSecondaryImmediateContext, m_pDialogResourceManager, 15 );

	V_RETURN( m_pSecondaryDevice->CreateTexture2D( &UIDepthStencilBufferDesc, NULL, &m_pSecondaryUIDepthStencilBuffer ) );
	V_RETURN( m_pSecondaryDevice->CreateDepthStencilView(m_pSecondaryUIDepthStencilBuffer, &UIDepthStencilViewDesc, &m_pSecondaryUIDepthStencilView));

	if(m_bMultiDevices)
	{
		UIBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
		V_RETURN( m_pSecondaryDevice->CreateTexture2D( &UIBufferDesc, NULL, &m_pSecondaryUIBuffer) );
		V_RETURN( m_pSecondaryDevice->CreateRenderTargetView( m_pSecondaryUIBuffer, &UIBufferRTVDesc, &m_pSecondaryUIBufferRTV ) );

		D3D11_TEXTURE2D_DESC StagingBufferDesc;
		ZeroMemory(&StagingBufferDesc,sizeof(StagingBufferDesc));
		m_pSecondaryUIBuffer->GetDesc(&StagingBufferDesc);
		StagingBufferDesc.BindFlags = 0;
		StagingBufferDesc.MiscFlags = 0;
		StagingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		StagingBufferDesc.Usage = D3D11_USAGE_STAGING;
		V_RETURN( m_pSecondaryDevice->CreateTexture2D( &StagingBufferDesc, NULL, &m_pStagingBuffer) );

		UIBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		V_RETURN( m_pPrimaryDevice->CreateTexture2D( &UIBufferDesc, NULL, &m_pPrimaryUIBuffer) );
		V_RETURN( m_pPrimaryDevice->CreateShaderResourceView( m_pPrimaryUIBuffer, &UIBufferSRVDesc, &m_pPrimaryUIBufferSRV) );

		InitializeWorkerThreads();
	}
	else
	{
		V_RETURN( m_pSecondaryDevice->CreateTexture2D( &UIBufferDesc, NULL, &m_pSecondaryUIBuffer) );
		V_RETURN( m_pSecondaryDevice->CreateRenderTargetView( m_pSecondaryUIBuffer, &UIBufferRTVDesc, &m_pSecondaryUIBufferRTV ) );
		V_RETURN( m_pPrimaryDevice->CreateShaderResourceView( m_pSecondaryUIBuffer, &UIBufferSRVDesc, &m_pPrimaryUIBufferSRV) );
	}

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
void AsyncRender::RenderText(bool bShowHelp)
{

    m_pTxtHelper->Begin();
    m_pTxtHelper->SetInsertionPos( 2, 0 );
    m_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    m_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    m_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    // Draw help
    if( bShowHelp )
    {
		m_pTxtHelper->SetInsertionPos( 2, m_Height - 20 * 6 );
        m_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        m_pTxtHelper->DrawTextLine( L"Controls:" );

        m_pTxtHelper->SetInsertionPos( 20, m_Height - 20 * 5 );
        m_pTxtHelper->DrawTextLine( L"Rotate model: Left mouse button\n"
                                    L"Rotate light: Right mouse button\n"
                                    L"Rotate camera: Middle mouse button\n"
                                    L"Zoom camera: Mouse wheel scroll\n" );

        m_pTxtHelper->SetInsertionPos( 350, m_Height - 20 * 5 );
        m_pTxtHelper->DrawTextLine( L"Hide help: F1\n"
                                    L"Quit: ESC\n" );
    }
    else
    {
        m_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        m_pTxtHelper->DrawTextLine( L"Press F1 for help" );
    }

    m_pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
void AsyncRender::OnReleasingSwapChain()
{
	m_pDialogResourceManager->OnD3D11ReleasingSwapChain();
}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
HRESULT AsyncRender::OnResizedSwapChain()
{
	HRESULT hr;
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc;
	pBackBufferSurfaceDesc = DXUTGetDXGIBackBufferSurfaceDesc();
	m_Width = pBackBufferSurfaceDesc->Width;
	m_Height = pBackBufferSurfaceDesc->Height;

	//const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc = DXUTGetDXGIBackBufferSurfaceDesc();
	V_RETURN(m_pDialogResourceManager->OnD3D11ResizedSwapChain(m_pSecondaryDevice, pBackBufferSurfaceDesc));
	V_RETURN(m_pD3DSettingsDlg->OnD3D11ResizedSwapChain(m_pSecondaryDevice, pBackBufferSurfaceDesc));

	m_pHUD->SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	m_pHUD->SetSize(170, 170);
	m_pSampleUI->SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300);
	m_pSampleUI->SetSize(170, 300);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
void AsyncRender::OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	m_fTime = fTime;
	m_fElapsedTime = fElapsedTime;
	m_bShowHelp = *((bool*)pUserContext);

	if(!m_bMultiDevices)
	{
		FrameRendering();
	}
	else
	{
		static bool first = TRUE;
		if(first)
		{
			ResumeThread( m_hRenderThread );
			SetEvent(m_hEmptyEvent);
			WaitForSingleObject(m_hFullEvent,INFINITE);
			SetEvent(m_hFullEvent);
			first = FALSE;
		}
	}
}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
//void AsyncRender::OnFrameRender(double fTime, float fElapsedTime, void* pUserContext)
//{
//    HRESULT hr;
//
//    // Assume this context is completely from scratch for purposes of subsequent HUD rendering	
//	m_pSecondaryImmediateContext->ClearState();
//	V( SetupViews( m_pSecondaryImmediateContext ) );
//
//	// Clear the render target
//	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
//	m_pSecondaryImmediateContext->ClearRenderTargetView( m_pSecondaryUIBufferRTV, ClearColor );
//	m_pSecondaryImmediateContext->ClearDepthStencilView( m_pSecondaryUIDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0 );
//
//	// Render the HUD
//	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
//	m_pHUD->OnRender( fElapsedTime );
//	m_pSampleUI->OnRender( fElapsedTime );
//	RenderText(m_bShowHelp);
//	DXUT_EndPerfEvent();
//
//	if(m_bMultiDevices)
//	{
//		//present the frame
//		m_pSecondaryImmediateContext->Flush();
//	}
//}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
void AsyncRender::OnDestroyDevice()
{
    m_pDialogResourceManager->OnD3D11DestroyDevice();
    m_pD3DSettingsDlg->OnD3D11DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();

    SAFE_DELETE( m_pTxtHelper );
	SAFE_DELETE( m_pTempBuffer );
              
    //SAFE_RELEASE( g_pVertexLayout11 );
    //SAFE_RELEASE( g_pVertexShader );
    //SAFE_RELEASE( g_pPixelShader );
    //SAFE_RELEASE( g_pSamPointClamp );
    //SAFE_RELEASE( g_pSamLinearWrap );
    //SAFE_RELEASE( g_pRasterizerStateNoCull );
    //SAFE_RELEASE( g_pRasterizerStateBackfaceCull );
    //SAFE_RELEASE( g_pRasterizerStateFrontfaceCull );
    //SAFE_RELEASE( g_pRasterizerStateNoCullWireFrame );
    //SAFE_RELEASE( g_pDepthStencilStateNoStencil );

	if(!m_bMultiDevices)	// don't release interfaces created by main thread
	{
		m_pSecondaryDevice = NULL;
		m_pSecondaryImmediateContext = NULL;
	}

	SAFE_RELEASE(m_pSecondaryDevice);
	SAFE_RELEASE(m_pSecondaryImmediateContext);
	SAFE_RELEASE(m_pSecondaryUIBuffer);
	SAFE_RELEASE(m_pSecondaryUIBufferRTV);
	SAFE_RELEASE(m_pSecondaryUIDepthStencilBuffer);
	SAFE_RELEASE(m_pSecondaryUIDepthStencilView);
	SAFE_RELEASE(m_pStagingBuffer);
	SAFE_RELEASE(m_pPrimaryUIBuffer);
	SAFE_RELEASE(m_pPrimaryUIBufferSRV)

	CloseHandle(m_hFullEvent);
	CloseHandle(m_hEmptyEvent);
	CloseHandle(m_hRenderThread);
}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
HRESULT AsyncRender::SetupViews( ID3D11DeviceContext* pd3dDeviceContext )
{
    HRESULT hr = S_OK;

    // Setup the viewport to match the backbuffer
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)m_Width;
    vp.Height = (FLOAT)m_Height;
    vp.MaxDepth = 1;
    vp.MinDepth = 0;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pd3dDeviceContext->RSSetViewports( 1, &vp );

    // Set the render targets
    ID3D11RenderTargetView* pRTV = m_pSecondaryUIBufferRTV;
    ID3D11DepthStencilView* pDSV = m_pSecondaryUIDepthStencilView;
    pd3dDeviceContext->OMSetRenderTargets( 1, &pRTV, pDSV );

    return hr;
}


//--------------------------------------------------------------------------------------
// QueryResourceView. dwMilliseconds:0 return
//--------------------------------------------------------------------------------------
ID3D11ShaderResourceView* AsyncRender::QueryResourceView(DWORD dwMilliseconds)
{
	if(!m_bMultiDevices)
	{
		return m_pPrimaryUIBufferSRV;
	}

	if( WaitForSingleObject(m_hFullEvent,dwMilliseconds) == WAIT_OBJECT_0 )
	{ 
		ID3D11Resource* pUITexture;
		m_pPrimaryUIBufferSRV->GetResource(&pUITexture);
		m_pPrimaryImmediateContext->UpdateSubresource(pUITexture,0,NULL,m_pTempBuffer,m_RowPitch,m_DepthPitch);
		SAFE_RELEASE(pUITexture);

		SetEvent(m_hEmptyEvent);
	}
	return m_pPrimaryUIBufferSRV;
}


//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
void AsyncRender::FrameStaging()
{
	m_pSecondaryImmediateContext->CopyResource(m_pStagingBuffer,m_pSecondaryUIBuffer);

	WaitForSingleObject( m_hEmptyEvent, INFINITE );

	//ID3D11Resource* pUITexture;
	D3D11_MAPPED_SUBRESOURCE mappedResource; 
	ZeroMemory(&mappedResource,sizeof(mappedResource));

	//注意：以下代码要修改，map时可能CopyResource还没有结束，map可能返回错误代码。D3D11_MAP_FLAG_DO_NOT_WAIT，D3DERROR_WASSTILLDRAWING
	m_pSecondaryImmediateContext->Map(m_pStagingBuffer,0,D3D11_MAP_READ,0,&mappedResource);
	
	//m_pPrimaryUIBufferSRV->GetResource(&pUITexture);
	//m_pPrimaryImmediateContext->UpdateSubresource(pUITexture,0,NULL,mappedResource.pData,mappedResource.RowPitch,mappedResource.DepthPitch);
	if(!m_pTempBuffer)
	{
		m_RowPitch = mappedResource.RowPitch;
		m_DepthPitch = mappedResource.DepthPitch;
		m_pTempBuffer = new BYTE[m_RowPitch*m_Height];
	}
	memcpy(m_pTempBuffer,mappedResource.pData,m_RowPitch*m_Height);
	m_pSecondaryImmediateContext->Unmap(m_pStagingBuffer,0);
	//SAFE_RELEASE(pUITexture);

	SetEvent(m_hFullEvent);
}


//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
//unsigned int WINAPI AsyncRender::_StagingProc( LPVOID lpParameter )
//{
//	AsyncRender* pRender = (AsyncRender*)lpParameter;
//	pRender->FrameStaging();
//	return 0;
//}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
void AsyncRender::FrameRendering()
{
    HRESULT hr;

	// Assume this context is completely from scratch for purposes of subsequent HUD rendering	
	m_pSecondaryImmediateContext->ClearState();
	V( SetupViews( m_pSecondaryImmediateContext ) );

	// Clear the render target
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pSecondaryImmediateContext->ClearRenderTargetView( m_pSecondaryUIBufferRTV, ClearColor );
	m_pSecondaryImmediateContext->ClearDepthStencilView( m_pSecondaryUIDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0 );

	// Render the HUD
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
	m_pHUD->OnRender( m_fElapsedTime );
	m_pSampleUI->OnRender( m_fElapsedTime );
	RenderText(m_bShowHelp);
	DXUT_EndPerfEvent();

	if(m_bMultiDevices)
	{
		//present the frame
		m_pSecondaryImmediateContext->Flush();
	}		
}

//--------------------------------------------------------------------------------------
// Sets the viewport, render target view, and depth stencil view.
//--------------------------------------------------------------------------------------
unsigned int WINAPI AsyncRender::_RenderProc( LPVOID lpParameter )
{
	AsyncRender* pRender = (AsyncRender*)lpParameter;
	//pRender->FrameRendering();
    for (;;)
    {
		//WaitForSingleObject( pRender->m_hBeginRenderingEvent, INFINITE );
		pRender->FrameRendering();
		pRender->FrameStaging();
	}
	return 0;
}

//--------------------------------------------------------------------------------------
// Create per-worker-thread resources
//--------------------------------------------------------------------------------------
void AsyncRender::InitializeWorkerThreads()
{
        m_hFullEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        m_hEmptyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        m_hRenderThread = ( HANDLE )_beginthreadex( 
            NULL, 
            0, 
            _RenderProc, 
            this, 
            CREATE_SUSPENDED, 
            NULL );

		//ResumeThread( m_hRenderThread );
}