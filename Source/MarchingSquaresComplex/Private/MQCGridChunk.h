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
#include "MQCVoxelTypes.h"
#include "MQCGeometryTypes.h"

class FMQCGridSurface;
class FMQCStencil;

class FMQCGridChunk
{
private:

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

    TIndirectArray<FMQCGridSurface> Surfaces;
    TArray<FMQCVoxel> Voxels;

    FIntPoint Position;
    FIntPoint BoundsMin;
    FIntPoint BoundsMax;

    int32 MapSize;
    int32 VoxelResolution;
    EMQCMaterialType MaterialType;

    const FMQCGridChunk* xNeighbor;
    const FMQCGridChunk* yNeighbor;
    const FMQCGridChunk* xyNeighbor;

    FMQCCell Cell;
    FMQCVoxel dummyX;
    FMQCVoxel dummyY;
    FMQCVoxel dummyT;

    void CreateSurfaces(const FMQCChunkConfig& Config);

    // -- Internal Triangulation Interface

    void TriangulateInternal();
    void SetStatesInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetCrossingsInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetMaterialsInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void EnqueueTask(const TFunction<void()>& Task);

    // -- Geometry Cache Functions

    void FillFirstRowCache();
    void SwapRowCaches();
    void CacheFirstCorner(const FMQCVoxel& voxel);
    void CacheNextEdgeAndCorner(int32 i, const FMQCVoxel& xMin, const FMQCVoxel& xMax);
    void CacheNextMiddleEdge(const FMQCVoxel& yMin, const FMQCVoxel& yMax);

    // -- Triangulation Functions
    
    void TriangulateCellRows();
    void TriangulateGapRow();
    void TriangulateGapCell(int32 i);
    void TriangulateCell(int32 i, const FMQCVoxel& a, const FMQCVoxel& b, const FMQCVoxel& c, const FMQCVoxel& d);

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

    // -- Fill Functions

    void FillA(const FMQCFeaturePoint& f);
    void FillB(const FMQCFeaturePoint& f);
    void FillC(const FMQCFeaturePoint& f);
    void FillD(const FMQCFeaturePoint& f);
    void FillABC(const FMQCFeaturePoint& f);
    void FillABD(const FMQCFeaturePoint& f);
    void FillACD(const FMQCFeaturePoint& f);
    void FillBCD(const FMQCFeaturePoint& f);
    void FillAB(const FMQCFeaturePoint& f);
    void FillAC(const FMQCFeaturePoint& f);
    void FillBD(const FMQCFeaturePoint& f);
    void FillCD(const FMQCFeaturePoint& f);
    void FillADToB(const FMQCFeaturePoint& f);
    void FillADToC(const FMQCFeaturePoint& f);
    void FillBCToA(const FMQCFeaturePoint& f);
    void FillBCToD(const FMQCFeaturePoint& f);
    void FillABCD();
    void FillJoinedCorners(
        const FMQCFeaturePoint& fA,
        const FMQCFeaturePoint& fB,
        const FMQCFeaturePoint& fC,
        const FMQCFeaturePoint& fD
        );

public:

    FMQCGridChunk();
    ~FMQCGridChunk();

    void Configure(const FMQCChunkConfig& Config);
    void ResetVoxels();

    void SetNeighbourX(const FMQCGridChunk* InNeighbour);
    void SetNeighbourY(const FMQCGridChunk* InNeighbour);
    void SetNeighbourXY(const FMQCGridChunk* InNeighbour);

    FORCEINLINE FIntPoint GetOffsetId() const
    {
        return Position;
    }

    FORCEINLINE bool IsPointOnChunk(const FIntPoint& Point) const
    {
        return (
            Point.X >= BoundsMin.X &&
            Point.Y >= BoundsMin.Y &&
            Point.X <= BoundsMax.X &&
            Point.Y <= BoundsMax.Y
            );
    }

    FORCEINLINE int32 GetVoxelResolution() const
    {
        return VoxelResolution;
    }

    FORCEINLINE bool HasSurface(int32 StateIndex) const
    {
        return Surfaces.IsValidIndex(StateIndex);
    }

    FORCEINLINE void WaitForAsyncTask()
    {
        if (OutstandingTask.IsValid())
        {
            OutstandingTask.Wait();
        }
    }

    FPMUMeshSection* GetSurfaceSection(int32 StateIndex);
    FPMUMeshSection* GetExtrudeSection(int32 StateIndex);
    FPMUMeshSection* GetSurfaceMaterialSection(int32 StateIndex, const FMQCMaterialBlend& Material);
    FPMUMeshSection* GetExtrudeMaterialSection(int32 StateIndex, const FMQCMaterialBlend& Material);

    int32 AppendEdgeSyncData(TArray<FMQCEdgeSyncData>& OutSyncData, int32 StateIndex) const;
    void GetEdgePoints(TArray<FMQCEdgePointData>& OutPointList, int32 StateIndex) const;
    void GetEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListIndex) const;
    void AppendConnectedEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListIndex) const;
    void GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const;

    void AddQuadFilter(const FIntPoint& Point, int32 StateIndex, bool bFilterExtrude);

    // Public Triangulation Interface

    void Triangulate();
    void SetStates(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetCrossings(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetMaterials(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);

    void TriangulateAsync();
    void SetStatesAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetCrossingsAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
    void SetMaterialsAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1);
};
