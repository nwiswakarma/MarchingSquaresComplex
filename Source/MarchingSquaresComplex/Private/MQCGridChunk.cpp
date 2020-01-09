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

#include "MQCGridChunk.h"
#include "MQCGridSurface.h"
#include "MQCStencil.h"

FMQCGridChunk::FMQCGridChunk()
    : xNeighbor(nullptr)
    , yNeighbor(nullptr)
    , xyNeighbor(nullptr)
{
}

FMQCGridChunk::~FMQCGridChunk()
{
    WaitForAsyncTask();
}

void FMQCGridChunk::Configure(const FMQCChunkConfig& Config)
{
    Position = Config.Position;
    MapSize = Config.MapSize;
    VoxelResolution = Config.VoxelResolution;
    MaterialType = Config.MaterialType;

    BoundsMin = Position;
    BoundsMax = Position+FIntPoint(VoxelResolution, VoxelResolution);

    Cell.sharpFeatureLimit = FMath::Cos(FMath::DegreesToRadians(Config.MaxFeatureAngle));
    Cell.parallelLimit     = FMath::Cos(FMath::DegreesToRadians(Config.MaxParallelAngle));

    Voxels.SetNumZeroed(VoxelResolution * VoxelResolution);
    
    for (int32 y=0, i=0; y<VoxelResolution; y++)
    for (int32 x=0     ; x<VoxelResolution; x++, i++)
    {
        Voxels[i].Set(x, y);
    }

    CreateSurfaces(Config);
}

void FMQCGridChunk::CreateSurfaces(const FMQCChunkConfig& GridConfig)
{
    // Construct renderer count

    const int32 StateCount = 1 + GridConfig.States.Num();

    for (int32 i=0; i<StateCount; ++i)
    {
        FMQCSurfaceConfig Config;
        Config.Position        = Position;
        Config.MapSize         = MapSize;
        Config.VoxelResolution = VoxelResolution;
        Config.ExtrusionHeight = GridConfig.ExtrusionHeight;
        Config.MaterialType    = GridConfig.MaterialType;

        if (i > 0)
        {
            int32 StateIndex = i-1;
            const FMQCSurfaceState& State(GridConfig.States[StateIndex]);

            Config.bGenerateExtrusion = State.bGenerateExtrusion;
            Config.bExtrusionSurface  = State.bExtrusionSurface;
            Config.bRemapEdgeUVs = State.bRemapEdgeUVs;
        }
        else
        {
            Config.bGenerateExtrusion = false;
            Config.bExtrusionSurface  = false;
            Config.bRemapEdgeUVs = false;
        }

        Surfaces.Add(new FMQCGridSurface(Config));
    }
}

void FMQCGridChunk::ResetVoxels()
{
    WaitForAsyncTask();

    for (FMQCVoxel& voxel : Voxels)
    {
        voxel.Init();
    }
}

void FMQCGridChunk::SetNeighbourX(const FMQCGridChunk* InNeighbour)
{
    xNeighbor = InNeighbour;
}

void FMQCGridChunk::SetNeighbourY(const FMQCGridChunk* InNeighbour)
{
    yNeighbor = InNeighbour;
}

void FMQCGridChunk::SetNeighbourXY(const FMQCGridChunk* InNeighbour)
{
    xyNeighbor = InNeighbour;
}

FPMUMeshSection* FMQCGridChunk::GetSurfaceSection(int32 StateIndex)
{
    return HasSurface(StateIndex)
        ? &Surfaces[StateIndex].GetSurfaceSection()
        : nullptr;
}

FPMUMeshSection* FMQCGridChunk::GetExtrudeSection(int32 StateIndex)
{
    return HasSurface(StateIndex)
        ? &Surfaces[StateIndex].GetExtrudeSection()
        : nullptr;
}

FPMUMeshSection* FMQCGridChunk::GetSurfaceMaterialSection(int32 StateIndex, const FMQCMaterialBlend& Material)
{
    FPMUMeshSection* Section = nullptr;

    if (HasSurface(StateIndex))
    {
        Section = Surfaces[StateIndex].GetSurfaceMaterialSection(Material);
    }

    return Section;
}

