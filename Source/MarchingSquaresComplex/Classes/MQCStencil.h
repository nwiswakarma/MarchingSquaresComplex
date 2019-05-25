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
#include "MQCMap.h"
#include "MQCStencil.generated.h"

class FMQCGridChunk;
struct FMQCVoxel;

class FMQCStencil
{
protected:

    int32 fillType;
    float centerX;
    float centerY;

	static void ValidateHorizontalNormal(FMQCVoxel& xMin, const FMQCVoxel& xMax);
	static void ValidateVerticalNormal(FMQCVoxel& yMin, const FMQCVoxel& yMax);

    void GetMapRange(int32& x0, int32& x1, int32& y0, int32& y1, const float voxelSize, const float chunkSize, const int32 chunkResolution) const
    {
        x0 = (int32)((GetXStart() - voxelSize) / chunkSize);
        if (x0 < 0)
        {
            x0 = 0;
        }

        x1 = (int32)((GetXEnd() + voxelSize) / chunkSize);
        if (x1 >= chunkResolution)
        {
            x1 = chunkResolution - 1;
        }

        y0 = (int32)((GetYStart() - voxelSize) / chunkSize);
        if (y0 < 0)
        {
            y0 = 0;
        }

        y1 = (int32)((GetYEnd() + voxelSize) / chunkSize);
        if (y1 >= chunkResolution)
        {
            y1 = chunkResolution - 1;
        }
    }

    void GetChunkRange(int32& x0, int32& x1, int32& y0, int32& y1, const float voxelSize, const int32 resolution) const
    {
        x0 = (int32)(GetXStart() / voxelSize);
        if (x0 < 0)
        {
            x0 = 0;
        }

        x1 = (int32)(GetXEnd() / voxelSize);
        if (x1 >= resolution)
        {
            x1 = resolution - 1;
        }

        y0 = (int32)(GetYStart() / voxelSize);
        if (y0 < 0)
        {
            y0 = 0;
        }

        y1 = (int32)(GetYEnd() / voxelSize);
        if (y1 >= resolution)
        {
            y1 = resolution - 1;
        }
    }

    virtual void FindHorizontalCrossing(FMQCVoxel& xMin, const FMQCVoxel& xMax) const = 0;

    virtual void FindVerticalCrossing(FMQCVoxel& yMin, const FMQCVoxel& yMax) const = 0;

    virtual void SetVoxels(FMQCGridChunk& Chunk);

    virtual void SetCrossings(FMQCGridChunk& Chunk);

    virtual void ApplyVoxels(FMQCGridChunk& Chunk, const int32 x0, const int32 x1, const int32 y0, const int32 y1);

    virtual void ApplyCrossings(FMQCGridChunk& Chunk, const int32 x0, const int32 x1, const int32 y0, const int32 y1);

public:

    int32 FillTypeSetting = 0;

    FMQCStencil() = default;
    virtual ~FMQCStencil() = default;

    virtual float GetXStart() const = 0;
    virtual float GetXEnd() const   = 0;
    virtual float GetYStart() const = 0;
    virtual float GetYEnd() const   = 0;

    FORCEINLINE int32 GetFillType() const
    {
        return fillType;
    }

    virtual void Initialize(const FMQCMap& VoxelMap)
    {
        fillType = FillTypeSetting;
    }

    virtual void SetFillType(int32 inFillType)
    {
        fillType = inFillType;
    }

    virtual void SetCenter(float x, float y)
    {
        centerX = x;
        centerY = y;
    }

    void SetHorizontalCrossing(FMQCVoxel& xMin, const FMQCVoxel& xMax) const;

    void SetVerticalCrossing(FMQCVoxel& yMin, const FMQCVoxel& yMax) const;

    virtual void EditMap(FMQCMap& Map, const FVector2D& center);

    virtual void EditStates(FMQCMap& Map, const FVector2D& center);

    virtual void EditCrossings(FMQCMap& Map, const FVector2D& center);

    virtual void GetChunkIndices(FMQCMap& Map, const FVector2D& center, TArray<int32>& OutIndices);

    virtual void ApplyVoxel(FMQCVoxel& voxel) const;
};

UCLASS(BlueprintType, Blueprintable)
class UMQCStencilRef : public UObject
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable)
    virtual void Clear()
    {
    }

    UFUNCTION(BlueprintCallable)
    virtual UMQCStencilRef* Copy(UObject* Outer)
    {
        return nullptr;
    }

    UFUNCTION(BlueprintCallable)
    virtual TArray<int32> GetChunkIndices(UMQCMapRef* MapRef)
    {
        return TArray<int32>();
    }

    UFUNCTION(BlueprintCallable)
    virtual TArray<int32> GetChunkIndicesAt(UMQCMapRef* MapRef, FVector2D Center)
    {
        return TArray<int32>();
    }

    UFUNCTION(BlueprintCallable)
    virtual void EditMap(UMQCMapRef* MapRef, FVector2D Center)
    {
    }

    virtual void EditStates(FMQCMap& Map)
    {
    }

    virtual void EditStatesAsync(FPSGWTAsyncTask& Task, FMQCMap& Map)
    {
    }

    virtual void EditCrossings(FMQCMap& Map)
    {
    }

    virtual void EditCrossingsAsync(FPSGWTAsyncTask& Task, FMQCMap& Map)
    {
    }

    virtual void EditMapAsync(FPSGWTAsyncTask& Task, FMQCMap& Map)
    {
    }

    virtual void EditMapAt(FMQCMap& Map, const FVector2D& Center)
    {
    }
};
