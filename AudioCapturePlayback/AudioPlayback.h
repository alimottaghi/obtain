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

//--------------------------------------------------------------//
// Audio Playback
//--------------------------------------------------------------//

#pragma once

#include "AudioHelpers.h"

namespace BeatTrackingAudio
{
    //Audio Playback Errors
    enum AUD_ERR
    {
        Success = 0UL,
        OutOfMemory,
        Unexpected,
        InvalidArgument,
        InvalidPartyType,
        Failure,
        FatalFailure,
        ProgrammersIssue,
        DeviceInUse,
        InUse = DeviceInUse,
        PortInUse = DeviceInUse,
        Unconfigured,
        BadConfiguration,

        BadDeviceID,
        InvalidSamplesPerFrame,
        InvalidSampleRate,
        InvalidChannelCount,
        BadInputFormat,
    };

	//Audio Playback Class
	class CAudioPlayback
	{
	public:
		virtual AUD_ERR				ConnectToDevice(UINT AudioOutputDeviceIndex, UINT SamplesPerSecond,
			UINT MaxConcurrentSubmittedFrames, USHORT Channels);
		virtual void				SetMinRequiredBuffersBeforeSubmit(UINT Count);
		virtual void				CloseDevice();
		virtual AUDIO_INOUT_STATE	GetState() const;
		virtual const WAVEFORMATEX* GetWaveFormat() const;

		virtual void				SetSampleFinishCallback(CSampleCallback* CB);

		virtual void				FlushPendingFrames(int Count = -1);
		virtual int					GetPendingFrames();
		virtual int					GetSubmittedFrames();
		virtual AUD_ERR				SubmitFrame(UINT NumSamples, void* WavePCMData);

		virtual void				SetPlaybackVolume(float Volume, float Freq);

        CAudioPlayback();
		virtual ~CAudioPlayback();

	private:
		AUD_ERR _SubmitNextBuffer(CLock* lock);
		AUD_ERR _SubmitEnoughBuffers();
		void _CollectGarbage();

		//Current Device
		WAVEFORMATEX				m_Format;
        IXAudio2*                   m_pXAudio;
        IXAudio2MasteringVoice*     m_pMaster;
        IXAudio2SourceVoice*        m_pSource;

		AUDIO_INOUT_STATE			m_State;
		UINT						m_MaxSubmits;
		UINT						m_MinRequiredBuffers;
		float						m_LastVolume;
        float                       m_FreqFactor;

		//Thread safety stuff
		CRITICAL_SECTION			m_CS;
		CRITICAL_SECTION			m_CS_Submit; //to prevent parallel calls to _SubmitEnoughBuffers()

        class CAudioCB : public IXAudio2VoiceCallback
        {
        public:
            void _stdcall OnVoiceProcessingPassStart(THIS_ UINT32 BytesRequired);
            void _stdcall OnVoiceProcessingPassEnd(THIS);
            void _stdcall OnStreamEnd(THIS);
            void  _stdcall OnBufferStart(THIS_ void* pBufferContext);
            void  _stdcall OnBufferEnd(THIS_ void* pBufferContext);
            void  _stdcall OnLoopEnd(THIS_ void* pBufferContext);
            void _stdcall  OnVoiceError(THIS_ void* pBufferContext, HRESULT Error);

            CAudioPlayback*         Parent;
        };
        CAudioCB                    m_CB;

		//Helper
		void _ApplyPlaybackOptions();

		//A simple callback provider (for the client)
		CSampleCallback*				m_SampleCB;

		//List of buffers
		std::list<CAudioBuffer*>	m_Buffers_Pending;
		std::list<CAudioBuffer*>	m_Buffers_Submitted;
		UINT						m_Buffers_Submitted_Garbage;
	};
};
