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
#include "MQCVoxel.h"
#include "MQCVoxelTypes.h"
#include "MQCGeometryTypes.h"
#include "MQCMaterial.h"

class FMQCGridSurface
{
private:

    friend class FMQCGridChunk;
    friend class FMQCGridRenderer;

    struct FEdgePoint
    {
        uint32 VertexIndex;
        FVector2D Normal;
        float Distance;
    };

    struct FEdgeLink
    {
        uint32 Value;
        FEdgeLink* Next;
    };

    struct FEdgeLinkList
    {
        int32 ElementCount;
        FEdgeLink* Head;
        FEdgeLink* Tail;

        FEdgeLinkList()
            : ElementCount(0)
            , Head(nullptr)
            , Tail(nullptr)
        {
        }

        ~FEdgeLinkList()
        {
            FEdgeLink* Node = Head;
            while (Node)
            {
                FEdgeLink* Next = Node->Next;
                delete Node;
                Node = Next;
            }
        }

        FORCEINLINE int32 Num() const
        {
            return ElementCount;
        }

        FORCEINLINE bool IsEmpty() const
        {
            return ElementCount == 0;
        }

        FORCEINLINE void Reset()
        {
            check(Head == nullptr);
            check(Tail == nullptr);
            ElementCount = 0;
        }

        FORCEINLINE void AddHead(uint32 Value)
        {
            FEdgeLink* Link = new FEdgeLink;
            Link->Value = Value;
            Link->Next = Head;

            if (Head)
            {
                Head = Link;
            }
            else
            {
                check(! Tail);
                Head = Link;
                Tail = Link;
            }

            ++ElementCount;
        }

        FORCEINLINE void AddTail(uint32 Value)
        {
            FEdgeLink* Link = new FEdgeLink;
            Link->Value = Value;
            Link->Next = nullptr;

            if (Tail)
            {
                check(! Tail->Next);
                Tail->Next = Link;
                Tail = Link;
            }
            else
            {
                check(! Head);
                Head = Link;
                Tail = Link;
            }

            ++ElementCount;
        }

        // Connect an edge to the list.
        // Index order is not commutative.
        // List must not be empty.
        inline bool Connect(uint32 InHeadIndex, uint32 InTailIndex)
        {
            check(Head);
            check(Tail);

            uint32 HeadIndex = Head->Value;
            uint32 TailIndex = Tail->Value;

            bool bHasConnection = false;

            // Connect edge at tail
            if (TailIndex == InHeadIndex)
            {
                AddTail(InTailIndex);
                bHasConnection = true;
            }
            // Connect edge at head
            else
            if (HeadIndex == InTailIndex)
            {
                AddHead(InHeadIndex);
                bHasConnection = true;
            }

            return bHasConnection;
        }

        // Merge another list.
        // Index order is not commutative.
        // Both lists must not be empty.
        inline bool Merge(FEdgeLinkList& LinkList)
        {
            check(Head);
            check(Tail);

            check(LinkList.Head);
            check(LinkList.Tail);

            if (LinkList.Head == LinkList.Tail)
            {
                return false;
            }

            uint32 HeadValue0 = Head->Value;
            uint32 TailValue0 = Tail->Value;

            uint32 HeadValue1 = LinkList.Head->Value;
            uint32 TailValue1 = LinkList.Tail->Value;

            bool bHasConnection = false;

            // Merge list at tail
            if (TailValue0 == HeadValue1)
            {
                Tail->Next = LinkList.Head->Next;
                delete LinkList.Head;
                Tail = LinkList.Tail;
                LinkList.Head = nullptr;
                LinkList.Tail = nullptr;
                LinkList.Reset();
                bHasConnection = true;
            }
            // Merge list at head
            else
            if (HeadValue0 == TailValue1)
            {
                LinkList.Tail->Next = Head->Next;
                delete Head;
                Head = LinkList.Head;
                LinkList.Head = nullptr;
                LinkList.Tail = nullptr;
                LinkList.Reset();
                bHasConnection = true;
            }

            return bHasConnection;
        }
    };

    struct FMeshData
    {
        FPMUMeshSection Section;
        TArray<FMQCMaterial> Materials;

        FORCEINLINE void AddFace(uint32 a, uint32 b, uint32 c)
        {
            if (a != b && a != c && b != c)
            {
                Section.Indices.Emplace(a);
                Section.Indices.Emplace(b);
                Section.Indices.Emplace(c);
                //UE_LOG(LogTemp,Warning, TEXT("%u %u %u"), a, b, c);
            }
        }

