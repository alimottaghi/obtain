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

//===================================================================//
// This part contains the code to run the frontend (GUI).
// The graphical user interface is mainly based on Direct2D/DXGI
//   graphics subsystem and Universal Windows Platform (UWP) framework.
// The app is intended to be run on Windows 8.1 or 10 or later.
//===================================================================//

#include "pch.h"
#include "Beat_Tracking_GUI.h"
#include "DirectXHelper.h"
#include "AlgorithmInterface.h"
#include "Graphs.h"
#include "Helper.h"

using namespace Beat_Tracking_App;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage;

#pragma comment(lib, "dxguid.lib")

//Define the global instance of Beat Tracking GUI
Beat_Tracking_GUI* g_BeatTrackingAppInstance = 0;

//Uncomment the following line to reduce graphics quality
//~~#define SIMPLE_GRAPHICS

//Commands: These commands are issued via GUI Buttons or the keyboard
#define CMD_OPEN        1
#define CMD_MICROPHONE  2
#define CMD_OPENANDLOG  3
#define CMD_FASTFWD     4
#define CMD_VOLUME_DOWN 5
#define CMD_VOLUME_UP   6
#define CMD_END         7

//======================================================================================//
// The Beat Tracking Demo Graphical User Interface (Wow!)
//======================================================================================//

// Loads and initializes application assets when the application is loaded.
Beat_Tracking_GUI::Beat_Tracking_GUI(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
    g_BeatTrackingAppInstance = this;

	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

    // Create device independent resources
    //Create text format
    ComPtr<IDWriteTextFormat> textFormat;
    DX::ThrowIfFailed(
        m_deviceResources->GetDWriteFactory()->CreateTextFormat(
            L"Consolas",
            nullptr,
            DWRITE_FONT_WEIGHT_LIGHT,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            18.0f,
            L"en-US",
            &textFormat
        )
    );

    DX::ThrowIfFailed(
        textFormat.As(&m_textFormat)
    );

    //Create images
    for (auto& i : m_Images)
        i = nullptr;

    static const LPCWSTR ImgFiles[] =
    {
        L"Assets/vBack.png",
        L"Assets/vCymbalLower.png",
        L"Assets/vCymbalUpper.png",
#ifndef SIMPLE_GRAPHICS
        L"Assets/vFore.png",
#else
        L"Assets/vAllLights.png",
#endif
        L"Assets/vL1.png",
        L"Assets/vL2.png",
        L"Assets/vL3.png",
        L"Assets/vStick.png",
        L"Assets/vPlay.png",
        L"Assets/vEnd.png",
        L"Assets/vMike.png",
    };

    for (int i = 0; i < (int)BeatTrackerImages::COUNT; i++)
    {
        m_Images[i] = DX::LoadD2DBitmap(
            ImgFiles[i],
            m_deviceResources->GetD2DDeviceContext(),
            m_deviceResources->GetWicImagingFactory());
    }

    //Create Color Matrix Effect
    m_deviceResources->GetD2DDeviceContext()->CreateEffect(CLSID_D2D1ColorMatrix, &m_colorMatrix);

    //Init Graphs
    BT_GlobalGraph_Setup();

    //Create buttons
    CGuiButton button;
    button.CallbackToClick = _OnButton; button.CallbackUserData = this;
    
    button.Text = L"Open Music\n(O)"; button.ID = CMD_OPEN;
    m_Buttons.push_back(button);

    button.Text = L"Use Microphone\n(M)"; button.ID = CMD_MICROPHONE;
    m_Buttons.push_back(button);

    button.Text = L"Volume +\n(2)"; button.ID = CMD_VOLUME_UP;
    m_Buttons.push_back(button);

    button.Text = L"Volume -\n(1)"; button.ID = CMD_VOLUME_DOWN;
    m_Buttons.push_back(button);

    button.Text = L"Fast Fwd\n(F6)"; button.ID = CMD_FASTFWD;
    m_Buttons.push_back(button);

    button.Text = L"Open Music\nAnd Log (L)"; button.ID = CMD_OPENANDLOG;
    m_Buttons.push_back(button);

    button.Text = L"Stop (ESC)"; button.ID = CMD_END;
    m_Buttons.push_back(button);

    //Create device dependent objects and automatically layout buttons
    OnDeviceRestored();

    //Do not record already
    _ProcessCommand(CMD_END);
}

