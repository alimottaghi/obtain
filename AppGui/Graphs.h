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

//=================================
//The plotting features...
//=================================

#pragma once

#define BT_GRAPH_LANE_NUM 2
#define BT_GRAPH_NUM      2
#define GRAPH_MAX_PTS     12000

class CGraph
{
public:
    D2D_COLOR_F Color[BT_GRAPH_LANE_NUM];
    bool ContinousDraw[BT_GRAPH_LANE_NUM];

    CGraph();
    ~CGraph();

    void SetTime(double t);
    void AddValue(int lane, double v);
    void Reset(double startTime = 0);
    void Draw(ID2D1SolidColorBrush* br, ID2D1DeviceContext* ctx, float w, float h);
    int MaxPoints();

private:
    struct DATA
    {
        double t, x;
    };
    std::vector<DATA> pts[BT_GRAPH_LANE_NUM];
    double time = 0;
    double tmin = 0;
    double tmax = 0;
    double ymax[BT_GRAPH_LANE_NUM];
    double ymin[BT_GRAPH_LANE_NUM];
    SRWLOCK lock;
};

//Global Graph Functions are defined in AlgorithmInterface.h
#include "GlobalGraphs.h"

//Define Graphs
extern CGraph g_Graph[BT_GRAPH_NUM];
