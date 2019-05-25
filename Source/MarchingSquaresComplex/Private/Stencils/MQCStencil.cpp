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

void FMQCStencil::ValidateHorizontalNormal(FMQCVoxel& xMin, const FMQCVoxel& xMax)
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

void FMQCStencil::ValidateVerticalNormal(FMQCVoxel& yMin, const FMQCVoxel& yMax)
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

void FMQCStencil::SetHorizontalCrossing(FMQCVoxel& xMin, const FMQCVoxel& xMax) const
{
    if (xMin.state != xMax.state)
    {
        FindHorizontalCrossing(xMin, xMax);
    }
    else
    {
        xMin.xEdge = -1.f; //TNumericLimits<float>::Lowest();
    }
}

void FMQCStencil::SetVerticalCrossing(FMQCVoxel& yMin, const FMQCVoxel& yMax) const
{
    if (yMin.state != yMax.state)
    {
        FindVerticalCrossing(yMin, yMax);
    }
    else
    {
        //yMin.yEdge = TNumericLimits<float>::Lowest();
        yMin.yEdge = -1.f; //TNumericLimits<float>::Lowest();
    }
}

void FMQCStencil::EditMap(FMQCMap& Map, const FVector2D& center)
{
    const float voxelSize = Map.voxelSize;
    const float chunkSize = Map.chunkSize;
    const int32 chunkResolution = Map.chunkResolution;
    int32 xStart, xEnd, yStart, yEnd;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(xStart, xEnd, yStart, yEnd, voxelSize, chunkSize, chunkResolution);

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            const float ChunkCenterX = center.X - x * chunkSize;
            const float ChunkCenterY = center.Y - y * chunkSize;
            SetCenter(ChunkCenterX, ChunkCenterY);
            SetVoxels(*Map.chunks[i]);
        }
    }

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            const float ChunkCenterX = center.X - x * chunkSize;
            const float ChunkCenterY = center.Y - y * chunkSize;
            FMQCGridChunk& Chunk(*Map.chunks[i]);

            SetCenter(ChunkCenterX, ChunkCenterY);
            SetCrossings(Chunk);
        }
    }
}

void FMQCStencil::EditStates(FMQCMap& Map, const FVector2D& center)
{
    const float voxelSize = Map.voxelSize;
    const float chunkSize = Map.chunkSize;
    const int32 chunkResolution = Map.chunkResolution;
    int32 xStart, xEnd, yStart, yEnd;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(xStart, xEnd, yStart, yEnd, voxelSize, chunkSize, chunkResolution);

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            const float ChunkCenterX = center.X - x * chunkSize;
            const float ChunkCenterY = center.Y - y * chunkSize;
            SetCenter(ChunkCenterX, ChunkCenterY);
            SetVoxels(*Map.chunks[i]);
        }
    }
}

void FMQCStencil::EditCrossings(FMQCMap& Map, const FVector2D& center)
{
    const float voxelSize = Map.voxelSize;
    const float chunkSize = Map.chunkSize;
    const int32 chunkResolution = Map.chunkResolution;
    int32 xStart, xEnd, yStart, yEnd;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(xStart, xEnd, yStart, yEnd, voxelSize, chunkSize, chunkResolution);

    for (int32 y=yEnd; y>=yStart; y--)
    {
        int32 i = y * chunkResolution + xEnd;

        for (int32 x=xEnd; x>=xStart; x--, i--)
        {
            const float ChunkCenterX = center.X - x * chunkSize;
            const float ChunkCenterY = center.Y - y * chunkSize;

            SetCenter(ChunkCenterX, ChunkCenterY);
            SetCrossings(*Map.chunks[i]);
        }
    }
}

void FMQCStencil::GetChunkIndices(FMQCMap& Map, const FVector2D& center, TArray<int32>& OutIndices)
{
    const float voxelSize = Map.voxelSize;
    const float chunkSize = Map.chunkSize;
    const int32 chunkResolution = Map.chunkResolution;
    int32 xStart, xEnd, yStart, yEnd;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(xStart, xEnd, yStart, yEnd, voxelSize, chunkSize, chunkResolution);

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
    GetChunkRange(xStart, xEnd, yStart, yEnd, Chunk.voxelSize, Chunk.voxelResolution);
    ApplyVoxels(Chunk, xStart, xEnd, yStart, yEnd);
}

void FMQCStencil::SetCrossings(FMQCGridChunk& Chunk)
{
    int32 xStart, xEnd, yStart, yEnd;
    GetChunkRange(xStart, xEnd, yStart, yEnd, Chunk.voxelSize, Chunk.voxelResolution);
    ApplyCrossings(Chunk, xStart, xEnd, yStart, yEnd);
}

void FMQCStencil::ApplyVoxel(FMQCVoxel& voxel) const
{
    const FVector2D& p(voxel.position);

    if (p.X >= GetXStart() && p.X <= GetXEnd() && p.Y >= GetYStart() && p.Y <= GetYEnd())
    {
        voxel.state = fillType;
    }
}

void FMQCStencil::ApplyVoxels(FMQCGridChunk& Chunk, const int32 x0, const int32 x1, const int32 y0, const int32 y1)
{
    Chunk.SetStates(*this, x0, x1, y0, y1);
}

void FMQCStencil::ApplyCrossings(FMQCGridChunk& Chunk, const int32 x0, const int32 x1, const int32 y0, const int32 y1)
{
    Chunk.SetCrossings(*this, x0, x1, y0, y1);
}
