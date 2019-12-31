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

//===========================================================//
// Facilities to interface the GUI App and the Beat
//   Tracking algorithm.
//===========================================================//

#include "pch.h"
#include "AlgorithmInterface.h"
#include "AudioCapturePlayback/AudioCapture.h"
#include "AudioCapturePlayback/AudioPlayback.h"
#include "AudioDecoder/audiodecodermediafoundation.h"

//Length of temporary audio buffer
#define AUD_TEMPORARY_LEN_MAX 16384

//Maximum number of microphone audio (capture) source frames unread
#define AUD_SOURCECAP_MAX 3

//Minimum number of microphone audio (capture) source frames to be
//  available before passing to DSP
#define AUD_SOURCECAP_MIN 1

//Maximum number of audio frames that has been sourced
//  to DSP and Playback, but are not yet finished playing.
#define AUD_FEEDDELTA_MAX 4

//Minimum number of audio frames required to begin playback.
#define AUD_FEEDDELTA_MIN 2

//Data Audio Facility
struct AUDIO_FOR_BEAT_TRACKER
{
    //Number of frames to read. Standard is 1.
    //  Any bigger value means to skip data so to simulate a 'fast forward'.
    int readSkipFactor = 1;

    //Name of the source file.
    std::wstring fileName = L"";
    std::wstring fileNameFriendly = L"";

    //An event is used to synchronize actions of the threads.
    HANDLE playbackSyncEvent = nullptr;

    //Sources (Capture/File) and Sink (Playback)
    BeatTrackingAudio::CAudioCapture* sourceCap = nullptr;
    AudioDecoderMediaFoundation* source = nullptr;
    BeatTrackingAudio::CAudioPlayback* sink = nullptr;

    //Flags
    bool sourceIsMike = false;      //Source is microphone (capture)
    bool hasEnded = false;          //Playback has ended
    bool mustReset = false;         //Audio system must reset

    //Temporary buffer
    short audio_buf_i[AUD_TEMPORARY_LEN_MAX];
    float audio_buf_f[AUD_TEMPORARY_LEN_MAX * 2];

    //Number of frames DSPed and queued for playback but
    //  not yet played.
    volatile unsigned int feedDelta = 0;

    //Sample callback for Audio Playback
    class CB : public CSampleCallback
    {
    public:
        AUDIO_FOR_BEAT_TRACKER* parent;

        virtual void OnSampleReady(void* Provider) {}
        virtual void OnSampleFinish(void* Provider)
        {
            InterlockedDecrement(&parent->feedDelta);
            SetEvent(parent->playbackSyncEvent);
        }
    } cb;

    void Zero()
    {
        playbackSyncEvent = nullptr;
        source = nullptr;
        sink = nullptr;
        hasEnded = false;
        mustReset = true;
        feedDelta = 0;
    }

    bool CanSource()
    {
        //Check if conditions are met to source audio
        if (mustReset ||
            hasEnded ||
            (source == NULL && !sourceIsMike) ||
            (sourceCap == NULL && sourceIsMike))
        {
            return false;
        }
        return true;
    }
};

AUDIO_FOR_BEAT_TRACKER g_BTAudio;

//======================================================================//
// Functions
//======================================================================//

