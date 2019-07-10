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
#include "MQCStencil.h"
#include "MQCMaterialUtility.h"

FMQCGridChunk::FMQCGridChunk()
{
}

FMQCGridChunk::~FMQCGridChunk()
{
    WaitForAsyncTask();
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

void FMQCGridChunk::Initialize(const FMQCChunkConfig& Config)
{
    position = Config.Position;
    mapSize = Config.MapSize;
    voxelResolution = Config.VoxelResolution;
    MaterialType = Config.MaterialType;

    cell.sharpFeatureLimit = FMath::Cos(FMath::DegreesToRadians(Config.MaxFeatureAngle));
    cell.parallelLimit     = FMath::Cos(FMath::DegreesToRadians(Config.MaxParallelAngle));

    voxels.SetNum(voxelResolution * voxelResolution);
    
    for (int32 y=0, i=0; y<voxelResolution; y++)
    for (int32 x=0     ; x<voxelResolution; x++, i++)
    {
        CreateVoxel(i, x, y);
    }

    CreateRenderers(Config);
}

void FMQCGridChunk::CreateRenderers(const FMQCChunkConfig& GridConfig)
{
    // Construct renderer count

    const int32 RendererCount = 1 + GridConfig.States.Num();
    Renderers.Reserve(RendererCount);

    for (int32 i=0; i<RendererCount; ++i)
    {
        FMQCSurfaceConfig Config;
        Config.Position        = position;
        Config.MapSize         = mapSize;
        Config.VoxelResolution = voxelResolution;
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

        Renderers.Emplace(Config);
    }
}

void FMQCGridChunk::ResetVoxels()
{
    WaitForAsyncTask();

    for (FMQCVoxel& voxel : voxels)
    {
        voxel.Reset();
    }
}

void FMQCGridChunk::TriangulateInternal()
{
    for (int32 i=1; i<Renderers.Num(); i++)
    {
        Renderers[i].Clear();
    }

    FillFirstRowCache();
    TriangulateCellRows();

    if (yNeighbor)
    {
        TriangulateGapRow();
    }

    for (int32 i=1; i<Renderers.Num(); i++)
    {
        Renderers[i].Finalize();
    }
}

void FMQCGridChunk::SetStatesInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasRenderer(Stencil.GetFillType()))
    {
        return;
    }

    for (int32 y=Y0; y<=Y1; y++)
    {
        int32 i = y*voxelResolution + X0;

        for (int32 x=X0; x<=X1; x++, i++)
        {
            Stencil.ApplyVoxel(voxels[i], position);
        }
    }
}

void FMQCGridChunk::SetCrossingsInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasRenderer(Stencil.GetFillType()))
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

    if (X1 == voxelResolution - 1)
    {
        X1 -= 1;
        bCrossGapX = xNeighbor != nullptr;
    }

    if (Y0 > 0)
    {
        Y0 -= 1;
    }

    if (Y1 == voxelResolution - 1)
    {
        Y1 -= 1;
        bIncludeLastRowY = true;
        bCrossGapY = yNeighbor != nullptr;
    }

    FMQCVoxel* a;
    FMQCVoxel* b;

    for (int32 y = Y0; y <= Y1; y++)
    {
        int32 i = y * voxelResolution + X0;
        b = &voxels[i];

        for (int32 x = X0; x <= X1; x++, i++)
        {
            a = b;
            b = &voxels[i + 1];
            Stencil.SetCrossingX(*a, *b, position);
            Stencil.SetCrossingY(*a, voxels[i + voxelResolution], position);
        }

        Stencil.SetCrossingY(*b, voxels[i + voxelResolution], position);

        if (bCrossGapX)
        {
            check(xNeighbor);
            const int32 neighborIndex = y * voxelResolution;
            if (xNeighbor->voxels.IsValidIndex(neighborIndex))
            {
                dummyX.BecomeXDummyOf(xNeighbor->voxels[neighborIndex], voxelResolution);
                Stencil.SetCrossingX(*b, dummyX, position);
            }
        }
    }

    if (bIncludeLastRowY)
    {
        int32 i = voxels.Num() - voxelResolution + X0;
        b = &voxels[i];

        for (int32 x = X0; x <= X1; x++, i++)
        {
            a = b;
            b = &voxels[i + 1];
            Stencil.SetCrossingX(*a, *b, position);

            if (bCrossGapY)
            {
                check(yNeighbor);
                check(yNeighbor->voxels.IsValidIndex(x));
                dummyY.BecomeYDummyOf(yNeighbor->voxels[x], voxelResolution);
                Stencil.SetCrossingY(*a, dummyY, position);
            }
        }

        if (bCrossGapY)
        {
            check(yNeighbor);
            const int32 neighborIndex = X1 + 1;
            if (yNeighbor->voxels.IsValidIndex(neighborIndex))
            {
                dummyY.BecomeYDummyOf(yNeighbor->voxels[neighborIndex], voxelResolution);
                Stencil.SetCrossingY(*b, dummyY, position);
            }
        }

        if (bCrossGapX)
        {
            check(xNeighbor);
            const int32 neighborIndex = voxels.Num() - voxelResolution;
            if (xNeighbor->voxels.IsValidIndex(neighborIndex))
            {
                dummyX.BecomeXDummyOf(xNeighbor->voxels[neighborIndex], voxelResolution);
                Stencil.SetCrossingX(*b, dummyX, position);
            }
        }
    }
}

