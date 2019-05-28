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

#include "GWTAsyncThreadPool.h"
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

    TSharedPtr<FGWTAsyncThreadPool> ThreadPool = nullptr;

    void InitializeSettings();
    void InitializeChunkSettings(int32 i, int32 x, int32 y, FMQCGridConfig& ChunkConfig);
    void InitializeChunk(int32 i, const FMQCGridConfig& ChunkConfig);
    void InitializeChunkAsync(int32 i, const FMQCGridConfig& ChunkConfig, FGWTAsyncTaskRef& TaskRef);
    void InitializeChunks();
    void InitializeChunksAsync(FGWTAsyncTaskRef& TaskRef);

public:

    FMQCMap() = default;

    ~FMQCMap()
    {
        Clear();
    }

	float maxFeatureAngle = 135.f;
	float maxParallelAngle = 8.f;

	int32 voxelResolution = 8;
	int32 chunkResolution = 2;
	float extrusionHeight = -1.f;

    TArray<FMQCSurfaceState> surfaceStates;
    TArray<class UStaticMesh*> meshPrefabs;

    void Initialize();
    void InitializeAsync(FGWTAsyncTaskRef& TaskRef);
    void CopyFrom(const FMQCMap& VoxelMap);
    void Clear();

    TSharedPtr<FGWTAsyncThreadPool> GetThreadPool();

    FORCEINLINE float GetVoxelCount() const
    {
        return (voxelResolution*chunkResolution) * (voxelResolution*chunkResolution);
    }

    // CHUNK FUNCTIONS

    bool HasChunk(int32 ChunkIndex) const;
    int32 GetChunkCount() const;
    const FMQCGridChunk& GetChunk(int32 ChunkIndex) const;
    void RefreshAllChunks();
    void RefreshAllChunksAsync(FGWTAsyncTaskRef& TaskRef);
    void ResetChunkStates(const TArray<int32>& ChunkIndices);
    void ResetAllChunkStates();

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

    UPROPERTY(BlueprintReadWrite, Category="GPU Settings")
    bool bUseGPUProgram = false;

    UPROPERTY(BlueprintReadWrite, Category="GPU Settings")
    FName GPUProgramName;

    UPROPERTY(BlueprintReadWrite, Category="GPU Settings")
    FString GPUProgramKernelName;

    UPROPERTY(BlueprintReadWrite, Category="GPU Settings")
    TArray<FString> GPUProgramSourcePaths;

    UPROPERTY(BlueprintReadWrite, Category="GPU Settings")
    TArray<FString> GPUProgramBuildOptions;

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
    void InitializeVoxelMapAsync(UPARAM(ref) FGWTAsyncTaskRef& TaskRef)
    {
        ApplyMapSettings();

        VoxelMap.InitializeAsync(TaskRef);
    }

    UFUNCTION(BlueprintCallable)
    void ClearVoxelMap()
    {
        VoxelMap.Clear();
    }

    UFUNCTION(BlueprintCallable)
    void Triangulate()
    {
        VoxelMap.RefreshAllChunks();
    }

    UFUNCTION(BlueprintCallable)
    void TriangulateAsync(UPARAM(ref) FGWTAsyncTaskRef& TaskRef)
    {
        return VoxelMap.RefreshAllChunksAsync(TaskRef);
    }

#if 0

    UFUNCTION(BlueprintCallable)
    void EditMapAsync(UPARAM(ref) FGWTAsyncTaskRef& TaskRef, const TArray<UMQCStencilRef*>& Stencils);

    UFUNCTION(BlueprintCallable)
    void EditStatesAsync(UPARAM(ref) FGWTAsyncTaskRef& TaskRef, const TArray<UMQCStencilRef*>& Stencils);

    UFUNCTION(BlueprintCallable)
    void EditCrossingsAsync(UPARAM(ref) FGWTAsyncTaskRef& TaskRef, const TArray<UMQCStencilRef*>& Stencils);

#endif

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
	FIntPoint GetVoxelDimension() const
    {
        return FIntPoint(
            VoxelResolution * ChunkResolution,
            VoxelResolution * ChunkResolution
            );
    }

	UFUNCTION(BlueprintCallable)
	FVector GetChunkPosition(int32 ChunkIndex) const;

	UFUNCTION(BlueprintCallable)
	void GetSection(FPMUMeshSection& Section, int32 ChunkIndex, int32 StateIndex) const;

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
