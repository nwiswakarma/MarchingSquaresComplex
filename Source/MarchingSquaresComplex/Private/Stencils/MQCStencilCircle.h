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
#include "Stencils/MQCStencilSquare.h"
#include "MQCStencilCircle.generated.h"

class FMQCStencilCircle : public FMQCStencilSquare
{
private:

	float sqrRadius;

	FVector2D ComputeNormal(float x, float y, const FMQCVoxel& other) const
    {
		if (fillType > other.state)
        {
			return FVector2D(x - centerX, y - centerY).GetSafeNormal();
		}
		else
        {
			return FVector2D(centerX - x, centerY - y).GetSafeNormal();
		}
	}

protected:

    virtual void FindHorizontalCrossing(FMQCVoxel& xMin, const FMQCVoxel& xMax) const override;
    virtual void FindVerticalCrossing(FMQCVoxel& yMin, const FMQCVoxel& yMax) const override;

public:
	
    virtual void Initialize(const FMQCMap& VoxelMap) override
    {
        FMQCStencilSquare::Initialize(VoxelMap);
		sqrRadius = radius * radius;
	}
	
    FORCEINLINE virtual void ApplyVoxel(FMQCVoxel& voxel) const override
    {
		float x = voxel.position.X - centerX;
		float y = voxel.position.Y - centerY;

		if (x * x + y * y <= sqrRadius)
        {
			voxel.state = fillType;
		}
	}
};

UCLASS(BlueprintType)
class UMQCStencilCircleRef : public UMQCStencilRef
{
    GENERATED_BODY()

    FMQCStencilCircle Stencil;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    float Radius = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    int32 FillType = 0;

    virtual void EditMap(UMQCMapRef* MapRef, FVector2D Center) override
    {
        if (IsValid(MapRef) && MapRef->IsInitialized())
        {
            Stencil.RadiusSetting = Radius;
            Stencil.FillTypeSetting = FillType;
            Stencil.EditMap(MapRef->GetMap(), Center);
        }
    }

    virtual TArray<int32> GetChunkIndicesAt(UMQCMapRef* MapRef, FVector2D Center) override
    {
        TArray<int32> ChunkIndices;

        if (IsValid(MapRef) && MapRef->IsInitialized())
        {
            Stencil.RadiusSetting = Radius;
            Stencil.FillTypeSetting = FillType;
            Stencil.GetChunkIndices(MapRef->GetMap(), Center, ChunkIndices);
        }

        return ChunkIndices;
    }
};
