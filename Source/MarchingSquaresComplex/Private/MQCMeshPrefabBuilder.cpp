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

#include "MQCMeshPrefabBuilder.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

#include "MQCMap.h"

UMQCPrefabBuilder::UMQCPrefabBuilder()
    : bPrefabInitialized(false)
{
}

const FStaticMeshVertexBuffers& UMQCPrefabBuilder::GetVertexBuffers(const UStaticMesh& Mesh, int32 LODIndex) const
{
    return Mesh.RenderData->LODResources[LODIndex].VertexBuffers;
}

const FStaticMeshSection& UMQCPrefabBuilder::GetSection(const UStaticMesh& Mesh, int32 LODIndex, int32 SectionIndex) const
{
    return Mesh.RenderData->LODResources[LODIndex].Sections[SectionIndex];
}

FIndexArrayView UMQCPrefabBuilder::GetIndexBuffer(const UStaticMesh& Mesh, int32 LODIndex) const
{
    return Mesh.RenderData->LODResources[LODIndex].IndexBuffer.GetArrayView();
}

void UMQCPrefabBuilder::GetPrefabGeometryCount(const FPrefabData& Prefab, uint32& OutNumVertices, uint32& OutNumIndices) const
{
    check(IsValidPrefab(Prefab));

    const FStaticMeshVertexBuffers& VBs(GetVertexBuffers(Prefab));
    const FPositionVertexBuffer& PositionVB(VBs.PositionVertexBuffer);
    const FStaticMeshSection& Section(GetSection(Prefab));

    OutNumVertices = PositionVB.GetNumVertices();
    OutNumIndices = Section.NumTriangles * 3;
}

bool UMQCPrefabBuilder::IsValidPrefab(const UStaticMesh* Mesh, int32 LODIndex, int32 SectionIndex) const
{
    const int32 L = LODIndex;
    const int32 S = SectionIndex;
    return
        Mesh &&
        Mesh->bAllowCPUAccess &&
        Mesh->RenderData &&
        Mesh->RenderData->LODResources.IsValidIndex(L) && 
        Mesh->RenderData->LODResources[L].Sections.IsValidIndex(S);
}

void UMQCPrefabBuilder::ResetPrefabs()
{
    PreparedPrefabs.Reset();
    bPrefabInitialized = false;
}

void UMQCPrefabBuilder::InitializePrefabs()
{
    if (bPrefabInitialized)
    {
        return;
    }

    // Generate prepared prefab data
    for (const FMQCPrefabInputData& Input : MeshPrefabs)
    {
        // Skip invalid prefabs
        if (! IsValidPrefab(Input))
        {
            continue;
        }

        // Build prepared prefab data

        const UStaticMesh& Mesh(*Input.Mesh);

        const int32 LODIndex = Input.LODIndex;
        const int32 SectionIndex = Input.SectionIndex;

        const FStaticMeshVertexBuffers& VBs(GetVertexBuffers(Mesh, LODIndex));
        const FPositionVertexBuffer& PositionVB(VBs.PositionVertexBuffer);
        const FStaticMeshVertexBuffer& UVTanVB(VBs.StaticMeshVertexBuffer);
        const FColorVertexBuffer& ColorVB(VBs.ColorVertexBuffer);

        int32 NumVertices = PositionVB.GetNumVertices();

        check(NumVertices == UVTanVB.GetNumVertices());
        check(NumVertices == ColorVB.GetNumVertices());

        // Generate sorted index by distance to X zero

        TArray<uint32> SortedIndexAlongX;

        SortedIndexAlongX.Reserve(NumVertices);

        for (int32 i=0; i<NumVertices; ++i)
        {
            SortedIndexAlongX.Emplace(i);
        }

        SortedIndexAlongX.Sort([&PositionVB](const uint32& a, const uint32& b)
            {
                return PositionVB.VertexPosition(a).X < PositionVB.VertexPosition(b).X;
            } );

        // Find surface and extrude bridge indices

        TArray<uint32> SurfaceIndices;
        TArray<uint32> ExtrudeIndices;

        for (int32 i=0; i<NumVertices; ++i)
        {
            const uint32 vi = SortedIndexAlongX[i];
            const FVector& Position(PositionVB.VertexPosition(vi));
            const FColor& Color(ColorVB.VertexColor(vi));

            if (Color.R > 127)
            {
                SurfaceIndices.Emplace(vi);
            }
            else
            if (Color.G > 127)
            {
                ExtrudeIndices.Emplace(vi);
            }
        }

        // Generate new prefab data

        PreparedPrefabs.SetNum(PreparedPrefabs.Num()+1);
        FPrefabData& Prefab(PreparedPrefabs.Last());
        FBox Bounds(Mesh.GetBoundingBox());

        Prefab.Mesh = &Mesh;
        Prefab.LODIndex = LODIndex;
        Prefab.SectionIndex = SectionIndex;
        Prefab.Bounds.Min = FVector2D(Bounds.Min);
        Prefab.Bounds.Max = FVector2D(Bounds.Max);
        Prefab.Length = Prefab.Bounds.GetSize().X;
        Prefab.SortedIndexAlongX = MoveTemp(SortedIndexAlongX);
        Prefab.SurfaceIndices = MoveTemp(SurfaceIndices);
        Prefab.ExtrudeIndices = MoveTemp(ExtrudeIndices);

        check(Prefab.Length > 0.f);
    }

    // Generated sorted prefabs by size along X

    SortedPrefabs.Reserve(GetPreparedPrefabCount());

    for (int32 i=0; i<GetPreparedPrefabCount(); ++i)
    {
        SortedPrefabs.Emplace(i);
    }

    SortedPrefabs.Sort([this](const uint32& a, const uint32& b)
        {
            return PreparedPrefabs[a].Length < PreparedPrefabs[b].Length;
        } );
}

