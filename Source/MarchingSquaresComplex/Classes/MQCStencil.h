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
#include "MQCVoxelTypes.h"
#include "MQCMaterial.h"
#include "MQCStencil.generated.h"

class FMQCGridChunk;
class FMQCMap;

class MARCHINGSQUARESCOMPLEX_API FMQCStencil
{
protected:

    uint8 fillType;
    float centerX;
    float centerY;
    FMQCMaterial Material;
    EMQCMaterialType MaterialType;
    EMQCMaterialBlendType MaterialBlendType;

	static void ValidateNormalX(FMQCVoxel& xMin, const FMQCVoxel& xMax);
	static void ValidateNormalY(FMQCVoxel& yMin, const FMQCVoxel& yMax);

    void GetMapRange(int32& X0, int32& X1, int32& Y0, int32& Y1, const int32 VoxelResolution, const int32 ChunkResolution) const;
    void GetChunkRange(int32& X0, int32& X1, int32& Y0, int32& Y1, const FMQCGridChunk& Chunk) const;
    void GetChunkIndices(TArray<int32>& ChunkIndices, const int32 VoxelResolution, const int32 ChunkResolution) const;
    void GetChunks(TArray<FMQCGridChunk*>& Chunks, FMQCMap& Map, const TArray<int32>& ChunkIndices) const;

    virtual int32 GetBoundsMinX() const = 0;
    virtual int32 GetBoundsMaxX() const = 0;
    virtual int32 GetBoundsMinY() const = 0;
    virtual int32 GetBoundsMaxY() const = 0;

    virtual void FindCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FIntPoint& ChunkOffset) const = 0;
    virtual void FindCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FIntPoint& ChunkOffset) const = 0;

    virtual FMQCMaterial GetMaterialFor(const FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const;
    virtual void GetMaterialBlendTyped(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const;
    virtual void GetMaterialBlendColor(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const;
    virtual void GetMaterialBlendSingleIndex(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const;
    virtual void GetMaterialBlendDoubleIndex(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const;
    virtual void GetMaterialBlendTripleIndex(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const;

    virtual void SetVoxels(FMQCGridChunk& Chunk);
    virtual void SetVoxels(const TArray<FMQCGridChunk*>& Chunks);
    virtual void SetCrossings(FMQCGridChunk& Chunk);
    virtual void SetCrossings(const TArray<FMQCGridChunk*>& Chunks);
    virtual void SetMaterials(FMQCGridChunk& Chunk);
    virtual void SetMaterials(const TArray<FMQCGridChunk*>& Chunks);

public:

    uint8 FillTypeSetting = 0;
    FMQCMaterial MaterialSetting;
    EMQCMaterialBlendType MaterialBlendSetting;
    bool bEnableAsync = false;

    FMQCStencil() = default;
    virtual ~FMQCStencil() = default;

    virtual void Initialize(const FMQCMap& VoxelMap);

    FORCEINLINE int32 GetFillType() const
    {
        return fillType;
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

    void SetCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FIntPoint& ChunkOffset) const;
    void SetCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FIntPoint& ChunkOffset) const;

    virtual void EditMap(FMQCMap& Map, const FVector2D& center);
    virtual void EditMaterial(FMQCMap& Map, const FVector2D& center);

    virtual void ApplyVoxel(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
    {
        // No Implementation
    }

    virtual void ApplyMaterial(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
    {
        // No Implementation
    }
};

UCLASS(BlueprintType, Blueprintable)
class MARCHINGSQUARESCOMPLEX_API UMQCStencilRef : public UObject
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableAsync = false;

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
    virtual void EditMapAt(UMQCMapRef* MapRef, FVector2D Center)
    {
    }

    UFUNCTION(BlueprintCallable)
    virtual void EditMap(UMQCMapRef* MapRef);

    UFUNCTION(BlueprintCallable)
    virtual void EditMaterialAt(UMQCMapRef* MapRef, FVector2D Center)
    {
    }

    UFUNCTION(BlueprintCallable)
    virtual void EditMaterial(UMQCMapRef* MapRef);

    virtual FMQCStencil* GetStencil()
    {
        return nullptr;
    }
};
