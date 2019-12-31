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

#pragma once

#ifdef __cplusplus
#define EXT_C extern "C"
#else
#define EXT_C extern
#endif

//==========================================================//
// Functions that help interfacing the beat tracking
//   algorithm and the audio subsystem
//==========================================================//
EXT_C void BT_Audio_Init();
EXT_C void BT_Audio_Uninit();
EXT_C void BT_Audio_ChangeSource(const wchar_t* FileName, const wchar_t* FriendlyName);
EXT_C int  BT_Audio_RestartRequired();
EXT_C int  BT_Audio_FromMicrophone();
EXT_C int  BT_Audio_NoMoreAudio();
EXT_C void BT_Audio_SinkSamplesToPlay(const double* outSamples, int SampleCount);
EXT_C void BT_Audio_SourceSamplesToDSP(double* outSamples, int SampleCount);
EXT_C void BT_Audio_AfterDSPStep();
EXT_C const wchar_t* BT_Audio_GetSourceName();
EXT_C void BT_Audio_SetReadSkipFactor(int Factor);
EXT_C void BT_Audio_SetPlaybackOptions(float Volume, float FreqFactor);

#define BT_AUDIO_SOURCE_CAPTURE     L"*cap"

//==========================================================//
// Our Own Defined Logging/Plotting System
//==========================================================//

#include "GlobalGraphs.h"

//The ultimate purpose: Beat Detection
EXT_C void BT_Beat_SetPower(double timeOfRecord, float BeatPower);

//==========================================================//
// Algorithm Thread
//==========================================================//
EXT_C void BT_Algorithm_Start();
EXT_C void BT_Algorithm_Stop();
EXT_C void BT_Idleness();

//==========================================================//
// There are stuff from the Simulink Generated code that
//   we don't want such as MAT-file logging and MATLAB
//   libaries.
//==========================================================//
#include "Port_Unified.h"

