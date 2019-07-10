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

#include "MQCStencilTri.h"

#ifndef MQC_VOXEL_DEBUG_LEGACY

void FMQCStencilTri::FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FIntPoint& ChunkOffset) const
{
    FVector Segment0(xMin.position, 0.f);
    FVector Segment1(xMax.position.X, Segment0.Y, 0.f);

    FVector Intersection;
    FVector2D Normal;

    if (FindIntersection(Segment0, Segment1, Intersection, Normal))
    {
        const float x = Intersection.X;
        if (xMin.voxelState == fillType)
        {
            const float xEdge = x-xMin.position.X;
            if (xMin.xEdge < 0.f || xMin.xEdge < xEdge)
            {
                xMin.xEdge = xEdge;
                xMin.xNormal = ComputeNormal(Normal, xMax);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
        else
        if (xMax.voxelState == fillType)
        {
            const float xEdge = 1.f - (xMax.position.X-x);
            if (xMin.xEdge < 0.f || xMin.xEdge > xEdge)
            {
                xMin.xEdge = xEdge;
                xMin.xNormal = ComputeNormal(Normal, xMin);
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
}

void FMQCStencilTri::FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FIntPoint& ChunkOffset) const
{
    FVector Segment0(yMin.position, 0.f);
    FVector Segment1(Segment0.X, yMax.position.Y, 0.f);

    FVector Intersection;
    FVector2D Normal;

    if (FindIntersection(Segment0, Segment1, Intersection, Normal))
    {
        const float y = Intersection.Y;
        if (yMin.voxelState == fillType)
        {
            const float yEdge = y-yMin.position.Y;
            if (yMin.yEdge < 0.f || yMin.yEdge < yEdge)
            {
                yMin.yEdge = yEdge;
                yMin.yNormal = ComputeNormal(Normal, yMax);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
        else
        if (yMax.voxelState == fillType)
        {
            const float yEdge = 1.f - (yMax.position.Y-y);
            if (yMin.yEdge < 0.f || yMin.yEdge > yEdge)
            {
                yMin.yEdge = yEdge;
                yMin.yNormal = ComputeNormal(Normal, yMin);
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
}

#else

void FMQCStencilTri::FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax) const
{
    FVector Segment0(xMin.position, 0.f);
    FVector Segment1(xMax.position.X, Segment0.Y, 0.f);

    FVector Intersection;
    FVector2D Normal;

    if (FindIntersection(Segment0, Segment1, Intersection, Normal))
    {
        if (xMin.voxelState == fillType)
        {
            float x = Intersection.X;
            if (xMin.xEdge == TNumericLimits<float>::Lowest() || xMin.xEdge < x)
            {
                xMin.xEdge = x;
                xMin.xNormal = ComputeNormal(Normal, xMax); //fillType ? Normal : -Normal;
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
        else if (xMax.voxelState == fillType)
        {
            float x = Intersection.X;
            if (xMin.xEdge == TNumericLimits<float>::Lowest() || xMin.xEdge > x)
            {
                xMin.xEdge = x;
                xMin.xNormal = ComputeNormal(Normal, xMin); //fillType ? Normal : -Normal;
            }
            else
            {
                ValidateNormalX(xMin, xMax);
            }
        }
    }
}

void FMQCStencilTri::FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax) const
{
    FVector Segment0(yMin.position, 0.f);
    FVector Segment1(Segment0.X, yMax.position.Y, 0.f);

    FVector Intersection;
    FVector2D Normal;

    if (FindIntersection(Segment0, Segment1, Intersection, Normal))
    {
        if (yMin.voxelState == fillType)
        {
            float y = Intersection.Y;
            if (yMin.yEdge == TNumericLimits<float>::Lowest() || yMin.yEdge < y)
            {
                yMin.yEdge = y;
                yMin.yNormal = ComputeNormal(Normal, yMax); //fillType ? Normal : -Normal;
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
        else if (yMax.voxelState == fillType)
        {
            float y = Intersection.Y;
            if (yMin.yEdge == TNumericLimits<float>::Lowest() || yMin.yEdge > y)
            {
                yMin.yEdge = y;
                yMin.yNormal = ComputeNormal(Normal, yMin); //fillType ? Normal : -Normal;
            }
            else
            {
                ValidateNormalY(yMin, yMax);
            }
        }
    }
}

#endif
