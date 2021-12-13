﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"

namespace D3D9on12
{
    _Check_return_ HRESULT APIENTRY GetCaps(_In_ HANDLE hAdapter, _Inout_ CONST D3DDDIARG_GETCAPS* pGetCaps)
    {
        D3D9on12_DDI_ENTRYPOINT_START(TRUE);
        Adapter* pAdapter = Adapter::GetAdapterFromHandle(hAdapter);
        if (pAdapter == nullptr)
        {
            RETURN_E_INVALIDARG_AND_CHECK();
        }

        switch (pGetCaps->Type)
        {
        case D3DDDICAPS_GETFORMATCOUNT:
            *reinterpret_cast<UINT*>(pGetCaps->pData) = pAdapter->GetFormatData(NULL);
            break;
        case D3DDDICAPS_GETFORMATDATA:
            pAdapter->GetFormatData(reinterpret_cast<FORMATOP*>(pGetCaps->pData), pGetCaps->DataSize / sizeof(FORMATOP));
            break;
        case D3DDDICAPS_GETD3D9CAPS:
            GetD3D9Caps(reinterpret_cast<D3DCAPS9*>(pGetCaps->pData));
            break;
        case D3DDDICAPS_GETMULTISAMPLEQUALITYLEVELS:
        {
            DDIMULTISAMPLEQUALITYLEVELSDATA* const pMSQL = reinterpret_cast<DDIMULTISAMPLEQUALITYLEVELSDATA*>(pGetCaps->pData);
            switch (pMSQL->MsType)
            {
            case D3DDDIMULTISAMPLE_NONE:
                pMSQL->QualityLevels = 0;
                break;
            case D3DDDIMULTISAMPLE_NONMASKABLE:
                pMSQL->QualityLevels = 3;
                break;
            default:
                D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS MSQL =
                {
                    ConvertFromDDIToDXGIFORMAT(pMSQL->Format),
                    (UINT)pMSQL->MsType,
                    D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
                    1 // Default num quality levels of 1
                };
                (void)pAdapter->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &MSQL, sizeof(MSQL));
                pMSQL->QualityLevels = MSQL.NumQualityLevels;
                break;
            }
            break;
        }
        case D3DDDICAPS_GETD3DQUERYCOUNT:
            *reinterpret_cast<UINT*>(pGetCaps->pData) = GetD3DQueryTypes(NULL, 0);
            break;
        case D3DDDICAPS_GETD3DQUERYDATA:
            GetD3DQueryTypes(reinterpret_cast<D3DQUERYTYPE*>(pGetCaps->pData), pGetCaps->DataSize / sizeof(D3DDDIQUERYTYPE));
            break;

        case D3DDDICAPS_GETDECODEGUIDCOUNT:
        case D3DDDICAPS_GETDECODEGUIDS:
        case D3DDDICAPS_GETDECODERTFORMATCOUNT:
        case D3DDDICAPS_GETDECODERTFORMATS:
        case D3DDDICAPS_GETDECODECOMPRESSEDBUFFERINFOCOUNT:
        case D3DDDICAPS_GETDECODECOMPRESSEDBUFFERINFO:
        case D3DDDICAPS_GETDECODECONFIGURATIONCOUNT:
        case D3DDDICAPS_GETDECODECONFIGURATIONS:
        case D3DDDICAPS_GETVIDEOPROCESSORRTFORMATCOUNT:
        case D3DDDICAPS_GETVIDEOPROCESSORDEVICEGUIDCOUNT:
        case D3DDDICAPS_GETEXTENSIONGUIDCOUNT:
        case D3DDDICAPS_GETVIDEOPROCESSORDEVICEGUIDS:
        case D3DDDICAPS_GETVIDEOPROCESSORRTFORMATS:
        case D3DDDICAPS_GETVIDEOPROCESSORRTSUBSTREAMFORMATCOUNT:
        case D3DDDICAPS_GETVIDEOPROCESSORRTSUBSTREAMFORMATS:
        case D3DDDICAPS_DXVAHD_GETVPOUTPUTFORMATS:
        case D3DDDICAPS_DXVAHD_GETVPINPUTFORMATS:
        case D3DDDICAPS_GETVIDEOPROCESSORCAPS:
        case D3DDDICAPS_GETPROCAMPRANGE:
        case D3DDDICAPS_FILTERPROPERTYRANGE:
        case D3DDDICAPS_GETEXTENSIONGUIDS:
        case D3DDDICAPS_GETEXTENSIONCAPS:
        case D3DDDICAPS_DXVAHD_GETVPDEVCAPS:
        case D3DDDICAPS_DXVAHD_GETVPCAPS:
        case D3DDDICAPS_DXVAHD_GETVPCUSTOMRATES:
        case D3DDDICAPS_DXVAHD_GETVPFILTERRANGE:
            pAdapter->GetVideoCaps(pGetCaps);
            break;

        case D3DDDICAPS_GETGAMMARAMPCAPS:
            reinterpret_cast<DDIGAMMACAPS*>(pGetCaps->pData)->GammaCaps = GAMMA_CAP_RGB256x3x16;
            break;
        case D3DDDICAPS_GET_ARCHITECTURE_INFO:
            reinterpret_cast<D3DDDICAPS_ARCHITECTURE_INFO*>(pGetCaps->pData)->TileBasedDeferredRenderer = FALSE;
            break;
        case D3DDDICAPS_GETCONTENTPROTECTIONCAPS:
            reinterpret_cast<D3DCONTENTPROTECTIONCAPS*>(pGetCaps->pData)->Caps = 0;
            break;
        case D3DDDICAPS_DDRAW:
            GetDDrawCaps(reinterpret_cast<DDRAW_CAPS*>(pGetCaps->pData));
            break;
        case D3DDDICAPS_DDRAW_MODE_SPECIFIC:
            ZeroMemory(pGetCaps->pData, sizeof(DDRAW_MODE_SPECIFIC_CAPS));
            break;
        case D3DDDICAPS_GETD3D7CAPS:
            GetD3D7Caps(reinterpret_cast<D3DHAL_D3DEXTENDEDCAPS*>(pGetCaps->pData));
            break;
        case D3DDDICAPS_GETD3D3CAPS:
            GetD3D3Caps(reinterpret_cast<D3DHAL_GLOBALDRIVERDATA*>(pGetCaps->pData));
            break;
        case D3DDDICAPS_GETD3D8CAPS:
            GetD3D8Caps(reinterpret_cast<D3DCAPS8*>(pGetCaps->pData));
            break;
        default:
            DebugBreak();
            return E_FAIL;
        }
        D3D9on12_DDI_ENTRYPOINT_END_AND_RETURN_HR(S_OK);
    }

    _Check_return_ HRESULT APIENTRY CreateDevice(_In_ HANDLE hAdapter, _Inout_ D3DDDIARG_CREATEDEVICE* pCreateDevice)
    {
        D3D9on12_DDI_ENTRYPOINT_START(TRUE);

        HRESULT hr = S_OK;
        Adapter* pNewAdapter = Adapter::GetAdapterFromHandle(hAdapter);

        if (pCreateDevice->pCallbacks == nullptr || pNewAdapter == nullptr)
        {
            RETURN_E_INVALIDARG_AND_CHECK()
        }

        //Device* pNewDevice = new Device(*pNewAdapter, *pCreateDevice );
        DeviceFactory<Device> deviceFactory;
        Device* pNewDevice = deviceFactory.CreateDevice(*pNewAdapter, *pCreateDevice);
        if (pNewDevice == nullptr)
        {
            hr = E_OUTOFMEMORY;
        }

        D3D9on12_DDI_ENTRYPOINT_END_AND_RETURN_HR(hr);
    }

    _Check_return_ HRESULT APIENTRY CloseAdapter(_In_ HANDLE hAdapter)
    {
        D3D9on12_DDI_ENTRYPOINT_START(TRUE);
        Adapter* pAdapter = Adapter::GetAdapterFromHandle(hAdapter);
        if (pAdapter == nullptr)
        {
            RETURN_E_INVALIDARG_AND_CHECK();
        }

        HRESULT hr = pAdapter->Destroy();
        CHECK_HR(hr);

        SAFE_DELETE(pAdapter);

        D3D9on12_DDI_ENTRYPOINT_END_AND_RETURN_HR(hr);
    }

    _Check_return_ HRESULT APIENTRY OpenAdapter(_Inout_ D3DDDIARG_OPENADAPTER*)
    {
        D3D9on12_DDI_ENTRYPOINT_START(TRUE);
        // This should never be called, 
        D3D9on12_DDI_ENTRYPOINT_END_AND_RETURN_HR(E_NOTIMPL);
    }

    _Check_return_ HRESULT APIENTRY OpenAdapter_Private(_Inout_ D3DDDIARG_OPENADAPTER* pOpenAdapter, _In_ LUID *pLUID, _In_opt_ D3D9ON12_CREATE_DEVICE_ARGS *pArgs)
    {
        D3D9on12_DDI_ENTRYPOINT_START(TRUE);
        if (pOpenAdapter == nullptr)
        {
            RETURN_E_INVALIDARG_AND_CHECK()
        }

        if (RegistryConstants::g_cBreakOnLoad)
        {
            DebugBreak();
        }

        AdapterFactory<Adapter> adapterFactory;
        Adapter* pNewAdapter = adapterFactory.CreateAdapter(*pOpenAdapter, pLUID, pArgs);
        if (pNewAdapter == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        D3D9on12_DDI_ENTRYPOINT_END_AND_RETURN_HR(S_OK);
    }

    Adapter::Adapter( _Inout_ D3DDDIARG_OPENADAPTER& OpenAdapter ) :
        m_AdapterCallbacks( *OpenAdapter.pAdapterCallbacks ),
        m_pDevice( nullptr )
    {
    }

    Adapter::~Adapter()
    {

    }

    HRESULT Adapter::Destroy()
    {
        return S_OK;
    }

    void Adapter::InitDebugLayer()
    {
        Check9on12(GetDevice());

        CComPtr<ID3D12Debug> pDebug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
        {
            pDebug->EnableDebugLayer();
        }

        {
            CComPtr<ID3D12InfoQueue> pIQ;
            if (SUCCEEDED(GetDevice()->QueryInterface(&pIQ)))
            {
                ThrowFailure(pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true));
                ThrowFailure(pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true));
                ThrowFailure(pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true));
                ThrowFailure(pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, true));
                ThrowFailure(pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE, true));

                pIQ->ClearStoredMessages();

                D3D12_MESSAGE_ID ignore[] = {
                    D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_DEPTHSTENCILVIEW_NOT_SET,
                    D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET,
                    D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE, D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_REFLECTSHAREDPROPERTIES_INVALIDOBJECT,
                    D3D12_MESSAGE_ID_DRAW_EMPTY_SCISSOR_RECTANGLE
                };

                D3D12_INFO_QUEUE_FILTER filter = {};

                D3D12_MESSAGE_SEVERITY Severity = D3D12_MESSAGE_SEVERITY_INFO;
                filter.DenyList.NumSeverities = 1;
                filter.DenyList.pSeverityList = &Severity;
                filter.DenyList.NumIDs = _countof(ignore);
                filter.DenyList.pIDList = ignore;

                ThrowFailure(pIQ->PushStorageFilter(&filter));
            }
        }
    }

    _Use_decl_annotations_
    void Adapter::GetVideoCaps(CONST D3DDDIARG_GETCAPS* pGetCaps)
    {
        assert(m_pDevice);
        m_pDevice->EnsureVideoDevice();
        VideoDevice *pVideoDevice = m_pDevice->GetVideoDevice();
        pVideoDevice->GetCaps(pGetCaps);
    }