//Source audio frame to be DSPed
EXT_C void BT_Audio_SourceSamplesToDSP(double* outSamples, int SampleCount)
{
    if (!outSamples || SampleCount <= 0)
        return;

    memset(outSamples, 0, SampleCount * sizeof(*outSamples));

    if (g_BTAudio.CanSource() == false)
    {
        //!#! Must not get here: DSP must first check BT_Audio_NoMoreAudio()
        //  and if it is the case, it must not called this function.
        Sleep(1);
        return;
    }

    //!#! Please notice:
    //Method of synchronization of DSP to achieve real-time-ness.
    //1. If sourcing from microphone, the DSP must wait until enough number
    //  of audio frames are available from the microphone.
    //  If not enough frames are ready, DSP will be blocked **HERE**
    //2. If sourcing from an audio file, the DSP will have to synchronize
    //  to the playback. The DSP can continue when no more than AUD_FEEDDELTA_MAX
    //  frames are sent to playback.
    //  If more than AUD_FEEDDELTA_MAX frames are waiting to be played,
    //  DSP is not blocked here but in **BT_Audio_SinkSamplesToPlay()**.
    //3. Generally, there must be a system precise timer to synchronize the
    //  DSP with, but we simply avoided that to maintain simplicity.

    if (g_BTAudio.sourceIsMike)
    {
        //Source audio from microphone
        while (g_BTAudio.sourceCap->CountPackets() > AUD_SOURCECAP_MAX)
        {
            //Discard
            g_BTAudio.sourceCap->DiscardNextPacket();
        }

        while (g_BTAudio.sourceCap->CountPackets() < AUD_SOURCECAP_MIN)
        {
            //Wait for more
            g_BTAudio.sourceCap->WaitForNextPacket();
        }
        
        //!#! TBC: GetNextPacket() requires SampleCount to be 1024.
        g_BTAudio.sourceCap->GetNextPacket(outSamples, SampleCount);
    }
    else
    {
        //Source audio from file. Samples must be converted from
        //  integer format (PCM -- usually 16-bit) to floating point.
        //  Luckily, the external library we use (libaudiodecoder)
        //  automatically converts the data to floating point.
        //Input file may have 1 or 2 channels.
        //  If it is stereo, read double number of samples and then
        //  get average to convert to mono.
        int remain = SampleCount;
        double* ptrDst = outSamples;
        int numBlocksToRead = __max(g_BTAudio.readSkipFactor, 1);

        while (remain)
        {
            int readLen = remain;
            if (readLen > AUD_TEMPORARY_LEN_MAX)
                readLen = AUD_TEMPORARY_LEN_MAX;

            int readLenActual = readLen;
            if (g_BTAudio.source->channels() == 2)
                readLenActual *= 2;

            //The 'readSkipFactor' parameter may be used to read more than one block
            //  and discard all but last one in order to 'fast forward'.
            for (int j = 1; j <= numBlocksToRead; j++)
                g_BTAudio.source->read(readLenActual, g_BTAudio.audio_buf_f, g_BTAudio.hasEnded);

            //Convert from stereo to mono needed?
            float* tempBuf = g_BTAudio.audio_buf_f;
            if (g_BTAudio.source->channels() == 2)
            {
                for (int i = 0; i < readLen; i++)
                {
                    float x = (*tempBuf + *(tempBuf+1)) / 2;
                    *(ptrDst++) = x;
                    tempBuf += 2;
                }
            }
            else
            {
                //Not conversion required
                //!#! Could use memcpy.
                for (int i = 0; i < readLen; i++)
                {
                    *(ptrDst++) = *(tempBuf++);
                }
            }

            remain -= readLen;
        }
    }
}

//Sink audio samples from Beat Tracking DSP for playback.
EXT_C void BT_Audio_SinkSamplesToPlay(const double* outSamples, int SampleCount)
{
    //!#! Please notice: Check BT_Audio_SourceSamplesToDSP() for
    //  some important notes.

    if (g_BTAudio.mustReset)
    {
        return;
    }

    if (!g_BTAudio.sourceIsMike)
    {
        //As mentioned in the note before, in the case that audio source is a
        //  file, DSP must be synched with playback, here:
        while (g_BTAudio.feedDelta >= AUD_FEEDDELTA_MAX)
        {
            //Wait for a finished block to avoid overload and
            //  also to synchronize DSP for real-time-ness.
            WaitForSingleObject(g_BTAudio.playbackSyncEvent, INFINITE);
        }
    }

    InterlockedIncrement(&g_BTAudio.feedDelta);

    //Send to Playback
    //Audio must be converted from double to short. (Floating point to integer).
    //Use the temporary buffer to do this.
    int remain = SampleCount;
    while (remain)
    {
        int writeLen = remain;
        if (writeLen > AUD_TEMPORARY_LEN_MAX)
            writeLen = AUD_TEMPORARY_LEN_MAX;

        //The channel count is OK (both mono). But the output format must be
        //  converted from float to short.
        for (int i = 0; i < writeLen; i++)
        {
            g_BTAudio.audio_buf_i[i] = (short)(outSamples[i] * 32767);
        }

        remain -= writeLen;

        g_BTAudio.sink->SubmitFrame(writeLen, g_BTAudio.audio_buf_i);
    }
}

