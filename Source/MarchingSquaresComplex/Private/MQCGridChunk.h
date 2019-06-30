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

class FMQCGridSurface;

class FMQCGridChunk
{
private:

    friend class FMQCMap;
    friend class FMQCStencil;

    class FAsyncTask
    {
        TFunction<void()> Task;
        TPromise<void>& Promise;
    public:
        FAsyncTask(const TFunction<void()>& InTask, TPromise<void>& InPromise)
            : Task(InTask)
            , Promise(InPromise)
        {
        }
        static FORCEINLINE TStatId GetStatId()
        {
            RETURN_QUICK_DECLARE_CYCLE_STAT(FMQCGridChunk_AsyncTask, STATGROUP_TaskGraphTasks);
        }
        static FORCEINLINE ENamedThreads::Type GetDesiredThread()
        {
            return ENamedThreads::AnyHiPriThreadHiPriTask;
        }
        static FORCEINLINE ESubsequentsMode::Type GetSubsequentsMode() 
        { 
            return ESubsequentsMode::FireAndForget; 
        }
        void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
        {
            Task();
            Promise.SetValue();
        }
    };

    TFuture<void> OutstandingTask;

    FMQCCell cell;

    TArray<FMQCGridRenderer> Renderers;
    TArray<FMQCVoxel> voxels;

    int32 mapSize;
    int32 voxelResolution;

    FMQCVoxel dummyX;
    FMQCVoxel dummyY;
    FMQCVoxel dummyT;

    void EnqueueTask(const TFunction<void()>& Task);

    void CreateRenderers(const FMQCChunkConfig& Config);

    void TriangulateInternal();
    void SetStatesInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetCrossingsInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetMaterialsInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);

    // Triangulation Functions

    void FillFirstRowCache();
    void CacheFirstCorner(const FMQCVoxel& voxel);
    void CacheNextEdgeAndCorner(int32 i, const FMQCVoxel& xMin, const FMQCVoxel& xMax);
    void CacheNextMiddleEdge(const FMQCVoxel& yMin, const FMQCVoxel& yMax);
    void SwapRowCaches();
    
    void TriangulateCellRows();
    void TriangulateGapRow();
    void TriangulateGapCell(int32 i);

    void TriangulateCell(int32 i, const FMQCVoxel& a, const FMQCVoxel& b, const FMQCVoxel& c, const FMQCVoxel& d);

