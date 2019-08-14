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
#include "Mesh/Simplifier/PMUMeshSimplifierOptions.h"

#include "MQCVoxelTypes.h"
#include "MQCGeometryTypes.h"
#include "MQCMaterial.h"
#include "MQCMap.generated.h"

class FMQCGridChunk;
class UPMUMeshComponent;

class MARCHINGSQUARESCOMPLEX_API FMQCMap
{
private:

    typedef TArray<FMQCEdgeSyncData> FEdgeSyncList;
    typedef TArray<FEdgeSyncList> FStateEdgeSyncList;

    int32 VoxelResolution;
    int32 ChunkResolution;
    float MaxFeatureAngle;
    float MaxParallelAngle;
    float ExtrusionHeight;
    EMQCMaterialType MaterialType;
    TArray<FMQCSurfaceState> SurfaceStates;

    TArray<FMQCGridChunk*> Chunks;
    TArray<FStateEdgeSyncList> EdgeSyncGroups;

    bool bRequireFinalizeAsync = false;

    void InitializeSettings(const FMQCMapConfig& MapConfig);
    void InitializeChunk(int32 i, int32 x, int32 y);
    void InitializeChunks();
    void ResolveChunkEdgeData();
    void ResolveChunkEdgeData(int32 StateIndex);

public:

    FMQCMap();
    ~FMQCMap();

    void Initialize(const FMQCMapConfig& MapConfig);
    void Clear();

    FORCEINLINE int32 GetVoxelResolution() const
    {
        return VoxelResolution;
    }

    FORCEINLINE int32 GetChunkResolution() const
    {
        return ChunkResolution;
    }

    FORCEINLINE int32 GetVoxelDimension() const
    {
        return ChunkResolution * VoxelResolution;
    }

    bool IsWithinDimension(int32 X, int32 Y) const
    {
        return X < GetVoxelDimension() && Y < GetVoxelDimension();
    }

    FORCEINLINE int32 GetStateCount() const
    {
        return SurfaceStates.Num();
    }

    FORCEINLINE bool HasState(int32 StateIndex) const
    {
        return StateIndex > 0 && StateIndex <= GetStateCount();
    }

    FORCEINLINE EMQCMaterialType GetMaterialType() const
    {
        return MaterialType;
    }

    FMQCMaterial GetTypedMaterial(uint8 MaterialIndex, const FLinearColor& MaterialColor);

    // CHUNK FUNCTIONS

    bool HasChunk(int32 ChunkIndex) const;
    int32 GetChunkCount() const;
    int32 GetChunkIndex(int32 ChunkX, int32 ChunkY) const;
    int32 GetChunkIndexByPoint(int32 X, int32 Y) const;
    const FMQCGridChunk& GetChunk(int32 ChunkIndex) const;
    FMQCGridChunk& GetChunk(int32 ChunkIndex);
    void GetChunks(TArray<FMQCGridChunk*>& OutChunks, const FIntPoint& BoundsMin, const FIntPoint& BoundsMax);

    // TRIANGULATION FUNCTIONS

    void Triangulate();
    void TriangulateAsync();
    void WaitForAsyncTask();
    void FinalizeAsync();
    void ResetChunkStates(const TArray<int32>& ChunkIndices);
    void ResetAllChunkStates();

    // GEOMETRY FUNCTIONS

    void AddQuadFilter(const FIntPoint& Point, int32 StateIndex, bool bExtrudeFilter);
    int32 GetEdgePointListCount(int32 StateIndex) const;
    void GetEdgePoints(TArray<FMQCEdgePointList>& OutPointList, int32 StateIndex) const;
    void GetEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListIndex) const;
    void GetEdgePointsByChunkSurface(TArray<FMQCEdgePointData>& OutPointList, int32 ChunkIndex, int32 StateIndex) const;
};

UCLASS(BlueprintType, Blueprintable)
class MARCHINGSQUARESCOMPLEX_API UMQCMapRef : public UObject
{
    GENERATED_BODY()

