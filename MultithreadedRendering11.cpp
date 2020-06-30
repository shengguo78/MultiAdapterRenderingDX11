//--------------------------------------------------------------------------------------
// File: MultithreadedRendering11.cpp
//
// This sample shows a simple example of the Microsoft Direct3D's High-Level 
// Shader Language (HLSL) using the Effect interface. 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "Common.h"
#include "AsyncRender.h"


//--------------------------------------------------------------------------------------
// FULL SCREEN QUAD
//--------------------------------------------------------------------------------------
ID3D11Buffer*				g_pQuadVB = NULL;
ID3D11VertexShader*			g_pFullScreenQuadVS = NULL;
ID3D11InputLayout*			g_pFullScreenQuadLayout = NULL;
ID3D11PixelShader*			g_pFullScreenQuadPS = NULL;
// blend&DS states
ID3D11BlendState*			g_UICompositeBlendState = NULL;
ID3D11DepthStencilState*	g_UICompositeDepthStencilState = NULL;

HRESULT		CreateFullScreenQuad(ID3D11Device* pd3dDevice, int width, int height);
void		DrawFullScreenQuad(ID3D11DeviceContext* pD3DImmediateContext);
void		CompositeUIIntoScene( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN                    1
#define IDC_TOGGLEREF                           3
#define IDC_CHANGEDEVICE                        4
#define IDC_TOGGLEWIRE                          5
#define IDC_DEVICECONTEXT_GROUP                 6
#define IDC_TOGGLELIGHTVIEW                     12

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
bool                        g_bClearStateUponBeginCommandList = false;
bool                        g_bClearStateUponFinishCommandList = false;
bool                        g_bClearStateUponExecuteCommandList = false;

CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
#ifdef ADJUSTABLE_LIGHT
CDXUTDirectionWidget        g_LightControl;
#endif
CD3DSettingsDlg             g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                 g_HUD;                  // manages the 3D   
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
CDXUTTextHelper*            g_pTxtHelper = NULL;
bool                        g_bShowHelp = false;    // If true, it renders the UI control text
bool                        g_bWireFrame = false;

AsyncRender					g_AsyncRender;
//--------------------------------------------------------------------------------------
// Default view parameters
//--------------------------------------------------------------------------------------
CModelViewerCamera          g_Camera;               // A model viewing camera
D3DXVECTOR3                 g_vDefaultEye           ( 30.0f, 150.0f, -150.0f );
D3DXVECTOR3                 g_vDefaultLookAt        ( 0.0f, 60.0f, 0.0f );
D3DXVECTOR3                 g_vUp                   ( 0.0f, 1.0f, 0.0f );
D3DXVECTOR3                 g_vDown                 = -g_vUp;
FLOAT                       g_fNearPlane            = 2.0f;
FLOAT                       g_fFarPlane             = 4000.0f;
FLOAT                       g_fFOV                  = D3DX_PI / 4.0f;
D3DXVECTOR3                 g_vSceneCenter          ( 0.0f, 350.0f, 0.0f );
FLOAT                       g_fSceneRadius          = 600.0f;
FLOAT                       g_fDefaultCameraRadius  = 300.0f;
FLOAT                       g_fMinCameraRadius      = 150.0f;
FLOAT                       g_fMaxCameraRadius      = 450.0f;
#ifdef RENDER_SCENE_LIGHT_POV
bool                        g_bRenderSceneLightPOV  = false;
#endif

//--------------------------------------------------------------------------------------
// Lighting params (to be read from content when the pipeline supports it)
//--------------------------------------------------------------------------------------
D3DXVECTOR4                 g_vAmbientColor         ( 0.04f * 0.760f, 0.04f * 0.793f, 0.04f * 0.822f, 1.000f );
D3DXVECTOR4                 g_vMirrorTint           ( 0.3f, 0.5f, 1.0f, 1.0f );
D3DXVECTOR4                 g_vLightColor[g_iNumLights];
D3DXVECTOR3                 g_vLightPos[g_iNumLights];
D3DXVECTOR3                 g_vLightDir[g_iNumLights];
FLOAT                       g_fLightFalloffDistEnd[g_iNumLights];
FLOAT                       g_fLightFalloffDistRange[g_iNumLights];
FLOAT                       g_fLightFalloffCosAngleEnd[g_iNumLights];
FLOAT                       g_fLightFalloffCosAngleRange[g_iNumLights];
FLOAT                       g_fLightFOV[g_iNumLights];
FLOAT                       g_fLightAspect[g_iNumLights];
FLOAT                       g_fLightNearPlane[g_iNumLights];
FLOAT                       g_fLightFarPlane[g_iNumLights];

// The scene data
CMultiDeviceContextDXUTMesh g_Mesh11;

//--------------------------------------------------------------------------------------
// Rendering interfaces
//--------------------------------------------------------------------------------------
ID3D11InputLayout*          g_pVertexLayout11 = NULL;
ID3D11VertexShader*         g_pVertexShader = NULL;
ID3D11PixelShader*          g_pPixelShader = NULL;
ID3D11SamplerState*         g_pSamPointClamp = NULL;
ID3D11SamplerState*         g_pSamLinearWrap = NULL;
ID3D11RasterizerState*      g_pRasterizerStateNoCull = NULL;
ID3D11RasterizerState*      g_pRasterizerStateBackfaceCull = NULL;
ID3D11RasterizerState*      g_pRasterizerStateFrontfaceCull = NULL;
ID3D11RasterizerState*      g_pRasterizerStateNoCullWireFrame = NULL;
ID3D11DepthStencilState*    g_pDepthStencilStateNoStencil = NULL;

UINT                        g_iCBVSPerObjectBind = 0;
UINT                        g_iCBVSPerSceneBind = 1;
UINT                        g_iCBPSPerObjectBind = 0;
UINT                        g_iCBPSPerLightBind = 1;
UINT                        g_iCBPSPerSceneBind = 2;
ID3D11Buffer*               g_pcbVSPerObject = NULL;
ID3D11Buffer*               g_pcbVSPerScene = NULL;
ID3D11Buffer*               g_pcbPSPerObject = NULL;
ID3D11Buffer*               g_pcbPSPerLight = NULL;
ID3D11Buffer*               g_pcbPSPerScene = NULL;

//--------------------------------------------------------------------------------------
// Shadow map data and interface
//--------------------------------------------------------------------------------------
ID3D11Texture2D*            g_pShadowTexture[g_iNumShadows] = { NULL };
ID3D11ShaderResourceView*   g_pShadowResourceView[g_iNumShadows] = { NULL };
ID3D11DepthStencilView*     g_pShadowDepthStencilView[g_iNumShadows] = { NULL };
D3D11_VIEWPORT              g_ShadowViewport[g_iNumShadows] = { 0 };
FLOAT                       g_fShadowResolutionX[g_iNumShadows];
FLOAT                       g_fShadowResolutionY[g_iNumShadows];

SceneParamsStatic           g_StaticParamsDirect;
SceneParamsStatic           g_StaticParamsShadow[g_iNumShadows];


//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

void InitApp();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    
    InitApp();
    DXUTInit( true, true, lpCmdLine ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Multi-Adapter UI Rendering" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); 
    int iY = 30;
    int iYo = 26;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += iYo, 170, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += iYo, 170, 22, VK_F2 );
#ifdef RENDER_SCENE_LIGHT_POV
    g_HUD.AddButton( IDC_TOGGLELIGHTVIEW, L"Toggle view (F4)", 0, iY += iYo, 170, 22, VK_F4 );
#endif
    g_HUD.AddButton( IDC_TOGGLEWIRE, L"Toggle Wires (F6)", 0, iY += iYo, 170, 22, VK_F6 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 or D3D11 device, allowing the app to 
// modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // Uncomment this to get debug information from D3D11
    //pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    static float fTotalTime = 0.0f;
    fTotalTime += fElapsedTime;

    // Jigger the overhead lights --- these are hard-coded to indices 1,2,3
    // Ideally, we'd attach the lights to the relevant objects in the mesh
    // file and animate those objects.  But for now, just some hard-coded
    // swinging...
    float fCycle1X = 0.0f;
    float fCycle1Z = 0.20f * sinf( 2.0f * ( fTotalTime + 0.0f * D3DX_PI ) );
    g_vLightDir[1] = g_vDown + D3DXVECTOR3( fCycle1X, 0.0f, fCycle1Z );
    D3DXVec3Normalize( &g_vLightDir[1], &g_vLightDir[1] );

    float fCycle2X = 0.10f * cosf( 1.6f * ( fTotalTime + 0.3f * D3DX_PI ) );
    float fCycle2Z = 0.10f * sinf( 1.6f * ( fTotalTime + 0.0f * D3DX_PI ) );
    g_vLightDir[2] = g_vDown + D3DXVECTOR3( fCycle2X, 0.0f, fCycle2Z );
    D3DXVec3Normalize( &g_vLightDir[2], &g_vLightDir[2] );

    float fCycle3X = 0.30f * cosf( 2.4f * ( fTotalTime + 0.3f * D3DX_PI ) );
    float fCycle3Z = 0.0f;
    g_vLightDir[3] = g_vDown + D3DXVECTOR3( fCycle3X, 0.0f, fCycle3Z );
    D3DXVec3Normalize( &g_vLightDir[3], &g_vLightDir[3] );

    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );

	g_AsyncRender.OnFrameMove(fTime, fElapsedTime, &g_bShowHelp);
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}