Beat_Tracking_GUI::~Beat_Tracking_GUI()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Notifies renderers that device dependent resources need to be released.
void Beat_Tracking_GUI::OnDeviceLost()
{
    m_brush.Reset();
}

// Notifies renderers that device dependent resources may now be recreated.
void Beat_Tracking_GUI::OnDeviceRestored()
{
    OnDeviceLost();

    //Brush
    DX::ThrowIfFailed(
        m_deviceResources->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),
            &m_brush)
    );

    //Reposition buttons
    LayoutButtons();
}

// Updates the application state once per frame.
void Beat_Tracking_GUI::Update() 
{
	// Update scene objects.
    m_ET = (float)m_timer.GetElapsedSeconds();
	m_timer.Tick([&]()
	{
        this->FrameMove((float)m_timer.GetElapsedSeconds());
	});
    m_timer.ResetElapsedTime();
}

//Update the app
void Beat_Tracking_GUI::FrameMove(float fET)
{
    //Read beat state
    m_beat = m_beat_async.exchange(0);

    // Update animation
    m_CymbalState = m_CymbalState * exp(-fET * 6);
    m_StickState = m_StickState * exp(-fET * 6);

    //Flash the lights!?
    if (m_beat != 0 || m_UpdateColor)
    {
        m_UpdateColor = false;

        //Move the music instrument
        m_StickState = 1;
        m_CymbalState = 1;

        //Generate random light colors
        for (int i = 0; i < 3; i++)
        {
            m_LightHue[i] += RANDOM * (i * 1.5f + 3.0f) / 6;
            m_LightHue[i] = fmod(m_LightHue[i], 1.0f);

            float L = RANDOM * 0.1f + 0.3f;

            DX::HSLToRGB(&m_LightColor[i], m_LightHue[i], 1.0f, L);

            m_LightColor[i].w = RANDOM * 0.2f + 0.9f;
        }
    }

    //Animate background color
    static const XMVECTORF32 colorSrc = { 1.0f, 0.7f, 1.0f, 1.0f };
    static const XMVECTORF32 colorDest = { 0.13f, 0.38f, 0.52f, 1.0f };

    if (m_beat != 0)
        m_backcolor = *(DirectX::XMFLOAT4*)&colorSrc;
    else
    {
        DirectX::XMVECTOR v1 = DirectX::XMLoadFloat4(&m_backcolor);
        v1 = DirectX::XMVectorLerp(colorDest.v, v1, exp(-m_ET*6.0f));
        DirectX::XMStoreFloat4(&m_backcolor, v1);
    }

    //The recording status sign
    if (BT_Audio_FromMicrophone() || !BT_Audio_NoMoreAudio())
        m_StatusSign = 1 + (m_StatusSign - 1) * (float)expf(-fET / 0.3f);
    else
        m_StatusSign = m_StatusSign * (float)expf(-fET / 1.0f);

    //Reset beat
    m_beat = 0;

    // Some keyboard commands are processed here
    bool fastFwdAudio = false;
    auto Wnd = Windows::UI::Core::CoreWindow::GetForCurrentThread();

    if (Wnd->GetAsyncKeyState(VirtualKey::Number1) != CoreVirtualKeyStates::None ||
        _GetButton(CMD_VOLUME_DOWN)->State == ButtonState::Down)
    {
        m_PlaybackVolume -= fET * 0.4f;
    }
    else if (Wnd->GetAsyncKeyState(VirtualKey::Number2) != CoreVirtualKeyStates::None ||
        _GetButton(CMD_VOLUME_UP)->State == ButtonState::Down)
    {
        m_PlaybackVolume += fET * 0.4f;
    }
    if (Wnd->GetAsyncKeyState(VirtualKey::F6) != CoreVirtualKeyStates::None ||
        _GetButton(CMD_FASTFWD)->State == ButtonState::Down)
        fastFwdAudio = true;

    // Update Audio
    if (m_PlaybackVolume < 0)
        m_PlaybackVolume = 0;
    if (m_PlaybackVolume > 1)
        m_PlaybackVolume = 1;

    if (fastFwdAudio)
    {
        BT_Audio_SetReadSkipFactor(10);
        BT_Audio_SetPlaybackOptions(m_PlaybackVolume, 2);
    }
    else
    {
        BT_Audio_SetReadSkipFactor(1);
        BT_Audio_SetPlaybackOptions(m_PlaybackVolume, 1);
    }

    // Update display text.
    uint32 fps = m_timer.GetFramesPerSecond();
    wchar_t text[768];
#ifndef SIMPLE_GRAPHICS
    swprintf_s(text,
        L"FPS:         %u\n"
        L"Beat:        %lf\n"
        L"Onset SS Σ:  %lf\n"
        L"Aud. Feed Δ: %g\n"
        L"Graph Time:  %0.3f (%d pt)\n"
        L"File:        %s\n"
        L"Volume:      %2.0f%%\n"
        L"DV %lf,%lf,%lf,%lf",
        fps,
        g_DV[GDV_BPM],
        g_DV[GDV_OSS],
        g_DV[GDV_FEEDDELTA],
        g_DV[GDV_GTIME],
        __max(g_Graph[0].MaxPoints(), g_Graph[1].MaxPoints()),
        BT_Audio_GetSourceName(),
        m_PlaybackVolume * 100,
        g_DV[4], g_DV[5], g_DV[6], g_DV[7]
    );
#else
    swprintf_s(text,
        L"FPS %u, Beat %lf",
        fps, g_DV[GDV_BPM]);
#endif

    m_text = text;

    //Layout the text
    ComPtr<IDWriteTextLayout> textLayout;

    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    DX::ThrowIfFailed(
        m_deviceResources->GetDWriteFactory()->CreateTextLayout(
            m_text.c_str(),
            (uint32)m_text.length(),
            m_textFormat.Get(),
            (float)m_deviceResources->GetD2DDeviceContext()->GetPixelSize().width,
            40.0f,
            &textLayout
        )
    );

    DX::ThrowIfFailed(textLayout.As(&m_textLayout));
    DX::ThrowIfFailed(m_textLayout->GetMetrics(&m_textMetrics));

    //Call all buttons for animating
    for (auto iter = m_Buttons.begin(); iter != m_Buttons.end(); iter++)
    {
        (*iter).Update(fET);
    }
}