void FMQCGridChunk::SetMaterialsInternal(const FMQCStencil& Stencil, int32 X0, int32 X1, int32 Y0, int32 Y1)
{
    // Invalid stencil fill type, abort
    if (! HasRenderer(Stencil.GetFillType()))
    {
        return;
    }

    for (int32 y=Y0; y<=Y1; y++)
    {
        int32 i = y*voxelResolution + X0;

        for (int32 x=X0; x<=X1; x++, i++)
        {
            Stencil.ApplyMaterial(voxels[i], position);
        }
    }
}

FMQCGridSurface& FMQCGridChunk::GetSurface(int32 StateIndex)
{
    return Renderers[StateIndex].GetSurface();
}

const FMQCGridSurface& FMQCGridChunk::GetSurface(int32 StateIndex) const
{
    return Renderers[StateIndex].GetSurface();
}

void FMQCGridChunk::GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const
{
    for (int32 i=1; i<Renderers.Num(); i++)
    {
        GetSurface(i).GetMaterialSet(MaterialSet);
    }
}

FPMUMeshSection* FMQCGridChunk::GetMaterialSection(int32 StateIndex, const FMQCMaterialBlend& Material)
{
    FPMUMeshSection* Section = nullptr;

    if (HasRenderer(StateIndex))
    {
        Section = GetSurface(StateIndex).MaterialSectionMap.Find(Material);
    }

    return Section;
}

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
    if (! HasRenderer(Stencil.GetFillType()))
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
    if (! HasRenderer(Stencil.GetFillType()))
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
    if (! HasRenderer(Stencil.GetFillType()))
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

void FMQCGridChunk::FillFirstRowCache()
{
    CacheFirstCorner(voxels[0]);

    int32 i;
    for (i=0; i<voxelResolution-1; i++)
    {
        CacheNextEdgeAndCorner(i, voxels[i], voxels[i + 1]);
    }

    if (xNeighbor)
    {
        dummyX.BecomeXDummyOf(xNeighbor->voxels[0], voxelResolution);
        CacheNextEdgeAndCorner(i, voxels[i], dummyX);
    }
}

void FMQCGridChunk::CacheFirstCorner(const FMQCVoxel& voxel)
{
    if (voxel.IsFilled())
    {
        check(Renderers.IsValidIndex(voxel.voxelState));
        Renderers[voxel.voxelState].CacheFirstCorner(voxel);
    }
}

void FMQCGridChunk::CacheNextEdgeAndCorner(int32 i, const FMQCVoxel& xMin, const FMQCVoxel& xMax)
{
    FMQCGridRenderer& RendererMin(Renderers[xMin.voxelState]);
    FMQCGridRenderer& RendererMax(Renderers[xMax.voxelState]);

    const FMQCMaterial& MaterialMin(xMin.Material);
    const FMQCMaterial& MaterialMax(xMax.Material);

    const bool bFilledMin = xMin.IsFilled();
    const bool bFilledMax = xMax.IsFilled();

    const uint8 StateMin = xMin.voxelState;
    const uint8 StateMax = xMax.voxelState;

    if (bFilledMax)
    {
        RendererMax.CacheNextCorner(i, xMax);
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
                RendererMin.CacheEdgeXMinToMax(i, xMin, EdgeMaterial);
                RendererMax.CacheEdgeXMaxToMin(i, xMin, EdgeMaterial);
            }
            else
            {
                RendererMin.CacheEdgeXWallMinToMax(i, xMin, MaterialMin);
            }
        }
        else
        {
            RendererMax.CacheEdgeXWallMaxToMin(i, xMin, MaterialMax);
        }
    }
}

