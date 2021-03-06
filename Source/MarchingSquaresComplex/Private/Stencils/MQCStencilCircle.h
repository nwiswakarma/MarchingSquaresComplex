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
    float MaterialBlendRadius;
    float MaterialBlendRadiusInv;

	FVector2D ComputeNormal(float x, float y, const FMQCVoxel& other) const;

protected:

    virtual void FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FIntPoint& ChunkOffset) const override;
    virtual void FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FIntPoint& ChunkOffset) const override;
    virtual FMQCMaterial GetMaterialFor(const FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const override;

public:

    float MaterialBlendRadiusSetting;

    virtual void Initialize(const FMQCMap& VoxelMap) override;
    virtual void ApplyVoxel(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const override;
    virtual void ApplyMaterial(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const override;
};

UCLASS(BlueprintType)
class UMQCStencilCircleRef : public UMQCStencilRef
{
    GENERATED_BODY()

    FMQCStencilCircle Stencil;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    float Radius = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    uint8 FillType = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 MaterialIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor MaterialColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMQCMaterialBlendType MaterialBlendType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaterialBlendRadius = 1.f;

    virtual void EditMapAt(UMQCMapRef* MapRef, FVector2D Center) override;
    virtual void EditMaterialAt(UMQCMapRef* MapRef, FVector2D Center) override;
};