        FORCEINLINE void AddFaceNoCheck(uint32 a, uint32 b, uint32 c)
        {
            Section.Indices.Emplace(a);
            Section.Indices.Emplace(b);
            Section.Indices.Emplace(c);
            //UE_LOG(LogTemp,Warning, TEXT("%u %u %u NO CHECK"), a, b, c);
        }
    };

    typedef TMap<uint32, uint32> FIndexMap;
    typedef TArray<struct FEdgePoint> FEdgePointList;

	const FName SHAPE_HEIGHT_MAP_NAME   = TEXT("PMU_VOXEL_SHAPE_HEIGHT_MAP");
	const FName SURFACE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_SURFACE_HEIGHT_MAP");
	const FName EXTRUDE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_EXTRUDE_HEIGHT_MAP");

    bool bGenerateExtrusion;
    bool bExtrusionSurface;
    bool bRemapEdgeUVs;

	int32 VoxelResolution;
    int32 VoxelCount;
    bool bRequireHash16;
    float MapSize;
    float MapSizeInv;
	float ExtrusionHeight;
    FIntPoint Position;

    EMQCMaterialType MaterialType;

	TArray<uint32> cornersMinArr;
	TArray<uint32> cornersMaxArr;

	TArray<uint32> xEdgesMinArr;
	TArray<uint32> xEdgesMaxArr;

	uint32* cornersMin;
	uint32* cornersMax;

	uint32* xEdgesMin;
	uint32* xEdgesMax;

	uint32 yEdgeMin;
	uint32 yEdgeMax;

    TMap<uint32, uint32> VertexMap;
    TMap<uint32, uint32> EdgeMap;

    TIndirectArray<FEdgeLinkList> EdgeLinkLists;
    TArray<FMQCEdgeSyncData> EdgeSyncList;
    TArray<FEdgePointList> EdgePointLists;

    FMeshData SurfaceMeshData;
    FMeshData ExtrudeMeshData;
    FMeshData EdgeMeshData;

    TMap<FMQCMaterialBlend, FIndexMap> MaterialIndexMaps;
    TMap<FMQCMaterialBlend, FPMUMeshSection> MaterialSectionMap;

    void ResetGeometry();
    void ReserveGeometry();
    void CompactGeometry();

    void ReserveGeometry(FMeshData& MeshData);
    void CompactGeometry(FMeshData& MeshData);

    void GenerateEdgeGeometry();

public:

    FMQCGridSurface();
    FMQCGridSurface(const FMQCSurfaceConfig& Config);
    ~FMQCGridSurface();

    void Configure(const FMQCSurfaceConfig& Config);
    void Initialize();
	void Finalize();
	void Clear();

    void GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const;
    void RemapEdgeUVs(int32 EdgeListId, float UVStart, float UVEnd);

    void GetEdgePoint(FMQCEdgePoint& EdgePoint, const FEdgePoint& SourcePoint, float DistanceOffset) const;
    void GetConnectedEdgePoints(TArray<FMQCEdgePoint>& OutPoints, const FMQCEdgeSyncData& SyncData, float DistanceOffset) const;

    FORCEINLINE uint32 GetVertexCount() const
    {
        return !bExtrusionSurface
            ? SurfaceMeshData.Section.Positions.Num()
            : ExtrudeMeshData.Section.Positions.Num();
    }

    FORCEINLINE FPMUMeshSection& GetSurfaceSection()
    {
        return SurfaceMeshData.Section;
    }

    FORCEINLINE FPMUMeshSection& GetExtrudeSection()
    {
        return ExtrudeMeshData.Section;
    }

    FORCEINLINE FPMUMeshSection& GetEdgeSection()
    {
        return EdgeMeshData.Section;
    }

    FORCEINLINE const FPMUMeshSection& GetSurfaceSection() const
    {
        return SurfaceMeshData.Section;
    }

    FORCEINLINE const FPMUMeshSection& GetExtrudeSection() const
    {
        return ExtrudeMeshData.Section;
    }

    FORCEINLINE const FPMUMeshSection& GetEdgeSection() const
    {
        return EdgeMeshData.Section;
    }