void FMQCGridChunk::CacheNextMiddleEdge(int32 i, const FMQCVoxel& yMin, const FMQCVoxel& yMax)
{
    for (int32 r=1; r<Renderers.Num(); r++)
    {
        Renderers[r].PrepareCacheForNextCell();
    }
    if (yMin.voxelState != yMax.voxelState)
    {
        FMQCGridRenderer& RendererMin(Renderers[yMin.voxelState]);
        FMQCGridRenderer& RendererMax(Renderers[yMax.voxelState]);

        const FMQCMaterial& MaterialMin(yMin.Material);
        const FMQCMaterial& MaterialMax(yMax.Material);

        if (yMin.IsFilled())
        {
            if (yMax.IsFilled())
            {
                FMQCMaterial EdgeMaterial = (yMin.GetYEdge() > .5f)
                    ? MaterialMax
                    : MaterialMin;
                RendererMin.CacheEdgeYMinToMax(i, yMin, EdgeMaterial);
                RendererMax.CacheEdgeYMaxToMin(i, yMin, EdgeMaterial);
            }
            else
            {
                RendererMin.CacheEdgeYWallMinToMax(i, yMin, MaterialMin);
            }
        }
        else
        {
            RendererMax.CacheEdgeYWallMaxToMin(i, yMin, MaterialMax);
        }
    }
}

void FMQCGridChunk::SwapRowCaches()
{
    for (int32 i=1; i<Renderers.Num(); i++)
    {
        Renderers[i].PrepareCacheForNextRow();
    }
}