FPMUMeshSection* FMQCGridChunk::GetExtrudeMaterialSection(int32 StateIndex, const FMQCMaterialBlend& Material)
{
    FPMUMeshSection* Section = nullptr;

    if (HasSurface(StateIndex))
    {
        Section = Surfaces[StateIndex].GetExtrudeMaterialSection(Material);
    }

    return Section;
}

int32 FMQCGridChunk::AppendEdgeSyncData(TArray<FMQCEdgeSyncData>& OutSyncData, int32 StateIndex) const
{
    if (HasSurface(StateIndex))
    {
        int32 StartIndex = OutSyncData.Num();
        Surfaces[StateIndex].AppendEdgeSyncData(OutSyncData);
        return StartIndex;
    }
    else
    {
        return -1;
    }
}

void FMQCGridChunk::GetEdgePoints(TArray<FMQCEdgePointData>& OutPointList, int32 StateIndex) const
{
    if (HasSurface(StateIndex))
    {
        Surfaces[StateIndex].GetEdgePoints(OutPointList);
    }
}

void FMQCGridChunk::GetEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListIndex) const
{
    if (HasSurface(StateIndex))
    {
        Surfaces[StateIndex].GetEdgePoints(OutPoints, EdgeListIndex);
    }
}

void FMQCGridChunk::AppendConnectedEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListIndex) const
{
    if (HasSurface(StateIndex))
    {
        Surfaces[StateIndex].AppendConnectedEdgePoints(OutPoints, EdgeListIndex);
    }
}

void FMQCGridChunk::GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const
{
    for (int32 StateIndex=1; StateIndex<Surfaces.Num(); StateIndex++)
    {
        Surfaces[StateIndex].GetMaterialSet(MaterialSet);
    }
}

void FMQCGridChunk::AddQuadFilter(const FIntPoint& Point, int32 StateIndex, bool bExtrudeGeometry)
{
    check((Point.X-Position.X) >= 0);
    check((Point.Y-Position.Y) >= 0);
    check((Point.X-Position.X) < VoxelResolution);
    check((Point.Y-Position.Y) < VoxelResolution);

    if (StateIndex > 0 && HasSurface(StateIndex))
    {
        Surfaces[StateIndex].AddQuadFilter(Point, bExtrudeGeometry);
    }
}

uint32 FMQCGridChunk::AddVertex(const FVector2D& Point, const FMQCMaterial& Material, int32 StateIndex, bool bExtrudeGeometry)
{
    return (StateIndex > 0 && HasSurface(StateIndex))
        ? Surfaces[StateIndex].AddVertexMapped(Point, Material)
        : ~0U;
}

void FMQCGridChunk::AddFace(int32 a, int32 b, int32 c, int32 StateIndex, bool bExtrudeGeometry)
{
    if (StateIndex > 0 && HasSurface(StateIndex))
    {
        Surfaces[StateIndex].AddFace(a, b, c);
    }
}

// Public Triangulation Interface

void FMQCGridChunk::Triangulate()
{
    WaitForAsyncTask();
    TriangulateInternal();
}

void FMQCGridChunk::SetStates(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    WaitForAsyncTask();
    SetStatesInternal(Stencil, X0, X1, Y0, Y1);
}

void FMQCGridChunk::SetCrossings(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    WaitForAsyncTask();
    SetCrossingsInternal(Stencil, X0, X1, Y0, Y1);
}

void FMQCGridChunk::SetMaterials(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    WaitForAsyncTask();
    SetMaterialsInternal(Stencil, X0, X1, Y0, Y1);
}

void FMQCGridChunk::TriangulateAsync()
{
    EnqueueTask([this](){ TriangulateInternal(); });
}

