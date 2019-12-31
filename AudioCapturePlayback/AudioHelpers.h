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

#include "Helper.h"

namespace BeatTrackingAudio
{

    //Some class names
    class CAudioCapture;
    class CAudioPlayback;

    //Status of an audio buffer
    enum class AUDIO_BUFFER_STATE : int
    {
	    Unused,
	    Filling,
	    Full,
	    Sinking,
    };

    //Class for an audio buffer.
    class CAudioBuffer : public CReference
    {
    public:
	    virtual AUDIO_BUFFER_STATE GetState() const;
	    virtual void* GetData() const;
	    virtual DWORD GetDataLength() const;

    private:
	    CAudioBuffer();
	    virtual ~CAudioBuffer();

	    friend CAudioCapture;
	    friend CAudioPlayback;

	    AUDIO_BUFFER_STATE		m_State;
	    void*					m_Data;
	    DWORD					m_DataLength;
    };

    //Audio Capture/Playback State
    enum class AUDIO_INOUT_STATE : int
    {
	    Stopped,
	    Capturing,
	    Playing,
	    Error,
    };

} //end of namespace
