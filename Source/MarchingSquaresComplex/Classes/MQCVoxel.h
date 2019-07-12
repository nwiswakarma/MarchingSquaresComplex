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
#include "MQCMaterial.h"

#define MQC_ENCODE_EDGE_CONST 254.999f
#define MQC_DECODE_EDGE_CONST 0.003937f

struct MARCHINGSQUARESCOMPLEX_API FMQCVoxel
{
    uint8 voxelState;
    uint8 pointState;
    FMQCMaterial Material;

    uint8 EdgeX;
    uint8 EdgeY;

    FIntPoint Position;
    FVector2D xNormal;
    FVector2D yNormal;

    FMQCVoxel()
        : voxelState(0)
        , pointState(0)
        , EdgeX(0xFF)
        , EdgeY(0xFF)
    {
    }

    FMQCVoxel(int32 x, int32 y)
    {
        Set(x, y);
    }

    FORCEINLINE static uint8 EncodeEdge(float Alpha)
    {
        return (uint8)(FMath::Clamp(Alpha, 0.f, 1.f) * MQC_ENCODE_EDGE_CONST);
    }

    FORCEINLINE bool IsFilled() const
    {
        return voxelState > 0;
    }

    FORCEINLINE void InvalidateEdgeX()
    {
        EdgeX = 0xFF;
    }

    FORCEINLINE void InvalidateEdgeY()
    {
        EdgeY = 0xFF;
    }

    FORCEINLINE bool HasValidEdgeX() const
    {
        return EdgeX < 0xFF;
    }

    FORCEINLINE bool HasValidEdgeY() const
    {
        return EdgeY < 0xFF;
    }

    FORCEINLINE float GetXEdge() const
    {
        return HasValidEdgeX() ? ((float)(EdgeX)*MQC_DECODE_EDGE_CONST) : 0.f;
    }

    FORCEINLINE float GetYEdge() const
    {
        return HasValidEdgeY() ? ((float)(EdgeY)*MQC_DECODE_EDGE_CONST) : 0.f;
    }

    FORCEINLINE FVector2D GetXEdgePoint() const
    {
        return FVector2D(Position.X+GetXEdge(), Position.Y);
    }
    
    FORCEINLINE FVector2D GetYEdgePoint() const
    {
        return FVector2D(Position.X, Position.Y+GetYEdge());
    }

    FORCEINLINE FVector2D GetPosition() const
    {
        return Position;
    }

    FORCEINLINE void Reset()
    {
        EdgeX = 0xFF;
        EdgeY = 0xFF;
        voxelState = 0;
        pointState = 0;
        Material = FMQCMaterial(ForceInitToZero);
    }

    FORCEINLINE void Set(int32 X, int32 Y)
    {
        Reset();
        Position.X = X;
        Position.Y = Y;
    }

    FORCEINLINE void BecomeXDummyOf(const FMQCVoxel& Voxel, int32 Offset)
    {
        (*this) = Voxel;
        Position.X += Offset;
    }
    
    FORCEINLINE void BecomeYDummyOf(const FMQCVoxel& Voxel, float Offset)
    {
        (*this) = Voxel;
        Position.Y += Offset;
    }

    FORCEINLINE void BecomeXYDummyOf(const FMQCVoxel& Voxel, float Offset)
    {
        (*this) = Voxel;
        Position.X += Offset;
        Position.Y += Offset;
    }
};
