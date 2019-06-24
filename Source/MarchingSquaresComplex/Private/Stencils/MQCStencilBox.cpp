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

#include "MQCStencilBox.h"

#ifndef MQC_VOXEL_DEBUG_LEGACY

void FMQCStencilBox::FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FVector2D& ChunkOffset) const
{
    if (xMin.position.Y < GetYStart() || xMin.position.Y > GetYEnd())
    {
        return;
    }

    if (xMin.voxelState == fillType)
    {
        const float x = GetXEnd();
        if (xMin.position.X <= x && xMax.position.X >= x)
        {
            const float xEdge = x-xMin.position.X;
            if (xMin.xEdge < 0.f || xMin.xEdge < xEdge)
            {
                xMin.xEdge = x-xMin.position.X;
                xMin.xNormal = FVector2D(fillType ? 1.f : -1.f, 0.f);
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
        const float x = GetXStart();
        if (xMin.position.X <= x && xMax.position.X >= x)
        {
            const float xEdge = 1.f - (xMax.position.X-x);
            if (xMin.xEdge < 0.f || xMin.xEdge > xEdge)
            {
                xMin.xEdge = xEdge;
                xMin.xNormal = FVector2D(fillType ? -1.f : 1.f, 0.f);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
}

void FMQCStencilBox::FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FVector2D& ChunkOffset) const
{
    if (yMin.position.X < GetXStart() || yMin.position.X > GetXEnd())
    {
        return;
    }

    if (yMin.voxelState == fillType)
    {
        const float y = GetYEnd();
        if (yMin.position.Y <= y && yMax.position.Y >= y)
        {
            const float yEdge = y-yMin.position.Y;
            if (yMin.yEdge < 0.f || yMin.yEdge < yEdge)
            {
                yMin.yEdge = yEdge;
                yMin.yNormal = FVector2D(0.f, fillType ? 1.f : -1.f);
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
        const float y = GetYStart();
        if (yMin.position.Y <= y && yMax.position.Y >= y)
        {
            const float yEdge = 1.f - (yMax.position.Y-y);
            if (yMin.yEdge < 0.f || yMin.yEdge > yEdge)
            {
                yMin.yEdge = yEdge;
                yMin.yNormal = FVector2D(0.f, fillType ? -1.f : 1.f);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
}

#else

void FMQCStencilBox::FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax) const
{
    if (xMin.position.Y < GetYStart() || xMin.position.Y > GetYEnd())
    {
        return;
    }

    if (xMin.voxelState == fillType)
    {
        if (xMin.position.X <= GetXEnd() && xMax.position.X >= GetXEnd())
        {
            if (xMin.xEdge == TNumericLimits<float>::Lowest() || xMin.xEdge < GetXEnd())
            {
                xMin.xEdge = GetXEnd();
                xMin.xNormal = FVector2D(fillType ? 1.f : -1.f, 0.f);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
    else if (xMax.voxelState == fillType)
    {
        if (xMin.position.X <= GetXStart() && xMax.position.X >= GetXStart())
        {
            if (xMin.xEdge == TNumericLimits<float>::Lowest() || xMin.xEdge > GetXStart())
            {
                xMin.xEdge = GetXStart();
                xMin.xNormal = FVector2D(fillType ? -1.f : 1.f, 0.f);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
}

void FMQCStencilBox::FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax) const
{
    if (yMin.position.X < GetXStart() || yMin.position.X > GetXEnd())
    {
        return;
    }

    if (yMin.voxelState == fillType)
    {
        if (yMin.position.Y <= GetYEnd() && yMax.position.Y >= GetYEnd())
        {
            if (yMin.yEdge == TNumericLimits<float>::Lowest() || yMin.yEdge < GetYEnd())
            {
                yMin.yEdge = GetYEnd();
                yMin.yNormal = FVector2D(0.f, fillType ? 1.f : -1.f);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
    else if (yMax.voxelState == fillType)
    {
        if (yMin.position.Y <= GetYStart() && yMax.position.Y >= GetYStart())
        {
            if (yMin.yEdge == TNumericLimits<float>::Lowest() || yMin.yEdge > GetYStart())
            {
                yMin.yEdge = GetYStart();
                yMin.yNormal = FVector2D(0.f, fillType ? -1.f : 1.f);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
}

#endif
