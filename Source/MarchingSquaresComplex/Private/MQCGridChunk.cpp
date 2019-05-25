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

void FMQCGridChunk::Initialize(const FMQCGridConfig& Config)
{
    position = Config.Position;
    gridSize = Config.ChunkSize;
    voxelResolution = Config.VoxelResolution;
    voxelSize       = gridSize / voxelResolution;

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

void FMQCGridChunk::CopyFrom(const FMQCGridChunk& Chunk)
{
    position        = Chunk.position;
    gridSize        = Chunk.gridSize;
    voxelResolution = Chunk.voxelResolution;
    voxelSize       = Chunk.voxelSize;

    cell            = Chunk.cell;
    voxels          = Chunk.voxels;

    // Construct & copy renderers

    renderers.SetNum(Chunk.renderers.Num());

    for (int32 i=0; i<renderers.Num(); ++i)
    {
        renderers[i].CopyFrom(Chunk.renderers[i]);
    }
}

void FMQCGridChunk::CreateRenderers(const FMQCGridConfig& GridConfig)
{
    // Construct renderer count

    const int32 rendererCount = 1 + GridConfig.States.Num();
    renderers.Reserve(rendererCount);

    for (int32 i=0; i<rendererCount; ++i)
    {
        FMQCSurfaceConfig Config;
        Config.Position        = position;
        Config.VoxelResolution = voxelResolution;
        Config.ChunkSize       = gridSize;
        Config.MapSize         = GridConfig.MapSize;
        Config.ExtrusionHeight = GridConfig.ExtrusionHeight;

        if (i > 0)
        {
            int32 StateIndex = i-1;
            const FMQCSurfaceState& State(GridConfig.States[StateIndex]);

            Config.bGenerateExtrusion = State.bGenerateExtrusion;
            Config.bExtrusionSurface  = State.bExtrusionSurface;
        }
        else
        {
            Config.bGenerateExtrusion = false;
            Config.bExtrusionSurface  = false;
        }

        renderers.Emplace(Config);
    }
}

void FMQCGridChunk::ResetVoxels()
{
    for (FMQCVoxel& voxel : voxels)
    {
        voxel.Reset();
    }
}

void FMQCGridChunk::Refresh()
{
    Triangulate();
}

void FMQCGridChunk::Triangulate()
{
    for (int32 i=1; i<renderers.Num(); i++)
    {
        renderers[i].Clear();
    }

    FillFirstRowCache();
    TriangulateCellRows();

    if (yNeighbor)
    {
        TriangulateGapRow();
    }

    for (int32 i=1; i<renderers.Num(); i++)
    {
        renderers[i].Apply();
    }
}

void FMQCGridChunk::SetStates(const FMQCStencil& stencil, int32 xStart, int32 xEnd, int32 yStart, int32 yEnd)
{
    // Invalid stencil fill type, abort
    if (! HasRenderer(stencil.GetFillType()))
    {
        return;
    }

    for (int32 y=yStart; y<=yEnd; y++)
    {
        int32 i = y*voxelResolution + xStart;

        for (int32 x=xStart; x<=xEnd; x++, i++)
        {
            stencil.ApplyVoxel(voxels[i]);
        }
    }
}

void FMQCGridChunk::SetCrossings(const FMQCStencil& stencil, int32 xStart, int32 xEnd, int32 yStart, int32 yEnd)
{
    // Invalid stencil fill type, abort
    if (! HasRenderer(stencil.GetFillType()))
    {
        return;
    }

    bool includeLastVerticalRow = false;
    bool crossHorizontalGap = false;
    bool crossVerticalGap = false;
    
    if (xStart > 0)
    {
        xStart -= 1;
    }

    if (xEnd == voxelResolution - 1)
    {
        xEnd -= 1;
        crossHorizontalGap = xNeighbor != nullptr;
    }

    if (yStart > 0)
    {
        yStart -= 1;
    }

    if (yEnd == voxelResolution - 1)
    {
        yEnd -= 1;
        includeLastVerticalRow = true;
        crossVerticalGap = yNeighbor != nullptr;
    }

    FMQCVoxel* a;
    FMQCVoxel* b;

    for (int32 y = yStart; y <= yEnd; y++)
    {
        int32 i = y * voxelResolution + xStart;
        b = &voxels[i];

        for (int32 x = xStart; x <= xEnd; x++, i++)
        {
            a = b;
            b = &voxels[i + 1];
            stencil.SetHorizontalCrossing(*a, *b);
            stencil.SetVerticalCrossing(*a, voxels[i + voxelResolution]);
        }

        stencil.SetVerticalCrossing(*b, voxels[i + voxelResolution]);

        if (crossHorizontalGap)
        {
            check(xNeighbor);
            const int32 neighborIndex = y * voxelResolution;
            if (xNeighbor->voxels.IsValidIndex(neighborIndex))
            {
                dummyX.BecomeXDummyOf(xNeighbor->voxels[neighborIndex], gridSize);
                stencil.SetHorizontalCrossing(*b, dummyX);
            }
        }
    }

    if (includeLastVerticalRow)
    {
        int32 i = voxels.Num() - voxelResolution + xStart;
        b = &voxels[i];

        for (int32 x = xStart; x <= xEnd; x++, i++)
        {
            a = b;
            b = &voxels[i + 1];
            stencil.SetHorizontalCrossing(*a, *b);

            if (crossVerticalGap)
            {
                check(yNeighbor);
                check(yNeighbor->voxels.IsValidIndex(x));
                dummyY.BecomeYDummyOf(yNeighbor->voxels[x], gridSize);
                stencil.SetVerticalCrossing(*a, dummyY);
            }
        }

        if (crossVerticalGap)
        {
            check(yNeighbor);
            const int32 neighborIndex = xEnd + 1;
            if (yNeighbor->voxels.IsValidIndex(neighborIndex))
            {
                dummyY.BecomeYDummyOf(yNeighbor->voxels[neighborIndex], gridSize);
                stencil.SetVerticalCrossing(*b, dummyY);
            }
        }

        if (crossHorizontalGap)
        {
            check(xNeighbor);
            const int32 neighborIndex = voxels.Num() - voxelResolution;
            if (xNeighbor->voxels.IsValidIndex(neighborIndex))
            {
                dummyX.BecomeXDummyOf(xNeighbor->voxels[neighborIndex], gridSize);
                stencil.SetHorizontalCrossing(*b, dummyX);
            }
        }
    }
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
        dummyX.BecomeXDummyOf(xNeighbor->voxels[0], gridSize);
        CacheNextEdgeAndCorner(i, voxels[i], dummyX);
    }
}