void FMQCGridChunk::SetStatesAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasSurface(Stencil.GetFillType()))
    {
        return;
    }

    struct FTaskParam
    {
        const FMQCStencil* Stencil;
        int32 X0;
        int32 X1;
        int32 Y0;
        int32 Y1;
    };
    FTaskParam Param = { &Stencil, X0, X1, Y0, Y1 };

    EnqueueTask(
        [this, Param]()
        {
            SetStatesInternal(
                *Param.Stencil,
                Param.X0,
                Param.X1,
                Param.Y0,
                Param.Y1
                );
        } );
}

void FMQCGridChunk::SetCrossingsAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasSurface(Stencil.GetFillType()))
    {
        return;
    }

    struct FTaskParam
    {
        const FMQCStencil* Stencil;
        int32 X0;
        int32 X1;
        int32 Y0;
        int32 Y1;
    };
    FTaskParam Param = { &Stencil, X0, X1, Y0, Y1 };

    EnqueueTask(
        [this, Param]()
        {
            SetCrossingsInternal(*Param.Stencil, Param.X0, Param.X1, Param.Y0, Param.Y1);
        } );
}

void FMQCGridChunk::SetMaterialsAsync(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasSurface(Stencil.GetFillType()))
    {
        return;
    }

    struct FTaskParam
    {
        const FMQCStencil* Stencil;
        int32 X0;
        int32 X1;
        int32 Y0;
        int32 Y1;
    };
    FTaskParam Param = { &Stencil, X0, X1, Y0, Y1 };

    EnqueueTask(
        [this, Param]()
        {
            SetMaterialsInternal(*Param.Stencil, Param.X0, Param.X1, Param.Y0, Param.Y1);
        } );
}

// -- Internal Triangulation Interface

void FMQCGridChunk::TriangulateInternal()
{
    for (int32 i=1; i<Surfaces.Num(); i++)
    {
        Surfaces[i].Initialize();
    }

    FillFirstRowCache();
    TriangulateCellRows();

    if (yNeighbor)
    {
        TriangulateGapRow();
    }

    for (int32 i=1; i<Surfaces.Num(); i++)
    {
        Surfaces[i].Finalize();
    }
}

void FMQCGridChunk::SetStatesInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasSurface(Stencil.GetFillType()))
    {
        return;
    }

    for (int32 y=Y0; y<=Y1; y++)
    {
        int32 i = y*VoxelResolution + X0;

        for (int32 x=X0; x<=X1; x++, i++)
        {
            Stencil.ApplyVoxel(Voxels[i], Position);
        }
    }
}