//--------------------------------------------------------------------------------------
// Create D3D11 resources for the Lights
//--------------------------------------------------------------------------------------
void InitializeLights()
{
    // Our hand-tuned approximation to the sky light
    //g_vLightColor[0] =                  D3DXVECTOR4( 1.5f * 0.160f, 1.5f * 0.341f, 1.5f * 1.000f, 1.000f );
    g_vLightColor[0] =                  D3DXVECTOR4( 3.0f * 0.160f, 3.0f * 0.341f, 3.0f * 1.000f, 1.000f );
    g_vLightDir[0] =                    D3DXVECTOR3( -0.67f, -0.71f, +0.21f );
    g_vLightPos[0] =                    g_vSceneCenter - g_fSceneRadius * g_vLightDir[0];
    g_fLightFOV[0] =                    D3DX_PI / 4.0f;

    // The three overhead lamps
    g_vLightColor[1] =                  D3DXVECTOR4( 0.4f * 0.895f, 0.4f * 0.634f, 0.4f * 0.626f, 1.0f );
    g_vLightPos[1] =                    D3DXVECTOR3( 0.0f, 400.0f, -250.0f );
    g_vLightDir[1] =                    D3DXVECTOR3( 0.00f, -1.00f, 0.00f );
    g_fLightFOV[1] =                    65.0f * ( D3DX_PI / 180.0f );
    
    g_vLightColor[2] =                  D3DXVECTOR4( 0.5f * 0.388f, 0.5f * 0.641f, 0.5f * 0.401f, 1.0f );
    g_vLightPos[2] =                    D3DXVECTOR3( 0.0f, 400.0f, 0.0f );
    g_vLightDir[2] =                    D3DXVECTOR3( 0.00f, -1.00f, 0.00f );
    g_fLightFOV[2] =                    65.0f * ( D3DX_PI / 180.0f );
    
    g_vLightColor[3] =                  D3DXVECTOR4( 0.4f * 1.000f, 0.4f * 0.837f, 0.4f * 0.848f, 1.0f );
    g_vLightPos[3] =                    D3DXVECTOR3( 0.0f, 400.0f, 250.0f );
    g_vLightDir[3] =                    D3DXVECTOR3( 0.00f, -1.00f, 0.00f );
    g_fLightFOV[3] =                    65.0f * ( D3DX_PI / 180.0f );
    
    // For the time beings, let's make these params follow the same pattern for all lights
    for ( int iLight = 0; iLight < g_iNumLights; ++iLight )
    {
        g_fLightAspect[iLight] = 1.0f;
        g_fLightNearPlane[iLight] = 100.f;
        g_fLightFarPlane[iLight] = 2.0f * g_fSceneRadius;

        g_fLightFalloffDistEnd[iLight] = g_fLightFarPlane[iLight];
        g_fLightFalloffDistRange[iLight] = 100.0f;

        g_fLightFalloffCosAngleEnd[iLight] = cosf( g_fLightFOV[iLight] / 2.0f );
        g_fLightFalloffCosAngleRange[iLight] = 0.1f;

        D3DXVec3Normalize( &g_vLightDir[iLight], &g_vLightDir[iLight] );
    }

#ifdef ADJUSTABLE_LIGHT
    // The adjustable light is number 0
    g_LightControl.SetLightDirection( g_vLightDir[0] );
#endif
}

