//--------------------------------------------------------------------------------------
// File: AsyncRender.h
//
// Extended implementation of DXUT Mesh for M/T rendering
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

typedef struct ASYNC_RESOURCE_DESC
    {
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
	CDXUTDialogResourceManager* DialogResourceManager;
	CD3DSettingsDlg* D3DSettingsDlg;
	CDXUTDialog* HUD;
	CDXUTDialog* SampleUI;
    } 	ASYNC_RESOURCE_DESC;


class AsyncRender
{
public:
    AsyncRender();
    ~AsyncRender();
	IDXGIAdapter*	QuerySecondaryAdapter(ID3D11Device* pPrimaryDevice, bool bWARP = FALSE);
	HRESULT			OnCreateDevice(ASYNC_RESOURCE_DESC* pDesc, IDXGIAdapter* pSecondaryAapter=NULL);
	HRESULT			OnResizedSwapChain();
	void			OnReleasingSwapChain();
	void			OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
	//void			OnFrameRender(double fTime, float fElapsedTime, void* pUserContext);
	void			OnDestroyDevice();
	ID3D11ShaderResourceView*	QueryResourceView(DWORD dwMilliseconds = 0);

protected:
	void			RenderText(bool bShowHelp);
	HRESULT			SetupViews( ID3D11DeviceContext* pd3dDeviceContext );
	void			InitializeWorkerThreads();
	void			FrameRendering();
	void			FrameStaging();
	static unsigned int WINAPI _RenderProc( LPVOID lpParameter );

private:
	bool						 m_bMultiDevices;
	CDXUTDialogResourceManager*  m_pDialogResourceManager; // manager for shared resources of dialogs
	CD3DSettingsDlg*             m_pD3DSettingsDlg;       // Device settings dialog
	CDXUTDialog*                 m_pHUD;                  // manages the 3D   
	CDXUTDialog*                 m_pSampleUI;             // dialog for sample specific controls
	CDXUTTextHelper* 			 m_pTxtHelper;

	bool						m_bShowHelp;
	double						m_fTime;
	float						m_fElapsedTime;

    UINT						m_Width;
    UINT						m_Height;
	DXGI_FORMAT					m_Format;

	// D3D Interfaces
	ID3D11Device*				m_pPrimaryDevice;
	ID3D11DeviceContext*		m_pPrimaryImmediateContext;
	ID3D11Texture2D*            m_pPrimaryUIBuffer;
	ID3D11ShaderResourceView*   m_pPrimaryUIBufferSRV;	

	ID3D11Device*				m_pSecondaryDevice;
	ID3D11DeviceContext*		m_pSecondaryImmediateContext;
	ID3D11Texture2D*            m_pSecondaryUIBuffer;
	ID3D11RenderTargetView*     m_pSecondaryUIBufferRTV;
	ID3D11Texture2D*            m_pSecondaryUIDepthStencilBuffer;
	ID3D11DepthStencilView*     m_pSecondaryUIDepthStencilView;
	ID3D11Texture2D*            m_pStagingBuffer;

	// Thread Interfaces
	HANDLE                      m_hRenderThread;
	HANDLE                      m_hFullEvent;
	HANDLE                      m_hEmptyEvent;

	BYTE*						m_pTempBuffer;
	UINT						m_DepthPitch;
	UINT						m_RowPitch;
};