void FMQCGridChunk::SetCrossingsInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasSurface(Stencil.GetFillType()))
    {
        return;
    }

    bool bIncludeLastRowY = false;
    bool bCrossGapX = false;
    bool bCrossGapY = false;
    
    if (X0 > 0)
    {
        X0 -= 1;
    }

    if (X1 == VoxelResolution - 1)
    {
        X1 -= 1;
        bCrossGapX = xNeighbor != nullptr;
    }

    if (Y0 > 0)
    {
        Y0 -= 1;
    }

    if (Y1 == VoxelResolution - 1)
    {
        Y1 -= 1;
        bIncludeLastRowY = true;
        bCrossGapY = yNeighbor != nullptr;
    }

    FMQCVoxel* a;
    FMQCVoxel* b;

    for (int32 y = Y0; y <= Y1; y++)
    {
        int32 i = y * VoxelResolution + X0;
        b = &Voxels[i];

        for (int32 x = X0; x <= X1; x++, i++)
        {
            a = b;
            b = &Voxels[i + 1];
            Stencil.SetCrossingX(*a, *b, Position);
            Stencil.SetCrossingY(*a, Voxels[i + VoxelResolution], Position);
        }

        Stencil.SetCrossingY(*b, Voxels[i + VoxelResolution], Position);

        if (bCrossGapX)
        {
            check(xNeighbor);
            const int32 neighborIndex = y * VoxelResolution;
            if (xNeighbor->Voxels.IsValidIndex(neighborIndex))
            {
                dummyX.BecomeXDummyOf(xNeighbor->Voxels[neighborIndex], VoxelResolution);
                Stencil.SetCrossingX(*b, dummyX, Position);
            }
        }
    }

    if (bIncludeLastRowY)
    {
        int32 i = Voxels.Num() - VoxelResolution + X0;
        b = &Voxels[i];

        for (int32 x = X0; x <= X1; x++, i++)
        {
            a = b;
            b = &Voxels[i + 1];
            Stencil.SetCrossingX(*a, *b, Position);

            if (bCrossGapY)
            {
                check(yNeighbor);
                check(yNeighbor->Voxels.IsValidIndex(x));
                dummyY.BecomeYDummyOf(yNeighbor->Voxels[x], VoxelResolution);
                Stencil.SetCrossingY(*a, dummyY, Position);
            }
        }

        if (bCrossGapY)
        {
            check(yNeighbor);
            const int32 neighborIndex = X1 + 1;
            if (yNeighbor->Voxels.IsValidIndex(neighborIndex))
            {
                dummyY.BecomeYDummyOf(yNeighbor->Voxels[neighborIndex], VoxelResolution);
                Stencil.SetCrossingY(*b, dummyY, Position);
            }
        }

        if (bCrossGapX)
        {
            check(xNeighbor);
            const int32 neighborIndex = Voxels.Num() - VoxelResolution;
            if (xNeighbor->Voxels.IsValidIndex(neighborIndex))
            {
                dummyX.BecomeXDummyOf(xNeighbor->Voxels[neighborIndex], VoxelResolution);
                Stencil.SetCrossingX(*b, dummyX, Position);
            }
        }
    }
}

void FMQCGridChunk::SetMaterialsInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasSurface(Stencil.GetFillType()))
    {
        return;
    }

    for (int32 y=Y0; y<=Y1; y++)
    {
        int32 i = y*VoxelResolution + X0;

        for (int32 x=X0; x<=X1; x++, i++)
        {
            Stencil.ApplyMaterial(Voxels[i], Position);
        }
    }
}

void FMQCGridChunk::EnqueueTask(const TFunction<void()>& Task)
{
    // Wait for any outstanding async task
    WaitForAsyncTask();

    // Create task promise
    TPromise<void>* TaskPromise;
    TaskPromise = new TPromise<void>([TaskPromise](){ delete TaskPromise; });

    // Assign task future
    OutstandingTask = TaskPromise->GetFuture();

    // Dispatch task when ready
    TGraphTask<FAsyncTask>::CreateTask().ConstructAndDispatchWhenReady(
        Task,
        *TaskPromise
        );
}

// -- Geometry Cache Functions

void FMQCGridChunk::FillFirstRowCache()
{
    CacheFirstCorner(Voxels[0]);

    int32 i;
    for (i=0; i<VoxelResolution-1; i++)
    {
        CacheNextEdgeAndCorner(i, Voxels[i], Voxels[i + 1]);
    }

    if (xNeighbor)
    {
        dummyX.BecomeXDummyOf(xNeighbor->Voxels[0], VoxelResolution);
        CacheNextEdgeAndCorner(i, Voxels[i], dummyX);
    }
}

void FMQCGridChunk::SwapRowCaches()
{
    for (int32 i=1; i<Surfaces.Num(); i++)
    {
        Surfaces[i].PrepareCacheForNextRow();
    }
}

void FMQCGridChunk::CacheFirstCorner(const FMQCVoxel& voxel)
{
    if (voxel.IsFilled())
    {
        check(Surfaces.IsValidIndex(voxel.voxelState));
        Surfaces[voxel.voxelState].CacheFirstCorner(voxel);
    }
}

