////////////////////////////////////////////////////////////////////////////////
//
// MIT License
// 
// Copyright (c) 2018-2019 Nuraga Wiswakarma
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
// 

#pragma once

#include "CoreMinimal.h"
#include "Mesh/PMUMeshTypes.h"
#include "MQCVoxel.h"
#include "MQCCell.h"
#include "MQCFeaturePoint.h"
#include "MQCGridRenderer.h"
#include "MQCGridSurface.h"
#include "MQCVoxelTypes.h"

class FMQCGridChunk
{
private:

    friend class FMQCMap;
    friend class FMQCStencil;

    FMQCCell cell;

    TArray<FMQCGridRenderer> renderers;
    TArray<FMQCVoxel> voxels;

    float gridSize;
    float voxelSize;
    int32 voxelResolution;

    FMQCVoxel dummyX;
    FMQCVoxel dummyY;
    FMQCVoxel dummyT;

    void CreateRenderers(const FMQCGridConfig& Config);
    void ResetVoxels();
    void Refresh();
    void Triangulate();
    void SetStates(const FMQCStencil& stencil, int32 xStart, int32 xEnd, int32 yStart, int32 yEnd);
    void SetCrossings(const FMQCStencil& stencil, int32 xStart, int32 xEnd, int32 yStart, int32 yEnd);

    void FillFirstRowCache();
    void CacheFirstCorner(const FMQCVoxel& voxel);
    void CacheNextEdgeAndCorner(int32 i, const FMQCVoxel& xMin, const FMQCVoxel& xMax);
    void CacheNextMiddleEdge(const FMQCVoxel& yMin, const FMQCVoxel& yMax);
    void SwapRowCaches();
    
    void TriangulateCellRows();
    void TriangulateGapRow();
    void TriangulateGapCell(int32 i);

    // Triangulation Functions

    void TriangulateCell(int32 i, const FMQCVoxel& a, const FMQCVoxel& b, const FMQCVoxel& c, const FMQCVoxel& d);

public:

    FVector2D position;

    FMQCGridChunk* xNeighbor = nullptr;
    FMQCGridChunk* yNeighbor = nullptr;
    FMQCGridChunk* xyNeighbor = nullptr;

    void Initialize(const FMQCGridConfig& Config);
    void CopyFrom(const FMQCGridChunk& Chunk);

    FORCEINLINE bool HasRenderer(int32 RendererIndex) const
    {
        return renderers.IsValidIndex(RendererIndex);
    }

    FORCEINLINE int32 GetVertexCount(int32 StateIndex) const
    {
        return HasRenderer(StateIndex)
            ? renderers[StateIndex].GetSurface().GetVertexCount()
            : 0;
    }

    FORCEINLINE const FPMUMeshSection* GetSection(int32 StateIndex) const
    {
        if (HasRenderer(StateIndex))
        {
            return &renderers[StateIndex].GetSurface().Section;
        }

        return nullptr;
    }

private:

    FORCEINLINE void CreateVoxel(int32 i, int32 x, int32 y)
    {
        voxels[i].Set(x, y, voxelSize);
    }
    
    FORCEINLINE void FillA(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillA(cell, f);
        }
    }

    FORCEINLINE void FillB(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            renderers[cell.b.state].FillB(cell, f);
        }
    }
    
    FORCEINLINE void FillC(const FMQCFeaturePoint& f)
    {
        if (cell.c.IsFilled())
        {
            renderers[cell.c.state].FillC(cell, f);
        }
    }
    
    FORCEINLINE void FillD(const FMQCFeaturePoint& f)
    {
        if (cell.d.IsFilled())
        {
            renderers[cell.d.state].FillD(cell, f);
        }
    }

    FORCEINLINE void FillABC(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillABC(cell, f);
        }
    }
    
    FORCEINLINE void FillABD(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillABD(cell, f);
        }
    }
    
    FORCEINLINE void FillACD(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillACD(cell, f);
        }
    }
    
    FORCEINLINE void FillBCD(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            renderers[cell.b.state].FillBCD(cell, f);
        }
    }

    FORCEINLINE void FillAB(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillAB(cell, f);
        }
    }
    
    FORCEINLINE void FillAC(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillAC(cell, f);
        }
    }
    
    FORCEINLINE void FillBD(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            renderers[cell.b.state].FillBD(cell, f);
        }
    }
    
    FORCEINLINE void FillCD(const FMQCFeaturePoint& f)
    {
        if (cell.c.IsFilled())
        {
            renderers[cell.c.state].FillCD(cell, f);
        }
    }

    FORCEINLINE void FillADToB(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillADToB(cell, f);
        }
    }
    
    FORCEINLINE void FillADToC(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillADToC(cell, f);
        }
    }
    
    FORCEINLINE void FillBCToA(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            renderers[cell.b.state].FillBCToA(cell, f);
        }
    }
    
    FORCEINLINE void FillBCToD(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            renderers[cell.b.state].FillBCToD(cell, f);
        }
    }

    FORCEINLINE void FillABCD()
    {
        if (cell.a.IsFilled())
        {
            renderers[cell.a.state].FillABCD(cell);
        }
    }

    FORCEINLINE void FillJoinedCorners(const FMQCFeaturePoint& fA, const FMQCFeaturePoint& fB, const FMQCFeaturePoint& fC, const FMQCFeaturePoint& fD)
    {
        
        FMQCFeaturePoint point = FMQCFeaturePoint::Average(fA, fB, fC, fD);
        if (!point.exists)
        {
            point.position = cell.GetAverageNESW();
            point.exists = true;
        }
        FillA(point);
        FillB(point);
        FillC(point);
        FillD(point);
    }

    // TRIANGULATION FUNCTIONS

    void Triangulate0000();
    void Triangulate0001();
    void Triangulate0010();
    void Triangulate0100();
    void Triangulate0111();
    void Triangulate0011();
    void Triangulate0101();
    void Triangulate0012();
    void Triangulate0102();
    void Triangulate0121();
    void Triangulate0122();
    void Triangulate0110();
    void Triangulate0112();
    void Triangulate0120();
    void Triangulate0123();
};
