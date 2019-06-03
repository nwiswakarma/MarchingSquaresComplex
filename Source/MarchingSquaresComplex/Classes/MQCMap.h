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

#include "Mesh/PMUMeshTypes.h"

#include "MQCVoxelTypes.h"
#include "MQCMap.generated.h"

class FMQCGridChunk;

class MARCHINGSQUARESCOMPLEX_API FMQCMap
{
private:

    struct FPrefabData
    {
        TArray<FBox2D> Bounds;

        FPrefabData(const TArray<FBox2D>& InBounds)
            : Bounds(InBounds)
        {
        }
    };

    friend class FMQCStencil;

    TArray<FPrefabData> AppliedPrefabs;
	TArray<FMQCGridChunk*> chunks;

    void InitializeSettings();
    void InitializeChunkSettings(int32 i, int32 x, int32 y, FMQCChunkConfig& ChunkConfig);
    void InitializeChunk(int32 i, const FMQCChunkConfig& ChunkConfig);
    void InitializeChunks();

public:

    FMQCMap() = default;

    ~FMQCMap()
    {
        Clear();
    }

	float MaxFeatureAngle = 135.f;
	float MaxParallelAngle = 8.f;

	int32 voxelResolution = 8;
	int32 chunkResolution = 2;
	float extrusionHeight = -1.f;

    TArray<FMQCSurfaceState> SurfaceStates;
    TArray<class UStaticMesh*> meshPrefabs;

    void Initialize();
    void CopyFrom(const FMQCMap& VoxelMap);
    void Clear();

	FORCEINLINE int32 GetVoxelCount() const
    {
        return chunkResolution * voxelResolution;
    }

    // CHUNK FUNCTIONS

    bool HasChunk(int32 ChunkIndex) const;
    int32 GetChunkCount() const;
    const FMQCGridChunk& GetChunk(int32 ChunkIndex) const;
    FMQCGridChunk& GetChunk(int32 ChunkIndex);
    void Triangulate();
    void TriangulateAsync();
    void ResetChunkStates(const TArray<int32>& ChunkIndices);
    void ResetAllChunkStates();
    void WaitForAsyncTask();

    // PREFAB FUNCTIONS

	FORCEINLINE bool HasPrefab(int32 PrefabIndex) const
    {
        return meshPrefabs.IsValidIndex(PrefabIndex) && IsValid(meshPrefabs[PrefabIndex]);
    }

    bool IsPrefabValid(int32 PrefabIndex, int32 LODIndex, int32 SectionIndex) const;
    bool HasIntersectingBounds(const TArray<FBox2D>& Bounds) const;
	void GetPrefabBounds(int32 PrefabIndex, TArray<FBox2D>& Bounds) const;
	TArray<FBox2D> GetPrefabBounds(int32 PrefabIndex) const;

    bool TryPlacePrefabAt(int32 PrefabIndex, const FVector2D& Center);
};

UCLASS(BlueprintType, Blueprintable)
class UMQCMapRef : public UObject
{
    GENERATED_BODY()

    FMQCMap VoxelMap;

