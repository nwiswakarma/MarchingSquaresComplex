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

#define MQC_VOXEL_EDGE_MAX 0xFE
#define MQC_ENCODE_EDGE_CONST 254.999f
#define MQC_DECODE_EDGE_CONST 0.003937f
#define MQC_MIN_SNORM8 -127
#define MQC_MAX_SNORM8 127

struct MARCHINGSQUARESCOMPLEX_API FMQCPointNormal
{
public: 
	union
	{
		struct
		{
			int8	X, Y;

		};
		uint16		Packed;
	}				Vector;

	FMQCPointNormal() { Vector.Packed = 0; }
	FMQCPointNormal(const FVector2D& InVector) { *this = InVector; }
	FMQCPointNormal(int8 InX, int8 InY) { Vector.X = InX; Vector.Y = InY; }

	void operator=(const FVector2D& InVector);
	FMQCPointNormal operator-() const;
	FVector2D ToFVector2D() const;
};

FORCEINLINE void FMQCPointNormal::operator=(const FVector2D& InVector)
{
	const float Scale = MAX_int8;
	Vector.X = (int8)FMath::Clamp<int32>(FMath::RoundToInt(InVector.X * Scale), MIN_int8, MAX_int8);
	Vector.Y = (int8)FMath::Clamp<int32>(FMath::RoundToInt(InVector.Y * Scale), MIN_int8, MAX_int8);
}

FORCEINLINE FMQCPointNormal FMQCPointNormal::operator-() const
{
	return FMQCPointNormal(-Vector.X, -Vector.Y);
}

FORCEINLINE FVector2D FMQCPointNormal::ToFVector2D() const
{
	return FVector2D(Vector.X, Vector.Y) / 127.f;
}

struct MARCHINGSQUARESCOMPLEX_API FMQCVoxel
{
    uint8 voxelState;
    uint8 pointState;
    FMQCMaterial Material;

    uint8 EdgeX;
    uint8 EdgeY;

    FIntPoint Position;
    FMQCPointNormal NormalX;
    FMQCPointNormal NormalY;

    // Query

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

    FORCEINLINE FMQCMaterial GetMaterial() const
    {
        return Material;
    }

    // Mutation

    FORCEINLINE void Init()
    {
        EdgeX = 0xFF;
        EdgeY = 0xFF;
        voxelState = 0;
        pointState = 0;
        Material = FMQCMaterial(ForceInitToZero);
    }

    FORCEINLINE void Set(int32 X, int32 Y)
    {
        Init();
        Position.X = X;
        Position.Y = Y;
    }

    FORCEINLINE void SetNormalX(int8 InX, int8 InY)
    {
        NormalX.Vector.X = InX;
        NormalX.Vector.Y = InY;
    }

    FORCEINLINE void SetNormalY(int8 InX, int8 InY)
    {
        NormalY.Vector.X = InX;
        NormalY.Vector.Y = InY;
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

    // Edge Encoding

    FORCEINLINE static uint8 EncodeEdge(float Alpha)
    {
        return (uint8)(FMath::Clamp(Alpha, 0.f, 1.f) * MQC_ENCODE_EDGE_CONST);
    }
};