    FMQCMap VoxelMap;

public:

    UPROPERTY(EditAnywhere, Category="Map Settings")
    FMQCMapConfig MapConfig;

    FORCEINLINE FMQCMap& GetMap()
    {
        return VoxelMap;
    }

    FORCEINLINE const FMQCMap& GetMap() const
    {
        return VoxelMap;
    }

    FORCEINLINE FMQCMaterial GetTypedMaterial(uint8 MaterialIndex, const FLinearColor& MaterialColor)
    {
        return VoxelMap.GetTypedMaterial(MaterialIndex, MaterialColor);
    }

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE bool IsInitialized() const;

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
    FORCEINLINE_DEBUGGABLE int32 GetVoxelDimension() const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE FIntPoint GetVoxelDimension2D() const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE float GetVectorDimension() const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE FVector2D GetVectorDimension2D() const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE FVector2D GetCenter() const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE FVector2D GetMeshInverseScale() const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE int32 GetStateCount() const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE bool HasState(int32 StateIndex) const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE bool HasChunk(int32 ChunkIndex) const;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE int32 GetChunkCount() const;

    UFUNCTION(BlueprintCallable)
    FVector GetChunkPosition(int32 ChunkIndex) const;

    UFUNCTION(BlueprintCallable)
    FPMUMeshSectionRef GetSurfaceSection(int32 ChunkIndex, int32 StateIndex);

    UFUNCTION(BlueprintCallable)
    FPMUMeshSectionRef GetExtrudeSection(int32 ChunkIndex, int32 StateIndex);

    UFUNCTION(BlueprintCallable)
    void GetEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListIndex);

    UFUNCTION(BlueprintCallable)
    void GetEdgePointsByChunkSurface(TArray<FMQCEdgePointData>& OutPointList, int32 ChunkIndex, int32 StateIndex);

    UFUNCTION(BlueprintCallable)
    FORCEINLINE_DEBUGGABLE int32 GetEdgePointListCount(int32 StateIndex) const;

    UFUNCTION(BlueprintCallable)
    void AddQuadFilters(const TArray<FIntPoint>& Points, int32 StateIndex, bool bFilterExtrude);

    UFUNCTION(BlueprintCallable)
    void AddQuadFiltersByBounds(const FIntPoint& BoundsMin, const FIntPoint& BoundsMax, int32 StateIndex, bool bFilterExtrude);
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
    UPMUMeshComponent* GetSurfaceMesh(int32 MeshIndex);

public:

    UPROPERTY(EditAnywhere, Category="Map Settings")
    FMQCMapConfig MapConfig;

    AMQCMap();
    ~AMQCMap();

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

    UFUNCTION(BlueprintCallable)
    void ApplyHeightMap(
        int32 StateIndex,
        UTexture* HeightTexture = nullptr,
        bool bGenerateTangents = false,
        float HeightScale = 1.f
        );

    UFUNCTION(BlueprintCallable)
    void CalculateMeshNormal(int32 StateIndex);

    UFUNCTION(BlueprintCallable)
    void SimplifyMesh(int32 StateIndex, FPMUMeshSimplifierOptions Options);
};

// Inlines

FORCEINLINE bool FMQCMap::HasChunk(int32 ChunkIndex) const
{
    return Chunks.IsValidIndex(ChunkIndex);
}

FORCEINLINE int32 FMQCMap::GetChunkCount() const
{
    return Chunks.Num();
}

FORCEINLINE int32 FMQCMap::GetChunkIndex(int32 ChunkX, int32 ChunkY) const
{
    return ChunkX + ChunkY * ChunkResolution;
}

FORCEINLINE int32 FMQCMap::GetChunkIndexByPoint(int32 X, int32 Y) const
{
    return (X/VoxelResolution) + (Y/VoxelResolution) * ChunkResolution;
}

FORCEINLINE const FMQCGridChunk& FMQCMap::GetChunk(int32 ChunkIndex) const
{
    return *Chunks[ChunkIndex];
}