//Helper to draw a color-tint image
void Beat_Tracking_GUI::DrawImageHelper(ID2D1Bitmap* image,
    D2D1::ColorF colorTint, D2D1_RECT_F targetRect)
{
    if (!image)
        return;

    ID2D1DeviceContext* context = m_deviceResources->GetD2DDeviceContext();

    //Compute image transform
    D2D1_MATRIX_3X2_F mat =
        D2D1::Matrix3x2F::Scale((targetRect.right - targetRect.left) / image->GetPixelSize().width,
        (targetRect.bottom - targetRect.top) / image->GetPixelSize().height)
        *
        D2D1::Matrix3x2F::Translation(targetRect.left, targetRect.top);

    if (colorTint.a > 0)
    {
        //Color Tint needed
        m_colorMatrix->SetInput(0, image);
        D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
            colorTint.r, 0, 0, 0,
            0, colorTint.g, 0, 0,
            0, 0, colorTint.b, 0,
            0, 0, 0, colorTint.a,
            0, 0, 0, 0);
        m_colorMatrix->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
        context->SetTransform(&mat);
        context->DrawImage(m_colorMatrix.Get(),
#ifdef SIMPLE_GRAPHICS
            D2D1_INTERPOLATION_MODE::D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
#else
            D2D1_INTERPOLATION_MODE::D2D1_INTERPOLATION_MODE_CUBIC
#endif
        );
    }
    else
    {
        //Color tint not needed
        context->SetTransform(&mat);
        context->DrawImage(image,
#ifdef SIMPLE_GRAPHICS
            D2D1_INTERPOLATION_MODE::D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
#else
            D2D1_INTERPOLATION_MODE::D2D1_INTERPOLATION_MODE_CUBIC
#endif
        );
    }
}

