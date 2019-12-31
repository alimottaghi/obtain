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

#include <wrl.h>
#include <wrl/client.h>
#include <ppltasks.h>

#include "collection.h"

// Windows
// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//DirectX
#include <xaudio2.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>

#include <wincodec.h>

#include <DirectXColors.h>
#include <DirectXMath.h>

//C/C++
#include <string>
#include <list>
#include <vector>
#include <list>
#include <memory>

#include <math.h>
#include <stdio.h>
#include <agile.h>
#include <concrt.h>
#include <time.h>
#include <stdlib.h>

