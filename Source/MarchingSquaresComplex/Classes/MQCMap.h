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
#include "MQCMaterial.h"
#include "MQCMap.generated.h"

class FMQCGridChunk;
class UPMUMeshComponent;

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

    bool bRequireFinalizeAsync = false;

    void InitializeSettings();
    void InitializeChunkSettings(int32 i, int32 x, int32 y, FMQCChunkConfig& ChunkConfig);
    void InitializeChunk(int32 i, const FMQCChunkConfig& ChunkConfig);
    void InitializeChunks();
    void ResolveChunkEdgeData();
    void ResolveChunkEdgeData(int32 StateIndex);

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

    EMQCMaterialType MaterialType;

    TArray<FMQCSurfaceState> SurfaceStates;
    TArray<class UStaticMesh*> meshPrefabs;

    void Initialize();
    void Clear();

    FORCEINLINE EMQCMaterialType GetMaterialType() const
    {
        return MaterialType;
    }

    FORCEINLINE int32 GetVoxelCount() const
    {
        return chunkResolution * voxelResolution;
    }

    FORCEINLINE int32 GetStateCount() const
    {
        return SurfaceStates.Num();
    }

    // CHUNK FUNCTIONS

    bool HasChunk(int32 ChunkIndex) const;
    int32 GetChunkCount() const;
    const FMQCGridChunk& GetChunk(int32 ChunkIndex) const;
    FMQCGridChunk& GetChunk(int32 ChunkIndex);
    void Triangulate();
    void TriangulateAsync();
    void FinalizeAsync();
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
class MARCHINGSQUARESCOMPLEX_API UMQCMapRef : public UObject
{
    GENERATED_BODY()

    FMQCMap VoxelMap;

public:

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite, meta=(ClampMin="1", UIMin="1"))
    int32 VoxelResolution;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite, meta=(ClampMin="1", UIMin="1"))
    int32 ChunkResolution;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    EMQCMaterialType MaterialType;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    TArray<FMQCSurfaceState> SurfaceStates;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    float ExtrusionHeight;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    float MaxFeatureAngle;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    float MaxParallelAngle;

    UPROPERTY(BlueprintReadWrite, Category="Prefabs")
    TArray<class UStaticMesh*> MeshPrefabs;

    UMQCMapRef();
    ~UMQCMapRef();

    FORCEINLINE FMQCMap& GetMap()
    {
        return VoxelMap;
    }

    FORCEINLINE const FMQCMap& GetMap() const
    {
        return VoxelMap;
    }

    void ApplyMapSettings();
    FMQCMaterial GetTypedMaterial(uint8 MaterialIndex, const FLinearColor& MaterialColor);

    UFUNCTION(BlueprintCallable)
    bool IsInitialized() const;

    UFUNCTION(BlueprintCallable)
    void InitializeVoxelMap();

    UFUNCTION(BlueprintCallable)
    void ClearVoxelMap();

    UFUNCTION(BlueprintCallable)
    void Triangulate();

    UFUNCTION(BlueprintCallable)
    void TriangulateAsync();

    UFUNCTION(BlueprintCallable)
    void WaitForAsyncTask();

    UFUNCTION(BlueprintCallable)
    void FinalizeAsync();

    UFUNCTION(BlueprintCallable)
    void ResetChunkStates(const TArray<int32>& ChunkIndices);

    UFUNCTION(BlueprintCallable)
    void ResetAllChunkStates();

    UFUNCTION(BlueprintCallable)
    float GetMapSize() const;

    UFUNCTION(BlueprintCallable)
    FVector2D GetCenter() const;

    UFUNCTION(BlueprintCallable)
    FVector2D GetMapDimension() const;

    UFUNCTION(BlueprintCallable)
    int32 GetVoxelCount() const;

    UFUNCTION(BlueprintCallable)
    FIntPoint GetVoxelDimension() const;

    UFUNCTION(BlueprintCallable)
    FVector2D GetMeshInverseScale() const;

    UFUNCTION(BlueprintCallable)
    bool HasChunk(int32 ChunkIndex) const;

    UFUNCTION(BlueprintCallable)
    int32 GetChunkCount() const;

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
class MARCHINGSQUARESCOMPLEX_API AMQCMap : public AActor
{
    GENERATED_BODY()

protected:

    enum FMeshGroupId
    {
        MGI_SURFACE = 0,
        MGI_EXTRUDE = 1,
        MGI_EDGE = 2
    };