    FORCEINLINE FVector2D GetPositionByHash(uint32 Hash) const
    {
        const uint32* IndexPtr = VertexMap.Find(Hash);

        if (IndexPtr)
        {
            return !bExtrusionSurface
                ? FVector2D(GetSurfaceSection().Positions[*IndexPtr])
                : FVector2D(GetExtrudeSection().Positions[*IndexPtr]);
        }

        return FVector2D();
    }

    FORCEINLINE FVector2D GetPositionByIndex(uint32 Index) const
    {
        return !bExtrusionSurface
            ? FVector2D(GetSurfaceSection().Positions[Index])
            : FVector2D(GetExtrudeSection().Positions[Index]);
    }

    FORCEINLINE int32 AppendEdgeSyncData(TArray<FMQCEdgeSyncData>& OutSyncData) const
    {
        int32 StartIndex = OutSyncData.Num();
        OutSyncData.Append(EdgeSyncList);
        return StartIndex;
    }

    // -- Corner and Edge Caching

	void PrepareCacheForNextCell();
	void PrepareCacheForNextRow();
	void CacheFirstCorner(const FMQCVoxel& voxel);
	void CacheNextCorner(int32 i, const FMQCVoxel& voxel);
	void CacheEdgeX(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material);
	void CacheEdgeXWithWall(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material);
	void CacheEdgeY(const FMQCVoxel& voxel, const FMQCMaterial& Material);
	void CacheEdgeYWithWall(const FMQCVoxel& voxel, const FMQCMaterial& Material);
	int32 CacheFeaturePoint(const FMQCFeaturePoint& f);

    // -- Fill Functions