public:

    FMQCGridChunk();
    ~FMQCGridChunk();

    FVector2D position;

    FMQCGridChunk* xNeighbor = nullptr;
    FMQCGridChunk* yNeighbor = nullptr;
    FMQCGridChunk* xyNeighbor = nullptr;

    void Initialize(const FMQCChunkConfig& Config);
    void ResetVoxels();

    void Triangulate();
    void SetStates(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetCrossings(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetMaterials(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);

    void TriangulateAsync();
    void SetStatesAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetCrossingsAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetMaterialsAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);

    FMQCGridSurface& GetSurface(int32 StateIndex);
    const FMQCGridSurface& GetSurface(int32 StateIndex) const;

    void GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const;
    FPMUMeshSection* GetMaterialSection(int32 StateIndex, const FMQCMaterialBlend& Material);

    FORCEINLINE FIntPoint GetOffsetId() const
    {
        return FIntPoint(position.X+.5f, position.Y+.5f);
    }

    FORCEINLINE bool HasRenderer(int32 RendererIndex) const
    {
        return Renderers.IsValidIndex(RendererIndex);
    }

    FORCEINLINE FPMUMeshSection* GetSurfaceSection(int32 StateIndex)
    {
        return HasRenderer(StateIndex)
            ? &Renderers[StateIndex].GetSurface().GetSurfaceSection()
            : nullptr;
    }

    FORCEINLINE FPMUMeshSection* GetExtrudeSection(int32 StateIndex)
    {
        return HasRenderer(StateIndex)
            ? &Renderers[StateIndex].GetSurface().GetExtrudeSection()
            : nullptr;
    }

    FORCEINLINE FPMUMeshSection* GetEdgeSection(int32 StateIndex)
    {
        return HasRenderer(StateIndex)
            ? &Renderers[StateIndex].GetSurface().GetEdgeSection()
            : nullptr;
    }

    FORCEINLINE int32 AppendEdgeSyncData(int32 StateIndex, TArray<FEdgeSyncData>& OutSyncData) const
    {
        if (HasRenderer(StateIndex))
        {
            int32 StartIndex = OutSyncData.Num();
            Renderers[StateIndex].GetSurface().AppendEdgeSyncData(OutSyncData);
            return StartIndex;
        }
        else
        {
            return -1;
        }
    }

    FORCEINLINE void RemapEdgeUVs(int32 StateIndex, int32 EdgeListId, float UVStart, float UVEnd)
    {
        if (HasRenderer(StateIndex))
        {
            Renderers[StateIndex].GetSurface().RemapEdgeUVs(EdgeListId, UVStart, UVEnd);
        }
    }

    FORCEINLINE void WaitForAsyncTask()
    {
        if (OutstandingTask.IsValid())
        {
            OutstandingTask.Wait();
        }
    }

private:

    FORCEINLINE void CreateVoxel(int32 i, int32 x, int32 y)
    {
        voxels[i].Set(x, y);
    }
    
    FORCEINLINE void FillA(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillA(cell, f);
        }
    }

    FORCEINLINE void FillB(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            Renderers[cell.b.voxelState].FillB(cell, f);
        }
    }
    
    FORCEINLINE void FillC(const FMQCFeaturePoint& f)
    {
        if (cell.c.IsFilled())
        {
            Renderers[cell.c.voxelState].FillC(cell, f);
        }
    }
    
    FORCEINLINE void FillD(const FMQCFeaturePoint& f)
    {
        if (cell.d.IsFilled())
        {
            Renderers[cell.d.voxelState].FillD(cell, f);
        }
    }

    FORCEINLINE void FillABC(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillABC(cell, f);
        }
    }
    
    FORCEINLINE void FillABD(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillABD(cell, f);
        }
    }
    
    FORCEINLINE void FillACD(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillACD(cell, f);
        }
    }
    
    FORCEINLINE void FillBCD(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            Renderers[cell.b.voxelState].FillBCD(cell, f);
        }
    }

    FORCEINLINE void FillAB(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillAB(cell, f);
        }
    }
    
    FORCEINLINE void FillAC(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillAC(cell, f);
        }
    }
    
    FORCEINLINE void FillBD(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            Renderers[cell.b.voxelState].FillBD(cell, f);
        }
    }
    
    FORCEINLINE void FillCD(const FMQCFeaturePoint& f)
    {
        if (cell.c.IsFilled())
        {
            Renderers[cell.c.voxelState].FillCD(cell, f);
        }
    }

    FORCEINLINE void FillADToB(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillADToB(cell, f);
        }
    }
    
    FORCEINLINE void FillADToC(const FMQCFeaturePoint& f)
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillADToC(cell, f);
        }
    }
    
    FORCEINLINE void FillBCToA(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            Renderers[cell.b.voxelState].FillBCToA(cell, f);
        }
    }
    
    FORCEINLINE void FillBCToD(const FMQCFeaturePoint& f)
    {
        if (cell.b.IsFilled())
        {
            Renderers[cell.b.voxelState].FillBCToD(cell, f);
        }
    }

    FORCEINLINE void FillABCD()
    {
        if (cell.a.IsFilled())
        {
            Renderers[cell.a.voxelState].FillABCD(cell);
        }
    }

    FORCEINLINE void FillJoinedCorners(
        const FMQCFeaturePoint& fA,
        const FMQCFeaturePoint& fB,
        const FMQCFeaturePoint& fC,
        const FMQCFeaturePoint& fD
        )
    {
        FMQCFeaturePoint point = cell.GetFeatureAverage(fA, fB, fC, fD);
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