    UPROPERTY(BlueprintGetter=K2_GetMap)
    UMQCMapRef* MapRef;

    UPROPERTY(BlueprintGetter=K2_GetMeshAnchor)
    USceneComponent* MeshAnchor;

    UPROPERTY()
    TArray<UPMUMeshComponent*> SurfaceMeshComponents;

    void InitializeMeshComponents(TArray<UPMUMeshComponent*>& MeshComponents);
    UPMUMeshComponent* GetOrAddMeshComponent(TArray<UPMUMeshComponent*>& MeshComponents, int32 MeshIndex);

public:

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite, meta=(ClampMin="1", UIMin="1"))
    int32 VoxelResolution;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite, meta=(ClampMin="1", UIMin="1"))
    int32 ChunkResolution;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    EMQCMaterialType MaterialType;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    TArray<FMQCSurfaceState> SurfaceStates;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    float ExtrusionHeight;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    float MaxFeatureAngle;

    UPROPERTY(EditAnywhere, Category="Map Settings", BlueprintReadWrite)
    float MaxParallelAngle;

    AMQCMap();
    ~AMQCMap();

    UPROPERTY(BlueprintReadWrite, Category="Prefabs")
    TArray<class UStaticMesh*> MeshPrefabs;

    UFUNCTION(BlueprintCallable)
    void Initialize();

    UFUNCTION(BlueprintCallable)
    bool HasValidMap() const
    {
        return IsValid(MapRef);
    }

    UFUNCTION(BlueprintGetter)
    UMQCMapRef* K2_GetMap() const;
	FORCEINLINE UMQCMapRef* GetMap() const { return MapRef; }

    UFUNCTION(BlueprintGetter)
    USceneComponent* K2_GetMeshAnchor() const;
    FORCEINLINE USceneComponent* GetMeshAnchor() const { return MeshAnchor; }

    UFUNCTION(BlueprintCallable)
    void Triangulate(bool bAsync = false, bool bWaitForAsyncToFinish = false);

    UFUNCTION(BlueprintCallable)
    void GenerateMapMesh();

    UFUNCTION(BlueprintCallable)
    void GenerateMaterialMesh(
        int32 StateIndex,
        uint8 MaterialIndex0,
        uint8 MaterialIndex1,
        uint8 MaterialIndex2,
        bool bUseTripleIndex,
        UMaterialInterface* Material
        );
};

// Blueprint Inlines

FORCEINLINE_DEBUGGABLE bool UMQCMapRef::IsInitialized() const
{
    return HasChunk(0);
}

FORCEINLINE_DEBUGGABLE float UMQCMapRef::GetMapSize() const
{
    return VoxelMap.voxelResolution * VoxelMap.chunkResolution;
}

FORCEINLINE_DEBUGGABLE FVector2D UMQCMapRef::GetCenter() const
{
    float Center = (GetMapSize() - 1.f) * .5f;
    return FVector2D(Center, Center);
}

FORCEINLINE_DEBUGGABLE FVector2D UMQCMapRef::GetMapDimension() const
{
    const float MapSize = GetMapSize();
    return FVector2D(MapSize, MapSize);
}

FORCEINLINE_DEBUGGABLE int32 UMQCMapRef::GetVoxelCount() const
{
    return VoxelMap.GetVoxelCount();
}

FORCEINLINE_DEBUGGABLE FIntPoint UMQCMapRef::GetVoxelDimension() const
{
    return FIntPoint(
        VoxelMap.voxelResolution * VoxelMap.chunkResolution,
        VoxelMap.voxelResolution * VoxelMap.chunkResolution
        );
}

FORCEINLINE_DEBUGGABLE FVector2D UMQCMapRef::GetMeshInverseScale() const
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

FORCEINLINE_DEBUGGABLE bool UMQCMapRef::HasChunk(int32 ChunkIndex) const
{
    return VoxelMap.HasChunk(ChunkIndex);
}

FORCEINLINE_DEBUGGABLE int32 UMQCMapRef::GetChunkCount() const
{
    return VoxelMap.GetChunkCount();
}

FORCEINLINE_DEBUGGABLE UMQCMapRef* AMQCMap::K2_GetMap() const
{
    return GetMap();
}

FORCEINLINE_DEBUGGABLE USceneComponent* AMQCMap::K2_GetMeshAnchor() const
{
    return GetMeshAnchor();
}
