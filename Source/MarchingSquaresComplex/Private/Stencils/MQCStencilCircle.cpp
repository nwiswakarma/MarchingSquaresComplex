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

void FMQCStencilCircle::FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FVector2D& ChunkOffset) const
{
    float ChunkCenterX = centerX - ChunkOffset.X;
    float ChunkCenterY = centerY - ChunkOffset.Y;

    float y2 = xMin.position.Y - ChunkCenterY;
    y2 *= y2;

    if (xMin.voxelState == fillType)
    {
        float x = xMin.position.X - ChunkCenterX;
        if (x * x + y2 <= sqrRadius)
        {
            x = ChunkCenterX + FMath::Sqrt(sqrRadius - y2);
            const float xEdge = x-xMin.position.X;
            if (xMin.xEdge < 0.f || xMin.xEdge < xEdge)
            {
                xMin.xEdge = xEdge;
                xMin.xNormal = ComputeNormal(x, xMin.position.Y, xMax);
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
        float x = xMax.position.X - ChunkCenterX;
        if (x * x + y2 <= sqrRadius)
        {
            x = ChunkCenterX - FMath::Sqrt(sqrRadius - y2);
            const float xEdge = 1.f - (xMax.position.X-x);
            if (xMin.xEdge < 0.f || xMin.xEdge > xEdge)
            {
                xMin.xEdge = xEdge;
                xMin.xNormal = ComputeNormal(x, xMin.position.Y, xMin);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
}

void FMQCStencilCircle::FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FVector2D& ChunkOffset) const
{
    float ChunkCenterX = centerX - ChunkOffset.X;
    float ChunkCenterY = centerY - ChunkOffset.Y;

    float x2 = yMin.position.X - ChunkCenterX;
    x2 *= x2;

    if (yMin.voxelState == fillType)
    {
        float y = yMin.position.Y - ChunkCenterY;
        if (y * y + x2 <= sqrRadius)
        {
            y = ChunkCenterY + FMath::Sqrt(sqrRadius - x2);
            const float yEdge = y-yMin.position.Y;
            if (yMin.yEdge < 0.f || yMin.yEdge < yEdge)
            {
                yMin.yEdge = yEdge;
                yMin.yNormal = ComputeNormal(yMin.position.X, y, yMax);
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
        float y = yMax.position.Y - ChunkCenterY;
        if (y * y + x2 <= sqrRadius)
        {
            y = ChunkCenterY - FMath::Sqrt(sqrRadius - x2);
            const float yEdge = 1.f - (yMax.position.Y-y);
            if (yMin.yEdge < 0.f || yMin.yEdge > yEdge)
            {
                yMin.yEdge = yEdge;
                yMin.yNormal = ComputeNormal(yMin.position.X, y, yMin);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
}

void FMQCStencilCircle::ApplyVoxel(FMQCVoxel& voxel, const FVector2D& ChunkOffset) const
{
    float ChunkCenterX = centerX - ChunkOffset.X;
    float ChunkCenterY = centerY - ChunkOffset.Y;

    float x = voxel.position.X - ChunkCenterX;
    float y = voxel.position.Y - ChunkCenterY;

    if (x * x + y * y <= sqrRadius)
    {
        voxel.voxelState = fillType;
    }
}
