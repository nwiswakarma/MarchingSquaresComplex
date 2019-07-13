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

#include "MQCStencilCircle.h"

FVector2D FMQCStencilCircle::ComputeNormal(float x, float y, const FMQCVoxel& other) const
{
    return (fillType > other.voxelState)
        ? FVector2D(x-centerX, y-centerY).GetSafeNormal()
        : FVector2D(centerX-x, centerY-y).GetSafeNormal();
}

void FMQCStencilCircle::FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FIntPoint& ChunkOffset) const
{
    FVector2D PositionMin = xMin.GetPosition();
    FVector2D PositionMax = xMax.GetPosition();

    float ChunkCenterX = centerX - ChunkOffset.X;
    float ChunkCenterY = centerY - ChunkOffset.Y;

    float y2 = PositionMin.Y - ChunkCenterY;
    y2 *= y2;

    if (xMin.voxelState == fillType)
    {
        float x = PositionMin.X - ChunkCenterX;
        if (x * x + y2 <= sqrRadius)
        {
            x = ChunkCenterX + FMath::Sqrt(sqrRadius - y2);
            const float EdgeAlpha = x-PositionMin.X;
            const uint8 EdgeX = FMQCVoxel::EncodeEdge(EdgeAlpha);
            if (!xMin.HasValidEdgeX() || xMin.EdgeX < EdgeX)
            {
                xMin.EdgeX = EdgeX;
                xMin.NormalX = ComputeNormal(x, PositionMin.Y, xMax);
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
        float x = PositionMax.X - ChunkCenterX;
        if (x * x + y2 <= sqrRadius)
        {
            x = ChunkCenterX - FMath::Sqrt(sqrRadius - y2);
            const float EdgeAlpha = 1.f - (PositionMax.X-x);
            const uint8 EdgeX = FMQCVoxel::EncodeEdge(EdgeAlpha);
            if (!xMin.HasValidEdgeX() || xMin.EdgeX > EdgeX)
            {
                xMin.EdgeX = EdgeX;
                xMin.NormalX = ComputeNormal(x, PositionMin.Y, xMin);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
}

void FMQCStencilCircle::FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FIntPoint& ChunkOffset) const
{
    FVector2D PositionMin = yMin.GetPosition();
    FVector2D PositionMax = yMax.GetPosition();

    float ChunkCenterX = centerX - ChunkOffset.X;
    float ChunkCenterY = centerY - ChunkOffset.Y;

    float x2 = PositionMin.X - ChunkCenterX;
    x2 *= x2;

    if (yMin.voxelState == fillType)
    {
        float y = PositionMin.Y - ChunkCenterY;
        if (y * y + x2 <= sqrRadius)
        {
            y = ChunkCenterY + FMath::Sqrt(sqrRadius - x2);
            const float EdgeAlpha = y-PositionMin.Y;
            const uint8 EdgeY = FMQCVoxel::EncodeEdge(EdgeAlpha);
            if (!yMin.HasValidEdgeY() || yMin.EdgeY < EdgeY)
            {
                yMin.EdgeY = EdgeY;
                yMin.NormalY = ComputeNormal(PositionMin.X, y, yMax);
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
        float y = PositionMax.Y - ChunkCenterY;
        if (y * y + x2 <= sqrRadius)
        {
            y = ChunkCenterY - FMath::Sqrt(sqrRadius - x2);
            const float EdgeAlpha = 1.f - (PositionMax.Y-y);
            const uint8 EdgeY = FMQCVoxel::EncodeEdge(EdgeAlpha);
            if (!yMin.HasValidEdgeY() || yMin.EdgeY > EdgeY)
            {
                yMin.EdgeY = EdgeY;
                yMin.NormalY = ComputeNormal(PositionMin.X, y, yMin);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
}

FMQCMaterial FMQCStencilCircle::GetMaterialFor(const FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
{
    float DistToCenter = GetVoxelToChunk(Voxel, ChunkOffset).Size();
    float Alpha = 1.f-FMath::Clamp((DistToCenter-MaterialBlendRadius)*MaterialBlendRadiusInv, 0.f, 1.f);

    FMQCMaterial VoxelMaterial;
    GetMaterialBlendTyped(VoxelMaterial, Voxel.Material, Alpha);

    return VoxelMaterial;
}

void FMQCStencilCircle::Initialize(const FMQCMap& VoxelMap)
{
    FMQCStencilSquare::Initialize(VoxelMap);

    sqrRadius = radius * radius;

    float BlendRadius = FMath::Max(0.f, radius-MaterialBlendRadiusSetting);
    float BlendRange = FMath::Clamp((radius-BlendRadius), 0.f, radius);

    MaterialBlendRadius = BlendRadius;
    MaterialBlendRadiusInv = BlendRange > KINDA_SMALL_NUMBER ? (1.f/BlendRange) : 1.f;
}

void FMQCStencilCircle::ApplyVoxel(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
{
    FVector2D VoxelToChunk(GetVoxelToChunk(Voxel, ChunkOffset));

    if (VoxelToChunk.SizeSquared() <= sqrRadius)
    {
        Voxel.voxelState = fillType;
    }
}

void FMQCStencilCircle::ApplyMaterial(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
{
    FVector2D VoxelToChunk(GetVoxelToChunk(Voxel, ChunkOffset));

    if (VoxelToChunk.SizeSquared() <= sqrRadius)
    {
        Voxel.Material = GetMaterialFor(Voxel, ChunkOffset);
    }
}