//--------------------------------------------------------------------------------------
// Create D3D11 resources for the shadows
//--------------------------------------------------------------------------------------
HRESULT InitializeShadows( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    for ( int iShadow = 0; iShadow < g_iNumShadows; ++iShadow )
    {
        // constant for now
        g_fShadowResolutionX[iShadow] = 2048.0f;
        g_fShadowResolutionY[iShadow] = 2048.0f;

        // The shadow map, along with depth-stencil and texture view
        D3D11_TEXTURE2D_DESC ShadowDesc = {
            ( int) g_fShadowResolutionX[iShadow],   // UINT Width;
            ( int) g_fShadowResolutionY[iShadow],   // UINT Height;
            1,                                      // UINT MipLevels;
            1,                                      // UINT ArraySize;
            DXGI_FORMAT_R32_TYPELESS,               // DXGI_FORMAT Format;
            { 1, 0, },                              // DXGI_SAMPLE_DESC SampleDesc;
            D3D11_USAGE_DEFAULT,                    // D3D11_USAGE Usage;
            D3D11_BIND_SHADER_RESOURCE 
                | D3D11_BIND_DEPTH_STENCIL,         // UINT BindFlags;
            0,                                      // UINT CPUAccessFlags;
            0,                                      // UINT MiscFlags;
        };
        D3D11_DEPTH_STENCIL_VIEW_DESC ShadowDepthStencilViewDesc = {
            DXGI_FORMAT_D32_FLOAT,                  // DXGI_FORMAT Format;
            D3D11_DSV_DIMENSION_TEXTURE2D,          // D3D11_DSV_DIMENSION ViewDimension;
            0,                                      // UINT ReadOnlyUsage;
            {0, },                                  // D3D11_TEX2D_RTV Texture2D;
        };
        D3D11_SHADER_RESOURCE_VIEW_DESC ShadowResourceViewDesc = {
            DXGI_FORMAT_R32_FLOAT,                  // DXGI_FORMAT Format;
            D3D11_SRV_DIMENSION_TEXTURE2D,          // D3D11_SRV_DIMENSION ViewDimension;
            {0, 1, },                               // D3D11_TEX2D_SRV Texture2D;
        };
        V_RETURN( pd3dDevice->CreateTexture2D( &ShadowDesc, NULL, &g_pShadowTexture[iShadow] ) );
        DXUT_SetDebugName( g_pShadowTexture[iShadow], "Shadow" );

        V_RETURN( pd3dDevice->CreateDepthStencilView( g_pShadowTexture[iShadow], &ShadowDepthStencilViewDesc, 
            &g_pShadowDepthStencilView[iShadow] ) );
        DXUT_SetDebugName( g_pShadowDepthStencilView[iShadow], "Shadow DSV" );

        V_RETURN( pd3dDevice->CreateShaderResourceView( g_pShadowTexture[iShadow], &ShadowResourceViewDesc, 
            &g_pShadowResourceView[iShadow] ) );
        DXUT_SetDebugName( g_pShadowResourceView[iShadow] , "Shadow RSV" );

        g_ShadowViewport[iShadow].Width = g_fShadowResolutionX[iShadow];
        g_ShadowViewport[iShadow].Height = g_fShadowResolutionY[iShadow];
        g_ShadowViewport[iShadow].MinDepth = 0;
        g_ShadowViewport[iShadow].MaxDepth = 1;
        g_ShadowViewport[iShadow].TopLeftX = 0;
        g_ShadowViewport[iShadow].TopLeftY = 0;

        // The parameters to pass to per-chunk threads for the shadow scenes
        g_StaticParamsShadow[iShadow].m_pDepthStencilState = g_pDepthStencilStateNoStencil;
        g_StaticParamsShadow[iShadow].m_iStencilRef = 0;
        g_StaticParamsShadow[iShadow].m_pRasterizerState = g_pRasterizerStateFrontfaceCull;
        g_StaticParamsShadow[iShadow].m_vMirrorPlane = D3DXPLANE( 0.0f, 0.0f, 0.0f, 0.0f );
        g_StaticParamsShadow[iShadow].m_vTintColor = D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 1.0f );
        g_StaticParamsShadow[iShadow].m_pDepthStencilView = g_pShadowDepthStencilView[iShadow];
        g_StaticParamsShadow[iShadow].m_pViewport = &g_ShadowViewport[iShadow];
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Helper functions for querying information about the processors in the current
// system.  ( Copied from the doc page for GetLogicalProcessorInformation() )
//--------------------------------------------------------------------------------------
typedef BOOL (WINAPI *LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, 
    PDWORD);


//  Helper function to count bits in the processor mask
static DWORD CountBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
    DWORD bitSetCount = 0;
    DWORD bitTest = 1 << LSHIFT;
    DWORD i;

    for( i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest)?1:0);
        bitTest/=2;
    }

    return bitSetCount;
}

