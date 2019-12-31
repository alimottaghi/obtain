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

#pragma once

#include "DeviceResources.h"

enum class ButtonState
{
    Up,
    Hover,
    Down,
};

typedef void(* fnButtonClick)(class CGuiButton* btn);

class CGuiButton
{
public:
    std::wstring        Text;
    D2D1_RECT_F         Position;
    D2D1_COLOR_F        BackColor;
    D2D1_COLOR_F        ForeColor;
    ButtonState         State;
    float               StateAnimation;
    int                 ID;
    fnButtonClick       CallbackToClick;
    void*               CallbackUserData;
    bool                WasPressed;

    CGuiButton();

    void Update(float fET);
    void Render(DX::DeviceResources* devRes, IDWriteTextFormat* textFmt, ID2D1SolidColorBrush* brush);
    bool OnMouseMove(bool alreadyProcessed, float px, float py);
    bool OnMouseDown(bool alreadyProcessed, float px, float py);
    bool OnMouseUp(bool alreadyProcessed, float px, float py);
};