#define DDI_MULTISAMPLE_TYPE(x) (1 << ((x) - 1))  

    const int __DDI_MULTISAMPLE = DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_NONMASKABLE) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_2_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_8_SAMPLES);
    const int __D3DFMTOP_TEXTURE =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VERTEXTEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_DMAP;
    const int __D3DFMTOP_STRECTTOFROM = D3DFORMAT_OP_CONVERT_TO_ARGB | D3DFORMAT_MEMBEROFGROUP_ARGB;
    const int __D3DFMTOP_TEXTURE2D =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VERTEXTEXTURE |
        D3DFORMAT_OP_DMAP;
    const int __D3DFMTOP_RENDERTARGET =
        D3DFORMAT_OP_OFFSCREENPLAIN |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    const int  __D3DFMTOP_DEPTHSTENCIL =
        D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH |
        D3DFORMAT_OP_ZSTENCIL;
    const int __D3DFMTOP_DISPLAY =
        D3DFORMAT_OP_DISPLAYMODE |
        D3DFORMAT_OP_3DACCELERATION;

    static DWORD GetSRVSupportFlags(D3D12_FEATURE_DATA_FORMAT_SUPPORT &formatSupport)
    {
        DWORD formatOperations = 0;
        if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D)
        {
            formatOperations |= __D3DFMTOP_TEXTURE2D;
        }

        if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE3D)
        {
            formatOperations |= D3DFORMAT_OP_VOLUMETEXTURE;
        }

        if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURECUBE)
        {
            formatOperations |= D3DFORMAT_OP_CUBETEXTURE;
        }
        return formatOperations;
    }

    UINT Adapter::GetFormatData(FORMATOP* pFormatOPs, UINT uiNumFormats)
    {
        typedef struct
        {
            D3DFORMAT dwFourCC;
            DWORD     dwExplicitOperations;
            WORD      wBltMSTypes;
            WORD      wPrivateFormatBitCount;
        } t_format_desc;
        static const t_format_desc s_surfFormats[] =
        {
            // Display Formats  
            { D3DFMT_X8R8G8B8, __D3DFMTOP_DISPLAY | D3DFORMAT_OP_SRGBREAD | D3DFORMAT_OP_SRGBWRITE | __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_R5G6B5, __D3DFMTOP_DISPLAY | __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            // Render Target __D3DFMTOP_STRECTTOFROMFormats  
            { D3DFMT_A8R8G8B8, D3DFORMAT_OP_SRGBREAD | D3DFORMAT_OP_SRGBWRITE | __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_A8B8G8R8, D3DFORMAT_OP_SRGBREAD | D3DFORMAT_OP_SRGBWRITE | __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_A1R5G5B5, __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_X1R5G5B5, __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_A4R4G4B4, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_X4R4G4B4, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_A16B16G16R16, __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_G16R16, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_A2B10G10R10,   __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_A2R10G10B10,   __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_A16B16G16R16F, __D3DFMTOP_STRECTTOFROM, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_R16F, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_G16R16F, D3DFORMAT_OP_NOALPHABLEND, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_R32F, D3DFORMAT_OP_NOALPHABLEND, 0, 0 },
            { D3DFMT_G32R32F, D3DFORMAT_OP_NOALPHABLEND, 0, 0 },
            { D3DFMT_A32B32G32R32F, D3DFORMAT_OP_NOALPHABLEND, 0, 0 },
            { D3DFMT_V8U8, D3DFORMAT_OP_BUMPMAP, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_Q8W8V8U8, D3DFORMAT_OP_BUMPMAP, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_V16U16, D3DFORMAT_OP_BUMPMAP, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_Q16W16V16U16,  D3DFORMAT_OP_BUMPMAP, __DDI_MULTISAMPLE, 0 },
            // No equivalent in 11 DDI (only WARP can do)  
            // Possible to emulate 9_x behavior using R8/R16 format and distribute red channel to blue/green.  
            { D3DFMT_L8, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_L16, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_A8, 0, __DDI_MULTISAMPLE, 0 },
            // Not required for 9_x feature levels.  
            { D3DFMT_A8L8, 0, __DDI_MULTISAMPLE, 0 },
            // Texture Formats                          
            { D3DFMT_DXT1, D3DFORMAT_OP_SRGBREAD, 0, 0 },
            { D3DFMT_DXT2, D3DFORMAT_OP_SRGBREAD, 0, 0 },
            { D3DFMT_DXT3, D3DFORMAT_OP_SRGBREAD, 0, 0 },
            { D3DFMT_DXT4, D3DFORMAT_OP_SRGBREAD, 0, 0 },
            { D3DFMT_DXT5, D3DFORMAT_OP_SRGBREAD, 0, 0 },

            // IHVs didn't report the "true" BPP for IHV block compressed formats and instead
            // seemed to always report 8. Reporting less than 8BPP (which is true for ATI1) causes issues 
            // in the DX9 runtime which assumes that the BPP is always at least 8 BPP. The root of this issue
            // comes from the fact that the DX9 runtime doesn't account for FOURCC formats that are compressed
            // and IHVs chose to establish a pattern of working around this quirk in the runtime. As a result, 
            // this causes the runtime to report incorrect pitch values to both the driver and the app at 
            // Lock()/LockRect. Drivers work around this by understanding that the pitch won't account for the 
            // fact that BPP is based on 4x4 compressed blocks and multiply the pitch accordingly. 
            //
            // Presumably ISVs must have been informed of this as well and are aware that the pitch they get from 
            // the runtime must be multiplied or ignored and calculated by the app.
            { D3DFMT_ATI1, D3DFORMAT_OP_SRGBREAD | D3DFORMAT_OP_PIXELSIZE, 0, BPP_FOR_IHV_BLOCK_COMPRESSED_FORMATS }, // A.K.A BC4
            { D3DFMT_ATI2, D3DFORMAT_OP_SRGBREAD | D3DFORMAT_OP_PIXELSIZE, 0, BPP_FOR_IHV_BLOCK_COMPRESSED_FORMATS }, // A.K.A BC5

                                                                                                                      // Not required for 9_x feature levels.  
                                                                                                                      // { D3DFMT_P8,            __D3DFMTOP_TEXTURE | D3DFORMAT_OP_OFFSCREENPLAIN, 0, 0 },  
                                                                                                                      // { D3DFMT_A8P8,          __D3DFMTOP_TEXTURE, 0, 0 },  
                                                                                                                      // DepthStencil Formats  
            { D3DFMT_D24S8, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_D24FS8, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_D24X8, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_D16,   0, __DDI_MULTISAMPLE, 0 },
            // Not required for 9_x feature levels.  
            // { D3DFMT_D16_LOCKABLE,  __D3DFMTOP_DEPTHSTENCIL, 0, 0 },  
            // { D3DFMT_D32F_LOCKABLE, __D3DFMTOP_DEPTHSTENCIL, 0, 0 },  
            // YUV Formats  
            { D3DFMT_AYUV, D3DFORMAT_OP_OFFSCREENPLAIN | D3DFORMAT_OP_PIXELSIZE | D3DFORMAT_OP_CONVERT_TO_ARGB, 0, 32 },
            { D3DFMT_YUY2, D3DFORMAT_OP_OFFSCREENPLAIN | D3DFORMAT_OP_PIXELSIZE | D3DFORMAT_OP_CONVERT_TO_ARGB, 0, 16 },
            { D3DFMT_NV12, D3DFORMAT_OP_OFFSCREENPLAIN | D3DFORMAT_OP_PIXELSIZE | D3DFORMAT_OP_CONVERT_TO_ARGB, 0, 16 },
            { D3DFMT_YV12, D3DFORMAT_OP_OFFSCREENPLAIN | D3DFORMAT_OP_PIXELSIZE | D3DFORMAT_OP_CONVERT_TO_ARGB, 0, 16 }, // Via emulation as NV12
            { D3DFMT_P010, D3DFORMAT_OP_OFFSCREENPLAIN | D3DFORMAT_OP_PIXELSIZE | D3DFORMAT_OP_CONVERT_TO_ARGB, 0, 32 },
            //FourCC formats
            { D3DFMT_INTZ, 0, __DDI_MULTISAMPLE, 0 },
            // RAWZ requires special handling that we don't really understand. Cutting until we need it, then we can understand it.
            //{ D3DFMT_RAWZ, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_DF16, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_DF24, 0, __DDI_MULTISAMPLE, 0 },
            { D3DFMT_NULL, __D3DFMTOP_TEXTURE | __D3DFMTOP_RENDERTARGET | D3DFORMAT_OP_BUMPMAP, __DDI_MULTISAMPLE, 0 },
            // RESZ is not a format that you can actually create a resource with, but reporting it lets apps know they can use the feature
            { D3DFMT_RESZ, 0, 0, 0 },
            // ATOC is not a format that you can actually create a resource with, but reporting it lets apps know they can use the feature
            { D3DFMT_ATOC, D3DFORMAT_OP_OFFSCREENPLAIN, 0, 0 },
        };

        static_assert(_countof(s_surfFormats) == MAX_SURFACE_FORMATS, "Size constant must be kept in sync for internal query of GetFormatData.");


        UINT numSupportedFormats = 0;
        for (auto &formatEntry : s_surfFormats)
        {
            bool bFormatUsesSpecialHandling = (formatEntry.dwFourCC == D3DFMT_NULL ||
                formatEntry.dwFourCC == D3DFMT_RESZ || formatEntry.dwFourCC == D3DFMT_ATOC);

            bool bD3D12SupportsFormat = false;
            D3D12_FEATURE_DATA_FORMAT_SUPPORT dxgiFormatSupport;
            DWORD formatOperations = formatEntry.dwExplicitOperations;
            if (!bFormatUsesSpecialHandling)
            {
                dxgiFormatSupport.Format = ConvertToDXGIFORMAT(formatEntry.dwFourCC);
                bD3D12SupportsFormat = SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &dxgiFormatSupport, sizeof(dxgiFormatSupport)));

                if (bD3D12SupportsFormat)
                {
                    if (formatOperations & __D3DFMTOP_DISPLAY) Check9on12(dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET);

                    // Avoid reporting RTV support for bump maps since the ref driver doesn't support this, otherwise this lights up
                    // unexpected behavior in tests when the cap is lit due to ref driver comparison. IHVs appear to follow this pattern as well
                    const bool bIsBumpFormat = (formatEntry.dwExplicitOperations & D3DFORMAT_OP_BUMPMAP) != 0;
                    if (!CD3D11FormatHelper::YUV(dxgiFormatSupport.Format)  && dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET && !bIsBumpFormat)
                    {
                        formatOperations |= __D3DFMTOP_RENDERTARGET;
                    }

                    if (dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL)
                    {
                        formatOperations |= __D3DFMTOP_DEPTHSTENCIL;
                    }

                    if (IsDepthStencilFormat(dxgiFormatSupport.Format))
                    {
                        D3D12_FEATURE_DATA_FORMAT_SUPPORT srvFormatSupport;
                        srvFormatSupport.Format = ConvertDepthFormatToCompanionSRVFormat(dxgiFormatSupport.Format);
                        ThrowFailure(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &srvFormatSupport, sizeof(srvFormatSupport)));

                        formatOperations |= GetSRVSupportFlags(srvFormatSupport);
                    }
                    // Not currently supporting SRV operations on YUV surfaces
                    else if (!CD3D11FormatHelper::YUV(dxgiFormatSupport.Format) || cSupportsPlanarSRVs)
                    {
                        formatOperations |= GetSRVSupportFlags(dxgiFormatSupport);
                    }

                    if ((formatOperations & __D3DFMTOP_RENDERTARGET) == __D3DFMTOP_RENDERTARGET &&
                       (dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET) != 0 &&
                       (dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE) != 0)
                    {
                        formatOperations |= __DDI_MULTISAMPLE;
                    }

                    // Gen mips operations use a draw that requires the source to be an SRV and the dest to be an RTV
                    if ((dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0 &&
                        (dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0 &&
                        (dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_MIP) &&
                        formatEntry.dwFourCC != D3DFMT_A2R10G10B10)
                    {
                        formatOperations |= D3DFORMAT_OP_AUTOGENMIPMAP;
                    }
                }
            }

            if (bFormatUsesSpecialHandling || bD3D12SupportsFormat)
            {
                if (pFormatOPs)
                {
                    pFormatOPs[numSupportedFormats].Format = (D3DDDIFORMAT)formatEntry.dwFourCC;
                    pFormatOPs[numSupportedFormats].Operations = formatOperations;
                    pFormatOPs[numSupportedFormats].FlipMsTypes = formatEntry.wBltMSTypes;
                    pFormatOPs[numSupportedFormats].BltMsTypes = formatEntry.wBltMSTypes;
                    pFormatOPs[numSupportedFormats].PrivateFormatBitCount = formatEntry.wPrivateFormatBitCount;

                    if (numSupportedFormats == uiNumFormats) break;
                }

                numSupportedFormats++;
            }
        }

        return numSupportedFormats;
    }

    bool Adapter::RequiresYUY2BlitWorkaround() const
    {
        return m_HWIDs.vendorID == MAKEFOURCC('Q', 'C', 'O', 'M');
    }

    void Adapter::LogAdapterCreated( LUID *pluid, HRESULT hr )
    {
        //do nothing
        UNREFERENCED_PARAMETER( pluid );
        UNREFERENCED_PARAMETER( hr );
    }
};