	void FillA(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillB(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillC(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillD(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillABC(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillABD(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillACD(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillBCD(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillAB(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillAC(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillBD(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillCD(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillADToB(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillADToC(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillBCToA(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillBCToD(const FMQCCell& cell, const FMQCFeaturePoint& f);
	void FillABCD(const FMQCCell& cell);

private:

    // -- Triangulation Functions

    // Triangulation \wo feature point
	void AddQuadABCD(int32 i);
	void AddTriangleA(int32 i, const bool bWall0);
	void AddTriangleB(int32 i, const bool bWall0);
	void AddTriangleC(int32 i, const bool bWall0);
	void AddTriangleD(int32 i, const bool bWall0);
	void AddPentagonABC(int32 i, const bool bWall0);
	void AddPentagonABD(int32 i, const bool bWall0);
	void AddPentagonACD(int32 i, const bool bWall0);
	void AddPentagonBCD(int32 i, const bool bWall0);
	void AddQuadAB(int32 i, const bool bWall0);
	void AddQuadAC(int32 i, const bool bWall0);
	void AddQuadBD(int32 i, const bool bWall0);
	void AddQuadCD(int32 i, const bool bWall0);
	void AddQuadBCToA(int32 i, const bool bWall0);
	void AddQuadBCToD(int32 i, const bool bWall0);
	void AddQuadADToB(int32 i, const bool bWall0);
	void AddQuadADToC(int32 i, const bool bWall0);

    // Triangulation \w feature point
	void AddQuadA(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1);
	void AddQuadB(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1);
	void AddQuadC(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1);
	void AddQuadD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1);
	void AddHexagonABC(int32 i, int32 FeatureVertexIndex, const bool bWall0);
	void AddHexagonABD(int32 i, int32 FeatureVertexIndex, const bool bWall0);
	void AddHexagonACD(int32 i, int32 FeatureVertexIndex, const bool bWall0);
	void AddHexagonBCD(int32 i, int32 FeatureVertexIndex, const bool bWall0);
	void AddPentagonAB(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1);
	void AddPentagonAC(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1);
	void AddPentagonBD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1);
	void AddPentagonCD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1);
	void AddPentagonBCToA(int32 i, int32 FeatureVertexIndex, const bool bWall0);
	void AddPentagonBCToD(int32 i, int32 FeatureVertexIndex, const bool bWall0);
	void AddPentagonADToB(int32 i, int32 FeatureVertexIndex, const bool bWall0);
	void AddPentagonADToC(int32 i, int32 FeatureVertexIndex, const bool bWall0);

    // -- Geometry Generation Functions

	void AddTriangle(uint32 a, uint32 b, uint32 c);
	void AddQuad(uint32 a, uint32 b, uint32 c, uint32 d);
	void AddQuadCorners(uint32 a, uint32 b, uint32 c, uint32 d);
	void AddPentagon(uint32 a, uint32 b, uint32 c, uint32 d, uint32 e);
	void AddHexagon(uint32 a, uint32 b, uint32 c, uint32 d, uint32 e, uint32 f);

    static uint32 DuplicateVertex(
        const FMeshData& SrcMeshData,
        FMeshData& DstMeshData,
        uint32 VertexIndex
        );

    static uint32 DuplicateVertex(
        const FPMUMeshSection& SrcSection,
        FPMUMeshSection& DstSection,
        uint32 VertexIndex
        );

    void GenerateEdgeVertex(TArray<uint32>& EdgeIndices, uint32 SourceIndex);
    float GenerateEdgeSegment(TArray<uint32>& EdgeIndices0, TArray<uint32>& EdgeIndices1);
    void GenerateEdgeSyncData(int32 EdgeListId);

    void AddVertex(
        const FVector2D& Vertex,
        const FMQCMaterial& Material,
        bool bIsExtrusion
        );

    void AddEdge(uint32 a, uint32 b);
    void AddMaterialFace(uint32 a, uint32 b, uint32 c);

    inline uint32 AddVertex2(uint32 Hash, const FVector2D& Vertex, const FMQCMaterial& Material)
    {
        uint32* IndexPtr = VertexMap.Find(Hash);

        if (IndexPtr)
        {
            return *IndexPtr;
        }

        uint32 Index = GetVertexCount();

        AddVertex(Vertex, Material, bExtrusionSurface);

        if (bGenerateExtrusion)
        {
            AddVertex(Vertex, Material, true);
        }

        VertexMap.Emplace(Hash, Index);

        return Index;
    }

    FORCEINLINE void MapEdge(uint32 VertexIndex, uint32 Hash)
    {
        EdgeMap.Emplace(VertexIndex, Hash);
    }

    FORCEINLINE void AddEdge(uint32 a, uint32 b, uint32 c)
    {
        if (bGenerateExtrusion)
        {
            AddEdge(a, c);
            AddEdge(c, b);
        }
    }

    FORCEINLINE void AddMaterialFaceSafe(uint32 a, uint32 b, uint32 c)
    {
        if (a != b && a != c && b != c)
        {
            AddMaterialFace(a, b, c);
        }
    }
};

// -- Corner and Edge Caching

FORCEINLINE void FMQCGridSurface::PrepareCacheForNextCell()
{
    yEdgeMin = yEdgeMax;
}

FORCEINLINE void FMQCGridSurface::PrepareCacheForNextRow()
{
    Swap(cornersMin, cornersMax);
    Swap(xEdgesMin, xEdgesMax);
}

FORCEINLINE void FMQCGridSurface::CacheFirstCorner(const FMQCVoxel& voxel)
{
    uint32 Hash = bRequireHash16
        ? voxel.GetPositionOnlyHash(Position)
        : voxel.GetPositionOnlyHash8(Position);
    cornersMax[0] = AddVertex2(Hash, voxel.GetPosition(), voxel.Material);
}

FORCEINLINE void FMQCGridSurface::CacheNextCorner(int32 i, const FMQCVoxel& voxel)
{
    uint32 Hash = bRequireHash16
        ? voxel.GetPositionOnlyHash(Position)
        : voxel.GetPositionOnlyHash8(Position);
    cornersMax[i+1] = AddVertex2(Hash, voxel.GetPosition(), voxel.Material);
}

FORCEINLINE void FMQCGridSurface::CacheEdgeX(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
{
    uint32 Hash = bRequireHash16
        ? voxel.GetEdgePointHashX(Position)
        : voxel.GetEdgePointHashX8(Position);
    FVector2D EdgePoint(voxel.GetXEdgePoint());
    xEdgesMax[i] = AddVertex2(Hash, EdgePoint, Material);
    MapEdge(xEdgesMax[i], Hash);
}

FORCEINLINE void FMQCGridSurface::CacheEdgeXWithWall(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
{
    CacheEdgeX(i, voxel, Material);
}

FORCEINLINE void FMQCGridSurface::CacheEdgeY(const FMQCVoxel& voxel, const FMQCMaterial& Material)
{
    uint32 Hash = bRequireHash16
        ? voxel.GetEdgePointHashY(Position)
        : voxel.GetEdgePointHashY8(Position);
    FVector2D EdgePoint(voxel.GetYEdgePoint());
    yEdgeMax = AddVertex2(Hash, EdgePoint, Material);
    MapEdge(yEdgeMax, Hash);
}

FORCEINLINE void FMQCGridSurface::CacheEdgeYWithWall(const FMQCVoxel& voxel, const FMQCMaterial& Material)
{
    CacheEdgeY(voxel, Material);
}

FORCEINLINE int32 FMQCGridSurface::CacheFeaturePoint(const FMQCFeaturePoint& f)
{
    check(f.exists);
    uint32 Hash = bRequireHash16 ? f.GetHash(Position) : f.GetHash8(Position);
    uint32 FeaturePointIndex = AddVertex2(Hash, f.position, f.Material);
    MapEdge(FeaturePointIndex, Hash);
    return FeaturePointIndex;
}

// -- Fill Functions

FORCEINLINE void FMQCGridSurface::FillA(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        uint32 FeaturePointIndex = CacheFeaturePoint(f);
        AddQuadA(cell.i, FeaturePointIndex, !cell.c.IsFilled(), !cell.b.IsFilled());
    }
    else
    {
        AddTriangleA(cell.i, !cell.b.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillB(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddQuadB(cell.i, FeaturePointIndex, !cell.a.IsFilled(), !cell.d.IsFilled());
    }
    else
    {
        AddTriangleB(cell.i, !cell.a.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillC(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddQuadC(cell.i, FeaturePointIndex, !cell.d.IsFilled(), !cell.a.IsFilled());
    }
    else
    {
        AddTriangleC(cell.i, !cell.a.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillD(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddQuadD(cell.i, FeaturePointIndex, !cell.b.IsFilled(), !cell.c.IsFilled());
    }
    else
    {
        AddTriangleD(cell.i, !cell.b.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillABC(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddHexagonABC(cell.i, FeaturePointIndex, !cell.d.IsFilled());
    }
    else
    {
        AddPentagonABC(cell.i, !cell.d.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillABD(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddHexagonABD(cell.i, FeaturePointIndex, !cell.c.IsFilled());
    }
    else
    {
        AddPentagonABD(cell.i, !cell.c.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillACD(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddHexagonACD(cell.i, FeaturePointIndex, !cell.b.IsFilled());
    }
    else
    {
        AddPentagonACD(cell.i, !cell.b.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillBCD(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddHexagonBCD(cell.i, FeaturePointIndex, !cell.a.IsFilled());
    }
    else
    {
        AddPentagonBCD(cell.i, !cell.a.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillAB(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddPentagonAB(cell.i, FeaturePointIndex, !cell.c.IsFilled(), !cell.d.IsFilled());
    }
    else
    {
        AddQuadAB(cell.i, !cell.c.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillAC(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddPentagonAC(cell.i, FeaturePointIndex, !cell.d.IsFilled(), !cell.b.IsFilled());
    }
    else
    {
        AddQuadAC(cell.i, !cell.b.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillBD(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddPentagonBD(cell.i, FeaturePointIndex, !cell.a.IsFilled(), !cell.c.IsFilled());
    }
    else
    {
        AddQuadBD(cell.i, !cell.a.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillCD(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddPentagonCD(cell.i, FeaturePointIndex, !cell.b.IsFilled(), !cell.a.IsFilled());
    }
    else
    {
        AddQuadCD(cell.i, !cell.a.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillADToB(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddPentagonADToB(cell.i, FeaturePointIndex, !cell.b.IsFilled());
    }
    else
    {
        AddQuadADToB(cell.i, !cell.b.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillADToC(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddPentagonADToC(cell.i, FeaturePointIndex, !cell.c.IsFilled());
    }
    else
    {
        AddQuadADToC(cell.i, !cell.c.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillBCToA(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddPentagonBCToA(cell.i, FeaturePointIndex, !cell.a.IsFilled());
    }
    else
    {
        AddQuadBCToA(cell.i, !cell.a.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillBCToD(const FMQCCell& cell, const FMQCFeaturePoint& f)
{
    if (f.exists)
    {
        int32 FeaturePointIndex = CacheFeaturePoint(f);
        AddPentagonBCToD(cell.i, FeaturePointIndex, !cell.d.IsFilled());
    }
    else
    {
        AddQuadBCToD(cell.i, !cell.d.IsFilled());
    }
}

FORCEINLINE void FMQCGridSurface::FillABCD(const FMQCCell& cell)
{
    AddQuadABCD(cell.i);
}

// -- Triangulation Functions

FORCEINLINE void FMQCGridSurface::AddQuadABCD(int32 i)
{
    AddQuadCorners(cornersMin[i], cornersMax[i], cornersMax[i + 1], cornersMin[i + 1]);
}

FORCEINLINE void FMQCGridSurface::AddTriangleA(int32 i, const bool bWall0)
{
    AddTriangle(cornersMin[i], yEdgeMin, xEdgesMin[i]);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddQuadA(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddQuad(FeatureVertexIndex, xEdgesMin[i], cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddTriangleB(int32 i, const bool bWall0)
{
    AddTriangle(cornersMin[i + 1], xEdgesMin[i], yEdgeMax);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddQuadB(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddQuad(FeatureVertexIndex, yEdgeMax, cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddTriangleC(int32 i, const bool bWall0)
{
    AddTriangle(cornersMax[i], xEdgesMax[i], yEdgeMin);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddQuadC(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddQuad(FeatureVertexIndex, yEdgeMin, cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddTriangleD(int32 i, const bool bWall0)
{
    AddTriangle(cornersMax[i + 1], yEdgeMax, xEdgesMax[i]);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddQuadD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddQuad(FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonABC(int32 i, const bool bWall0)
{
    AddPentagon(
        cornersMin[i], cornersMax[i], xEdgesMax[i],
        yEdgeMax, cornersMin[i + 1]);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddHexagonABC(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddHexagon(
        FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
        cornersMin[i], cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMax, FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddPentagonABD(int32 i, const bool bWall0)
{
    AddPentagon(
        cornersMin[i + 1], cornersMin[i], yEdgeMin,
        xEdgesMax[i], cornersMax[i + 1]);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddHexagonABD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddHexagon(
        FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
        cornersMin[i + 1], cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMax[i], FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddPentagonACD(int32 i, const bool bWall0)
{
    AddPentagon(
        cornersMax[i], cornersMax[i + 1], yEdgeMax,
        xEdgesMin[i], cornersMin[i]);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddHexagonACD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddHexagon(
        FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
        cornersMax[i], cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMin[i], FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddPentagonBCD(int32 i, const bool bWall0)
{
    AddPentagon(
        cornersMax[i + 1], cornersMin[i + 1], xEdgesMin[i],
        yEdgeMin, cornersMax[i]);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddHexagonBCD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddHexagon(
        FeatureVertexIndex, yEdgeMin, cornersMax[i],
        cornersMax[i + 1], cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMin, FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddQuadAB(int32 i, const bool bWall0)
{
    AddQuad(cornersMin[i], yEdgeMin, yEdgeMax, cornersMin[i + 1]);
    if (bWall0) AddEdge(yEdgeMin, yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddPentagonAB(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddPentagon(
        FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
        cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddQuadAC(int32 i, const bool bWall0)
{
    AddQuad(cornersMin[i], cornersMax[i], xEdgesMax[i], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMax[i], xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonAC(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddPentagon(
        FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
        cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddQuadBD(int32 i, const bool bWall0)
{
    AddQuad(xEdgesMin[i], xEdgesMax[i], cornersMax[i + 1], cornersMin[i + 1]);
    if (bWall0) AddEdge(xEdgesMin[i], xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonBD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddPentagon(
        FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
        cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddQuadCD(int32 i, const bool bWall0)
{
    AddQuad(yEdgeMin, cornersMax[i], cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddPentagonCD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddPentagon(
        FeatureVertexIndex, yEdgeMin, cornersMax[i],
        cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddQuadBCToA(int32 i, const bool bWall0)
{
    AddQuad(yEdgeMin, cornersMax[i], cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddPentagonBCToA(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddPentagon(
        FeatureVertexIndex, yEdgeMin, cornersMax[i],
        cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMin, FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddQuadBCToD(int32 i, const bool bWall0)
{
    AddQuad(yEdgeMax, cornersMin[i + 1], cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddPentagonBCToD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddPentagon(
        FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
        cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMax, FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddQuadADToB(int32 i, const bool bWall0)
{
    AddQuad(xEdgesMin[i], cornersMin[i], cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonADToB(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddPentagon(
        FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
        cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMin[i], FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddQuadADToC(int32 i, const bool bWall0)
{
    AddQuad(xEdgesMax[i], cornersMax[i + 1], cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonADToC(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddPentagon(
        FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
        cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMax[i], FeatureVertexIndex);
}
