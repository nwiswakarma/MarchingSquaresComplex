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
#include "MQCStencilBox.generated.h"

class FMQCStencilBox : public FMQCStencil
{
protected:

    FBox2D bounds;
    FVector2D boundsCenter;
    FVector2D boundsExtents;

    virtual void FindHorizontalCrossing(FMQCVoxel& xMin, const FMQCVoxel& xMax) const override;
    virtual void FindVerticalCrossing(FMQCVoxel& yMin, const FMQCVoxel& yMax) const override;

public:

    FBox2D BoundsSetting;

    FMQCStencilBox() = default;

    FMQCStencilBox(const FBox2D& InBounds)
    {
        SetBounds(InBounds);
    }

    FMQCStencilBox(const FBox2D& InBounds, const int32 InFillType)
    {
        SetBounds(InBounds);
        FillTypeSetting = InFillType;
    }

    FORCEINLINE const FBox2D& GetBounds() const
    {
        return bounds;
    }

    FORCEINLINE const FVector2D& GetShift() const
    {
        return bounds.Min;
    }

    FORCEINLINE const FVector2D& GetExtents() const
    {
        return boundsExtents;
    }

    FORCEINLINE const FVector2D& GetCenter() const
    {
        return boundsCenter;
    }

    FORCEINLINE virtual float GetXStart() const
    {
        return centerX - boundsExtents.X;
    }
    
    FORCEINLINE virtual float GetXEnd() const
    {
        return centerX + boundsExtents.X;
    }
    
    FORCEINLINE virtual float GetYStart() const
    {
        return centerY - boundsExtents.Y;
    }
    
    FORCEINLINE virtual float GetYEnd() const
    {
        return centerY + boundsExtents.Y;
    }

    void SetBounds(const FBox2D& InBounds)
    {
        bounds = InBounds;
        boundsExtents = bounds.GetExtent();
        bounds.GetCenterAndExtents(boundsCenter, boundsExtents);
    }

    virtual void Initialize(const FMQCMap& VoxelMap) override
    {
        FMQCStencil::Initialize(VoxelMap);

        if (! bounds.bIsValid)
        {
            SetBounds(BoundsSetting);
        }
    }
};

UCLASS(BlueprintType)
class UMQCStencilBoxRef : public UMQCStencilRef
{
    GENERATED_BODY()

    FMQCStencilBox Stencil;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    FBox2D Bounds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    int32 FillType = 0;

    virtual void EditMap(UMQCMapRef* MapRef, FVector2D Center) override
    {
        if (IsValid(MapRef) && MapRef->IsInitialized())
        {
            Stencil.BoundsSetting = Bounds;
            Stencil.FillTypeSetting = FillType;
            Stencil.EditMap(MapRef->GetMap(), Center);
        }
    }
};

UCLASS(BlueprintType)
class UPMUVoxelStencilMultiBoxRef : public UMQCStencilRef
{
    GENERATED_BODY()

    TArray<FMQCStencilBox> Stencils;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    int32 FillType = 0;

    virtual void EditMap(UMQCMapRef* MapRef, FVector2D Center) override
    {
        if (IsValid(MapRef) && MapRef->IsInitialized())
        {
            FMQCMap& Map(MapRef->GetMap());

            // Edit map states
            for (FMQCStencilBox& Stencil : Stencils)
            {
                FVector2D BoxCenter = Center + Stencil.GetCenter();
                Stencil.EditStates(Map, BoxCenter);
            }

            // Edit map voxel crossings
            for (FMQCStencilBox& Stencil : Stencils)
            {
                FVector2D BoxCenter = Center + Stencil.GetCenter();
                Stencil.EditCrossings(Map, BoxCenter);
            }
        }
    }

    UFUNCTION(BlueprintCallable)
    void AddBox(FBox2D Bounds)
    {
        Stencils.Emplace(Bounds, FillType);
    }

    UFUNCTION(BlueprintCallable)
    void AddBoxes(const TArray<FBox2D>& Bounds)
    {
        for (int32 i=0; i<Bounds.Num(); ++i)
        {
            Stencils.Emplace(Bounds[i], FillType);
        }
    }
};
