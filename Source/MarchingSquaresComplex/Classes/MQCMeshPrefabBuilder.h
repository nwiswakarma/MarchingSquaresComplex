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
#include "MQCGeometryTypes.h"
#include "Mesh/PMUMeshTypes.h"
#include "MQCMeshPrefabBuilder.generated.h"

class FMQCMap;
class UMQCMapRef;
class UStaticMesh;
struct FStaticMeshVertexBuffers;
struct FStaticMeshSection;

USTRUCT(BlueprintType)
struct MARCHINGSQUARESCOMPLEX_API FMQCPrefabInputData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMesh* Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LODIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SectionIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SurfaceSocketPrefix = TEXT("MQC_PFB_SRF_");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ExtrudeSocketPrefix = TEXT("MQC_PFB_EXT_");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Offset;
};

UCLASS(BlueprintType, Blueprintable)
class MARCHINGSQUARESCOMPLEX_API UMQCPrefabBuilder : public UObject
{
    GENERATED_BODY()

    struct FPrefabData
    {
        const UStaticMesh* Mesh;
        int32 LODIndex;
        int32 SectionIndex;
        FBox2D Bounds;
        float Length;
        TArray<uint32> SortedIndexAlongX;
        TArray<uint32> SurfaceIndices;
        TArray<uint32> ExtrudeIndices;
    };

    struct FPrefabBuffers
    {
        TArray<FVector> Positions;
        TArray<FVector2D> UVs;
        TArray<uint32> Tangents;
        TArray<uint32> Indices;
    };

    typedef TMap<const FPrefabData*, FPrefabBuffers> FPrefabBufferMap;

    TArray<FPrefabData> PreparedPrefabs;
    TArray<uint32> SortedPrefabs;
    bool bPrefabInitialized;

    FPMUMeshSection GeneratedSection;

    void AllocateSection(FPMUMeshSection& OutSection, uint32 NumVertices, uint32 NumIndices);

    uint32 CopyPrefabToSection(FPMUMeshSection& OutSection, FPrefabBufferMap& PrefabBufferMap, const FPrefabData& Prefab);
    uint32 CopyPrefabToSection(FPMUMeshSection& OutSection, const FPrefabBuffers& PrefabBuffers);

    const FStaticMeshVertexBuffers& GetVertexBuffers(const UStaticMesh& Mesh, int32 LODIndex) const;
    const FStaticMeshSection& GetSection(const UStaticMesh& Mesh, int32 LODIndex, int32 SectionIndex) const;
    FIndexArrayView GetIndexBuffer(const UStaticMesh& Mesh, int32 LODIndex) const;

    void GetPrefabGeometryCount(const FPrefabData& Prefab, uint32& OutNumVertices, uint32& OutNumIndices) const;

    FORCEINLINE const FStaticMeshVertexBuffers& GetVertexBuffers(const FPrefabData& Prefab) const
    {
        check(Prefab.Mesh != nullptr);
        return GetVertexBuffers(*Prefab.Mesh, Prefab.LODIndex);
    }

    FORCEINLINE const FStaticMeshSection& GetSection(const FPrefabData& Prefab) const
    {
        check(Prefab.Mesh != nullptr);
        return GetSection(*Prefab.Mesh, Prefab.LODIndex, Prefab.SectionIndex);
    }

    FORCEINLINE FIndexArrayView GetIndexBuffer(const FPrefabData& Prefab) const
    {
        check(Prefab.Mesh != nullptr);
        return GetIndexBuffer(*Prefab.Mesh, Prefab.LODIndex);
    }

    FORCEINLINE const FPrefabData& GetSortedPrefab(uint32 i) const
    {
        return PreparedPrefabs[SortedPrefabs[i]];
    }

public:

    UPROPERTY(EditAnywhere, Category="Prefab Settings", BlueprintReadWrite)
    TArray<FMQCPrefabInputData> MeshPrefabs;

    UMQCPrefabBuilder();

    bool IsValidPrefab(const UStaticMesh* Mesh, int32 LODIndex, int32 SectionIndex) const;

    FORCEINLINE bool IsValidPrefab(const FMQCPrefabInputData& Input) const
    {
        return IsValidPrefab(Input.Mesh, Input.LODIndex, Input.SectionIndex);
    }

    FORCEINLINE bool IsValidPrefab(const FPrefabData& Prefab) const
    {
        return IsValidPrefab(Prefab.Mesh, Prefab.LODIndex, Prefab.SectionIndex);
    }

    FORCEINLINE int32 GetPreparedPrefabCount() const
    {
        return PreparedPrefabs.Num();
    }
    
    void ResetPrefabs();
    void InitializePrefabs();
    void BuildEdgePrefabs(FMQCMap& Map, int32 StateIndex);
    void BuildEdgePrefabs(FMQCMap& Map, const TArray<FMQCEdgePoint>& EdgePoints);

    UFUNCTION(BlueprintCallable, meta=(DisplayName="Reset Prefabs"))
    void K2_ResetPrefabs();

    UFUNCTION(BlueprintCallable, meta=(DisplayName="Build Edge Prefabs"))
    void K2_BuildEdgePrefabs(UMQCMapRef* MapRef, int32 StateIndex);

    UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Edge Section"))
    FPMUMeshSectionRef K2_GetEdgeSection();
};

FORCEINLINE_DEBUGGABLE void UMQCPrefabBuilder::K2_ResetPrefabs()
{
    ResetPrefabs();
}

FORCEINLINE_DEBUGGABLE FPMUMeshSectionRef UMQCPrefabBuilder::K2_GetEdgeSection()
{
    return FPMUMeshSectionRef(GeneratedSection);
}
