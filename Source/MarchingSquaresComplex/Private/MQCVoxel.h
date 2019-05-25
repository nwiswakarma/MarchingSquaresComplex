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

#pragma once

#include "CoreMinimal.h"

enum class EPMUVoxelDT : uint8
{
	X,
	Y,
	XY
};

struct FMQCVoxel
{
    int32 state = 0;

    float xEdge;
    float yEdge;

    FVector2D position;
    FVector2D xNormal;
    FVector2D yNormal;

    FMQCVoxel() = default;

    FMQCVoxel(int32 x, int32 y, float size)
    {
        Set(x, y, size);
    }

    FORCEINLINE bool IsFilled() const
    {
        return state > 0;
    }

    FORCEINLINE FVector2D GetXEdgePoint() const
    {
#ifndef MQC_VOXEL_DEBUG_LEGACY
        return FVector2D(position.X+FMath::Max(0.f, xEdge), position.Y);
#else
        return FVector2D(xEdge, position.Y);
#endif
    }
    
    FORCEINLINE FVector2D GetYEdgePoint() const
    {
#ifndef MQC_VOXEL_DEBUG_LEGACY
        return FVector2D(position.X, position.Y+FMath::Max(0.f, yEdge));
#else
        return FVector2D(position.X, yEdge);
#endif
    }

    FORCEINLINE void Reset()
    {
        state = 0;
#ifndef MQC_VOXEL_DEBUG_LEGACY
        xEdge = -1.f;
        yEdge = -1.f;
#else
        xEdge = TNumericLimits<float>::Lowest();
        yEdge = TNumericLimits<float>::Lowest();
#endif
    }

    FORCEINLINE void Set(int32 x, int32 y, float size)
    {
        position.X = (x + 0.5f) * size;
        position.Y = (y + 0.5f) * size;

#ifndef MQC_VOXEL_DEBUG_LEGACY
        xEdge = -1.f;
        yEdge = -1.f;
#else
        xEdge = TNumericLimits<float>::Lowest();
        yEdge = TNumericLimits<float>::Lowest();
#endif

        state = 0;
    }

    FORCEINLINE void BecomeXDummyOf(const FMQCVoxel& voxel, float offset)
    {
        state = voxel.state;
        position = voxel.position;
        position.X += offset;
#ifndef MQC_VOXEL_DEBUG_LEGACY
        xEdge = voxel.xEdge;
        yEdge = voxel.yEdge;
#else
        xEdge = voxel.xEdge + offset;
        yEdge = voxel.yEdge;
#endif
        yNormal = voxel.yNormal;
    }
    
    FORCEINLINE void BecomeYDummyOf(const FMQCVoxel& voxel, float offset)
    {
        state = voxel.state;
        position = voxel.position;
        position.Y += offset;
#ifndef MQC_VOXEL_DEBUG_LEGACY
        xEdge = voxel.xEdge;
        yEdge = voxel.yEdge;
#else
        xEdge = voxel.xEdge;
        yEdge = voxel.yEdge + offset;
#endif
        xNormal = voxel.xNormal;
    }

    FORCEINLINE void BecomeXYDummyOf(const FMQCVoxel& voxel, float offset)
    {
        state = voxel.state;
        position = voxel.position;
        position.X += offset;
        position.Y += offset;
#ifndef MQC_VOXEL_DEBUG_LEGACY
        xEdge = voxel.xEdge;
        yEdge = voxel.yEdge;
#else
        xEdge = voxel.xEdge + offset;
        yEdge = voxel.yEdge + offset;
#endif
    }
};