//Some other helpers
inline D2D1_RECT_F RectAdd(D2D1_RECT_F a, D2D1_RECT_F b)
{
    return D2D1::RectF(a.left + b.left, a.top + b.top, a.right + b.right, a.bottom + b.bottom);
}

inline D2D1_RECT_F RectSub(D2D1_RECT_F a, D2D1_RECT_F b)
{
    return D2D1::RectF(a.left - b.left, a.top - b.top, a.right - b.right, a.bottom - b.bottom);
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool Beat_Tracking_GUI::Render() 
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Reset the viewport to target the whole screen.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
    context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), (float*)&m_backcolor);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//Rendering the screen
    RenderScreen();return true;
}

void Beat_Tracking_GUI::LayoutButtons()
{
    //Get screen size
    ID2D1DeviceContext* context = m_deviceResources->GetD2DDeviceContext();
    float sW = (float)context->GetPixelSize().width;
    float sH = (float)context->GetPixelSize().height;

    float minWH = __min(sW, sH);

    //Place buttons on the bottom right corner
    float btnW = minWH * 0.25f;
    float btnH = minWH * 0.07f;
    float spc = minWH * 0.01f;
    float r = sW - minWH * 0.25f;
    float b = sH - spc;
    
    _GetButton(CMD_MICROPHONE)->Position =
        D2D1::RectF(r - btnW, b - btnH, r, b);
    _GetButton(CMD_OPENANDLOG)->Position =
        D2D1::RectF(r - btnW*2 - spc, b - btnH, r - btnW - spc, b);
    _GetButton(CMD_OPEN)->Position =
        D2D1::RectF(r - btnW*3 - spc*2, b - btnH, r - btnW*2 - spc*2, b);
    _GetButton(CMD_VOLUME_UP)->Position =
        D2D1::RectF(r - btnW, b - btnH*2 - spc, r, b - btnH - spc);
    _GetButton(CMD_VOLUME_DOWN)->Position =
        D2D1::RectF(r - btnW * 2 - spc, b - btnH * 2 - spc, r - btnW - spc, b - btnH - spc);
    _GetButton(CMD_FASTFWD)->Position =
        D2D1::RectF(r - btnW * 3 - spc*2, b - btnH * 2 - spc, r - btnW * 2 - spc*2, b - btnH - spc);
    _GetButton(CMD_END)->Position =
        D2D1::RectF(r - btnW * 3 - spc * 2, b - btnH * 2.75f - 2*spc, r, b - btnH * 2 - 2*spc);
}