void FMQCGridChunk::CacheFirstCorner(const FMQCVoxel& voxel)
{
    if (voxel.IsFilled())
    {
        check(renderers.IsValidIndex(voxel.state));
        renderers[voxel.state].CacheFirstCorner(voxel);
    }
}

void FMQCGridChunk::CacheNextEdgeAndCorner(int32 i, const FMQCVoxel& xMin, const FMQCVoxel& xMax)
{
    if (xMin.state != xMax.state)
    {
        if (xMin.IsFilled())
        {
            if (xMax.IsFilled())
            {
                renderers[xMin.state].CacheXEdge(i, xMin);
                renderers[xMax.state].CacheXEdge(i, xMin);
            }
            else
            {
                renderers[xMin.state].CacheXEdgeWithWall(i, xMin);
            }
        }
        else
        {
            renderers[xMax.state].CacheXEdgeWithWall(i, xMin);
        }
    }
    if (xMax.IsFilled())
    {
        renderers[xMax.state].CacheNextCorner(i, xMax);
    }
}

void FMQCGridChunk::CacheNextMiddleEdge(const FMQCVoxel& yMin, const FMQCVoxel& yMax)
{
    for (int32 i=1; i<renderers.Num(); i++)
    {
        renderers[i].PrepareCacheForNextCell();
    }
    if (yMin.state != yMax.state)
    {
        if (yMin.IsFilled())
        {
            if (yMax.IsFilled())
            {
                renderers[yMin.state].CacheYEdge(yMin);
                renderers[yMax.state].CacheYEdge(yMin);
            }
            else
            {
                renderers[yMin.state].CacheYEdgeWithWall(yMin);
            }
        }
        else
        {
            renderers[yMax.state].CacheYEdgeWithWall(yMin);
        }
    }
}

void FMQCGridChunk::SwapRowCaches()
{
    for (int32 i=1; i<renderers.Num(); i++)
    {
        renderers[i].PrepareCacheForNextRow();
    }
}

void FMQCGridChunk::TriangulateCellRows()
{
    int32 cells = voxelResolution - 1;
    for (int32 i=0, y=0; y<cells; y++, i++)
    {
        SwapRowCaches();
        CacheFirstCorner(voxels[i + voxelResolution]);
        CacheNextMiddleEdge(voxels[i], voxels[i + voxelResolution]);

        for (int32 x=0; x<cells; x++, i++)
        {
            const FMQCVoxel&
                a(voxels[i]),
                b(voxels[i + 1]),
                c(voxels[i + voxelResolution]),
                d(voxels[i + voxelResolution + 1]);
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

    dummyY.BecomeYDummyOf(yNeighbor->voxels[0], gridSize);
    int32 cells = voxelResolution - 1;
    int32 offset = cells * voxelResolution;
    SwapRowCaches();
    CacheFirstCorner(dummyY);
    CacheNextMiddleEdge(voxels[cells * voxelResolution], dummyY);

    for (int32 x = 0; x < cells; x++)
    {
        Swap(dummyT, dummyY);
        dummyY.BecomeYDummyOf(yNeighbor->voxels[x + 1], gridSize);

        CacheNextEdgeAndCorner(x, dummyT, dummyY);
        CacheNextMiddleEdge(voxels[x + offset + 1], dummyY);
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

        dummyT.BecomeXYDummyOf(xyNeighbor->voxels[0], gridSize);

        CacheNextEdgeAndCorner(cells, dummyY, dummyT);
        CacheNextMiddleEdge(dummyX, dummyT);
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
    dummyX.BecomeXDummyOf(xNeighbor->voxels[i + 1], gridSize);

    int32 cacheIndex = voxelResolution - 1;
    CacheNextEdgeAndCorner(cacheIndex, voxels[i + voxelResolution], dummyX);
    CacheNextMiddleEdge(dummyT, dummyX);

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

    if (a.state == b.state)
    {
        if (a.state == c.state)
        {
            if (a.state == d.state)
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
            if (a.state == d.state)
            {
                Triangulate0010();
            }
            else if (c.state == d.state)
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
        if (a.state == c.state)
        {
            if (a.state == d.state)
            {
                Triangulate0100();
            }
            else if (b.state == d.state)
            {
                Triangulate0101();
            }
            else
            {
                Triangulate0102();
            }
        }
        else if (b.state == c.state)
        {
            if (a.state == d.state)
            {
                Triangulate0110();
            }
            else if (b.state == d.state)
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
            if (a.state == d.state)
            {
                Triangulate0120();
            }
            else if (b.state == d.state)
            {
                Triangulate0121();
            }
            else if (c.state == d.state)
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