void FMQCGridChunk::CacheNextEdgeAndCorner(int32 i, const FMQCVoxel& xMin, const FMQCVoxel& xMax)
{
    FMQCGridSurface& SurfaceMin(Surfaces[xMin.voxelState]);
    FMQCGridSurface& SurfaceMax(Surfaces[xMax.voxelState]);

    const FMQCMaterial& MaterialMin(xMin.Material);
    const FMQCMaterial& MaterialMax(xMax.Material);

    const bool bFilledMin = xMin.IsFilled();
    const bool bFilledMax = xMax.IsFilled();

    const uint8 StateMin = xMin.voxelState;
    const uint8 StateMax = xMax.voxelState;

    if (bFilledMax)
    {
        SurfaceMax.CacheNextCorner(i, xMax);
    }

    if (StateMin != StateMax)
    {
        if (bFilledMin)
        {
            if (bFilledMax)
            {
                FMQCMaterial EdgeMaterial = (xMin.GetXEdge() > .5f)
                    ? MaterialMax
                    : MaterialMin;
                SurfaceMin.CacheEdgeX(i, xMin, EdgeMaterial);
                SurfaceMax.CacheEdgeX(i, xMin, EdgeMaterial);
            }
            else
            {
                SurfaceMin.CacheEdgeX(i, xMin, MaterialMin);
            }
        }
        else
        {
            SurfaceMax.CacheEdgeX(i, xMin, MaterialMax);
        }
    }
}

void FMQCGridChunk::CacheNextMiddleEdge(const FMQCVoxel& yMin, const FMQCVoxel& yMax)
{
    for (int32 r=1; r<Surfaces.Num(); r++)
    {
        Surfaces[r].PrepareCacheForNextCell();
    }
    if (yMin.voxelState != yMax.voxelState)
    {
        FMQCGridSurface& SurfaceMin(Surfaces[yMin.voxelState]);
        FMQCGridSurface& SurfaceMax(Surfaces[yMax.voxelState]);

        const FMQCMaterial& MaterialMin(yMin.Material);
        const FMQCMaterial& MaterialMax(yMax.Material);

        if (yMin.IsFilled())
        {
            if (yMax.IsFilled())
            {
                FMQCMaterial EdgeMaterial = (yMin.GetYEdge() > .5f)
                    ? MaterialMax
                    : MaterialMin;
                SurfaceMin.CacheEdgeY(yMin, EdgeMaterial);
                SurfaceMax.CacheEdgeY(yMin, EdgeMaterial);
            }
            else
            {
                SurfaceMin.CacheEdgeY(yMin, MaterialMin);
            }
        }
        else
        {
            SurfaceMax.CacheEdgeY(yMin, MaterialMax);
        }
    }
}

// -- Triangulation Functions

void FMQCGridChunk::TriangulateCellRows()
{
    int32 cells = VoxelResolution - 1;
    for (int32 i=0, y=0; y<cells; y++, i++)
    {
        SwapRowCaches();
        CacheFirstCorner(Voxels[i + VoxelResolution]);
        CacheNextMiddleEdge(Voxels[i], Voxels[i + VoxelResolution]);

        for (int32 x=0; x<cells; x++, i++)
        {
            const FMQCVoxel&
                a(Voxels[i]),
                b(Voxels[i + 1]),
                c(Voxels[i + VoxelResolution]),
                d(Voxels[i + VoxelResolution + 1]);
            CacheNextEdgeAndCorner(x, c, d);
            CacheNextMiddleEdge(b, d);
            TriangulateCell(x, a, b, c, d);
        }

        if (xNeighbor)
        {
            TriangulateGapCell(i);
        }
    }
}