void Beat_Tracking_GUI::RenderScreen()
{
    ID2D1DeviceContext* context = m_deviceResources->GetD2DDeviceContext();
    Windows::Foundation::Size logicalSize = m_deviceResources->GetLogicalSize();
    D2D1_MATRIX_3X2_F matIden = D2D1::IdentityMatrix();

    context->BeginDraw();
    context->SetUnitMode(D2D1_UNIT_MODE_PIXELS);

    //Get screen size
    float sW = (float)context->GetPixelSize().width;
    float sH = (float)context->GetPixelSize().height;
    //float dpiFX, dpiFY;
    //context->GetDpi(&dpiFX, &dpiFY); dpiFX /= 96; dpiFY /= 96;
    float tScale = sH / 1000;

    float minWH = __min(sW, sH);
    float iW = minWH * 0.8f;
    float iH = iW;

    //Draw the music studio
    D2D_RECT_F studioRect =
        D2D1::RectF(sW - iW * 1.05f, iH * 0.01f, sW - 0.05f * iW, iH * 1.01f);

    D2D_RECT_F cymbalOffset;
    cymbalOffset.left = iW * 0.008f * (1 - m_CymbalState);
    cymbalOffset.top =  iW * 0.02f * (1 - m_CymbalState);
    cymbalOffset.right = cymbalOffset.left;
    cymbalOffset.bottom = cymbalOffset.top;

    D2D_RECT_F stickOffset;
    stickOffset.left = iW * 0.02f * (1 - m_StickState);
    stickOffset.top = -iW * 0.02f * (1 - m_StickState);
    stickOffset.right = stickOffset.left;
    stickOffset.bottom = stickOffset.top;

    DrawImageHelper(m_Images[(int)BeatTrackerImages::vBack].Get(), D2D1::ColorF(1, 1, 1, 0), studioRect);
    DrawImageHelper(m_Images[(int)BeatTrackerImages::vCymbalLower].Get(), D2D1::ColorF(1, 1, 1, 0), RectAdd(studioRect, cymbalOffset));
    DrawImageHelper(m_Images[(int)BeatTrackerImages::vCymbalUpper].Get(), D2D1::ColorF(1, 1, 1, 0), RectSub(studioRect, cymbalOffset));
    DrawImageHelper(m_Images[(int)BeatTrackerImages::vStick].Get(), D2D1::ColorF(1, 1, 1, 0), RectSub(studioRect, stickOffset));
#ifndef SIMPLE_GRAPHICS
    DrawImageHelper(m_Images[(int)BeatTrackerImages::vL1].Get(), *(D2D1::ColorF*)&m_LightColor[0], studioRect);
    DrawImageHelper(m_Images[(int)BeatTrackerImages::vL2].Get(), *(D2D1::ColorF*)&m_LightColor[1], studioRect);
    DrawImageHelper(m_Images[(int)BeatTrackerImages::vL3].Get(), *(D2D1::ColorF*)&m_LightColor[2], studioRect);
#endif
    DrawImageHelper(m_Images[(int)BeatTrackerImages::vFore].Get(), D2D1::ColorF(1, 1, 1, 0), studioRect);

    //Draw Graphs
    D2D1::Matrix3x2F mat, m2;
    D2D1_RECT_F rcGraph = D2D1::RectF(0, 0, sW * 0.6f, sH * 0.3f);
    mat = D2D1::Matrix3x2F::Translation(20, 20);
    m2 = D2D1::Matrix3x2F::Scale(tScale, tScale);
    context->SetTransform(mat);
    context->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    g_Graph[0].Draw(m_brush.Get(), context, rcGraph.right, rcGraph.bottom);

    auto strDraw = L"Onset Signal Strength";
    m_brush->SetColor(D2D1::ColorF::ColorF(D2D1::ColorF::White));
    context->SetTransform(m2 * mat);
    context->DrawText(strDraw, wcslen(strDraw), m_textFormat.Get(),
        rcGraph, m_brush.Get());

    mat = D2D1::Matrix3x2F::Translation(20, 20 + sH * 0.3f + 20);
    context->SetTransform(mat);
    context->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    g_Graph[1].Draw(m_brush.Get(), context, rcGraph.right, rcGraph.bottom);

    strDraw = L"Signal Flux";
    m_brush->SetColor(D2D1::ColorF::ColorF(D2D1::ColorF::White));
    context->SetTransform(m2 * mat);
    context->DrawText(strDraw, wcslen(strDraw), m_textFormat.Get(),
        rcGraph, m_brush.Get());

    //Draw Text
    // Position on the bottom left corner
    context->SetUnitMode(D2D1_UNIT_MODE_DIPS);
    D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(
        0,
        logicalSize.Height - m_textMetrics.height
    );

    context->SetTransform(screenTranslation * m_deviceResources->GetOrientationTransform2D());

    DX::ThrowIfFailed(
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)
    );

    m_brush->SetColor(D2D1::ColorF::ColorF(D2D1::ColorF::White));
    context->DrawTextLayout(
        D2D1::Point2F(0.f, 0.f),
        m_textLayout.Get(),
        m_brush.Get()
    );

    //Draw Status sign
    context->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    {
        D2D1::Matrix3x2F matIden = D2D1::Matrix3x2F::Identity();
        D2D_RECT_F statusSign;
        float statusSignW = minWH * 0.18f;
        float offset = statusSignW * 0.2f;
        float expansion = m_StatusSign * sin(6.2830f / 0.5f*(float)m_timer.GetTotalSeconds())
            * statusSignW * 0.04f;
        statusSign.left = sW - statusSignW - offset - expansion;
        statusSign.right = sW - offset + expansion;
        statusSign.top = sH + (-statusSignW - offset - expansion) * 1.5f;
        statusSign.bottom = sH + (-offset + expansion) * 1.5f;

        context->SetTransform(matIden);

        BeatTrackerImages signImg;
        if (BT_Audio_FromMicrophone())
            signImg = BeatTrackerImages::vMike;
        else if (BT_Audio_NoMoreAudio())
            signImg = BeatTrackerImages::vEnd;
        else
            signImg = BeatTrackerImages::vPlay;
        DrawImageHelper(m_Images[(int)signImg].Get(), D2D1::ColorF(1, 1, 1, 0), statusSign);
    }

    //Call all buttons for rendering
    for (auto iter = m_Buttons.begin(); iter != m_Buttons.end(); iter++)
    {
        (*iter).Render(m_deviceResources.get(), m_textFormat.Get(), m_brush.Get());
    }

    // Ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
    // is lost. It will be handled during the next call to Present.
    HRESULT hr = context->EndDraw();
    if (hr != D2DERR_RECREATE_TARGET)
    {
        DX::ThrowIfFailed(hr);
    }
}

