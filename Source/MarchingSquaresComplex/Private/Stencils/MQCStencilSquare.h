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
#include "MQCVoxel.h"
#include "MQCGridChunk.h"
#include "MQCMap.h"
#include "MQCStencil.h"
#include "MQCStencilSquare.generated.h"

class FMQCStencilSquare : public FMQCStencil
{
protected:

    float radius;

    FORCEINLINE virtual int32 GetBoundsMinX() const
    {
        return FMath::RoundToInt(centerX-radius);
    }

    FORCEINLINE virtual int32 GetBoundsMaxX() const
    {
        return FMath::RoundToInt(centerX+radius);
    }

    FORCEINLINE virtual int32 GetBoundsMinY() const
    {
        return FMath::RoundToInt(centerY-radius);
    }

    FORCEINLINE virtual int32 GetBoundsMaxY() const
    {
        return FMath::RoundToInt(centerY+radius);
    }

    FORCEINLINE FVector2D GetChunkCenter(const FIntPoint& ChunkOffset) const
    {
        return FVector2D(centerX, centerY) - FVector2D(ChunkOffset);
    }

    FORCEINLINE FVector2D GetVoxelToChunk(const FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
    {
        return GetChunkCenter(ChunkOffset) - Voxel.position;
    }

    virtual void FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FIntPoint& ChunkOffset) const override;
    virtual void FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FIntPoint& ChunkOffset) const override;

public:

    float RadiusSetting = 0.f;

    virtual void Initialize(const FMQCMap& VoxelMap) override;
    virtual void ApplyVoxel(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const override;
};

UCLASS()
class UMQCStencilSquareRef : public UMQCStencilRef
{
    GENERATED_BODY()

    FMQCStencilSquare Stencil;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    float Radius = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    uint8 FillType = 0;

    virtual void EditMapAt(UMQCMapRef* MapRef, FVector2D Center) override
    {
        if (IsValid(MapRef) && MapRef->IsInitialized())
        {
            Stencil.RadiusSetting = Radius;
            Stencil.FillTypeSetting = FillType;
            Stencil.EditMap(MapRef->GetMap(), Center);
        }
    }

    virtual void EditMaterialAt(UMQCMapRef* MapRef, FVector2D Center) override
    {
        if (IsValid(MapRef) && MapRef->IsInitialized())
        {
            Stencil.RadiusSetting = Radius;
            Stencil.FillTypeSetting = FillType;
            Stencil.EditMaterial(MapRef->GetMap(), Center);
        }
    }
};