void FMQCGridChunk::TriangulateGapRow()
{
    check(yNeighbor != nullptr);

    dummyY.BecomeYDummyOf(yNeighbor->Voxels[0], VoxelResolution);
    int32 cells = VoxelResolution - 1;
    int32 offset = cells * VoxelResolution;
    SwapRowCaches();
    CacheFirstCorner(dummyY);
    CacheNextMiddleEdge(Voxels[cells * VoxelResolution], dummyY);

    for (int32 x=0; x<cells; x++)
    {
        Swap(dummyT, dummyY);
        dummyY.BecomeYDummyOf(yNeighbor->Voxels[x + 1], VoxelResolution);

        CacheNextEdgeAndCorner(x, dummyT, dummyY);
        CacheNextMiddleEdge(Voxels[x + offset + 1], dummyY);
        TriangulateCell(
            x,
            Voxels[x + offset],
            Voxels[x + offset + 1],
            dummyT,
            dummyY
            );
    }

    if (xNeighbor)
    {
        check(xyNeighbor != nullptr);

        dummyT.BecomeXYDummyOf(xyNeighbor->Voxels[0], VoxelResolution);

        CacheNextEdgeAndCorner(cells, dummyY, dummyT);
        CacheNextMiddleEdge(dummyX, dummyT);
        TriangulateCell(
            cells,
            Voxels[Voxels.Num() - 1],
            dummyX,
            dummyY,
            dummyT
            );
    }
}

void FMQCGridChunk::TriangulateGapCell(int32 i)
{
    check(xNeighbor != nullptr);

    Swap(dummyT, dummyX);
    dummyX.BecomeXDummyOf(xNeighbor->Voxels[i + 1], VoxelResolution);

    int32 cacheIndex = VoxelResolution - 1;
    CacheNextEdgeAndCorner(cacheIndex, Voxels[i + VoxelResolution], dummyX);
    CacheNextMiddleEdge(dummyT, dummyX);

    TriangulateCell(
        cacheIndex,
        Voxels[i],
        dummyT,
        Voxels[i + VoxelResolution],
        dummyX
        );
}

void FMQCGridChunk::TriangulateCell(int32 i, const FMQCVoxel& a, const FMQCVoxel& b, const FMQCVoxel& c, const FMQCVoxel& d)
{
    Cell.i = i;
    Cell.a = a;
    Cell.b = b;
    Cell.c = c;
    Cell.d = d;

    if (a.voxelState == b.voxelState)
    {
        if (a.voxelState == c.voxelState)
        {
            if (a.voxelState == d.voxelState)
            {
                Triangulate0000();
            }
            else
            {
                Triangulate0001();
            }
        }
        else
        {
            if (a.voxelState == d.voxelState)
            {
                Triangulate0010();
            }
            else if (c.voxelState == d.voxelState)
            {
                Triangulate0011();
            }
            else
            {
                Triangulate0012();
            }
        }
    }
    else
    {
        if (a.voxelState == c.voxelState)
        {
            if (a.voxelState == d.voxelState)
            {
                Triangulate0100();
            }
            else if (b.voxelState == d.voxelState)
            {
                Triangulate0101();
            }
            else
            {
                Triangulate0102();
            }
        }
        else if (b.voxelState == c.voxelState)
        {
            if (a.voxelState == d.voxelState)
            {
                Triangulate0110();
            }
            else if (b.voxelState == d.voxelState)
            {
                Triangulate0111();
            }
            else
            {
                Triangulate0112();
            }
        }
        else
        {
            if (a.voxelState == d.voxelState)
            {
                Triangulate0120();
            }
            else if (b.voxelState == d.voxelState)
            {
                Triangulate0121();
            }
            else if (c.voxelState == d.voxelState)
            {
                Triangulate0122();
            }
            else
            {
                Triangulate0123();
            }
        }
    }
}

void FMQCGridChunk::Triangulate0000()
{
    FillABCD();
}

void FMQCGridChunk::Triangulate0001()
{
    FMQCFeaturePoint f(Cell.GetFeatureNE());
    FillABC(f);
    FillD(f);
}

void FMQCGridChunk::Triangulate0010()
{
    FMQCFeaturePoint f(Cell.GetFeatureNW());
    FillABD(f);
    FillC(f);
}

void FMQCGridChunk::Triangulate0100()
{
    FMQCFeaturePoint f(Cell.GetFeatureSE());
    FillACD(f);
    FillB(f);
}

