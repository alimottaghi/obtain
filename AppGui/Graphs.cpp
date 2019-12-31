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

//=========================================================//
// The Graph (Plot) Module
//   The graph object is defined here and two instances
//   of it are created and set up.
//=========================================================//

#include "pch.h"
#include "DirectXHelper.h"
#include "Helper.h"
#include "Graphs.h"
#include "AlgorithmInterface.h"

typedef D2D1_POINT_2F PT;

//Debug Vars are also defined here
extern "C" double g_DV[8] = {};

//Define Graphs
CGraph g_Graph[2];
double g_GraphTime = 0;

//========================================
//Global Graph Functions
//========================================
EXT_C void BT_GlobalGraph_Setup()
{
    g_Graph[0].Color[0] = D2D1::ColorF::ColorF(D2D1::ColorF::Red);
    g_Graph[0].ContinousDraw[0] = false;
    g_Graph[0].Color[1] = D2D1::ColorF::ColorF(D2D1::ColorF::White);
    g_Graph[0].ContinousDraw[1] = true;
    g_Graph[1].Color[0] = D2D1::ColorF::ColorF(D2D1::ColorF::White);
    g_Graph[1].ContinousDraw[0] = true;
    g_Graph[1].Color[1] = D2D1::ColorF::ColorF(D2D1::ColorF::White);
    g_Graph[1].ContinousDraw[1] = true;
}

EXT_C void BT_GlobalGraph_Add(int Idx, int Lane, double val)
{
    if (Idx >= 0 && Idx < BT_GRAPH_NUM)
        g_Graph[Idx].AddValue(Lane, val);
}

EXT_C void BT_GlobalGraph_Reset()
{
    g_GraphTime = 0;
    for (int i = 0; i < BT_GRAPH_NUM; i++)
        g_Graph[i].Reset(g_GraphTime);
}

EXT_C void BT_GlobalGraph_SetTime(double T)
{
    g_GraphTime = T;
    for (int i = 0; i < BT_GRAPH_NUM; i++)
        g_Graph[i].SetTime(g_GraphTime);
    g_DV[GDV_GTIME] = T;
}

EXT_C void BT_GlobalGraph_CutTime()
{
    //Reset graphs but do not reset time. Record from now on.
    for (int i = 0; i < BT_GRAPH_NUM; i++)
        g_Graph[i].Reset(g_GraphTime);
}

//========================================
// Graph Class
//========================================
CGraph::CGraph()
{
    lock = SRWLOCK_INIT;
    for (int L = 0; L < BT_GRAPH_LANE_NUM; L++)
    {
        Color[L] = D2D1::ColorF::ColorF(D2D1::ColorF::White);
        ContinousDraw[L] = true;
    }
    Reset();
}

CGraph::~CGraph()
{
    for (int L = 0; L < BT_GRAPH_LANE_NUM; L++)
        pts[L].clear();
}

void CGraph::SetTime(double t)
{
    CLockSRW lock(this->lock);

    //No rewinding
    if (t < tmin)
        t = tmin;
    if (t < time)
        t = time;

    //Set time
    time = t;

    //Extend the time range if needed
    if (t > tmax)
        tmax = tmin + (t - tmin) * 1.5;
}

void CGraph::AddValue(int lane, double v)
{
    bool resetGraphs = false;

    CLockSRW lock(this->lock);

    if (v > ymax[lane])
        ymax[lane] = v;
    if (v < ymin[lane])
        ymin[lane] = v;

    if (lane < BT_GRAPH_LANE_NUM && lane >= 0)
    {
        pts[lane].push_back(DATA{ time,v });

        if (pts[lane].size() > GRAPH_MAX_PTS)
        {
            //One of the lanes is too big. Cut all of graphs in the app.
            resetGraphs = true;
        }
    }

    lock.Unlock();

    if (resetGraphs)
    {
        BT_GlobalGraph_CutTime();
    }
}

void CGraph::Reset(double startTime)
{
    CLockSRW lock(this->lock);

    for (int L = 0; L < BT_GRAPH_LANE_NUM; L++)
        pts[L].clear();
    time = startTime;
    for (int L = 0; L < BT_GRAPH_LANE_NUM; L++)
    {
        ymax[L] = 1e-6;
        ymin[L] = 0;
    }
    tmin = startTime;
    tmax = startTime + 5;
}

void CGraph::Draw(ID2D1SolidColorBrush* br, ID2D1DeviceContext* ctx, float w, float h)
{
    D2D1_COLOR_F c;

    //Aliased mode for increased speed
    ctx->SetAntialiasMode(D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_ALIASED);

    //Draw a background
    br->SetColor(D2D1_COLOR_F{0.4f,0.8f,1.0f,0.5f});
    ctx->FillRectangle(D2D1_RECT_F{ w, h }, br);

    //Draw simple axis
    c = D2D1::ColorF::ColorF(D2D1::ColorF::Black);
    br->SetColor(c);

    ctx->DrawLine(
        PT{ 0, 0 },
        PT{ 0, h },
        br, 3.0F
    );

    ctx->DrawLine(
        PT{ 0, h },
        PT{ w, h },
        br, 3.0F
    );    

    if (time <= 0)
    {
        //Nothing yet
        return;
    }

    //Draw all lanes
    for (int L = 0; L < BT_GRAPH_LANE_NUM; L++)
    {
        c = Color[L];
        br->SetColor(c);

        //Lock, read data, release
        std::vector<DATA> v1;
        {
            CLockSRW lock(this->lock);
            v1 = pts[L];
        }

        bool first = true;
        D2D_POINT_2F a1 = { 0, h }, a2;
        int count = v1.size();
        int skip = 1;
        int i = 0;
        double ysum = ymin[L];

        if (ContinousDraw[L])
        {
            //Reduce points to increase performance.
            skip = count / 1000 + 1;
        }

        for (auto p : v1)
        {
            i++;
            ysum = __max(ysum, p.x);
            if (i < skip)
            {
                continue;
            }
            else
            {
                i = 0;
            }

            if (!ContinousDraw[L] && (p.x - ymin[L]) < 0.001f * (ymax[L] - ymin[L]))
                continue;

            //ysum /= skip;
            a2.x = (float)((p.t - tmin) / (tmax - tmin) * w);
            a2.y = (float)((1 - (ysum - ymin[L]) / (ymax[L] - ymin[L])) * h);
            ysum = 0;

            if (!(a2.y < 0 || a2.y > h ||
                a1.y < 0 || a1.y > h))
            {
                if (ContinousDraw[L])
                {
                    //Draw a line
                    ctx->DrawLine(a1, a2, br, 1);
                }
                else
                {
                    //Draw a stem
                    a1.x = a2.x;
                    a1.y = h;
                    ctx->DrawLine(a1, a2, br, 2);
                }
            }

            first = false;
            a1 = a2;
        }
    }
}

int CGraph::MaxPoints()
{
    return __max(pts[0].size(), pts[1].size());
}