//Called from Audio subsystem to report last beat status.
void BeatTrackerGUI_Beat(double time, float beat)
{
    if (g_BeatTrackingAppInstance)
        g_BeatTrackingAppInstance->OnBeat(time, beat);
}

void Beat_Tracking_GUI::OnBeat(double time, float beat)
{
    if (beat > 0)
    {
        m_beat_async.exchange(beat);

        //Log
        _BeatLog(std::to_wstring(time) + L"\t");
    }
}

//Process Keyboard messages
void Beat_Tracking_GUI::OnKeyboard(Windows::UI::Core::KeyEventArgs ^ args)
{
    //On press of a key on keyboard...
    if (args->VirtualKey == Windows::System::VirtualKey::O)
    {
        _ProcessCommand(CMD_OPEN);
    }
    else if (args->VirtualKey == Windows::System::VirtualKey::M)
    {
        _ProcessCommand(CMD_MICROPHONE);
    }
    else if (args->VirtualKey == Windows::System::VirtualKey::L)
    {
        _ProcessCommand(CMD_OPENANDLOG);
    }
    else if (args->VirtualKey == Windows::System::VirtualKey::Escape)
    {
        _ProcessCommand(CMD_END);
    }
}

//Process mouse (pointer) messages
void Beat_Tracking_GUI::OnMouse(int eventType, float x, float y)
{
    //Coordinates are in DIPs. Convert to pixels.
    float dx, dy;
    m_deviceResources->GetD2DDeviceContext()->GetDpi(&dx, &dy);
    x *= (dx / 96);
    y *= (dy / 96);

    //Call all buttons for mouse button check
    bool processed = false;
    bool p;
    for (auto iter = m_Buttons.rbegin(); iter != m_Buttons.rend(); iter++)
    {
        if (eventType == 0)
            p = (*iter).OnMouseMove(processed, x, y);
        else if (eventType == 1)
            p = (*iter).OnMouseDown(processed, x, y);
        else if (eventType == 2)
            p = (*iter).OnMouseUp(processed, x, y);

        if (p)
            processed = true;
    }
}

//When a button is clicked...
void Beat_Tracking_GUI::_OnButton(CGuiButton* button)
{
    auto This = (Beat_Tracking_GUI*)button->CallbackUserData;
    if (!This)
        return;
    This->_ProcessCommand(button->ID);
}

//Get a button's ptr from its ID
CGuiButton* Beat_Tracking_GUI::_GetButton(int ID)
{
    for (auto& b : m_Buttons)
    {
        if (b.ID == ID)
            return &b;
    }
    return nullptr;
}