FORCEINLINE FMQCGridChunk& FMQCMap::GetChunk(int32 ChunkIndex)
{
    return *Chunks[ChunkIndex];
}

FORCEINLINE void FMQCMap::GetChunks(TArray<FMQCGridChunk*>& OutChunks, const FIntPoint& BoundsMin, const FIntPoint& BoundsMax)
{
    int32 ChunkMinX = FMath::Max(BoundsMin.X/VoxelResolution, 0);
    int32 ChunkMaxX = FMath::Min(BoundsMax.X/VoxelResolution, ChunkResolution-1);

    int32 ChunkMinY = FMath::Max(BoundsMin.Y/VoxelResolution, 0);
    int32 ChunkMaxY = FMath::Min(BoundsMax.Y/VoxelResolution, ChunkResolution-1);

    for (int32 y=ChunkMinY; y<=ChunkMaxY; ++y)
    for (int32 x=ChunkMinX; x<=ChunkMaxX; ++x)
    {
        OutChunks.Emplace(Chunks[GetChunkIndex(x, y)]);
    }
}

// Blueprint Inlines

FORCEINLINE_DEBUGGABLE bool UMQCMapRef::IsInitialized() const
{
    return HasChunk(0);
}

FORCEINLINE_DEBUGGABLE int32 UMQCMapRef::GetVoxelDimension() const
{
    return VoxelMap.GetVoxelDimension();
}

FORCEINLINE_DEBUGGABLE FIntPoint UMQCMapRef::GetVoxelDimension2D() const
{
    return FIntPoint(
        VoxelMap.GetVoxelDimension(),
        VoxelMap.GetVoxelDimension()
        );
}

FORCEINLINE_DEBUGGABLE float UMQCMapRef::GetVectorDimension() const
{
    return VoxelMap.GetVoxelDimension();
}

FORCEINLINE_DEBUGGABLE FVector2D UMQCMapRef::GetVectorDimension2D() const
{
    return FVector2D(
        GetVectorDimension(),
        GetVectorDimension()
        );
}

FORCEINLINE_DEBUGGABLE FVector2D UMQCMapRef::GetCenter() const
{
    float Center = (GetVectorDimension() - 1.f) * .5f;
    return FVector2D(Center, Center);
}

FORCEINLINE_DEBUGGABLE FVector2D UMQCMapRef::GetMeshInverseScale() const
{
    FIntPoint VoxelDimensions = GetVoxelDimension2D();
    FVector2D ScaleVector = FVector2D::UnitVector;

    if (VoxelDimensions.X > 1)
    {
        ScaleVector.X = 1.f/VoxelDimensions.X;
    }

    if (VoxelDimensions.Y > 1)
    {
        ScaleVector.Y = 1.f/VoxelDimensions.Y;
    }

    return ScaleVector;
}

FORCEINLINE_DEBUGGABLE int32 UMQCMapRef::GetStateCount() const
{
    return VoxelMap.GetStateCount();
}

FORCEINLINE_DEBUGGABLE bool UMQCMapRef::HasState(int32 StateIndex) const
{
    return VoxelMap.HasState(StateIndex);
}

FORCEINLINE_DEBUGGABLE bool UMQCMapRef::HasChunk(int32 ChunkIndex) const
{
    return VoxelMap.HasChunk(ChunkIndex);
}

FORCEINLINE_DEBUGGABLE int32 UMQCMapRef::GetChunkCount() const
{
    return VoxelMap.GetChunkCount();
}

FORCEINLINE_DEBUGGABLE int32 UMQCMapRef::GetEdgePointListCount(int32 StateIndex) const
{
    return IsInitialized() ? VoxelMap.GetEdgePointListCount(StateIndex) : 0;
}

FORCEINLINE_DEBUGGABLE UMQCMapRef* AMQCMap::K2_GetMap() const
{
    return GetMap();
}

FORCEINLINE_DEBUGGABLE USceneComponent* AMQCMap::K2_GetMeshAnchor() const
{
    return GetMeshAnchor();
}