static int GetPhysicalProcessorCount()
{
    DWORD procCoreCount = 0;    // Return 0 on any failure.  That'll show them.

    LPFN_GLPI Glpi;

    Glpi = (LPFN_GLPI) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),
        "GetLogicalProcessorInformation");
    if (NULL == Glpi) 
    {
        // GetLogicalProcessorInformation is not supported
        return procCoreCount;
    }

    BOOL done = FALSE;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    DWORD returnLength = 0;

    while (!done) 
    {
        DWORD rc = Glpi(buffer, &returnLength);

        if (FALSE == rc) 
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
            {
                if (buffer) 
                    free(buffer);

                buffer=(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                    returnLength);

                if (NULL == buffer) 
                {
                    // Allocation failure\n
                    return procCoreCount;
                }
            } 
            else 
            {
                // Unanticipated error
                return procCoreCount;
            }
        } 
        else done = TRUE;
    }

    DWORD byteOffset = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
    while (byteOffset < returnLength) 
    {
        if (ptr->Relationship == RelationProcessorCore) 
        {
            if(ptr->ProcessorCore.Flags)
            {
                //  Hyperthreading or SMT is enabled.
                //  Logical processors are on the same core.
                procCoreCount += 1;
            }
            else
            {
                //  Logical processors are on different cores.
                procCoreCount += CountBits(ptr->ProcessorMask);
            }
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    free (buffer);

    return procCoreCount;
}


//--------------------------------------------------------------------------------------
// Get Secondary graphics adapter (IG or WARP in Win8+) if it exists 
//--------------------------------------------------------------------------------------
IDXGIAdapter* GetSecondaryAdapter(ID3D11Device* pd3dPrimaryDevice, bool bWARP = FALSE)
{
	IDXGIDevice1 * pPrimaryDXGIDevice = NULL;
	IDXGIAdapter * pPrimaryDXGIAdapter = NULL;
	IDXGIAdapter * pSecondaryDXGIAdapter = NULL;
	IDXGIAdapter * pDXGIAdapter = NULL;
	IDXGIFactory1 * pFactory = NULL;
	DXGI_ADAPTER_DESC adapterDesc;
	DXGI_ADAPTER_DESC primaryAdapterDesc;

	if( FAILED(pd3dPrimaryDevice->QueryInterface(__uuidof(IDXGIDevice1), (void **)&pPrimaryDXGIDevice)) ||
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
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

	IDXGIAdapter* pSecondaryAdapter=NULL;
	pSecondaryAdapter = g_AsyncRender.QuerySecondaryAdapter(pd3dDevice,TRUE);

	ASYNC_RESOURCE_DESC desc;
	desc.Width = pBackBufferSurfaceDesc->Width;
	desc.Height = pBackBufferSurfaceDesc->Height;
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.DialogResourceManager = &g_DialogResourceManager;
	desc.D3DSettingsDlg = &g_D3DSettingsDlg;
	desc.HUD = &g_HUD;
	desc.SampleUI = &g_SampleUI;

	V_RETURN(g_AsyncRender.OnCreateDevice(&desc,pSecondaryAdapter));
	SAFE_RELEASE(pSecondaryAdapter);

	V_RETURN( CreateFullScreenQuad(pd3dDevice, desc.Width, desc.Height) );

	///////////////////////// Create the Main scene resources in discrete graphics////////////////////////////////////////
    // Compile the shaders 
    ID3DBlob* pVertexShaderBuffer = NULL;
    V_RETURN( CompileShaderFromFile( L"MultithreadedRendering11_VS.hlsl", "VSMain", "vs_4_0", &pVertexShaderBuffer ) );

    ID3DBlob* pPixelShaderBuffer = NULL;
    V_RETURN( CompileShaderFromFile( L"MultithreadedRendering11_PS.hlsl", "PSMain", "ps_4_0", &pPixelShaderBuffer ) );

    // Create the shaders
    V_RETURN( pd3dDevice->CreateVertexShader( pVertexShaderBuffer->GetBufferPointer(),
                                              pVertexShaderBuffer->GetBufferSize(), NULL, &g_pVertexShader ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pPixelShaderBuffer->GetBufferPointer(),
                                             pPixelShaderBuffer->GetBufferSize(), NULL, &g_pPixelShader ) );

    DXUT_SetDebugName( g_pVertexShader, "VSMain" );
    DXUT_SetDebugName( g_pPixelShader, "PSMain" );

    // Create our vertex input layout
    // The content exporter supports either compressed or uncompressed formats for 
    // normal/tangent/binormal.  Unfortunately the relevant compressed formats are
    // deprecated for DX10+.  So they required special handling in the vertex shader.
    // If we use uncompressed data here, need to also #define UNCOMPRESSED_VERTEX_DATA
    // in the HLSL file.
    const D3D11_INPUT_ELEMENT_DESC UncompressedLayout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,      0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    const D3D11_INPUT_ELEMENT_DESC CompressedLayout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R10G10B10A2_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R16G16_FLOAT,      0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R10G10B10A2_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

#ifdef UNCOMPRESSED_VERTEX_DATA
    V_RETURN( pd3dDevice->CreateInputLayout( UncompressedLayout, 
        ARRAYSIZE( UncompressedLayout ), 
        pVertexShaderBuffer->GetBufferPointer(),
        pVertexShaderBuffer->GetBufferSize(), 
        &g_pVertexLayout11 ) );
    DXUT_SetDebugName(g_pVertexLayout11, "Uncompressed" );
#else
    V_RETURN( pd3dDevice->CreateInputLayout( CompressedLayout, 
        ARRAYSIZE( CompressedLayout ), 
        pVertexShaderBuffer->GetBufferPointer(),
        pVertexShaderBuffer->GetBufferSize(), 
        &g_pVertexLayout11 ) );

    DXUT_SetDebugName(g_pVertexLayout11, "Compressed" );
#endif

  
    SAFE_RELEASE( pVertexShaderBuffer );
    SAFE_RELEASE( pPixelShaderBuffer );

    // The standard depth-stencil state
    D3D11_DEPTH_STENCIL_DESC DepthStencilDescNoStencil = {
        TRUE,                           // BOOL DepthEnable;
        D3D11_DEPTH_WRITE_MASK_ALL,     // D3D11_DEPTH_WRITE_MASK DepthWriteMask;
        D3D11_COMPARISON_LESS_EQUAL,    // D3D11_COMPARISON_FUNC DepthFunc;
        FALSE,                          // BOOL StencilEnable;
        0,                              // UINT8 StencilReadMask;
        0,                              // UINT8 StencilWriteMask;
        {                               // D3D11_DEPTH_STENCILOP_DESC FrontFace;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilFailOp;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilDepthFailOp;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilPassOp;
            D3D11_COMPARISON_NEVER,     // D3D11_COMPARISON_FUNC StencilFunc;
        }, 
        {                               // D3D11_DEPTH_STENCILOP_DESC BackFace;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilFailOp;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilDepthFailOp;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilPassOp;
            D3D11_COMPARISON_NEVER,     // D3D11_COMPARISON_FUNC StencilFunc;
        }, 
    };
    V_RETURN( pd3dDevice->CreateDepthStencilState( 
        &DepthStencilDescNoStencil, 
        &g_pDepthStencilStateNoStencil ) );
    DXUT_SetDebugName( g_pDepthStencilStateNoStencil, "No Stencil" );

    // Provide the intercept callback for CMultiDeviceContextDXUTMesh, which allows
    // us to farm out different mesh chunks to different device contexts
    void RenderMesh( CMultiDeviceContextDXUTMesh* pMesh, 
        UINT iMesh,
        bool bAdjacent,
        ID3D11DeviceContext* pd3dDeviceContext,
        UINT iDiffuseSlot,
        UINT iNormalSlot,
        UINT iSpecularSlot );
    MDC_SDKMESH_CALLBACKS11 MeshCallbacks;
    ZeroMemory( &MeshCallbacks, sizeof(MeshCallbacks) ); 
    MeshCallbacks.pRenderMesh = RenderMesh;

    // Load the mesh
    V_RETURN( g_Mesh11.Create( pd3dDevice, L"SquidRoom\\SquidRoom.sdkmesh", true, &MeshCallbacks ) );

    // Create sampler states for point/clamp (shadow map) and linear/wrap (everything else)
    D3D11_SAMPLER_DESC SamDesc;
    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamDesc.MipLODBias = 0.0f;
    SamDesc.MaxAnisotropy = 1;
    SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
    SamDesc.MinLOD = 0;
    SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamDesc, &g_pSamPointClamp ) );
    DXUT_SetDebugName( g_pSamPointClamp, "PointClamp" );

    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamDesc, &g_pSamLinearWrap ) );
    DXUT_SetDebugName( g_pSamLinearWrap, "LinearWrap" );

    // Setup constant buffers
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;

    Desc.ByteWidth = sizeof( CB_VS_PER_SCENE );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbVSPerScene ) );
    DXUT_SetDebugName( g_pcbVSPerScene, "CB_VS_PER_SCENE" );

    Desc.ByteWidth = sizeof( CB_VS_PER_OBJECT );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbVSPerObject ) );
    DXUT_SetDebugName( g_pcbVSPerObject, "CB_VS_PER_OBJECT" );

    Desc.ByteWidth = sizeof( CB_PS_PER_SCENE );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbPSPerScene ) );
    DXUT_SetDebugName( g_pcbPSPerScene, "CB_PS_PER_SCENE" );

    Desc.ByteWidth = sizeof( CB_PS_PER_OBJECT );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbPSPerObject ) );
    DXUT_SetDebugName( g_pcbPSPerObject, "CB_PS_PER_OBJECT" );

    Desc.ByteWidth = sizeof( CB_PS_PER_LIGHT );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbPSPerLight ) );
    DXUT_SetDebugName( g_pcbPSPerLight, "CB_PS_PER_LIGHT" );

    // Setup the camera's view parameters
    g_Camera.SetViewParams( &g_vDefaultEye, &g_vDefaultLookAt );
    g_Camera.SetRadius( g_fDefaultCameraRadius, g_fMinCameraRadius, g_fMaxCameraRadius );

    // Setup backface culling states:
    //  1) g_pRasterizerStateNoCull --- no culling (debugging only)
    //  2) g_pRasterizerStateBackfaceCull --- backface cull (mirror quads and the assets 
    //      reflected in the mirrors)
    //  3) g_pRasterizerStateFrontfaceCull --- frontface cull (pre-built assets from 
    //      the content pipeline)
    D3D11_RASTERIZER_DESC RasterizerDescNoCull = {
        D3D11_FILL_SOLID,   // D3D11_FILL_MODE FillMode;
        D3D11_CULL_NONE,    // D3D11_CULL_MODE CullMode;
        TRUE,               // BOOL FrontCounterClockwise;
        0,                  // INT DepthBias;
        0,                  // FLOAT DepthBiasClamp;
        0,                  // FLOAT SlopeScaledDepthBias;
        FALSE,              // BOOL DepthClipEnable;
        FALSE,              // BOOL ScissorEnable;
        TRUE,               // BOOL MultisampleEnable;
        FALSE,              // BOOL AntialiasedLineEnable;
    };
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterizerDescNoCull, &g_pRasterizerStateNoCull ) );
    DXUT_SetDebugName( g_pRasterizerStateNoCull, "NoCull" );

    RasterizerDescNoCull.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterizerDescNoCull, &g_pRasterizerStateNoCullWireFrame ) );
    DXUT_SetDebugName( g_pRasterizerStateNoCullWireFrame, "Wireframe" );

    D3D11_RASTERIZER_DESC RasterizerDescBackfaceCull = {
        D3D11_FILL_SOLID,   // D3D11_FILL_MODE FillMode;
        D3D11_CULL_BACK,    // D3D11_CULL_MODE CullMode;
        TRUE,               // BOOL FrontCounterClockwise;
        0,                  // INT DepthBias;
        0,                  // FLOAT DepthBiasClamp;
        0,                  // FLOAT SlopeScaledDepthBias;
        FALSE,              // BOOL DepthClipEnable;
        FALSE,              // BOOL ScissorEnable;
        TRUE,               // BOOL MultisampleEnable;
        FALSE,              // BOOL AntialiasedLineEnable;
    };
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterizerDescBackfaceCull, &g_pRasterizerStateBackfaceCull ) );
    DXUT_SetDebugName( g_pRasterizerStateBackfaceCull, "BackfaceCull" );

    D3D11_RASTERIZER_DESC RasterizerDescFrontfaceCull = {
        D3D11_FILL_SOLID,   // D3D11_FILL_MODE FillMode;
        D3D11_CULL_FRONT,   // D3D11_CULL_MODE CullMode;
        TRUE,               // BOOL FrontCounterClockwise;
        0,                  // INT DepthBias;
        0,                  // FLOAT DepthBiasClamp;
        0,                  // FLOAT SlopeScaledDepthBias;
        FALSE,              // BOOL DepthClipEnable;
        FALSE,              // BOOL ScissorEnable;
        TRUE,               // BOOL MultisampleEnable;
        FALSE,              // BOOL AntialiasedLineEnable;
    };
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterizerDescFrontfaceCull, &g_pRasterizerStateFrontfaceCull ) );
    DXUT_SetDebugName( g_pRasterizerStateFrontfaceCull, "FrontfaceCull" );

    // The parameters to pass to per-chunk threads for the main scene
    g_StaticParamsDirect.m_pDepthStencilState = g_pDepthStencilStateNoStencil;
    g_StaticParamsDirect.m_iStencilRef = 0;
    g_StaticParamsDirect.m_pRasterizerState = g_pRasterizerStateFrontfaceCull;
    g_StaticParamsDirect.m_vMirrorPlane = D3DXPLANE( 0.0f, 0.0f, 0.0f, 0.0f );
    g_StaticParamsDirect.m_vTintColor = D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 1.0f );
    g_StaticParamsDirect.m_pDepthStencilView = NULL;
    g_StaticParamsDirect.m_pViewport = NULL;

