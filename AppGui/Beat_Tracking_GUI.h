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

#pragma once

#include "StepTimer.h"
#include "DeviceResources.h"
#include "GuiButton.h"

// Renders Direct2D and 3D content on the screen.
namespace Beat_Tracking_App
{
    enum class BeatTrackerImages
    {
        vBack,
        vCymbalLower,
        vCymbalUpper,
        vFore,
        vL1,
        vL2,
        vL3,
        vStick,
        vPlay,
        vEnd,
        vMike,
        COUNT,
    };

	class Beat_Tracking_GUI : public DX::IDeviceNotify
	{
	public:
		Beat_Tracking_GUI(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~Beat_Tracking_GUI();

		void Update();
        void FrameMove(float elapsedTime);
		bool Render();
        void OnKeyboard(Windows::UI::Core::KeyEventArgs ^ args);
        void OnMouse(int eventType, float x, float y);

        void DrawImageHelper(ID2D1Bitmap* image,
            D2D1::ColorF colorTint, D2D1_RECT_F targetRect);
        void RenderScreen();

        void OnBeat(double time, float beat);

        void LayoutButtons();
        
		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // Some graphic resources
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_brush;
        Microsoft::WRL::ComPtr<ID2D1Effect>             m_colorMatrix;

        // Resources related to info text rendering.
        std::wstring                                    m_text;
        DWRITE_TEXT_METRICS	                            m_textMetrics;
        Microsoft::WRL::ComPtr<IDWriteTextLayout3>      m_textLayout;
        Microsoft::WRL::ComPtr<IDWriteTextFormat2>      m_textFormat;

        // Images
        Microsoft::WRL::ComPtr<ID2D1Bitmap>             m_Images[(int)BeatTrackerImages::COUNT];

        // Buttons
        std::list<CGuiButton>                           m_Buttons;
        static void _OnButton(CGuiButton* button);
        CGuiButton* _GetButton(int ID);

        //Commands
        void _ProcessCommand(int Cmd);
        void _SelectInputFile(bool alsoLogFile);

        //Logging
        Windows::Storage::StorageFile^                  m_logFile;
        void _BeatLogSelect(Windows::Storage::StorageFile^ file);
        void _BeatLog(std::wstring str);

		// Rendering loop timer.
		DX::StepTimer m_timer;

        // Graphic Variables for Animation
        float                   m_ET = 0;
        std::atomic<float>      m_beat_async = 0;
        float                   m_beat = 0;
        DirectX::XMFLOAT4       m_backcolor;
        float                   m_CymbalState = 0;
        float                   m_StickState = 0;
        float                   m_LightHue[3] = { 0.1f, 0.6f, 0.9f };
        DirectX::XMFLOAT4       m_LightColor[3];
        bool                    m_UpdateColor = true;
        float                   m_StatusSign = 1;

        // UI Options
        float                   m_PlaybackVolume = 1;
	};
}