//Audio Initialization function
EXT_C void BT_Audio_Init()
{
    //Uninit the Audio system: There might be already some object created
    //  that must be destroyed first.
    BT_Audio_Uninit();

    //Also reset the graphs
    BT_GlobalGraph_Reset();

    //Setup g_BTAudio
    g_BTAudio.sourceIsMike =
        g_BTAudio.fileName == L"" || g_BTAudio.fileName == BT_AUDIO_SOURCE_CAPTURE;
    g_BTAudio.cb.parent = &g_BTAudio;
    g_BTAudio.playbackSyncEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    //Open audio source
    if (g_BTAudio.sourceIsMike == false)
    {
        //File Mode
        if (wcscmp(g_BTAudio.fileName.c_str(), L"-") == 0)
        {
            //Open nothing
            g_BTAudio.source = NULL;
            g_BTAudio.hasEnded = true;
        }
        else
        {
            g_BTAudio.source = new AudioDecoderMediaFoundation(g_BTAudio.fileName);
            if (g_BTAudio.source->open() != AUDIODECODER_OK)
            {
                delete g_BTAudio.source;
                g_BTAudio.source = NULL;
                g_BTAudio.hasEnded = true;
            }
        }
    }
    else
    {
        //Microphone (Capture) mode
        g_BTAudio.sourceCap = BeatTrackingAudio::CAudioCapture::New();
        if (!g_BTAudio.sourceCap->ConnectToAudioCapturer(44100, 1, 16, 44100 * 20, 1024))
        {
            delete g_BTAudio.sourceCap;
            g_BTAudio.sourceCap = NULL;
            g_BTAudio.hasEnded = true;
        }

        g_BTAudio.fileNameFriendly = L"<< MICROPHONE >>";
    }

    //The 'mustReset' flag is set when the DSP has to restart and the
    //  audio system must be reinitialized. Clear the flag for now.
    g_BTAudio.mustReset = false;

    //Open audio sink (for playback)
    g_BTAudio.sink = new BeatTrackingAudio::CAudioPlayback();
    if (!g_BTAudio.sink)
    {
        g_BTAudio.hasEnded = true;
    }
    else
    {
        g_BTAudio.sink->SetSampleFinishCallback(&g_BTAudio.cb);
        g_BTAudio.sink->SetMinRequiredBuffersBeforeSubmit(AUD_FEEDDELTA_MIN);

        //!#! Assuming the following call won't fail.
        g_BTAudio.sink->ConnectToDevice(0, 44100, AUD_FEEDDELTA_MAX, 1);
    }
}

extern void BeatTrackerGUI_Beat(double time, float beat);

EXT_C void BT_Beat_SetPower(double timeOfRecord, float BeatPower)
{
    //Call GUI
    BeatTrackerGUI_Beat(timeOfRecord, BeatPower);
}

//Uninit audio
EXT_C void BT_Audio_Uninit()
{
    g_BTAudio.mustReset = true;

    if (g_BTAudio.source)
        delete g_BTAudio.source;
    g_BTAudio.source = nullptr;

    if (g_BTAudio.sourceCap)
        delete g_BTAudio.sourceCap;
    g_BTAudio.sourceCap = nullptr;

    if (g_BTAudio.sink)
        delete g_BTAudio.sink;
    g_BTAudio.sink = nullptr;

    if (g_BTAudio.playbackSyncEvent)
        CloseHandle(g_BTAudio.playbackSyncEvent);
    g_BTAudio.playbackSyncEvent = nullptr;

    g_BTAudio.Zero();
}

//This function will be called by GUI to change the audio source
//  DSP must be reset then.
EXT_C void BT_Audio_ChangeSource(const wchar_t* FileName, const wchar_t* FriendlyName)
{
    g_BTAudio.mustReset = true;
    g_BTAudio.fileName = FileName;
    g_BTAudio.fileNameFriendly = FriendlyName;
}

EXT_C int BT_Audio_RestartRequired()
{
    return (int)g_BTAudio.mustReset;
}

EXT_C int  BT_Audio_FromMicrophone()
{
    return (int)g_BTAudio.sourceIsMike;
}

EXT_C int BT_Audio_NoMoreAudio()
{
    return (int)(!g_BTAudio.CanSource());
}

//Some extra affairs at the end of every DSP step. Not much.
EXT_C void BT_Audio_AfterDSPStep()
{
    g_DV[GDV_FEEDDELTA] = g_BTAudio.feedDelta;
}

const wchar_t* BT_Audio_GetSourceName()
{
    return g_BTAudio.fileNameFriendly.c_str();
}

EXT_C void BT_Audio_SetReadSkipFactor(int Factor)
{
    g_BTAudio.readSkipFactor = Factor;
}

EXT_C void BT_Audio_SetPlaybackOptions(float Volume, float FreqFactor)
{
    if (g_BTAudio.sink)
        g_BTAudio.sink->SetPlaybackVolume(Volume, FreqFactor);
}


