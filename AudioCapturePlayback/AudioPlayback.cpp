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
// Audio Playback
//
// With the use of XAudio 2 library on Windows, this module simplifies
//   playback of audio sample blocks in a 'source-sink' manner.
//===================================================================//

#include "pch.h"
#include "AudioHelpers.h"
#include "AudioPlayback.h"

using namespace std;

//Some audio constants
#define AUDPLAY_FRAME_MIN_SAMPLES			60
#define AUDPLAY_FRAME_MAX_SAMPLES			1536000

#define AUDPLAY_SAMPLERATE_MIN				4000
#define AUDPLAY_SAMPLERATE_MAX				400000

#define AUDPLAY_CHANNELS_MAX				16

//=====================================================================//
	namespace BeatTrackingAudio {
//=====================================================================//

//====================================================//
// Internal Stuff
//====================================================//
//AUD_ERR MMResult_TprErr(MMRESULT errcode);

//====================================================//
// class CAudioPlayback
//====================================================//
CAudioPlayback::CAudioPlayback()
{
	InitializeCriticalSectionAndSpinCount(&m_CS, 10000);
	InitializeCriticalSectionAndSpinCount(&m_CS_Submit, 10000);
	m_State = AUDIO_INOUT_STATE::Stopped;
	ZeroMemory(&m_Format, sizeof(m_Format));
    m_pXAudio = nullptr;
    m_pMaster = nullptr;
    m_pSource = nullptr;
	m_SampleCB = nullptr;
    m_CB.Parent = this;
	m_MaxSubmits = 2;
	m_MinRequiredBuffers = 2;
	m_LastVolume = 1.0f;
    m_FreqFactor = 1.0f;
	m_Buffers_Submitted_Garbage = 0;
}

CAudioPlayback::~CAudioPlayback()
{
	CloseDevice();
	DeleteCriticalSection(&m_CS);
	DeleteCriticalSection(&m_CS_Submit);
	m_SampleCB = nullptr;
}

/*CAudioPlayback*	CAudioPlayback::New()
{
	return new CAudioPlayback();
}*/

void CAudioPlayback::SetSampleFinishCallback(CSampleCallback* CB)
{
	m_SampleCB = CB;
}

AUD_ERR CAudioPlayback::ConnectToDevice(UINT AudioDeviceIndex, UINT SamplesPerSecond,
	UINT MaxSubmittedFrames, USHORT Channels)
{
	AUD_ERR err = AUD_ERR::Failure;

	//Verify state and arguments
	if (m_State != AUDIO_INOUT_STATE::Stopped) return AUD_ERR::Unexpected;
	if (SamplesPerSecond < AUDPLAY_SAMPLERATE_MIN || SamplesPerSecond > AUDPLAY_SAMPLERATE_MAX)
		return AUD_ERR::InvalidSampleRate;
	if (Channels < 1 || Channels > AUDPLAY_CHANNELS_MAX)
		return AUD_ERR::InvalidChannelCount;
	if (MaxSubmittedFrames < 2) MaxSubmittedFrames = 2;

	//Prepare wave format
	m_Format.cbSize = sizeof(m_Format);
	m_Format.nChannels = Channels;
	m_Format.nSamplesPerSec = SamplesPerSecond;
	m_Format.wBitsPerSample = 16;
	m_Format.wFormatTag = WAVE_FORMAT_PCM;
	m_Format.nBlockAlign = m_Format.nChannels * m_Format.wBitsPerSample / 8;
	m_Format.nAvgBytesPerSec = m_Format.nBlockAlign * m_Format.nSamplesPerSec;

	//Open the device
    if (FAILED(XAudio2Create(&m_pXAudio)))
        return AUD_ERR::Failure;

    if (FAILED(m_pXAudio->StartEngine()))
        return AUD_ERR::Failure;

    if (FAILED(m_pXAudio->CreateMasteringVoice(&m_pMaster, Channels, SamplesPerSecond,
        0, NULL, NULL, AUDIO_STREAM_CATEGORY::AudioCategory_GameMedia)))
        return AUD_ERR::Failure;

    XAUDIO2_SEND_DESCRIPTOR listDesc[1] =
    {
        { 0, m_pMaster },
    };

    XAUDIO2_VOICE_SENDS list =
    {
        1, listDesc,
    };

    if (FAILED(m_pXAudio->CreateSourceVoice(&m_pSource, &m_Format, 0, 2.0F, &m_CB, &list, nullptr)))
        return AUD_ERR::Failure;

	//Apply current settings (volume, ...)
	_ApplyPlaybackOptions();

	//Store max frames to be submitted to the audio driver.
	m_MaxSubmits = MaxSubmittedFrames;

	//Begin!
	
	//Set state
	m_State = AUDIO_INOUT_STATE::Playing;
	return AUD_ERR::Success;
}

void CAudioPlayback::SetMinRequiredBuffersBeforeSubmit(UINT Count)
{
	if (Count < 1)
		Count = 1;
	if (Count > 256)
		Count = 256;
	m_MinRequiredBuffers = Count;
}

void CAudioPlayback::CloseDevice()
{
    if (m_pSource)
    {
        m_pSource->Stop();
        m_pSource->DestroyVoice();
        m_pSource = nullptr;
    }
    if (m_pMaster)
    {
        m_pMaster->DestroyVoice();
        m_pMaster = nullptr;
    }
    if (m_pXAudio)
    {
        m_pXAudio->StopEngine();
        m_pXAudio->Release();
    }

	for (auto iter = m_Buffers_Pending.begin(); iter != m_Buffers_Pending.end(); iter++)
	{
		auto Buf = *iter;
		Buf->Release();
	}
	for (auto iter = m_Buffers_Submitted.begin(); iter != m_Buffers_Submitted.end(); iter++)
	{
		auto Buf = *iter;
		Buf->Release();
	}
	m_Buffers_Pending.clear();
	m_Buffers_Submitted.clear();
	m_Buffers_Submitted_Garbage = 0;
	m_State = AUDIO_INOUT_STATE::Stopped;
}

AUDIO_INOUT_STATE CAudioPlayback::GetState() const
{
	return m_State;
}

const WAVEFORMATEX* CAudioPlayback::GetWaveFormat() const
{
    if (m_State == AUDIO_INOUT_STATE::Stopped) return nullptr;
	return &m_Format;
}

void CAudioPlayback::FlushPendingFrames(int Count)
{
	if (m_State == AUDIO_INOUT_STATE::Stopped) return;

	_CollectGarbage();

    //Ask XAudio to release our buffers
    m_pSource->FlushSourceBuffers();

	CLock lock(&m_CS);
	if (Count < 0 || Count > (int)m_Buffers_Pending.size())
		Count = (int)m_Buffers_Pending.size();
	
	//Take out pending buffers
	list<CAudioBuffer*> BuffersToDelete;
	for (int i = 0; i < Count; i++)
	{
		auto iter = m_Buffers_Pending.begin();
		BuffersToDelete.push_back(*iter);
		m_Buffers_Pending.erase(iter);
	}

	lock.Unlock();
	for (auto Buf : BuffersToDelete)
	{
		Buf->Release();
	}
}

int CAudioPlayback::GetPendingFrames()
{
	if (m_State != AUDIO_INOUT_STATE::Playing) return 0;
	return (int)m_Buffers_Pending.size();
}

int CAudioPlayback::GetSubmittedFrames()
{
	if (m_State != AUDIO_INOUT_STATE::Playing) return 0;
	return (int)m_Buffers_Submitted.size();
}

AUD_ERR CAudioPlayback::SubmitFrame(UINT NumSamples, void* WavePCMData)
{
	if (m_State != AUDIO_INOUT_STATE::Playing) return AUD_ERR::Unexpected;
	if (NumSamples < AUDPLAY_FRAME_MIN_SAMPLES || NumSamples > AUDPLAY_FRAME_MAX_SAMPLES)
		return AUD_ERR::InvalidSamplesPerFrame;

	//Create a buffer
	auto Buf = new (std::nothrow) CAudioBuffer();
	if (!Buf) return AUD_ERR::OutOfMemory;
	Buf->m_DataLength = NumSamples * m_Format.nBlockAlign;
	Buf->m_Data = malloc(Buf->m_DataLength);
	if (!Buf->m_Data)
	{
		Buf->Release();
		return AUD_ERR::OutOfMemory;
	}
	memcpy( Buf->m_Data, WavePCMData, Buf->m_DataLength );

	//Put this buffer in the pending list.
	{
		CLock lock(&m_CS);
		m_Buffers_Pending.push_back(Buf);
	}

	//!@!
	_CollectGarbage();

	//Submit
	_SubmitEnoughBuffers();
	return AUD_ERR::Success;
}

AUD_ERR CAudioPlayback::_SubmitEnoughBuffers()
{
	//No more than one thread is allowed to do this operation at once.
	//Locking is required but requires special care to avoid deadlock.
	if (!TryEnterCriticalSection(&m_CS_Submit))
	{
		return AUD_ERR::Success;
	}

	AUD_ERR err = AUD_ERR::Success;
	CLock lock(&m_CS);

	//If minimum number of buffers to submit is not reached, postpone the job.
	bool FirstSubmit = m_Buffers_Submitted.size() == 0;
	if (FirstSubmit && m_Buffers_Pending.size() < m_MinRequiredBuffers)
	{
		goto endOfSubmission;
	}

	//If number of submitted buffers is not high enough, submit this new buffer.
	if (m_Buffers_Submitted.size() < m_MaxSubmits)
	{
		int CountToSubmit = 1;
		if (FirstSubmit)
			CountToSubmit = m_MinRequiredBuffers;
		
		for (int i = 0; i < CountToSubmit; i++)
		{
			err = _SubmitNextBuffer(&lock);
			if (err != AUD_ERR::Success)
			{
				//Note: _SubmitNextBuffer() automatically deletes buffers on failure.
				goto endOfSubmission;
			}
		}
	}

	err = AUD_ERR::Success;

endOfSubmission:
	//CAUTION! Remember to unlock
	lock.Unlock();
	LeaveCriticalSection(&m_CS_Submit); //!@! WASBUGGY: m_CS
	return err;
}

AUD_ERR CAudioPlayback::_SubmitNextBuffer(CLock* lock)
{
	if (m_Buffers_Pending.size() == 0) return AUD_ERR::Failure;

	//((CAUTION: Do not lock while accessing WaveOut because the system might deadlock!))
	//((Note: On failure, remember to discard the buffer.))
	auto Buffer = *m_Buffers_Pending.begin();

    XAUDIO2_BUFFER buf;
    buf.AudioBytes = Buffer->m_DataLength;
    buf.pAudioData = (BYTE*)Buffer->m_Data;
    buf.Flags = 0;
    buf.LoopBegin = 0;
    buf.LoopCount = 0;
    buf.LoopLength = 0;
    buf.PlayBegin = 0;
    buf.PlayLength = 0;
    buf.pContext = this;
	lock->Unlock();

    //Submit
    m_pSource->SubmitSourceBuffer(&buf);
    if (m_Buffers_Submitted.size() == 0)
        m_pSource->Start();

	//Move from the pending list to the submitted list.
	lock->Lock();
	m_Buffers_Pending.pop_front();
	m_Buffers_Submitted.push_back(Buffer);
	return AUD_ERR::Success;
}

void CAudioPlayback::_CollectGarbage()
{
	UINT numGarbage = InterlockedExchange(&m_Buffers_Submitted_Garbage, 0);
	if (numGarbage == 0) return;

	CLock lock(&m_CS);

	for (UINT i = 0; i < numGarbage; i++)
	{
		auto CurrentBuffer = *m_Buffers_Submitted.begin();
		m_Buffers_Submitted.pop_front();

		lock.Unlock();
		CurrentBuffer->Release();
		lock.Lock();
	}
}

void _stdcall CAudioPlayback::CAudioCB::OnVoiceProcessingPassStart(THIS_ UINT32 BytesRequired)
{

}

void _stdcall CAudioPlayback::CAudioCB::OnVoiceProcessingPassEnd(THIS)
{

}

void _stdcall CAudioPlayback::CAudioCB::OnStreamEnd(THIS)
{

}

void _stdcall CAudioPlayback::CAudioCB::OnBufferStart(THIS_ void* pBufferContext)
{
}

void _stdcall CAudioPlayback::CAudioCB::OnBufferEnd(THIS_ void* pBufferContext)
{
    auto This = (CAudioPlayback*)pBufferContext; //Also: this->Parent
    if (!This) return;

    //Remember: This is a callback function. We should do the stuff quick.
    //The current buffer is done playback. Release it.
    This->m_Buffers_Submitted_Garbage++;

    //Submit next buffer(s).
    This->_SubmitEnoughBuffers();

    //Good...
    //Put a missed call on the user!
    if (This->m_SampleCB)
        This->m_SampleCB->OnSampleFinish((void*)This);

    //wchar_t S[100];
    //swprintf_s(S, L"A %d\n", rand());
    //OutputDebugString(S);
}

void _stdcall CAudioPlayback::CAudioCB::OnLoopEnd(THIS_ void* pBufferContext)
{

}

void _stdcall CAudioPlayback::CAudioCB::OnVoiceError(THIS_ void* pBufferContext, HRESULT Error)
{

}

void CAudioPlayback::SetPlaybackVolume(float Volume, float Freq)
{
	if (Volume < 0) Volume = 0;
	if (Volume > 1) Volume = 1;
    if (Freq < 0.001f) Freq = 0.001f;
    if (Freq > 10.0f) Freq = 10;
	m_LastVolume = Volume;
    m_FreqFactor = Freq;
	_ApplyPlaybackOptions();
}

void CAudioPlayback::_ApplyPlaybackOptions()
{
    if (m_pMaster)
        m_pMaster->SetVolume(m_LastVolume);
    if (m_pSource)
        m_pSource->SetFrequencyRatio(m_FreqFactor);
}

//=====================================================================//
// end of namespace
	}
//=====================================================================//