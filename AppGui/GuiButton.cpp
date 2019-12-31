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

//==================================================
//A simple button for the Beat Tracking GUI
//==================================================

#include "pch.h"
#include "DirectXHelper.h"
#include "Helper.h"
#include "GuiButton.h"

CGuiButton::CGuiButton()
{
    Text = L"Button";
    Position = D2D1::RectF(0, 0, 100, 20);
    BackColor = (D2D1::ColorF(0.2f, 0.5f, 0.7f, 0.7f));
    ForeColor = (D2D1::ColorF(1,1,1,1));
    State = ButtonState::Up;
    StateAnimation = 1;
    ID = 0;
    CallbackToClick = nullptr;
    CallbackUserData = nullptr;
    WasPressed = false;
}

void CGuiButton::Update(float fET)
{
    if (State == ButtonState::Up)
        StateAnimation = 1 + (StateAnimation - 1) * expf(-fET / 0.1f);
    else if (State == ButtonState::Down)
        StateAnimation = 0.5f + (StateAnimation - 0.5f) * expf(-fET / 0.05f);
    else if (State == ButtonState::Hover)
        StateAnimation = 2 + (StateAnimation - 2) * expf(-fET / 0.2f);
}

void CGuiButton::Render(DX::DeviceResources* devRes, IDWriteTextFormat* textFmt,
    ID2D1SolidColorBrush* brush)
{
    auto context = devRes->GetD2DDeviceContext();
    D2D1::Matrix3x2F matIden = D2D1::Matrix3x2F::Identity();
    context->SetTransform(matIden);
    context->SetUnitMode(D2D1_UNIT_MODE_DIPS);

    auto PosDIP = Position;
    float dpiFX, dpiFY;
    context->GetDpi(&dpiFX, &dpiFY); dpiFX /= 96; dpiFY /= 96;
    PosDIP.left /= dpiFX;
    PosDIP.right /= dpiFX;
    PosDIP.top /= dpiFY;
    PosDIP.bottom /= dpiFY;

    D2D1_COLOR_F b2 = BackColor;
    b2.r *= StateAnimation; b2.g *= StateAnimation; b2.b *= StateAnimation;

    brush->SetColor(b2);
    context->FillRectangle(PosDIP, brush);

    b2 = ForeColor;
    b2.r *= StateAnimation; b2.g *= StateAnimation; b2.b *= StateAnimation;

    brush->SetColor(b2);
    context->DrawRectangle(PosDIP, brush, 2);

    IDWriteTextLayout* layout;
    devRes->GetDWriteFactory()->CreateTextLayout(Text.c_str(), Text.length(), textFmt,
        PosDIP.right - PosDIP.left, PosDIP.bottom - PosDIP.top, &layout);

    layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    context->DrawTextLayout(
        D2D1::Point2F(PosDIP.left, PosDIP.top),
        layout, brush);

    Safe_Release(layout);
}

bool PointInRect(float x, float y, D2D1_RECT_F& rc)
{
    return x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom;
}

bool CGuiButton::OnMouseMove(bool alreadyProcessed, float px, float py)
{
    bool onBtn = !alreadyProcessed && PointInRect(px, py, Position);

    if (WasPressed)
    {
        if (onBtn)
            State = ButtonState::Down;
        else
            State = ButtonState::Up;
        return true;
    }
    else
    {
        if (onBtn)
            State = ButtonState::Hover;
        else
            State = ButtonState::Up;
        return onBtn;
    }
}

bool CGuiButton::OnMouseDown(bool alreadyProcessed, float px, float py)
{
    bool onBtn = !alreadyProcessed && PointInRect(px, py, Position);

    if (onBtn)
    {
        State = ButtonState::Down;
        WasPressed = true;
        return true;
    }
    return false;
}

bool CGuiButton::OnMouseUp(bool alreadyProcessed, float px, float py)
{
    bool onBtn = !alreadyProcessed && PointInRect(px, py, Position);

    if (WasPressed && onBtn)
    {
        State = ButtonState::Up;
        WasPressed = false;

        //Clicked
        if (CallbackToClick)
            CallbackToClick(this);
        return true;
    }
    else
        WasPressed = false;
    return false;
}
