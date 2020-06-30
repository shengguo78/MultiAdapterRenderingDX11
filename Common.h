#pragma once
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "resource.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include <process.h>
#include "MultiDeviceContextDXUTMesh.h"

// #defines for compile-time Debugging switches:
//#define ADJUSTABLE_LIGHT          // The 0th light is adjustable with the mouse (right mouse button down)
#define RENDER_SCENE_LIGHT_POV      // F4 toggles between the usual camera and the 0th light's point-of-view
//#define UNCOMPRESSED_VERTEX_DATA  // The sdkmesh file contained uncompressed vertex data

// By convention, the first n lights are shadow casting, and the rest just illuminate.
const int   g_iNumLights = 4;
const int   g_iNumShadows = 1;

//--------------------------------------------------------------------------------------
// Job queue structures
//--------------------------------------------------------------------------------------

// Everything necessary for scene setup which depends on
// which scene we're drawing (shadow/mirror/direct), but
// doesn't change per scene.
struct SceneParamsStatic
{
    ID3D11DepthStencilState*    m_pDepthStencilState;
    UINT8                       m_iStencilRef;

    ID3D11RasterizerState*      m_pRasterizerState;

    D3DXVECTOR4                 m_vTintColor;
    D3DXPLANE                   m_vMirrorPlane;

    // If m_pDepthStencilView is non-NULL then these 
    // are for a shadow map.  Otherwise, use the DXUT
    // defaults.
    ID3D11DepthStencilView*     m_pDepthStencilView;
    D3D11_VIEWPORT*             m_pViewport;
};

// Everything necessary for scene setup which depends on
// which scene we're drawing (shadow/mirror/direct), and
// also changes per scene.
struct SceneParamsDynamic
{
    D3DXMATRIX                  m_mViewProj;
};


//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
struct CB_VS_PER_OBJECT
{
    D3DXMATRIX m_mWorld;
};

struct CB_VS_PER_SCENE
{
    D3DXMATRIX m_mViewProj;
};


struct CB_PS_PER_OBJECT
{
    D3DXVECTOR4 m_vObjectColor;
};


struct CB_PS_PER_LIGHT
{
    struct LightDataStruct
    {
        D3DXMATRIX  m_mLightViewProj;
        D3DXVECTOR4 m_vLightPos;
        D3DXVECTOR4 m_vLightDir;
        D3DXVECTOR4 m_vLightColor;
        D3DXVECTOR4 m_vFalloffs;    // x = dist end, y = dist range, z = cos angle end, w = cos range
    } m_LightData[g_iNumLights];
};


struct CB_PS_PER_SCENE
{
    D3DXPLANE m_vMirrorPlane;
    D3DXVECTOR4 m_vAmbientColor;
    D3DXVECTOR4 m_vTintColor;
};

struct QuadVertex
{
    float pos[3];
    float UV[2];
};

HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );