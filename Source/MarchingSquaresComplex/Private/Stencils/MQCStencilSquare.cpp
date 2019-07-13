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

#include "MQCStencilSquare.h"

void FMQCStencilSquare::Initialize(const FMQCMap& VoxelMap)
{
    FMQCStencil::Initialize(VoxelMap);
    radius = FMath::Max(0.f, RadiusSetting);
}

void FMQCStencilSquare::FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FIntPoint& ChunkOffset) const
{
    FVector2D PositionMin = xMin.GetPosition();
    FVector2D PositionMax = xMax.GetPosition();

    float X0, X1, Y0, Y1;
    X0 = (centerX-radius)-ChunkOffset.X;
    X1 = (centerX+radius)-ChunkOffset.X;
    Y0 = (centerY-radius)-ChunkOffset.Y;
    Y1 = (centerY+radius)-ChunkOffset.Y;

    if (PositionMin.Y < Y0 || PositionMin.Y > Y1)
    {
        return;
    }

    if (xMin.voxelState == fillType)
    {
        const float x = X1;
        if (PositionMin.X <= x && PositionMax.X >= x)
        {
            const float EdgeAlpha = x-PositionMin.X;
            const uint8 EdgeX = FMQCVoxel::EncodeEdge(EdgeAlpha);
            if (!xMin.HasValidEdgeX() || xMin.EdgeX < EdgeX)
            {
                xMin.EdgeX = EdgeX;
                xMin.SetNormalX(fillType ? MAX_int8 : MIN_int8, 0);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
    else
    if (xMax.voxelState == fillType)
    {
        const float x = X0;
        if (PositionMin.X <= x && PositionMax.X >= x)
        {
            const float EdgeAlpha = 1.f - (PositionMax.X-x);
            const uint8 EdgeX = FMQCVoxel::EncodeEdge(EdgeAlpha);
            if (!xMin.HasValidEdgeX() || xMin.EdgeX > EdgeX)
            {
                xMin.EdgeX = EdgeX;
                xMin.SetNormalX(fillType ? MIN_int8 : MAX_int8, 0);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
}

void FMQCStencilSquare::FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FIntPoint& ChunkOffset) const
{
    FVector2D PositionMin = yMin.GetPosition();
    FVector2D PositionMax = yMax.GetPosition();

    float X0, X1, Y0, Y1;
    X0 = (centerX-radius)-ChunkOffset.X;
    X1 = (centerX+radius)-ChunkOffset.X;
    Y0 = (centerY-radius)-ChunkOffset.Y;
    Y1 = (centerY+radius)-ChunkOffset.Y;

    if (PositionMin.X < X0 || PositionMin.X > X1)
    {
        return;
    }

    if (yMin.voxelState == fillType)
    {
        const float y = Y1;
        if (PositionMin.Y <= y && PositionMax.Y >= y)
        {
            const float EdgeAlpha = y-PositionMin.Y;
            const uint8 EdgeY = FMQCVoxel::EncodeEdge(EdgeAlpha);
            if (!yMin.HasValidEdgeY() || yMin.EdgeY < EdgeY)
            {
                yMin.EdgeY = EdgeY;
                yMin.SetNormalY(0, fillType ? MAX_int8 : MIN_int8);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
    else
    if (yMax.voxelState == fillType)
    {
        const float y = Y0;
        if (PositionMin.Y <= y && PositionMax.Y >= y)
        {
            const float EdgeAlpha = 1.f - (PositionMax.Y-y);
            const uint8 EdgeY = FMQCVoxel::EncodeEdge(EdgeAlpha);
            if (!yMin.HasValidEdgeY() || yMin.EdgeY > EdgeY)
            {
                yMin.EdgeY = EdgeY;
                yMin.SetNormalY(0, fillType ? MIN_int8 : MAX_int8);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
}

void FMQCStencilSquare::ApplyVoxel(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
{
    FVector2D VoxelToChunk(GetVoxelToChunk(Voxel, ChunkOffset));

    if (FMath::Abs(VoxelToChunk.X) <= radius &&
        FMath::Abs(VoxelToChunk.Y) <= radius)
    {
        Voxel.voxelState = fillType;
    }
}
