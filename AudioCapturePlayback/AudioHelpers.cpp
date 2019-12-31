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

#include "pch.h"
#include "AudioHelpers.h"

//The following pragmas add the media foundation linker LIBs.
//  These are required for the operation of AudioDecoder.
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Mfcore.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")

namespace BeatTrackingAudio {

//====================================================//
// class CAudioBuffer
//====================================================//
CAudioBuffer::CAudioBuffer()
{
	m_State = AUDIO_BUFFER_STATE::Unused;
	m_Data = nullptr;
	m_DataLength = 0;
	//ZeroMemory(&m_WaveHeader, sizeof(m_WaveHeader));
}

CAudioBuffer::~CAudioBuffer()
{
	Safe_Free(m_Data);
	m_DataLength = 0;
	m_State = AUDIO_BUFFER_STATE::Unused;
}

AUDIO_BUFFER_STATE CAudioBuffer::GetState() const
{
	return m_State;
}

void* CAudioBuffer::GetData() const
{
	return m_Data;
}

DWORD CAudioBuffer::GetDataLength() const
{
	return m_DataLength;
}

} //end of namespace
