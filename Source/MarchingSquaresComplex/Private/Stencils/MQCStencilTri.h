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
#include "Poly/GULPolyUtilityLibrary.h"
#include "MQCStencil.h"
#include "MQCStencilTri.generated.h"

class FMQCStencilTri : public FMQCStencil
{
private:

    FVector Offsets[3];
    FVector Pos[3];
    FVector2D Nrm[3];

    FVector2D Shift;
    FBox2D Bounds;

    bool FindIntersection(const FVector& SegmentStart, const FVector& SegmentEnd, FVector& Intersection, FVector2D& Normal) const
    {
        if (FMath::SegmentIntersection2D(SegmentStart, SegmentEnd, Pos[0], Pos[1], Intersection))
        {
            Normal = Nrm[0];
            return true;
        }

        if (FMath::SegmentIntersection2D(SegmentStart, SegmentEnd, Pos[1], Pos[2], Intersection))
        {
            Normal = Nrm[1];
            return true;
        }

        if (FMath::SegmentIntersection2D(SegmentStart, SegmentEnd, Pos[2], Pos[0], Intersection))
        {
            Normal = Nrm[2];
            return true;
        }

        return false;
    }

	FORCEINLINE FVector2D ComputeNormal(const FVector2D& Normal, const FMQCVoxel& other) const
    {
        return (fillType > other.voxelState) ? -Normal : Normal;
	}

protected:

    FORCEINLINE virtual int32 GetBoundsMinX() const override
    {
        return centerX - Bounds.GetExtent().X;
    }
    
    FORCEINLINE virtual int32 GetBoundsMaxX() const override
    {
        return centerX + Bounds.GetExtent().X;
    }
    
    FORCEINLINE virtual int32 GetBoundsMinY() const override
    {
        return centerY - Bounds.GetExtent().Y;
    }
    
    FORCEINLINE virtual int32 GetBoundsMaxY() const override
    {
        return centerY + Bounds.GetExtent().Y;
    }

    virtual void FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FVector2D& ChunkOffset) const override;
    virtual void FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FVector2D& ChunkOffset) const override;

public:

    FMQCStencilTri() = default;

    FMQCStencilTri(const FVector& Pos0, const FVector& Pos1, const FVector& Pos2)
    {
        SetPositions(Pos0, Pos1, Pos2);
    }

    FMQCStencilTri(const FVector& Pos0, const FVector& Pos1, const FVector& Pos2, const int32 InFillType)
    {
        SetPositions(Pos0, Pos1, Pos2);
        FillTypeSetting = InFillType;
    }

    FORCEINLINE const FBox2D& GetBounds() const
    {
        return Bounds;
    }

    FORCEINLINE const FVector2D& GetShift() const
    {
        return Shift;
    }

    FORCEINLINE FVector2D GetShiftedBoundsCenter() const
    {
        return Shift + Bounds.GetCenter();
    }

    virtual void SetCenter(float x, float y) override
    {
        FMQCStencil::SetCenter(x, y);

        // Set triangle vertices

        const float x0 = GetBoundsMinX();
        const float y0 = GetBoundsMinY();
        const float x1 = GetBoundsMaxX();
        const float y1 = GetBoundsMaxY();

        for (int32 i=0; i<3; ++i)
        {
            const FVector& Offset(Offsets[i]);
            FVector& PosRef(Pos[i]);
            PosRef.X = FMath::Clamp(x0+Offset.X, x0, x1);
            PosRef.Y = FMath::Clamp(y0+Offset.Y, y0, y1);
            PosRef.Z = 0.f;
        }
    }

    FORCEINLINE virtual void ApplyVoxel(FMQCVoxel& Voxel, const FVector2D& ChunkOffset) const override
    {
        check(0);
#if 0
        if (UGULPolyUtilityLibrary::IsPointOnTri(FVector(Voxel.position, 0.f), Pos[0], Pos[1], Pos[2]))
        {
            Voxel.voxelState = fillType;
        }
#endif
    }

    void SetPositions(const FVector& Pos0, const FVector& Pos1, const FVector& Pos2)
    {
        // Calculate tri bounds

        Bounds = FBox2D(ForceInit);
        Bounds += FVector2D(Pos0);
        Bounds += FVector2D(Pos1);
        Bounds += FVector2D(Pos2);

        Shift = Bounds.Min;
        Bounds = Bounds.ShiftBy(-Shift);

        // Calculate local vertices

        Offsets[0] = Pos0 - FVector(Shift, 0.f);
        Offsets[1] = Pos1 - FVector(Shift, 0.f);
        Offsets[2] = Pos2 - FVector(Shift, 0.f);

        // Calculate tri normals

        FVector Nrm01 = (Pos0 - Pos1).GetUnsafeNormal();
        FVector Nrm12 = (Pos1 - Pos2).GetUnsafeNormal();
        FVector Nrm20 = (Pos2 - Pos0).GetUnsafeNormal();

        Nrm[0].Set(-Nrm01.Y, Nrm01.X);
        Nrm[1].Set(-Nrm12.Y, Nrm12.X);
        Nrm[2].Set(-Nrm20.Y, Nrm20.X);
    }
};

UCLASS()
class UMQCStencilTriRef : public UMQCStencilRef
{
    GENERATED_BODY()

    FMQCStencilTri Stencil;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
    int32 FillType = 0;

    UFUNCTION(BlueprintCallable)
    FVector2D GetShift() const
    {
        return Stencil.GetShift();
    }

    UFUNCTION(BlueprintCallable)
    FVector2D GetBoundsCenter() const
    {
        return Stencil.GetBounds().GetCenter();
    }

    UFUNCTION(BlueprintCallable)
    FVector2D GetShiftedBoundsCenter() const
    {
        return Stencil.GetShiftedBoundsCenter();
    }

    UFUNCTION(BlueprintCallable)
    void SetPositions(FVector Pos0, FVector Pos1, FVector Pos2, bool bInverse = false)
    {
        if (bInverse)
        {
            Stencil.SetPositions(Pos2, Pos1, Pos0);
        }
        else
        {
            Stencil.SetPositions(Pos0, Pos1, Pos2);
        }
    }

    virtual void EditMapAt(UMQCMapRef* MapRef, FVector2D Center) override
    {
        if (IsValid(MapRef) && MapRef->IsInitialized())
        {
            Stencil.FillTypeSetting = FillType;
            Stencil.EditMap(MapRef->GetMap(), Center);
        }
    }
};