void FMQCGridChunk::Triangulate0111()
{
    FMQCFeaturePoint f(Cell.GetFeatureSW());
    FillA(f);
    FillBCD(f);
}

void FMQCGridChunk::Triangulate0011()
{
    FMQCFeaturePoint f(Cell.GetFeatureEW());
    FillAB(f);
    FillCD(f);
}

void FMQCGridChunk::Triangulate0101()
{
    FMQCFeaturePoint f(Cell.GetFeatureNS());
    FillAC(f);
    FillBD(f);
}

void FMQCGridChunk::Triangulate0012()
{
    FMQCFeaturePoint f(Cell.GetFeatureNEW());
    FillAB(f);
    FillC(f);
    FillD(f);
}

void FMQCGridChunk::Triangulate0102()
{
    FMQCFeaturePoint f(Cell.GetFeatureNSE());
    FillAC(f);
    FillB(f);
    FillD(f);
}

void FMQCGridChunk::Triangulate0121()
{
    FMQCFeaturePoint f(Cell.GetFeatureNSW());
    FillA(f);
    FillBD(f);
    FillC(f);
}

void FMQCGridChunk::Triangulate0122()
{
    FMQCFeaturePoint f(Cell.GetFeatureSEW());
    FillA(f);
    FillB(f);
    FillCD(f);
}

void FMQCGridChunk::Triangulate0110()
{
    FMQCFeaturePoint fA(Cell.GetFeatureSW());
    FMQCFeaturePoint fB(Cell.GetFeatureSE());
    FMQCFeaturePoint fC(Cell.GetFeatureNW());
    FMQCFeaturePoint fD(Cell.GetFeatureNE());
    
    if (Cell.HasConnectionAD(fA, fD))
    {
        fB.exists &= Cell.IsInsideABD(fB.position);
        fC.exists &= Cell.IsInsideACD(fC.position);
        FillADToB(fB);
        FillADToC(fC);
        FillB(fB);
        FillC(fC);
    }
    else if (Cell.HasConnectionBC(fB, fC))
    {
        fA.exists &= Cell.IsInsideABC(fA.position);
        fD.exists &= Cell.IsInsideBCD(fD.position);
        FillA(fA);
        FillD(fD);
        FillBCToA(fA);
        FillBCToD(fD);
    }
    else if (Cell.a.IsFilled() && Cell.b.IsFilled())
    {
        FillJoinedCorners(fA, fB, fC, fD);
    }
    else
    {
        FillA(fA);
        FillB(fB);
        FillC(fC);
        FillD(fD);
    }
}

void FMQCGridChunk::Triangulate0112()
{
    FMQCFeaturePoint fA(Cell.GetFeatureSW());
    FMQCFeaturePoint fB(Cell.GetFeatureSE());
    FMQCFeaturePoint fC(Cell.GetFeatureNW());
    FMQCFeaturePoint fD(Cell.GetFeatureNE());

    if (Cell.HasConnectionBC(fB, fC))
    {
        fA.exists &= Cell.IsInsideABC(fA.position);
        fD.exists &= Cell.IsInsideBCD(fD.position);
        FillA(fA);
        FillD(fD);
        FillBCToA(fA);
        FillBCToD(fD);
    }
    else if (Cell.b.IsFilled() || Cell.HasConnectionAD(fA, fD))
    {
        FillJoinedCorners(fA, fB, fC, fD);
    }
    else
    {
        FillA(fA);
        FillD(fD);
    }
}

void FMQCGridChunk::Triangulate0120()
{
    FMQCFeaturePoint fA(Cell.GetFeatureSW());
    FMQCFeaturePoint fB(Cell.GetFeatureSE());
    FMQCFeaturePoint fC(Cell.GetFeatureNW());
    FMQCFeaturePoint fD(Cell.GetFeatureNE());

    if (Cell.HasConnectionAD(fA, fD))
    {
        fB.exists &= Cell.IsInsideABD(fB.position);
        fC.exists &= Cell.IsInsideACD(fC.position);
        FillADToB(fB);
        FillADToC(fC);
        FillB(fB);
        FillC(fC);
    }
    else if (Cell.a.IsFilled() || Cell.HasConnectionBC(fB, fC))
    {
        FillJoinedCorners(fA, fB, fC, fD);
    }
    else
    {
        FillB(fB);
        FillC(fC);
    }
}