void UMQCPrefabBuilder::BuildEdgePrefabs(FMQCMap& Map, int32 StateIndex)
{
    InitializePrefabs();

    // Empty prepared prefab, abort
    if (GetPreparedPrefabCount() < 1)
    {
        return;
    }

    TArray<FMQCEdgePointList> EdgeList;

    Map.GetEdgeList(EdgeList, StateIndex);

    for (int32 li=0; li<EdgeList.Num(); ++li)
    {
        const TArray<FMQCEdgePoint>& EdgePoints(EdgeList[li].Points);

        check(EdgePoints.Num() > 1);

        float EdgeLength = EdgePoints.Last().Distance;
        float ShortestPrefab = GetSortedPrefab(0).Length;

        // Only build prefabs on edge list longer than the shortest prefab
        if (EdgeLength > ShortestPrefab)
        {
            BuildEdgePrefabs(Map, EdgePoints);
        }
    }
}

void UMQCPrefabBuilder::BuildEdgePrefabs(FMQCMap& Map, const TArray<FMQCEdgePoint>& EdgePoints)
{
    check(EdgePoints.Num() > 1);

    float CurrentDistance = EdgePoints[0].Distance;
    float EndDistance = EdgePoints.Last().Distance;

    FPrefabBufferMap PrefabBufferMap;

    // Reset section data
    GeneratedSection.Reset();

    // Generate required generated prefab list

    struct FPrefabAllocation
    {
        const FPrefabData* Prefab;
        float Scale;
    };

    TArray<FPrefabAllocation> PrefabList;
    PrefabList.Reserve(EndDistance/GetSortedPrefab(0).Length);

    uint32 NumVertices = 0;
    uint32 NumIndices = 0;

    float OccupiedDistance = 0.f;

    static const float PREFAB_MINIMUM_SCALE = .4f;

    while (OccupiedDistance < EndDistance)
    {
        const FPrefabData& Prefab(GetSortedPrefab(0));
        const float PrefabLength = Prefab.Length;
        float Scale = 1.f;

        OccupiedDistance += PrefabLength;

        // Prefab go past end distance, scale down
        if (OccupiedDistance >= EndDistance)
        {
            float InvLength = 1.f/PrefabLength;
            float DeltaDist = EndDistance-(OccupiedDistance-PrefabLength);
            Scale = DeltaDist*InvLength;
            Scale = FMath::Max(0.f, Scale);

            // Scale is below threshold, scale up last prefab and break
            if (Scale < PREFAB_MINIMUM_SCALE)
            {
                check(PrefabList.Num() > 0);
                FPrefabAllocation& LastPrefab(PrefabList.Last());
                // Calculate scale up amount
                InvLength = 1.f/LastPrefab.Prefab->Length;
                Scale = 1.f+DeltaDist*InvLength;
                LastPrefab.Scale = Scale;
                break;
            }
        }

        // Assign new prefab allocation data

        FPrefabAllocation PrefabAlloc = { &Prefab, Scale };
        PrefabList.Emplace(PrefabAlloc);

        uint32 PrefabNumVertices;
        uint32 PrefabNumIndices;

        GetPrefabGeometryCount(Prefab, PrefabNumVertices, PrefabNumIndices);

        NumVertices += PrefabNumVertices;
        NumIndices += PrefabNumIndices;
    }

    AllocateSection(GeneratedSection, NumVertices, NumIndices);

    // Transform and copy prefab buffers to output sections

    int32 PointCount = EdgePoints.Num();
    int32 PointIndex = 0;

    for (int32 PrefabIndex=0; PrefabIndex<PrefabList.Num(); ++PrefabIndex)
    {
        const FPrefabAllocation& PrefabAlloc(PrefabList[PrefabIndex]);
        const FPrefabData& Prefab(*PrefabAlloc.Prefab);
        float PrefabScaleX(PrefabAlloc.Scale);

        // Copy prefab to section

        uint32 VertexOffsetIndex = CopyPrefabToSection(GeneratedSection, PrefabBufferMap, Prefab);

        // Transform positions along edges

        const FPrefabBuffers& PrefabBuffers(PrefabBufferMap.FindChecked(&Prefab));
        const TArray<FVector>& SrcPositions(PrefabBuffers.Positions);
        const TArray<uint32>& IndexAlongX(Prefab.SortedIndexAlongX);

        TArray<FVector>& DstPositions(GeneratedSection.Positions);

        float StartDistance = CurrentDistance;

        for (int32 i=0; i<IndexAlongX.Num(); ++i)
        {
            const uint32 SrcIndex = IndexAlongX[i];
            const uint32 DstIndex = VertexOffsetIndex+SrcIndex;
            const FVector SrcPos(SrcPositions[SrcIndex]);

            CurrentDistance = StartDistance+SrcPos.X*PrefabScaleX;

            // Find current edge point along prefab mesh X
            while ((PointIndex+1) < PointCount && CurrentDistance >= EdgePoints[PointIndex+1].Distance)
            {
                ++PointIndex;
            }

            // Transform position
#if 1
            const uint32 pi0 = FMath::Max(0, PointIndex-1);
            const uint32 pi1 = PointIndex;
            const uint32 pi2 = FMath::Min(PointIndex+1, PointCount-1);
            const FMQCEdgePoint& EdgePoint0(EdgePoints[pi0]);
            const FMQCEdgePoint& EdgePoint1(EdgePoints[pi1]);
            const FMQCEdgePoint& EdgePoint2(EdgePoints[pi2]);
            float Dist12 = EdgePoint2.Distance-EdgePoint1.Distance;
            float Dist1P = CurrentDistance-EdgePoint1.Distance;
            float DistAlpha = Dist12 > SMALL_NUMBER ? Dist1P/Dist12 : 0.f;
            FVector2D Normal0(EdgePoint0.Normal);
            FVector2D Normal1(EdgePoint1.Normal);
            FVector2D InterpNormal;
            InterpNormal = FMath::LerpStable(Normal0, Normal1, DistAlpha);
            InterpNormal.Normalize();
            FVector2D Offset(EdgePoint1.Position);
            FVector2D TransformedPoint(-InterpNormal.Y, InterpNormal.X);
            TransformedPoint *= SrcPos.Y;
            TransformedPoint += Offset+InterpNormal*Dist1P;
            DstPositions[DstIndex] = FVector(TransformedPoint, SrcPos.Z);
#else
            const FMQCEdgePoint& EdgePoint(EdgePoints[PointIndex]);
            float DistanceDelta = CurrentDistance - EdgePoint.Distance;
            FVector2D Offset(EdgePoint.Position);
            FVector2D TransformedPoint(-EdgePoint.Normal.Y, EdgePoint.Normal.X);
            TransformedPoint *= SrcPos.Y;
            TransformedPoint += Offset+EdgePoint.Normal*DistanceDelta;
            DstPositions[DstIndex] = FVector(TransformedPoint, SrcPos.Z);
#endif

            // Expand bounding box
            GeneratedSection.SectionLocalBox += DstPositions[DstIndex];
        }
    }
}