#ifdef DEBUG
    // These checks are important for avoiding implicit assumptions of D3D state carry-over 
    // across device contexts.  A very common source of error in multithreaded rendering  
    // is setting some state in one context and inadvertently relying on that state in 
    // another context.  Setting all these flags to true should expose all such errors
    // (at non-trivial performance cost).
    // 
    // The names mean a bit more than they say.  The flags force that state be cleared when:
    //
    //  1) We actually perform the action in question (e.g. call FinishCommandList)
    //  2) We reach any point in the frame when the action could have been
    // performed (e.g. we are using DEVICECONTEXT_IMMEDIATE but would otherwise 
    // have called FinishCommandList)
    //
    // This usage guarantees consistent behavior across the different pathways.
    //
    g_bClearStateUponBeginCommandList = true;
    g_bClearStateUponFinishCommandList = true;
    g_bClearStateUponExecuteCommandList = true;
#endif

    InitializeLights();

    V_RETURN( InitializeShadows( pd3dDevice ) );

    return S_OK;
}


HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    //V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    //V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
	V_RETURN( g_AsyncRender.OnResizedSwapChain() );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( g_fFOV, fAspectRatio, g_fNearPlane, g_fFarPlane );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Figure out the ViewProj matrix from the light's perspective
//--------------------------------------------------------------------------------------
void CalcLightViewProj( D3DXMATRIX* pmLightViewProj, int iLight )
{
    const D3DXVECTOR3& vLightDir = g_vLightDir[iLight];
    const D3DXVECTOR3& vLightPos = g_vLightPos[iLight];

    D3DXVECTOR3 vLookAt = vLightPos + g_fSceneRadius * vLightDir;

    D3DXMATRIX mLightView;
    D3DXMatrixLookAtLH( &mLightView, &vLightPos, &vLookAt, &g_vUp );

    D3DXMATRIX mLightProj;
    D3DXMatrixPerspectiveFovLH( &mLightProj, g_fLightFOV[iLight], g_fLightAspect[iLight], 
        g_fLightNearPlane[iLight], g_fLightFarPlane[iLight] );

    *pmLightViewProj = mLightView * mLightProj;
}