//Processing of some commands is done here.
void Beat_Tracking_GUI::_ProcessCommand(int Cmd)
{
    //Note: Some of commands are processed in the FrameMove() function
    if (Cmd == CMD_OPEN)
    {
        // === Pick Music File === //
        _SelectInputFile(false);
    }
    else if (Cmd == CMD_OPENANDLOG)
    {
        // === Pick Music File and Log === //
        _SelectInputFile(true);
    }
    else if (Cmd == CMD_MICROPHONE)
    {
        // === Microphone === //
        _BeatLogSelect(nullptr);
        BT_Audio_ChangeSource(L"*cap", L"");
    }
    else if (Cmd == CMD_END)
    {
        // === End === //
        _BeatLogSelect(nullptr);
        BT_Audio_ChangeSource(L"-", L"Stopped");
    }
}

//Open file selection pickers to select input music file and possibly
//  a beat log file.
void Beat_Tracking_GUI::_SelectInputFile(bool alsoLogFile)
{

    FileOpenPicker^ picker = ref new FileOpenPicker();
    picker->FileTypeFilter->Append(L"*");
    picker->ViewMode = PickerViewMode::Thumbnail;
    picker->CommitButtonText = L"Select Music File";
    create_task(picker->PickSingleFileAsync()).then([alsoLogFile, this](StorageFile^ file)
    {
        if (file)
        {
            auto afterSecondPick =
                [this, file, alsoLogFile](StorageFile^ logFile)
            {
                if (!logFile && alsoLogFile)
                    return;

                AccessCache::StorageApplicationPermissions::
                    FutureAccessList->AddOrReplace("MusicFile", file);

                if (logFile)
                    AccessCache::StorageApplicationPermissions::
                        FutureAccessList->AddOrReplace("LogFile", logFile);

                create_task(Windows::Storage::FileIO::ReadBufferAsync(file)).then([this, file, logFile](
                    Windows::Storage::Streams::IBuffer^ buf)
                {
                    auto reader = Windows::Storage::Streams::DataReader::FromBuffer(buf);
                    Platform::Array<byte>^ buffer = ref new Platform::Array<byte>(buf->Length);
                    reader->ReadBytes(buffer);

                    //For later simplicity on opening the file, copy the content to a temp location.

                    //Select a file name
                    static int localCopyK = 0;
                    localCopyK = (localCopyK + 1) % 3;
                    std::wstring cacheFolder = Windows::Storage::ApplicationData::Current->
                        LocalCacheFolder->Path->Data();
                    cacheFolder += L"\\localcopy.music" + std::to_wstring(localCopyK);

                    //Copy content to the target file.
                    DWORD dwL;
                    HANDLE hFile = CreateFile2(cacheFolder.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                        CREATE_ALWAYS, NULL);
                    WriteFile(hFile, buffer->Data, buffer->Length, &dwL, nullptr);
                    CloseHandle(hFile);

                    //If log file selected, store its reference.
                    _BeatLogSelect(logFile);

                    //Change source
                    std::wstring strFriendlyName = std::wstring(file->Name->Data()) + L" (Local Copy)";
                    BT_Audio_ChangeSource(cacheFolder.c_str(), strFriendlyName.c_str());
                });
            };

            if (alsoLogFile)
            {
                FileSavePicker^ picker2 = ref new FileSavePicker();

                auto logExts = ref new Platform::Collections::Vector<Platform::String^>();
                logExts->Append(".txt");
                picker2->FileTypeChoices->Insert("Beat Log File", logExts);
                picker2->DefaultFileExtension = ".txt";
                picker2->CommitButtonText = L"Select Beat Log File";

                create_task(picker2->PickSaveFileAsync()).then(afterSecondPick);
            }
            else
            {
                afterSecondPick(nullptr);
            }
        }
    });
}

void Beat_Tracking_GUI::_BeatLogSelect(Windows::Storage::StorageFile^ file)
{
    m_logFile = file;

    if (file)
    {
        try
        {
            block_to_complete(Windows::Storage::FileIO::WriteTextAsync(file,
                ref new Platform::String(L"")));
        }
        catch (...) {}
    }
}

void Beat_Tracking_GUI::_BeatLog(std::wstring str)
{
    if (m_logFile)
    {
        try
        {
            block_to_complete(Windows::Storage::FileIO::AppendTextAsync(m_logFile,
                ref new Platform::String(str.c_str())));
        }
        catch (...) {}
    }
}