void UMQCPrefabBuilder::AllocateSection(FPMUMeshSection& OutSection, uint32 NumVertices, uint32 NumIndices)
{
    OutSection.Positions.Reserve(NumVertices);
    OutSection.UVs.Reserve(NumVertices);
    OutSection.Tangents.Reserve(NumVertices*2);
    OutSection.Indices.Reserve(NumIndices);
}

uint32 UMQCPrefabBuilder::CopyPrefabToSection(FPMUMeshSection& OutSection, FPrefabBufferMap& PrefabBufferMap, const FPrefabData& Prefab)
{
    check(IsValidPrefab(Prefab));

    FPrefabBuffers* PrefabBuffers = PrefabBufferMap.Find(&Prefab);

    // Prefab buffers have not been mapped, create new one
    if (! PrefabBuffers)
    {
        PrefabBufferMap.Emplace(&Prefab, FPrefabBuffers());
        PrefabBuffers = &PrefabBufferMap.FindChecked(&Prefab);

        // Copy vertex buffer

        const FStaticMeshVertexBuffers& VBs(GetVertexBuffers(Prefab));
        const FPositionVertexBuffer& SrcPositions(VBs.PositionVertexBuffer);
        const FStaticMeshVertexBuffer& SrcUVTans(VBs.StaticMeshVertexBuffer);
        const uint32 NumVertices = SrcPositions.GetNumVertices();
        const bool bHasUV = SrcUVTans.GetNumTexCoords() > 0;

        check(SrcUVTans.GetNumVertices() == NumVertices);

        TArray<FVector>& DstPositions(PrefabBuffers->Positions);
        TArray<uint32>& DstTangents(PrefabBuffers->Tangents);
        TArray<FVector2D>& DstUVs(PrefabBuffers->UVs);

        DstPositions.SetNumUninitialized(NumVertices);
        DstTangents.SetNumUninitialized(NumVertices*2);

        if (bHasUV)
        {
            DstUVs.SetNumUninitialized(NumVertices);
        }

        for (uint32 i=0; i<NumVertices; ++i)
        {
            DstPositions[i] = SrcPositions.VertexPosition(i);

            FPackedNormal TangentX(SrcUVTans.VertexTangentX(i));
            FPackedNormal TangentZ(SrcUVTans.VertexTangentZ(i));
            DstTangents[i*2  ] = TangentX.Vector.Packed;
            DstTangents[i*2+1] = TangentZ.Vector.Packed;

            if (bHasUV)
            {
                DstUVs[i] = SrcUVTans.GetVertexUV(i, 0);
            }
        }

        // Copy index buffer

        const FStaticMeshSection& Section(GetSection(Prefab));

        FIndexArrayView SrcIndexBuffer = GetIndexBuffer(Prefab);
        const uint32 NumIndices = Section.NumTriangles * 3;
        const uint32 OnePastLastIndex = Section.FirstIndex + NumIndices;

        TArray<uint32>& DstIndices(PrefabBuffers->Indices);
        DstIndices.Reserve(NumIndices);

        for (uint32 i=Section.FirstIndex; i<OnePastLastIndex; ++i)
        {
            DstIndices.Emplace(SrcIndexBuffer[i]);
        }
    }

    // Copy prefab from mapped buffers
    return CopyPrefabToSection(OutSection, *PrefabBuffers);
}

