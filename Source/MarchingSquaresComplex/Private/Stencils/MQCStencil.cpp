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

#include "MQCStencil.h"
#include "MQCGridChunk.h"
#include "MQCVoxel.h"

void FMQCStencil::ValidateNormalX(FMQCVoxel& xMin, const FMQCVoxel& xMax)
{
    if (xMin.state < xMax.state)
    {
        if (xMin.xNormal.X > 0.f)
        {
            xMin.xNormal = -xMin.xNormal;
        }
    }
    else if (xMin.xNormal.X < 0.f)
    {
        xMin.xNormal = -xMin.xNormal;
    }
}

void FMQCStencil::ValidateNormalY(FMQCVoxel& yMin, const FMQCVoxel& yMax)
{
    if (yMin.state < yMax.state)
    {
        if (yMin.yNormal.Y > 0.f)
        {
            yMin.yNormal = -yMin.yNormal;
        }
    }
    else if (yMin.yNormal.Y < 0.f)
    {
        yMin.yNormal = -yMin.yNormal;
    }
}

void FMQCStencil::GetOffsetBounds(float& X0, float& X1, float& Y0, float& Y1, const FVector2D& Offset) const
{
    X0 = GetXStart() - Offset.X;
    X1 = GetXEnd()   - Offset.X;
    Y0 = GetYStart() - Offset.Y;
    Y1 = GetYEnd()   - Offset.Y;
}

void FMQCStencil::GetMapRange(int32& x0, int32& x1, int32& y0, int32& y1, const int32 voxelResolution, const int32 chunkResolution) const
{
    x0 = (int32)(GetXStart()) / voxelResolution;
    if (x0 < 0)
    {
        x0 = 0;
    }

    x1 = (int32)(GetXEnd()) / voxelResolution;
    if (x1 >= chunkResolution)
    {
        x1 = chunkResolution - 1;
    }

    y0 = (int32)(GetYStart()) / voxelResolution;
    if (y0 < 0)
    {
        y0 = 0;
    }

    y1 = (int32)(GetYEnd()) / voxelResolution;
    if (y1 >= chunkResolution)
    {
        y1 = chunkResolution - 1;
    }
}

void FMQCStencil::GetChunkRange(int32& x0, int32& x1, int32& y0, int32& y1, const FMQCGridChunk& Chunk) const
{
    const int32 resolution = Chunk.voxelResolution;
    const FIntPoint Offset = Chunk.GetOffsetId();

    x0 = (int32)(GetXStart()) - Offset.X;
    if (x0 < 0)
    {
        x0 = 0;
    }

    x1 = (int32)(GetXEnd()) - Offset.X;
    if (x1 >= resolution)
    {
        x1 = resolution - 1;
    }

    y0 = (int32)(GetYStart()) - Offset.Y;
    if (y0 < 0)
    {
        y0 = 0;
    }

    y1 = (int32)(GetYEnd()) - Offset.Y;
    if (y1 >= resolution)
    {
        y1 = resolution - 1;
    }
}

void FMQCStencil::SetCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FVector2D& ChunkOffset) const
{
    if (xMin.state != xMax.state)
    {
        FindCrossingX(xMin, xMax, ChunkOffset);
    }
    else
    {
#ifndef MQC_VOXEL_DEBUG_LEGACY
        xMin.xEdge = -1.f;
#else
        xMin.xEdge = TNumericLimits<float>::Lowest();
#endif
    }
}

void FMQCStencil::SetCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FVector2D& ChunkOffset) const
{
    if (yMin.state != yMax.state)
    {
        FindCrossingY(yMin, yMax, ChunkOffset);
    }
    else
    {
#ifndef MQC_VOXEL_DEBUG_LEGACY
        yMin.yEdge = -1.f;
#else
        yMin.yEdge = TNumericLimits<float>::Lowest();
#endif
    }
}

void FMQCStencil::EditMap(FMQCMap& Map, const FVector2D& center)
{
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;
    int32 xStart, xEnd, yStart, yEnd;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(xStart, xEnd, yStart, yEnd, voxelResolution, chunkResolution);

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            SetVoxels(*Map.chunks[i]);
        }
    }

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            SetCrossings(*Map.chunks[i]);
        }
    }
}

void FMQCStencil::EditStates(FMQCMap& Map, const FVector2D& center)
{
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;
    int32 xStart, xEnd, yStart, yEnd;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(xStart, xEnd, yStart, yEnd, voxelResolution, chunkResolution);

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            SetVoxels(*Map.chunks[i]);
        }
    }
}

void FMQCStencil::EditCrossings(FMQCMap& Map, const FVector2D& center)
{
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;
    int32 xStart, xEnd, yStart, yEnd;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(xStart, xEnd, yStart, yEnd, voxelResolution, chunkResolution);

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            SetCrossings(*Map.chunks[i]);
        }
    }
}

void FMQCStencil::GetChunkIndices(FMQCMap& Map, const FVector2D& center, TArray<int32>& OutIndices)
{
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;
    int32 xStart, xEnd, yStart, yEnd;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(xStart, xEnd, yStart, yEnd, voxelResolution, chunkResolution);

    OutIndices.Reset(chunkResolution * chunkResolution);

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            OutIndices.Emplace(i);
        }
    }

    OutIndices.Shrink();
}

void FMQCStencil::SetVoxels(FMQCGridChunk& Chunk)
{
    int32 xStart, xEnd, yStart, yEnd;
    GetChunkRange(xStart, xEnd, yStart, yEnd, Chunk);
    ApplyVoxels(Chunk, xStart, xEnd, yStart, yEnd);
}

void FMQCStencil::SetCrossings(FMQCGridChunk& Chunk)
{
    int32 xStart, xEnd, yStart, yEnd;
    GetChunkRange(xStart, xEnd, yStart, yEnd, Chunk);
    ApplyCrossings(Chunk, xStart, xEnd, yStart, yEnd);
}

void FMQCStencil::ApplyVoxels(FMQCGridChunk& Chunk, const int32 x0, const int32 x1, const int32 y0, const int32 y1)
{
    Chunk.SetStates(*this, x0, x1, y0, y1);
}

void FMQCStencil::ApplyCrossings(FMQCGridChunk& Chunk, const int32 x0, const int32 x1, const int32 y0, const int32 y1)
{
    Chunk.SetCrossings(*this, x0, x1, y0, y1);
}

void FMQCStencil::ApplyVoxel(FMQCVoxel& voxel) const
{
    const FVector2D& p(voxel.position);

    if (p.X >= GetXStart() && p.X <= GetXEnd() && p.Y >= GetYStart() && p.Y <= GetYEnd())
    {
        voxel.state = fillType;
    }
}

void FMQCStencil::ApplyVoxel(FMQCVoxel& voxel, const FVector2D& ChunkOffset) const
{
    const FVector2D& p(voxel.position);

    float X0, X1, Y0, Y1;
    GetOffsetBounds(X0, X1, Y0, Y1, ChunkOffset);

    if (p.X >= X0 && p.X <= X1 && p.Y >= Y0 && p.Y <= Y1)
    {
        voxel.state = fillType;
    }
}