void FMQCGridChunk::Triangulate0123()
{
    FillJoinedCorners(
        Cell.GetFeatureSW(), Cell.GetFeatureSE(),
        Cell.GetFeatureNW(), Cell.GetFeatureNE());
}

// -- Fill Functions

void FMQCGridChunk::FillA(const FMQCFeaturePoint& f)
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillA(Cell, f);
    }
}

void FMQCGridChunk::FillB(const FMQCFeaturePoint& f)
{
    if (Cell.b.IsFilled())
    {
        Surfaces[Cell.b.voxelState].FillB(Cell, f);
    }
}

void FMQCGridChunk::FillC(const FMQCFeaturePoint& f)
{
    if (Cell.c.IsFilled())
    {
        Surfaces[Cell.c.voxelState].FillC(Cell, f);
    }
}

void FMQCGridChunk::FillD(const FMQCFeaturePoint& f)
{
    if (Cell.d.IsFilled())
    {
        Surfaces[Cell.d.voxelState].FillD(Cell, f);
    }
}

void FMQCGridChunk::FillABC(const FMQCFeaturePoint& f)
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillABC(Cell, f);
    }
}

void FMQCGridChunk::FillABD(const FMQCFeaturePoint& f)
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillABD(Cell, f);
    }
}

void FMQCGridChunk::FillACD(const FMQCFeaturePoint& f)
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillACD(Cell, f);
    }
}

void FMQCGridChunk::FillBCD(const FMQCFeaturePoint& f)
{
    if (Cell.b.IsFilled())
    {
        Surfaces[Cell.b.voxelState].FillBCD(Cell, f);
    }
}

void FMQCGridChunk::FillAB(const FMQCFeaturePoint& f)
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillAB(Cell, f);
    }
}

void FMQCGridChunk::FillAC(const FMQCFeaturePoint& f)
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillAC(Cell, f);
    }
}

void FMQCGridChunk::FillBD(const FMQCFeaturePoint& f)
{
    if (Cell.b.IsFilled())
    {
        Surfaces[Cell.b.voxelState].FillBD(Cell, f);
    }
}

void FMQCGridChunk::FillCD(const FMQCFeaturePoint& f)
{
    if (Cell.c.IsFilled())
    {
        Surfaces[Cell.c.voxelState].FillCD(Cell, f);
    }
}

void FMQCGridChunk::FillADToB(const FMQCFeaturePoint& f)
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillADToB(Cell, f);
    }
}

void FMQCGridChunk::FillADToC(const FMQCFeaturePoint& f)
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillADToC(Cell, f);
    }
}

void FMQCGridChunk::FillBCToA(const FMQCFeaturePoint& f)
{
    if (Cell.b.IsFilled())
    {
        Surfaces[Cell.b.voxelState].FillBCToA(Cell, f);
    }
}

void FMQCGridChunk::FillBCToD(const FMQCFeaturePoint& f)
{
    if (Cell.b.IsFilled())
    {
        Surfaces[Cell.b.voxelState].FillBCToD(Cell, f);
    }
}

void FMQCGridChunk::FillABCD()
{
    if (Cell.a.IsFilled())
    {
        Surfaces[Cell.a.voxelState].FillABCD(Cell);
    }
}

void FMQCGridChunk::FillJoinedCorners(
    const FMQCFeaturePoint& fA,
    const FMQCFeaturePoint& fB,
    const FMQCFeaturePoint& fC,
    const FMQCFeaturePoint& fD
    )
{
    FMQCFeaturePoint point = Cell.GetFeatureAverage(fA, fB, fC, fD);
    FillA(point);
    FillB(point);
    FillC(point);
    FillD(point);
}