uint32 UMQCPrefabBuilder::CopyPrefabToSection(FPMUMeshSection& OutSection, const FPrefabBuffers& PrefabBuffers)
{
    // Copy buffers

    const TArray<FVector>& SrcPositions(PrefabBuffers.Positions);
    const TArray<FVector2D>& SrcUVs(PrefabBuffers.UVs);
    const TArray<uint32>& SrcTangents(PrefabBuffers.Tangents);
    const TArray<uint32>& SrcIndices(PrefabBuffers.Indices);

    TArray<FVector>& DstPositions(OutSection.Positions);
    TArray<FVector2D>& DstUVs(OutSection.UVs);
    TArray<uint32>& DstTangents(OutSection.Tangents);
    TArray<uint32>& DstIndices(OutSection.Indices);

    const uint32 OffsetIndex = DstPositions.Num();

    // Copy vertex buffers

    DstPositions.Append(SrcPositions);
    DstTangents.Append(SrcTangents);

    // Copy UV buffer
    if (SrcUVs.Num() == SrcPositions.Num())
    {
        DstUVs.Append(SrcUVs);
    }
    // Fill zeroed UV if prefab mesh have no valid UV buffer
    else
    {
        DstUVs.SetNumZeroed(DstUVs.Num()+SrcPositions.Num(), false);
    }

    // Copy index buffer with offset

    const uint32 NumIndices = SrcIndices.Num();

    for (uint32 i=0; i<NumIndices; ++i)
    {
        DstIndices.Emplace(OffsetIndex+SrcIndices[i]);
    }

    return OffsetIndex;
}

void UMQCPrefabBuilder::K2_BuildEdgePrefabs(UMQCMapRef* MapRef, int32 StateIndex)
{
    if (IsValid(MapRef) && MapRef->IsInitialized())
    {
        BuildEdgePrefabs(MapRef->GetMap(), StateIndex);
    }
}
