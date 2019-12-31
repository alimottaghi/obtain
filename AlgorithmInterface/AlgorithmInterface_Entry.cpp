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

//=====================================================//
// This module runs the extra thread that the algorithm
//   (Beat Tracking DSP) runs on.
// The DSP does not occur on the graphical thread.
//=====================================================//

#include "pch.h"
#include "AlgorithmInterface.h"
extern "C"
{
    #include "./Beat_Tracking_Stem_ert_rtw/Beat_Tracking_Stem.h"
}

//The entry point is defined in ert_main.cpp
extern int BeatTrackingMain();

//Thread Handle
HANDLE g_BeatThread = nullptr;

//Thread Entry Point
DWORD WINAPI BeatTracker_Thread(void* Params)
{
    return (DWORD)BeatTrackingMain();
}

//Start/Stop
EXT_C void BT_Algorithm_Start()
{
    if (g_BeatThread)
        return;
    g_BeatThread = CreateThread(nullptr, 65536,
        BeatTracker_Thread, nullptr, 0, nullptr);
}

EXT_C void BT_Algorithm_Stop()
{
    if (!g_BeatThread)
        return;

    //Trigger end of simulation
    rtmSetStopRequested(Beat_Tracking_Stem_M, true);
    
    //Wait for exit
    //Note: No TerminateThread on UWP apps!
    WaitForSingleObject(g_BeatThread, 1000);
    CloseHandle(g_BeatThread);
    g_BeatThread = nullptr;
}

EXT_C void BT_Idleness()
{
    //Algorithm is idle (probably no audio).
    Sleep(10);
}

