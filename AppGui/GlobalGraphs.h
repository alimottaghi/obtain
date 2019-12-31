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

//=======================================================//
// Global Graph functions to let graphs (plots) to be accessed
//   from any part of the app, including the DSP.
//=======================================================//

#pragma once

#ifndef EXT_C
#ifdef __cplusplus
#define EXT_C extern "C"
#else
#define EXT_C extern
#endif
#endif

//DVs (Debug Variables)
#define GDV_BPM         0
#define GDV_OSS         1
#define GDV_FEEDDELTA   2
#define GDV_GTIME       3

EXT_C double g_DV[8];

//Plotting
EXT_C void BT_GlobalGraph_Add(int PlotNumber, int Lane, double val);
EXT_C void BT_GlobalGraph_SetTime(double time);
EXT_C void BT_GlobalGraph_CutTime();
EXT_C void BT_GlobalGraph_Reset();
EXT_C void BT_GlobalGraph_Setup();

