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
    if (xMin.voxelState < xMax.voxelState)
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
    if (yMin.voxelState < yMax.voxelState)
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

void FMQCStencil::GetMapRange(TArray<int32>& ChunkIndices, const int32 voxelResolution, const int32 chunkResolution) const
{
    int32 X0, X1, Y0, Y1;
    GetMapRange(X0, X1, Y0, Y1, voxelResolution, chunkResolution);

    for (int32 y=Y1; y>=Y0; y--)
    {
        int32 i = y * chunkResolution + X1;

        for (int32 x=X1; x>=X0; x--, i--)
        {
            ChunkIndices.Emplace(i);
        }
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

void FMQCStencil::GetChunks(TArray<FMQCGridChunk*>& Chunks, FMQCMap& Map, const TArray<int32>& ChunkIndices) const
{
    Chunks.Reset(ChunkIndices.Num());

    for (int32 i : ChunkIndices)
    {
        Chunks.Emplace(Map.chunks[i]);
    }
}

void FMQCStencil::SetCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FVector2D& ChunkOffset) const
{
    if (xMin.voxelState != xMax.voxelState)
    {
        FindCrossingX(xMin, xMax, ChunkOffset);
    }
    else
    {
        xMin.xEdge = -1.f;
    }
}

void FMQCStencil::SetCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FVector2D& ChunkOffset) const
{
    if (yMin.voxelState != yMax.voxelState)
    {
        FindCrossingY(yMin, yMax, ChunkOffset);
    }
    else
    {
        yMin.yEdge = -1.f;
    }
}

void FMQCStencil::EditMap(FMQCMap& Map, const FVector2D& center)
{
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;

    TArray<int32> ChunkIndices;
    TArray<FMQCGridChunk*> Chunks;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(ChunkIndices, voxelResolution, chunkResolution);
    GetChunks(Chunks, Map, ChunkIndices);

    SetVoxels(Chunks);
    SetCrossings(Chunks);
}

void FMQCStencil::EditStates(FMQCMap& Map, const FVector2D& center)
{
    check(0);
#if 0
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;
    int32 X0, X1, Y0, Y1;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(X0, X1, Y0, Y1, voxelResolution, chunkResolution);

    for (int32 y=Y1; y>=Y0; y--)
    {
        int32 i = y * chunkResolution + X1;

        for (int32 x=X1; x>=X0; x--, i--)
        {
            SetVoxels(*Map.chunks[i]);
        }
    }
#endif
}

void FMQCStencil::EditCrossings(FMQCMap& Map, const FVector2D& center)
{
    check(0);
#if 0
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;
    int32 X0, X1, Y0, Y1;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(X0, X1, Y0, Y1, voxelResolution, chunkResolution);

    for (int32 y=Y1; y>=Y0; y--)
    {
        int32 i = y * chunkResolution + X1;

        for (int32 x=X1; x>=X0; x--, i--)
        {
            SetCrossings(*Map.chunks[i]);
        }
    }
#endif
}

void FMQCStencil::GetChunkIndices(FMQCMap& Map, const FVector2D& center, TArray<int32>& OutIndices)
{
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;
    int32 X0, X1, Y0, Y1;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(X0, X1, Y0, Y1, voxelResolution, chunkResolution);

    OutIndices.Reset(chunkResolution * chunkResolution);

    for (int32 y=Y1; y>=Y0; y--)
    {
        int32 i = y * chunkResolution + X1;

        for (int32 x=X1; x>=X0; x--, i--)
        {
            OutIndices.Emplace(i);
        }
    }

    OutIndices.Shrink();
}

void FMQCStencil::SetVoxels(FMQCGridChunk& Chunk)
{
    int32 X0, X1, Y0, Y1;
    GetChunkRange(X0, X1, Y0, Y1, Chunk);
    Chunk.SetStates(*this, X0, X1, Y0, Y1);
}

void FMQCStencil::SetVoxels(const TArray<FMQCGridChunk*>& Chunks)
{
    if (bEnableAsync)
    {
        FMQCStencil* Stencil(this);
        // ParallelFor() is used instead of SetStatesAsync() because
        // most other stencil operations rely on synchronized states result
        ParallelFor(
            Chunks.Num(),
            [Stencil, &Chunks](int32 ThreadIndex)
            {
                FMQCGridChunk* Chunk(Chunks[ThreadIndex]);
                int32 X0, X1, Y0, Y1;
                Stencil->GetChunkRange(X0, X1, Y0, Y1, *Chunk);
                Chunk->SetStates(*Stencil, X0, X1, Y0, Y1);
            } );
    }
    else
    {
        for (FMQCGridChunk* Chunk : Chunks)
        {
            int32 X0, X1, Y0, Y1;
            GetChunkRange(X0, X1, Y0, Y1, *Chunk);
            Chunk->SetStates(*this, X0, X1, Y0, Y1);
        }
    }
}

void FMQCStencil::SetCrossings(FMQCGridChunk& Chunk)
{
    int32 X0, X1, Y0, Y1;
    GetChunkRange(X0, X1, Y0, Y1, Chunk);
    Chunk.SetCrossings(*this, X0, X1, Y0, Y1);
}

void FMQCStencil::SetCrossings(const TArray<FMQCGridChunk*>& Chunks)
{
    for (FMQCGridChunk* Chunk : Chunks)
    {
        int32 X0, X1, Y0, Y1;
        GetChunkRange(X0, X1, Y0, Y1, *Chunk);

        if (bEnableAsync)
        {
            Chunk->SetCrossingsAsync(*this, X0, X1, Y0, Y1);
        }
        else
        {
            Chunk->SetCrossings(*this, X0, X1, Y0, Y1);
        }
    }
}

void FMQCStencil::ApplyVoxel(FMQCVoxel& voxel) const
{
    const FVector2D& p(voxel.position);

    if (p.X >= GetXStart() && p.X <= GetXEnd() && p.Y >= GetYStart() && p.Y <= GetYEnd())
    {
        voxel.voxelState = fillType;
    }
}

void FMQCStencil::ApplyVoxel(FMQCVoxel& voxel, const FVector2D& ChunkOffset) const
{
    const FVector2D& p(voxel.position);

    float X0, X1, Y0, Y1;
    GetOffsetBounds(X0, X1, Y0, Y1, ChunkOffset);

    if (p.X >= X0 && p.X <= X1 && p.Y >= Y0 && p.Y <= Y1)
    {
        voxel.voxelState = fillType;
    }
}
