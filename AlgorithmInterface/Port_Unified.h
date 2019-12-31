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

//======================================================================//
// The following are placeholders for MATLAB structures and functions
//   which we meant to omit on the porting of the code to the graphical
//   app. Logging infrastructure as well as ability to load MATLAB
//   libraries are not required.
//======================================================================//
#pragma once

#include "../Beat_Tracking_Stem_ert_rtw/rtwtypes.h"
#include "../Beat_Tracking_Stem_ert_rtw/builtin_typeid_types.h"

struct tagRTWLogSignalInfo
{
	int_T  d0;
	int_T* d1;
	int_T* d2;
	int_T* d3;
	boolean_T* d4;
	void** d5;
	int_T* d6;
	BuiltInDTypeId* d7;
	int_T* d8, *d9;
	const char_T ** d10;
	void* d11, *d12, *d13, *d14, *d15, *d16, *d17;
};
typedef struct tagRTWLogSignalInfo RTWLogSignalInfo;

struct tagRTWLogInfo
{
	int dummy;
	void* loggingInterval;
};
typedef struct tagRTWLogInfo RTWLogInfo;

struct tagStructLogVar
{
	int dummy;
};
typedef struct tagStructLogVar StructLogVar;

struct tagLogVar
{
	int dummy;
};
typedef struct tagLogVar LogVar;

#define round round2

#define GetErrorBuffer(...)			("")
#define LibOutputs_FromMMFile		(void)__noop
#define GetNullPointer()			(NULL)
#define rt_UpdateStructLogVar		(void)__noop
#define rt_UpdateLogVar				(void)__noop
#define LibUpdate_Audio				(void)__noop
#define rt_UpdateTXYLogVars			(void)__noop
#define rtliSetLogXSignalInfo		(void)__noop
#define rtliSetLogXSignalPtrs		(void)__noop
#define rtliSetLogT					(void)__noop
#define rtliSetLogX					(void)__noop
#define rtliSetLogXFinal			(void)__noop
#define rtliSetLogVarNameModifier	(void)__noop
#define rtliSetLogFormat			(void)__noop
#define rtliSetLogMaxRows			(void)__noop
#define rtliSetLogDecimation		(void)__noop
#define rtliSetLogY					(void)__noop
#define rtliSetLogYSignalInfo		(void)__noop
#define rtliSetLogYSignalPtrs		(void)__noop
#define rt_StartDataLoggingWithStartTime (void)__noop
#define CreateHostLibrary			(void)__noop
#define createAudioInfo				(void)__noop
#define createVideoInfo				(void)__noop
#define LibCreate_FromMMFile		(void)__noop
#define LibStart					(void)__noop
#define DestroyHostLibrary			(void)__noop
#define rt_CreateStructLogVar		(void*)__noop
#define rt_CreateLogVar				(void*)__noop
#define rt_CreateLogVar				(void*)__noop
#define LibCreate_Audio				(void)__noop
#define LibReset					(void)__noop
#define LibReset					(void)__noop
#define rt_StopDataLogging			(void)__noop
#define LibDestroy                  (void)__noop
#define LibTerminate                (void)__noop

#define NO_LOGVALDIMS				(-1)
