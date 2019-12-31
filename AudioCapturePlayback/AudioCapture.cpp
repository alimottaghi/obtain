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
// Audio Capturer
//   This module incorporates the Media interface of UWP Framework
//   to record audio from the microphone.
//   This module requires the availability of the UWP Framework
//   (can only be used on Windows Universal Apps) and therefore
//   needs the use of C++/CX extensions.
//===================================================================//

#include "pch.h"
#include "AudioHelpers.h"
#define AUDIO_CAP_INTERNALS
#include "AudioCapture.h"

//=====================================================================//
	namespace BeatTrackingAudio {
//=====================================================================//

//=============================================================//
// The following class is an implementation of the RandomAccess
//   interface, used to sink the sample data provided by the
//   media provider component of Windows.
//=============================================================//
ref class CStreamKhubu sealed : public IRandomAccessStream
{
private:
    Platform::IntPtr m_parentPtr;

public:
    property unsigned long long m_virtualPos;
    
    CStreamKhubu(Platform::IntPtr parent)
    {
        m_virtualPos = 0;
        m_parentPtr = parent;
    }

    virtual ~CStreamKhubu()
    {
    }

    virtual Windows::Storage::Streams::IRandomAccessStream^ CloneStream()
    {
        return nullptr;
    }

    virtual Windows::Storage::Streams::IInputStream^ GetInputStreamAt(unsigned long long position)
    {
        return nullptr;
    }

    virtual Windows::Storage::Streams::IOutputStream^ GetOutputStreamAt(unsigned long long position)
    {
        return nullptr;
    }

    virtual void Seek(unsigned long long position)
    {
        return;
    }

    virtual
    Windows::Foundation::IAsyncOperationWithProgress<Windows::Storage::Streams::IBuffer^, unsigned int>^
    ReadAsync(Windows::Storage::Streams::IBuffer^ buffer, unsigned int count, Windows::Storage::Streams::InputStreamOptions options)
    {
        return nullptr;
    }

    virtual
    Windows::Foundation::IAsyncOperationWithProgress<unsigned int, unsigned int>^
    WriteAsync(Windows::Storage::Streams::IBuffer^ buffer)
    {
        unsigned int wrLen = buffer->Length;
        m_virtualPos += wrLen;

        //Perform the job
        auto reader = Windows::Storage::Streams::DataReader::FromBuffer(buffer);
        auto arData = ref new Platform::Array<unsigned char>(buffer->Length);
        reader->ReadBytes(arData);
        ((CAudioCapture*)(void*)m_parentPtr)->_OnData(arData->Data, arData->Length);

        //Result some fucking IAsync because those motherfuckers
        //  have screwed our asses with their bullshit ASYNCHRONY standards...
        task_completion_event<unsigned int> tce;
        tce.set(wrLen);

        auto asyncOp = create_async([=](progress_reporter<unsigned int> prog) {
            prog.report(wrLen);
            return create_task(tce);
        });
        return asyncOp;
    }

    virtual
    Windows::Foundation::IAsyncOperation<bool>^
    FlushAsync()
    {
        task_completion_event<bool> tce;
        tce.set(true);
        auto asyncOp = create_async([=]() {
            return create_task(tce);
        });
        return asyncOp;
    }

    virtual property unsigned long long Size
    {
        unsigned long long get()
        {
            return m_virtualPos;
        }
        void set(unsigned long long s)
        {
        }
    }
    virtual property unsigned long long Position
    {
        unsigned long long get()
        {
            return m_virtualPos;
        }
    }
    virtual property bool CanRead
    {
        bool get()
        {
            return true;
        }
    }
    virtual property bool CanWrite
    {
        bool get()
        {
            return true;
        }
    }
};

//===================================================================//
// Audio Capturer Class
//===================================================================//
CAudioCapture::CAudioCapture()
{
    m_data = ref new AudioCaptureData();
    m_lock = SRWLOCK_INIT;
    m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CAudioCapture* CAudioCapture::New()
{
    return new CAudioCapture();
}

CAudioCapture::~CAudioCapture()
{
    Close();
    m_data = nullptr;
    if (m_event)
    {
        CloseHandle(m_event);
        m_event = nullptr;
    }
}

bool CAudioCapture::ConnectToAudioCapturer(int SampleRate, int Channels,
    int BitsPerSample, int MaximumSamples, int PacketSamples)
{
    Close();

    if (Channels != 1 || BitsPerSample != 16)
        return false;

    m_data->captureProfile = MediaEncodingProfile::CreateWav(AudioEncodingQuality::Auto);
    m_data->captureProfile->Audio = AudioEncodingProperties::CreatePcm(SampleRate, Channels, BitsPerSample);
    m_data->captureProfile->Audio->SampleRate = 44100;

    m_data->capture = ref new MediaCapture();

    auto initSetting = ref new MediaCaptureInitializationSettings();
    initSetting->StreamingCaptureMode = StreamingCaptureMode::Audio;
    block_to_complete( m_data->capture->InitializeAsync(initSetting) );

    m_data->stream = ref new CStreamKhubu((void*)(this));
    m_bytesPerSample = Channels * (BitsPerSample + 7) / 8;
    m_packetSample = PacketSamples;
    
    block_to_complete(m_data->capture->StartRecordToStreamAsync(m_data->captureProfile, m_data->stream));
    return true;
}

void CAudioCapture::Close()
{
    if (m_data->capture)
    {
        block_to_complete(m_data->capture->StopRecordAsync());
        m_data->capture = nullptr;
    }

    m_bytesPerSample = 0;
    m_packetSample = 0;
    m_data->captureProfile = nullptr;
    m_data->stream = nullptr;
    m_data->readBuf = nullptr;

    {
        CLockSRW lock(m_lock);
        for (auto p : m_bufs)
            if (p.data)
                free(p.data);
        m_bufs.clear();

        if (m_lastBuf.data)
        {
            free(m_lastBuf.data);
            m_lastBuf.data = NULL;
        }
        m_lastBuf.dataMaxLen = 0;
    }
}

int CAudioCapture::CountPackets()
{
    return m_bufs.size();
}

void CAudioCapture::DiscardNextPacket()
{
    if (m_bufs.size() == 0)
        return;

    MINIBUFFER buf;
    {
        CLockSRW lock(m_lock);
        buf = m_bufs.front();
        m_bufs.pop_front();
    }
    free(buf.data);
}

bool CAudioCapture::GetNextPacket(double* outData, int sampleCount)
{
    if (m_bufs.size() == 0)
        return false;
    
    MINIBUFFER buf;

    {
        CLockSRW lock(m_lock);
        buf = m_bufs.front();
        m_bufs.pop_front();
    }

    int samplesAvail = buf.dataUseLen / sizeof(short);
    short* rawData = (short*)buf.data;

    assert(m_packetSample == sampleCount);

    for (int i = 0; i < __min(samplesAvail, sampleCount); i++)
    {
        *(outData++) = *(rawData++) / 32768.0f;
    }
    
    free(buf.data);
    return true;
}

void CAudioCapture::WaitForNextPacket()
{
    if (!m_data->capture)
    {
        Sleep(0);
        return;
    }
    WaitForSingleObject(m_event, 1000);
}

void CAudioCapture::_OnData(void* newData, int dataLength)
{
    if (!newData)
        return;

    uint8_t* newDataPtr = (uint8_t*)newData;

    CLockSRW lock(m_lock);

    while (dataLength > 0)
    {
        if (!m_lastBuf.dataMaxLen)
        {
            unsigned int len = m_packetSample * sizeof(short);
            m_lastBuf.dataMaxLen = len;
            m_lastBuf.dataUseLen = 0;
            m_lastBuf.data = malloc(m_lastBuf.dataMaxLen);
        }

        int freeSpace = m_lastBuf.dataMaxLen - m_lastBuf.dataUseLen;
        int lenCopy = __min(freeSpace, dataLength);

        assert(freeSpace > 0);

        memcpy((uint8_t*)m_lastBuf.data + m_lastBuf.dataUseLen, newDataPtr, lenCopy);

        newDataPtr += lenCopy; //!@! WASBUGGY
        m_lastBuf.dataUseLen += lenCopy;
        dataLength -= lenCopy;

        if (m_lastBuf.dataUseLen >= m_lastBuf.dataMaxLen)
        {
            //Buffer Full: Store It and raise event
            m_bufs.push_back(*(MINIBUFFER*)&m_lastBuf);
            m_lastBuf.data = NULL;
            m_lastBuf.dataMaxLen = 0;
            m_lastBuf.dataUseLen = 0;
            SetEvent(m_event);
        }
    }
}

//=====================================================================//
// end of namespace
	}
//=====================================================================//