void FMQCGridChunk::TriangulateCellRows()
{
    int32 cells = voxelResolution - 1;
    for (int32 i=0, y=0; y<cells; y++, i++)
    {
        SwapRowCaches();
        CacheFirstCorner(voxels[i + voxelResolution]);
        CacheFirstMiddleEdge(voxels[i], voxels[i + voxelResolution]);

        for (int32 x=0; x<cells; x++, i++)
        {
            const FMQCVoxel&
                a(voxels[i]),
                b(voxels[i + 1]),
                c(voxels[i + voxelResolution]),
                d(voxels[i + voxelResolution + 1]);
            CacheNextEdgeAndCorner(x, c, d);
            CacheNextMiddleEdge(x, b, d);
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

    dummyY.BecomeYDummyOf(yNeighbor->voxels[0], voxelResolution);
    int32 cells = voxelResolution - 1;
    int32 offset = cells * voxelResolution;
    SwapRowCaches();
    CacheFirstCorner(dummyY);
    CacheFirstMiddleEdge(voxels[cells * voxelResolution], dummyY);

    for (int32 x=0; x<cells; x++)
    {
        Swap(dummyT, dummyY);
        dummyY.BecomeYDummyOf(yNeighbor->voxels[x + 1], voxelResolution);

        CacheNextEdgeAndCorner(x, dummyT, dummyY);
        CacheNextMiddleEdge(x, voxels[x + offset + 1], dummyY);
        TriangulateCell(
            x,
            voxels[x + offset],
            voxels[x + offset + 1],
            dummyT,
            dummyY
            );
    }

    if (xNeighbor)
    {
        check(xyNeighbor != nullptr);

        dummyT.BecomeXYDummyOf(xyNeighbor->voxels[0], voxelResolution);

        CacheNextEdgeAndCorner(cells, dummyY, dummyT);
        CacheNextMiddleEdge(cells, dummyX, dummyT);
        TriangulateCell(
            cells,
            voxels[voxels.Num() - 1],
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
    dummyX.BecomeXDummyOf(xNeighbor->voxels[i + 1], voxelResolution);

    int32 cacheIndex = voxelResolution - 1;
    CacheNextEdgeAndCorner(cacheIndex, voxels[i + voxelResolution], dummyX);
    CacheNextMiddleEdge(cacheIndex, dummyT, dummyX);

    TriangulateCell(
        cacheIndex,
        voxels[i],
        dummyT,
        voxels[i + voxelResolution],
        dummyX
        );
}

// Triangulation Functions

void FMQCGridChunk::TriangulateCell(int32 i, const FMQCVoxel& a, const FMQCVoxel& b, const FMQCVoxel& c, const FMQCVoxel& d)
{
    cell.i = i;
    cell.a = a;
    cell.b = b;
    cell.c = c;
    cell.d = d;

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
    FMQCFeaturePoint f(cell.GetFeatureNE());
    FillABC(f);
    FillD(f);
}

void FMQCGridChunk::Triangulate0010()
{
    FMQCFeaturePoint f(cell.GetFeatureNW());
    FillABD(f);
    FillC(f);
}

void FMQCGridChunk::Triangulate0100()
{
    FMQCFeaturePoint f(cell.GetFeatureSE());
    FillACD(f);
    FillB(f);
}

void FMQCGridChunk::Triangulate0111()
{
    FMQCFeaturePoint f(cell.GetFeatureSW());
    FillA(f);
    FillBCD(f);
}

void FMQCGridChunk::Triangulate0011()
{
    FMQCFeaturePoint f(cell.GetFeatureEW());
    FillAB(f);
    FillCD(f);
}

void FMQCGridChunk::Triangulate0101()
{
    FMQCFeaturePoint f(cell.GetFeatureNS());
    FillAC(f);
    FillBD(f);
}

void FMQCGridChunk::Triangulate0012()
{
    FMQCFeaturePoint f(cell.GetFeatureNEW());
    FillAB(f);
    FillC(f);
    FillD(f);
}

void FMQCGridChunk::Triangulate0102()
{
    FMQCFeaturePoint f(cell.GetFeatureNSE());
    FillAC(f);
    FillB(f);
    FillD(f);
}

void FMQCGridChunk::Triangulate0121()
{
    FMQCFeaturePoint f(cell.GetFeatureNSW());
    FillA(f);
    FillBD(f);
    FillC(f);
}

void FMQCGridChunk::Triangulate0122()
{
    FMQCFeaturePoint f(cell.GetFeatureSEW());
    FillA(f);
    FillB(f);
    FillCD(f);
}

void FMQCGridChunk::Triangulate0110()
{
    FMQCFeaturePoint fA(cell.GetFeatureSW());
    FMQCFeaturePoint fB(cell.GetFeatureSE());
    FMQCFeaturePoint fC(cell.GetFeatureNW());
    FMQCFeaturePoint fD(cell.GetFeatureNE());
    
    if (cell.HasConnectionAD(fA, fD))
    {
        fB.exists &= cell.IsInsideABD(fB.position);
        fC.exists &= cell.IsInsideACD(fC.position);
        FillADToB(fB);
        FillADToC(fC);
        FillB(fB);
        FillC(fC);
    }
    else if (cell.HasConnectionBC(fB, fC))
    {
        fA.exists &= cell.IsInsideABC(fA.position);
        fD.exists &= cell.IsInsideBCD(fD.position);
        FillA(fA);
        FillD(fD);
        FillBCToA(fA);
        FillBCToD(fD);
    }
    else if (cell.a.IsFilled() && cell.b.IsFilled())
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
    FMQCFeaturePoint fA(cell.GetFeatureSW());
    FMQCFeaturePoint fB(cell.GetFeatureSE());
    FMQCFeaturePoint fC(cell.GetFeatureNW());
    FMQCFeaturePoint fD(cell.GetFeatureNE());

    if (cell.HasConnectionBC(fB, fC))
    {
        fA.exists &= cell.IsInsideABC(fA.position);
        fD.exists &= cell.IsInsideBCD(fD.position);
        FillA(fA);
        FillD(fD);
        FillBCToA(fA);
        FillBCToD(fD);
    }
    else if (cell.b.IsFilled() || cell.HasConnectionAD(fA, fD))
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
    FMQCFeaturePoint fA(cell.GetFeatureSW());
    FMQCFeaturePoint fB(cell.GetFeatureSE());
    FMQCFeaturePoint fC(cell.GetFeatureNW());
    FMQCFeaturePoint fD(cell.GetFeatureNE());

    if (cell.HasConnectionAD(fA, fD))
    {
        fB.exists &= cell.IsInsideABD(fB.position);
        fC.exists &= cell.IsInsideACD(fC.position);
        FillADToB(fB);
        FillADToC(fC);
        FillB(fB);
        FillC(fC);
    }
    else if (cell.a.IsFilled() || cell.HasConnectionBC(fB, fC))
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
        cell.GetFeatureSW(), cell.GetFeatureSE(),
        cell.GetFeatureNW(), cell.GetFeatureNE());
}