    void ApplyMapSettings();

public:

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite, meta=(ClampMin="1", UIMin="1"))
	int32 VoxelResolution = 8;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite, meta=(ClampMin="1", UIMin="1"))
	int32 ChunkResolution = 2;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    TArray<FMQCSurfaceState> SurfaceStates;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
	float ExtrusionHeight = -1.f;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
	float MaxFeatureAngle = 135.f;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
	float MaxParallelAngle = 8.f;

    UPROPERTY(BlueprintReadWrite, Category="Prefabs")
    TArray<class UStaticMesh*> MeshPrefabs;

    UFUNCTION(BlueprintCallable)
    bool IsInitialized() const
    {
        return HasChunk(0);
    }

    FORCEINLINE FMQCMap& GetMap()
    {
        return VoxelMap;
    }

    FORCEINLINE const FMQCMap& GetMap() const
    {
        return VoxelMap;
    }

    UFUNCTION(BlueprintCallable)
    void InitializeVoxelMap()
    {
        ApplyMapSettings();

        VoxelMap.Initialize();
    }

    UFUNCTION(BlueprintCallable)
    void ClearVoxelMap()
    {
        VoxelMap.Clear();
    }

    UFUNCTION(BlueprintCallable)
    void Triangulate()
    {
        VoxelMap.Triangulate();
    }

    UFUNCTION(BlueprintCallable)
    void TriangulateAsync()
    {
        VoxelMap.TriangulateAsync();
    }

    UFUNCTION(BlueprintCallable)
    void WaitForAsyncTask()
    {
        VoxelMap.WaitForAsyncTask();
    }

    UFUNCTION(BlueprintCallable)
    UMQCMapRef* Copy(UObject* Outer) const;

    UFUNCTION(BlueprintCallable)
    void ResetChunkStates(const TArray<int32>& ChunkIndices);

    UFUNCTION(BlueprintCallable)
    void ResetAllChunkStates();

    UFUNCTION(BlueprintCallable)
    float GetMapSize() const
    {
        return VoxelMap.voxelResolution * VoxelMap.chunkResolution;
    }

    UFUNCTION(BlueprintCallable)
    FVector2D GetCenter() const
    {
        float Center = (GetMapSize() - 1.f) * .5f;
        return FVector2D(Center, Center);
    }

    UFUNCTION(BlueprintCallable)
    FVector2D GetMapDimension() const
    {
        const float MapSize = GetMapSize();
        return FVector2D(MapSize, MapSize);
    }

	UFUNCTION(BlueprintCallable)
	int32 GetVoxelCount() const
    {
        return VoxelMap.GetVoxelCount();
    }

	UFUNCTION(BlueprintCallable)
	FIntPoint GetVoxelDimension() const
    {
        return FIntPoint(
            VoxelMap.voxelResolution * VoxelMap.chunkResolution,
            VoxelMap.voxelResolution * VoxelMap.chunkResolution
            );
    }

	UFUNCTION(BlueprintCallable)
	FVector2D GetMeshInverseScale() const
    {
        FIntPoint VoxelDimension = GetVoxelDimension();
        FVector2D ScaleVector = FVector2D::UnitVector;

        if (VoxelDimension.X > 1)
        {
            ScaleVector.X = 1.f/VoxelDimension.X;
        }

        if (VoxelDimension.Y > 1)
        {
            ScaleVector.Y = 1.f/VoxelDimension.Y;
        }

        return ScaleVector;
    }

	UFUNCTION(BlueprintCallable)
	bool HasChunk(int32 ChunkIndex) const
    {
        return VoxelMap.HasChunk(ChunkIndex);
    }

	UFUNCTION(BlueprintCallable)
	int32 GetChunkCount() const
    {
        return VoxelMap.GetChunkCount();
    }

	UFUNCTION(BlueprintCallable)
	FVector GetChunkPosition(int32 ChunkIndex) const;

	UFUNCTION(BlueprintCallable)
	FPMUMeshSectionRef GetSurfaceSection(int32 ChunkIndex, int32 StateIndex);

	UFUNCTION(BlueprintCallable)
	FPMUMeshSectionRef GetExtrudeSection(int32 ChunkIndex, int32 StateIndex);

	UFUNCTION(BlueprintCallable)
	FPMUMeshSectionRef GetEdgeSection(int32 ChunkIndex, int32 StateIndex);

    // PREFAB FUNCTIONS

	UFUNCTION(BlueprintCallable)
	bool HasPrefab(int32 PrefabIndex) const;

	UFUNCTION(BlueprintCallable)
	TArray<FBox2D> GetPrefabBounds(int32 PrefabIndex) const;
};

UCLASS(BlueprintType)
class AMQCMap : public AActor
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", UIMin="1"))
	int32 VoxelResolution = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", UIMin="1"))
	int32 ChunkResolution = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMQCSurfaceState> SurfaceStates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExtrusionHeight = -1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxFeatureAngle = 135.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxParallelAngle = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<class UStaticMesh*> MeshPrefabs;

    UFUNCTION(BlueprintCallable)
    void ApplySettings(UMQCMapRef* MapRef) const
    {
        if (IsValid(MapRef))
        {
            MapRef->VoxelResolution = VoxelResolution;
            MapRef->ChunkResolution = ChunkResolution;
            MapRef->SurfaceStates = SurfaceStates;
            MapRef->ExtrusionHeight = ExtrusionHeight;
            MapRef->MaxFeatureAngle = MaxFeatureAngle;
            MapRef->MaxParallelAngle = MaxParallelAngle;
            MapRef->MeshPrefabs = MeshPrefabs;
        }
    }
};