//--------------------------------------------------------------------------------------
// The RenderMesh version which always calls the regular DXUT pathway.
// Here we set up the per-object constant buffers.
//--------------------------------------------------------------------------------------
void RenderMeshDirect( ID3D11DeviceContext* pd3dContext, 
                      UINT iMesh )
{
    HRESULT hr = S_OK;
    D3D11_MAPPED_SUBRESOURCE MappedResource;

    // Set the VS per-object constant data
    // This should eventually differ per object
    D3DXMATRIX mWorld;
    D3DXMatrixIdentity(&mWorld);    // should actually vary per-object

    V( pd3dContext->Map( g_pcbVSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_VS_PER_OBJECT* pVSPerObject = ( CB_VS_PER_OBJECT* )MappedResource.pData;
    D3DXMatrixTranspose( &pVSPerObject->m_mWorld, &mWorld );
    pd3dContext->Unmap( g_pcbVSPerObject, 0 );

    pd3dContext->VSSetConstantBuffers( g_iCBVSPerObjectBind, 1, &g_pcbVSPerObject );

    // Set the PS per-object constant data
    // This should eventually differ per object
    V( pd3dContext->Map( g_pcbPSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_PS_PER_OBJECT* pPSPerObject = ( CB_PS_PER_OBJECT* )MappedResource.pData;
    pPSPerObject->m_vObjectColor = D3DXVECTOR4( 1, 1, 1, 1 );
    pd3dContext->Unmap( g_pcbPSPerObject, 0 );

    pd3dContext->PSSetConstantBuffers( g_iCBPSPerObjectBind, 1, &g_pcbPSPerObject );

    g_Mesh11.RenderMesh( iMesh,
        false,
        pd3dContext,
        0,
        1,
        INVALID_SAMPLER_SLOT );
}


//--------------------------------------------------------------------------------------
// The RenderMesh version which may redirect to another device context and/or thread.  
// This function gets called from the main thread or a per-scene thread, but not from 
// a per-chunk worker thread.
//
// There are three cases to consider:
//
//  1) If we are not using per-chunk deferred contexts, the call gets routed straight 
// back to DXUT with the given device context.
//  2) If we are using singlethreaded per-chunk deferred contexts, the call gets added
// to the next deferred context, and the draw submission occurs inline here.
//  3) If we are using multithreaded per-chunk deferred contexts, the call gets recorded
// in the next per-chunk work queue, and the corresponding semaphore gets incremented.  
// The appropriate worker thread detects the semaphore signal, grabs the work queue
// entry, and submits the draw call from its deferred context.
// 
// We ignore most of the arguments to this function, because they are constant for this
// sample.
//--------------------------------------------------------------------------------------
void RenderMesh( CMultiDeviceContextDXUTMesh* pMesh, 
                UINT iMesh,
                bool bAdjacent,
                ID3D11DeviceContext* pd3dDeviceContext,
                UINT iDiffuseSlot,
                UINT iNormalSlot,
                UINT iSpecularSlot )
{
        // Draw as normal
        RenderMeshDirect( pd3dDeviceContext, iMesh );
}


//--------------------------------------------------------------------------------------
// Perform per-scene d3d context set-up.  This should be enough setup that you can
// start with a completely new device context, and then successfully call RenderMesh
// afterwards.
//--------------------------------------------------------------------------------------
HRESULT RenderSceneSetup( ID3D11DeviceContext* pd3dContext, const SceneParamsStatic* pStaticParams, 
                         const SceneParamsDynamic* pDynamicParams )
{
    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE MappedResource;

    BOOL bShadow = ( pStaticParams->m_pDepthStencilView != NULL );

    // Use all shadow maps as textures, or else one shadow map as depth-stencil
    if ( bShadow )
    {
        // No shadow maps as textures
        ID3D11ShaderResourceView* ppNullResources[g_iNumShadows] = { NULL };
        pd3dContext->PSSetShaderResources( 2, g_iNumShadows, ppNullResources );

        // Given shadow map as depth-stencil, no render target
        pd3dContext->RSSetViewports( 1, pStaticParams->m_pViewport );
        pd3dContext->OMSetRenderTargets( 0, NULL, pStaticParams->m_pDepthStencilView );
    }
    else
    {
        // Standard DXUT render target and depth-stencil
        V( DXUTSetupD3D11Views( pd3dContext ) );

        // All shadow maps as textures
        pd3dContext->PSSetShaderResources( 2, g_iNumShadows, g_pShadowResourceView );
    }

    // Set the depth-stencil state
    pd3dContext->OMSetDepthStencilState( pStaticParams->m_pDepthStencilState, pStaticParams->m_iStencilRef );

    // Set the rasterizer state
    pd3dContext->RSSetState( g_bWireFrame ? g_pRasterizerStateNoCullWireFrame: pStaticParams->m_pRasterizerState );

    // Set the shaders
    pd3dContext->VSSetShader( g_pVertexShader, NULL, 0 );

    // Set the vertex buffer format
    pd3dContext->IASetInputLayout( g_pVertexLayout11 );
    
    // Set the VS per-scene constant data
    V( pd3dContext->Map( g_pcbVSPerScene, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_VS_PER_SCENE* pVSPerScene = ( CB_VS_PER_SCENE* )MappedResource.pData;
    D3DXMatrixTranspose( &pVSPerScene->m_mViewProj, &pDynamicParams->m_mViewProj );
    pd3dContext->Unmap( g_pcbVSPerScene, 0 );

    pd3dContext->VSSetConstantBuffers( g_iCBVSPerSceneBind, 1, &g_pcbVSPerScene );

    if ( bShadow )
    {
        pd3dContext->PSSetShader( NULL, NULL, 0 );
    }
    else
    {
        pd3dContext->PSSetShader( g_pPixelShader, NULL, 0 );

        ID3D11SamplerState* ppSamplerStates[2] = { g_pSamPointClamp, g_pSamLinearWrap };
        pd3dContext->PSSetSamplers( 0, 2, ppSamplerStates );

        // Set the PS per-scene constant data
        // A user clip plane prevents drawing things into the mirror which are behind the mirror plane
        V( pd3dContext->Map( g_pcbPSPerScene, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
        CB_PS_PER_SCENE* pPSPerScene = ( CB_PS_PER_SCENE* )MappedResource.pData;
        pPSPerScene->m_vMirrorPlane = pStaticParams->m_vMirrorPlane;
        pPSPerScene->m_vAmbientColor = g_vAmbientColor;
        pPSPerScene->m_vTintColor = pStaticParams->m_vTintColor;
        pd3dContext->Unmap( g_pcbPSPerScene, 0 );

        pd3dContext->PSSetConstantBuffers( g_iCBPSPerSceneBind, 1, &g_pcbPSPerScene );

        // Set the PS per-light constant data
        V( pd3dContext->Map( g_pcbPSPerLight, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
        CB_PS_PER_LIGHT* pPSPerLight = ( CB_PS_PER_LIGHT* )MappedResource.pData;
        for ( int iLight = 0; iLight < g_iNumLights; ++iLight )
        {
            D3DXVECTOR4 vLightPos = D3DXVECTOR4( g_vLightPos[iLight].x, g_vLightPos[iLight].y, g_vLightPos[iLight].z, 1.0f ); 
            D3DXVECTOR4 vLightDir = D3DXVECTOR4( g_vLightDir[iLight].x, g_vLightDir[iLight].y, g_vLightDir[iLight].z, 0.0f ); 
            D3DXMATRIX mLightViewProj;

            CalcLightViewProj( &mLightViewProj, iLight );
            
            pPSPerLight->m_LightData[iLight].m_vLightColor = g_vLightColor[iLight];
            pPSPerLight->m_LightData[iLight].m_vLightPos = vLightPos;
            pPSPerLight->m_LightData[iLight].m_vLightDir = vLightDir;
            D3DXMatrixTranspose( &pPSPerLight->m_LightData[iLight].m_mLightViewProj, 
                &mLightViewProj );
            pPSPerLight->m_LightData[iLight].m_vFalloffs = D3DXVECTOR4(
                g_fLightFalloffDistEnd[iLight], 
                g_fLightFalloffDistRange[iLight], 
                g_fLightFalloffCosAngleEnd[iLight], 
                g_fLightFalloffCosAngleRange[iLight]);
        }
        pd3dContext->Unmap( g_pcbPSPerLight, 0 );

        pd3dContext->PSSetConstantBuffers( g_iCBPSPerLightBind, 1, &g_pcbPSPerLight );
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// Render the scene from either:
//      - The immediate context in main thread, or 
//      - A deferred context in the main thread, or
//      - A deferred context in a worker thread
//      - Several deferred contexts in the main thread, handling objects alternately 
//      - Several deferred contexts in worker threads, handling objects alternately
// The scene can be either the main scene, a mirror scene, or a shadow map scene
//--------------------------------------------------------------------------------------
HRESULT RenderScene( ID3D11DeviceContext* pd3dContext, const SceneParamsStatic *pStaticParams, 
                    const SceneParamsDynamic *pDynamicParams )
{
    HRESULT hr = S_OK;

    // Make sure we're not relying on any state being inherited
    if ( g_bClearStateUponBeginCommandList )
    {
        pd3dContext->ClearState();
    }

    // Clear the shadow buffer
    if ( pStaticParams->m_pDepthStencilView != NULL )
    {
        pd3dContext->ClearDepthStencilView( pStaticParams->m_pDepthStencilView, 
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0 );
    }

    V( RenderSceneSetup( pd3dContext, pStaticParams, pDynamicParams ) );

    //Render
    g_Mesh11.Render( pd3dContext, 0, 1 );

    // If we rendered directly, optionally clear state for consistent behavior with
    // the other render pathways.
    if ( g_bClearStateUponFinishCommandList || g_bClearStateUponExecuteCommandList )
    {
        pd3dContext->ClearState();
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// Render the shadow map
//--------------------------------------------------------------------------------------
VOID RenderShadow( int iShadow, ID3D11DeviceContext* pd3dContext )
{
    HRESULT hr;

    SceneParamsDynamic DynamicParams;   
    CalcLightViewProj( &DynamicParams.m_mViewProj, iShadow );

    V( RenderScene( pd3dContext, &g_StaticParamsShadow[iShadow], &DynamicParams ) );
}


//--------------------------------------------------------------------------------------
// Render the scene into the world (not into a mirror or a shadow map)
//--------------------------------------------------------------------------------------
VOID RenderSceneDirect( ID3D11DeviceContext* pd3dContext )
{
    HRESULT hr;
    SceneParamsDynamic DynamicParams;
    DynamicParams.m_mViewProj = *g_Camera.GetViewMatrix() * *g_Camera.GetProjMatrix();

#ifdef RENDER_SCENE_LIGHT_POV
    if ( g_bRenderSceneLightPOV )
    {
        CalcLightViewProj( &DynamicParams.m_mViewProj, 0 );
    }
#endif

    V( RenderScene( pd3dContext, &g_StaticParamsDirect, &DynamicParams ) );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

	//Render the HUD
	//g_AsyncRender.OnFrameRender(fTime, fElapsedTime, pUserContext);

#ifdef ADJUSTABLE_LIGHT
    g_vLightDir[0] = g_LightControl.GetLightDirection();
    g_vLightPos[0] = g_vSceneCenter - g_fSceneRadius * g_vLightDir[0];
#endif

    //if ( g_bClearStateUponBeginCommandList )
    {
        pd3dImmediateContext->ClearState();
        V( DXUTSetupD3D11Views( pd3dImmediateContext ) );
    }

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target
    //float ClearColor[4] = { 0.0f, 0.25f, 0.25f, 0.55f };
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.55f };
    pd3dImmediateContext->ClearRenderTargetView( DXUTGetD3D11RenderTargetView(), ClearColor );
    pd3dImmediateContext->ClearDepthStencilView( DXUTGetD3D11DepthStencilView(), 
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0 );

    // Perform the same tasks, serialized on the main thread using the immediate context
    for ( int iShadow = 0; iShadow < g_iNumShadows; ++iShadow )
    {
        RenderShadow( iShadow, pd3dImmediateContext );
    }

    RenderSceneDirect( pd3dImmediateContext );

    // If we rendered directly, optionally clear state for consistent behavior with
    // the other render pathways.
    //if ( g_bClearStateUponFinishCommandList || g_bClearStateUponExecuteCommandList )
    //{
    //    pd3dImmediateContext->ClearState();
    //}

	//Composit UI to main scene
	CompositeUIIntoScene(pd3dDevice, pd3dImmediateContext);
	//g_AsyncRender.QueryResourceView(0);
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    //g_DialogResourceManager.OnD3D11ReleasingSwapChain();
	g_AsyncRender.OnReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	g_AsyncRender.OnDestroyDevice();
    g_Mesh11.Destroy();
                
    SAFE_RELEASE( g_pVertexLayout11 );
    SAFE_RELEASE( g_pVertexShader );
    SAFE_RELEASE( g_pPixelShader );
    SAFE_RELEASE( g_pSamPointClamp );
    SAFE_RELEASE( g_pSamLinearWrap );
    SAFE_RELEASE( g_pRasterizerStateNoCull );
    SAFE_RELEASE( g_pRasterizerStateBackfaceCull );
    SAFE_RELEASE( g_pRasterizerStateFrontfaceCull );
    SAFE_RELEASE( g_pRasterizerStateNoCullWireFrame );

    for ( int iShadow = 0; iShadow < g_iNumShadows; ++iShadow )
    {
        SAFE_RELEASE( g_pShadowTexture[iShadow] );
        SAFE_RELEASE( g_pShadowResourceView[iShadow] );
        SAFE_RELEASE( g_pShadowDepthStencilView[iShadow] );
    }

    SAFE_RELEASE( g_pDepthStencilStateNoStencil );
    SAFE_RELEASE( g_pcbVSPerScene );
    SAFE_RELEASE( g_pcbVSPerObject );
    SAFE_RELEASE( g_pcbPSPerScene );
    SAFE_RELEASE( g_pcbPSPerObject );
    SAFE_RELEASE( g_pcbPSPerLight );

    SAFE_RELEASE( g_pQuadVB );
    SAFE_RELEASE( g_pFullScreenQuadVS );
	SAFE_RELEASE( g_pFullScreenQuadPS );
    SAFE_RELEASE( g_pFullScreenQuadLayout );
    SAFE_RELEASE( g_UICompositeBlendState );
    SAFE_RELEASE( g_UICompositeDepthStencilState ); 

}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

#ifdef ADJUSTABLE_LIGHT
    g_LightControl.HandleMessages( hWnd, uMsg, wParam, lParam );
#endif

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK  OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1:
                g_bShowHelp = !g_bShowHelp; break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;
#ifdef RENDER_SCENE_LIGHT_POV
        case IDC_TOGGLELIGHTVIEW:
            g_bRenderSceneLightPOV = !g_bRenderSceneLightPOV; break;
#endif
        case IDC_TOGGLEWIRE:
            g_bWireFrame = !g_bWireFrame; break;
    }

}


//--------------------------------------------------------------------------------------
// CreateFullScreenQuad
//--------------------------------------------------------------------------------------
HRESULT CreateFullScreenQuad(ID3D11Device* pd3dDevice, int width, int height)
{
    HRESULT hr;

	ID3DBlob* pBlob = NULL;
	V_RETURN( D3DX11CompileFromFile( L"FullScreenQuad.hlsl", NULL, NULL, "VSMain", "vs_4_0", NULL, NULL, NULL, &pBlob, NULL, NULL) );
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(),
                                        NULL, &g_pFullScreenQuadVS ) );
	
	D3D11_INPUT_ELEMENT_DESC InputLayout[] =
    { { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
    };
    V_RETURN( pd3dDevice->CreateInputLayout( InputLayout, ARRAYSIZE( InputLayout ),
        pBlob->GetBufferPointer(), pBlob->GetBufferSize(),
        &g_pFullScreenQuadLayout ) );
	SAFE_RELEASE( pBlob );

	ID3DBlob* pPixelShaderBuffer = NULL;
    V_RETURN( CompileShaderFromFile( L"FullScreenQuad.hlsl", "PSMain", "ps_4_0", &pPixelShaderBuffer ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pPixelShaderBuffer->GetBufferPointer(),
                                             pPixelShaderBuffer->GetBufferSize(), NULL, &g_pFullScreenQuadPS ) );
	SAFE_RELEASE( pPixelShaderBuffer );

    float data[] = 
    {	-1.0f,  1.0f, 0.0f, 
		 1.0f,  1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
    };
	
    D3D11_BUFFER_DESC BufferDesc;
    ZeroMemory( &BufferDesc, sizeof( BufferDesc ) );
    BufferDesc.ByteWidth = sizeof(float) * 3 * 4;
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA subresourceData;
    subresourceData.pSysMem = data;
    subresourceData.SysMemPitch = 0;
    subresourceData.SysMemSlicePitch = 0;

    V_RETURN( pd3dDevice->CreateBuffer(&BufferDesc, &subresourceData, &g_pQuadVB) );

    // Create UI blend state
    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = /*D3D11_BLEND_DEST_ALPHA*/D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        pd3dDevice->CreateBlendState(&desc, &g_UICompositeBlendState);
    }

    // Create UI depth stencil desc
    {
        D3D11_DEPTH_STENCIL_DESC DSDesc;
        DSDesc.DepthEnable                  = FALSE;
        DSDesc.DepthFunc                    = D3D11_COMPARISON_LESS_EQUAL;
        DSDesc.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ZERO;
        DSDesc.StencilEnable                = TRUE;
        DSDesc.StencilReadMask              = 0xFF;
        DSDesc.StencilWriteMask             = 0x00;
        DSDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        DSDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        DSDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        DSDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_EQUAL;
        DSDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
        DSDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_KEEP;
        DSDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
        DSDesc.BackFace.StencilFunc         = D3D11_COMPARISON_EQUAL;
        V_RETURN( hr = pd3dDevice->CreateDepthStencilState(&DSDesc, &g_UICompositeDepthStencilState ) );
    }

	return S_OK;
}


//--------------------------------------------------------------------------------------
// DrawFullScreenQuad
//--------------------------------------------------------------------------------------
void DrawFullScreenQuad(ID3D11DeviceContext* pD3DImmediateContext)
{
	pD3DImmediateContext->GSSetShader(NULL, NULL, 0);
    pD3DImmediateContext->VSSetShader(g_pFullScreenQuadVS, NULL, 0);
    pD3DImmediateContext->IASetInputLayout( g_pFullScreenQuadLayout );
    ID3D11Buffer* pVBs[] = { g_pQuadVB };
    UINT strides[] = {sizeof(float) * 3};
    UINT offsets[] = {0};
    pD3DImmediateContext->IASetVertexBuffers(0, 1, pVBs, strides, offsets);
    pD3DImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pD3DImmediateContext->Draw(4, 0);
}


//--------------------------------------------------------------------------------------
// CompositeUIIntoScene
//--------------------------------------------------------------------------------------
void CompositeUIIntoScene( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
    HRESULT hr;

	pd3dImmediateContext->ClearState();
	V( DXUTSetupD3D11Views( pd3dImmediateContext ) );
    //ID3D11DepthStencilState * pBackupState = NULL; UINT backupStencilRef = 0;
    //pd3dImmediateContext->OMGetDepthStencilState( &pBackupState, &backupStencilRef );
    //pd3dImmediateContext->OMSetDepthStencilState( g_UICompositeDepthStencilState, 0x01 );
	// get the old render targets
	ID3D11RenderTargetView* pOldRTV;
	ID3D11DepthStencilView* pOldDSV;
	pd3dImmediateContext->OMGetRenderTargets( 1, &pOldRTV, &pOldDSV );
	pd3dImmediateContext->OMSetRenderTargets(1, &pOldRTV, NULL);

	pd3dImmediateContext->PSSetSamplers(0, 1, &g_pSamPointClamp);

	ID3D11ShaderResourceView* pSRVs[] = { g_AsyncRender.QueryResourceView(0)};
	pd3dImmediateContext->PSSetShaderResources( 3, 1, pSRVs);

	pd3dImmediateContext->PSSetShader(g_pFullScreenQuadPS, NULL, NULL);

	pd3dImmediateContext->OMSetBlendState(g_UICompositeBlendState, 0, 0xffffffff);

	DrawFullScreenQuad(pd3dImmediateContext);

    //pd3dImmediateContext->OMSetDepthStencilState( pBackupState, backupStencilRef );
    //SAFE_RELEASE( pBackupState );
	pd3dImmediateContext->OMSetRenderTargets(1, &pOldRTV, pOldDSV);
	SAFE_RELEASE(pOldRTV);
	SAFE_RELEASE(pOldDSV);
}