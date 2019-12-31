//===================================================================//
//   Beat Tracking Alogrithm                                         //
//   Implemented on Software for Microsoft® Windows®                 //
//                                                                   //
//   Team of Sharif University of Technology - IRAN                  //
//                                                                   //
//   IEEE Signal Processing Cup 2017 - Beat Tracking Challenge       //
//                                                                   //
//   Please read the associated documentation on the procedure of    //
//   building the source code into executable app package and also   //
//   the installation.                                               //
//===================================================================//

#include "pch.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "Helper.h"

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Platform;

namespace DX
{
    //Load a bitmap for D2D using WIC
    ID2D1Bitmap* LoadD2DBitmap(LPCWSTR FileName, ID2D1RenderTarget *pRenderTarget,
        IWICImagingFactory *pIWICFactory)
    {
        IWICBitmapDecoder *pDecoder = NULL;
        IWICBitmapFrameDecode *pSource = NULL;
        IWICStream *pStream = NULL;
        IWICFormatConverter *pConverter = NULL;
        IWICBitmapScaler *pScaler = NULL;
        ID2D1Bitmap* pBitmap = nullptr;

        HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
            FileName,
            NULL,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
        );

        if (SUCCEEDED(hr))
        {
            // Create the initial frame.
            hr = pDecoder->GetFrame(0, &pSource);
        }

        if (SUCCEEDED(hr))
        {

            // Convert the image format to 32bppPBGRA
            // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
            hr = pIWICFactory->CreateFormatConverter(&pConverter);

        }


        if (SUCCEEDED(hr))
        {
            hr = pConverter->Initialize(
                pSource,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeMedianCut
            );
        }

        if (SUCCEEDED(hr))
        {

            // Create a Direct2D bitmap from the WIC bitmap.
            hr = pRenderTarget->CreateBitmapFromWicBitmap(
                pConverter,
                NULL,
                &pBitmap
            );
        }

        Safe_Release(pDecoder);
        Safe_Release(pSource);
        Safe_Release(pStream);
        Safe_Release(pConverter);
        Safe_Release(pScaler);

        return pBitmap;
    }

    //Calculate RGB from HSL (Hue, Sat, Lum)
    #define HUE(x) ( ((float)x) / 6.f )
    XMFLOAT4* HSLToRGB(XMFLOAT4* Dest, float H, float S, float L)
    {
        if (!Dest) return nullptr;
        if (H < 0 || H > 1) H = abs(fmodf(H, 1.0f));
        if (S < 0 || S > 1) S = abs(fmodf(S, 1.0f));
        if (L < 0 || L > 1) L = abs(fmodf(L, 1.0f));

        XMFLOAT4 Out;
        Out.w = 1;
        Out.x = Out.y = Out.z = 0;
        float HFac = fmodf(H, HUE(1)) / HUE(1);

        //Adjust Hue
        if (H <= HUE(1))
        {
            Out.x = 1; Out.y = HFac;
        }
        else if (H <= HUE(2))
        {
            Out.x = 1 - HFac; Out.y = 1;
        }
        else if (H <= HUE(3))
        {
            Out.y = 1; Out.z = HFac;
        }
        else if (H <= HUE(4))
        {
            Out.y = 1 - HFac; Out.z = 1;
        }
        else if (H <= HUE(5))
        {
            Out.z = 1; Out.x = HFac;
        }
        else if (H <= HUE(6))
        {
            Out.z = 1 - HFac; Out.x = 1;
        }

        //Adjust Saturation
        XMVECTOR vColor = XMLoadFloat4(&Out);
        XMColorAdjustSaturation(vColor, S);

        //Adjust Luminance
        const XMVECTOR vBlack = XMVectorSet(0, 0, 0, 1);
        const XMVECTOR vWhite = XMVectorSet(1, 1, 1, 1);
        if (L > 0.5f) vColor = XMVectorLerp(vColor, vWhite, (L - 0.5f) * 2);
        else vColor = XMVectorLerp(vBlack, vColor, 2 * L);

        //Done
        XMStoreFloat4(&Out, vColor);
        *Dest = Out;
        return Dest;
    }
}
