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

//==============================================================//
// Audio Capture
// Requirment: C++/CX and UWP Framework
//==============================================================//

#pragma once

#include "AudioHelpers.h"

#ifdef AUDIO_CAP_INTERNALS
#include <wrl.h>
#include <wrl/client.h>
#include <agile.h>
#include <concrt.h>
#include <ppltasks.h>

using namespace concurrency;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;
using namespace Windows::Storage::Streams;
#endif

namespace BeatTrackingAudio
{
#ifdef AUDIO_CAP_INTERNALS
    ref struct AudioCaptureData
    {
    public:
        property MediaCapture^ capture;
        property MediaEncodingProfile^ captureProfile;
        property IRandomAccessStream^ stream;
        property IInputStream^ readStream;
        property Buffer^ readBuf;
    };
#endif

    class CAudioCapture
    {
    public:
        static CAudioCapture* New();
        virtual ~CAudioCapture();

        bool ConnectToAudioCapturer(int SampleRate, int Channels, int BitsPerSample,
            int MaximumSamples, int PacketSamples);
        void Close();
        void WaitForNextPacket();
        int CountPackets();
        bool GetNextPacket(double* outData, int sampleCount);
        void DiscardNextPacket();

    private:
        CAudioCapture();

#ifdef AUDIO_CAP_INTERNALS
    public:
        void _OnData(void* data, int dataLength);

    private:
        AudioCaptureData^ m_data;
        int m_bytesPerSample = 0;
        int m_packetSample = 0;

        struct MINIBUFFER
        {
            void* data;
            unsigned int dataMaxLen;
            unsigned int dataUseLen;

            MINIBUFFER() : data(NULL), dataMaxLen(), dataUseLen() {}
        };

        volatile MINIBUFFER m_lastBuf;
        SRWLOCK m_lock;
        HANDLE m_event;
        std::list<MINIBUFFER> m_bufs;
#endif
    };
}
