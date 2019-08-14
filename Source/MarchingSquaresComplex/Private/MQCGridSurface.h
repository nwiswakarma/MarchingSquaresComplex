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
#include "GULMathLibrary.h"

class FMQCGridSurface
{
private:

    typedef TMap<uint32, uint32> FIndexMap;
    typedef TArray<uint32> FIndexArray;

    struct FMeshData
    {
        // Geometry Data
        FPMUMeshSection Section;
        TSet<uint32> QuadFilterHashSet;

        // Material Data
        TArray<FMQCMaterial> Materials;
        TMap<FMQCMaterialBlend, FPMUMeshSection> MaterialSectionMap;
        TMap<FMQCMaterialBlend, FIndexMap> MaterialIndexMap;

        // Geometry Generation
        FORCEINLINE void AddFace(uint32 a, uint32 b, uint32 c);
        FORCEINLINE void AddQuad(uint32 a, uint32 b, uint32 c, uint32 d);
        FORCEINLINE void AddQuadInversed(uint32 a, uint32 b, uint32 c, uint32 d);
        FORCEINLINE bool IsQuadFiltered(uint32 VertexIndex) const;
        FORCEINLINE uint32 DuplicateVertex(FPMUMeshSection& DstSection, uint32 SourceVertexIndex);

        // Material Geometry Generation

        FORCEINLINE void AddMaterialVertex(
            FPMUMeshSection& MaterialSection,
            FIndexMap& VertexIndexMap,
            uint32 VertexIndex,
            uint8 BlendA,
            uint8 BlendB,
            uint8 BlendC
            );

        FORCEINLINE void AddMaterialFace(
            const FMQCMaterialBlend& MaterialBlend,
            uint32 VertexIndexA,
            uint32 VertexIndexB,
            uint32 VertexIndexC,
            uint8 BlendsA[3],
            uint8 BlendsB[3],
            uint8 BlendsC[3]
            );
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

        FEdgeLinkList();
        ~FEdgeLinkList();

        int32 Num() const;
        bool IsEmpty() const;
        void Reset();

        void AddHead(uint32 Value);
        void AddTail(uint32 Value);

        bool Connect(uint32 InHeadIndex, uint32 InTailIndex);
        bool Merge(FEdgeLinkList& LinkList);
    };

	const FName SHAPE_HEIGHT_MAP_NAME   = TEXT("PMU_VOXEL_SHAPE_HEIGHT_MAP");
	const FName SURFACE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_SURFACE_HEIGHT_MAP");
	const FName EXTRUDE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_EXTRUDE_HEIGHT_MAP");

    bool bGenerateExtrusion;
    bool bExtrusionSurface;
    bool bRemapEdgeUVs;

	int32 VoxelResolution;
    int32 VoxelCount;
    float MapSize;
    float MapSizeInv;
	float ExtrusionHeight;
    FIntPoint ChunkPosition;

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

    FIndexMap VertexMap;

    TIndirectArray<FEdgeLinkList> EdgeLinkLists;
    TArray<FMQCEdgeSyncData> EdgeSyncList;
    TArray<FIndexArray> EdgePointIndexList;

    FMeshData SurfaceMeshData;
    FMeshData ExtrudeMeshData;

    void ResetGeometry();
    void ReserveGeometry();
    void CompactGeometry();

    void ReserveGeometry(FMeshData& MeshData);
    void CompactGeometry(FMeshData& MeshData);

public:

    FMQCGridSurface();
    FMQCGridSurface(const FMQCSurfaceConfig& Config);
    ~FMQCGridSurface();

    void Configure(const FMQCSurfaceConfig& Config);
    void Initialize();
	void Finalize();
	void Clear();

    void GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const;

    void GetEdgePoints(TArray<FMQCEdgePointData>& OutPointList) const;
    void GetEdgePoints(TArray<FVector2D>& OutPoints, int32 EdgeListIndex) const;
    void AppendConnectedEdgePoints(TArray<FVector2D>& OutPoints, int32 EdgeListIndex) const;

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

    FORCEINLINE const FPMUMeshSection& GetSurfaceSection() const
    {
        return SurfaceMeshData.Section;
    }

    FORCEINLINE const FPMUMeshSection& GetExtrudeSection() const
    {
        return ExtrudeMeshData.Section;
    }

    FORCEINLINE FPMUMeshSection* GetSurfaceMaterialSection(const FMQCMaterialBlend& Material)
    {
        return SurfaceMeshData.MaterialSectionMap.Find(Material);
    }

    FORCEINLINE FPMUMeshSection* GetExtrudeMaterialSection(const FMQCMaterialBlend& Material)
    {
        return ExtrudeMeshData.MaterialSectionMap.Find(Material);
    }

    FORCEINLINE const FPMUMeshSection* GetSurfaceMaterialSection(const FMQCMaterialBlend& Material) const
    {
        return SurfaceMeshData.MaterialSectionMap.Find(Material);
    }

    FORCEINLINE const FPMUMeshSection* GetExtrudeMaterialSection(const FMQCMaterialBlend& Material) const
    {
        return ExtrudeMeshData.MaterialSectionMap.Find(Material);
    }

    FVector2D GetPositionByIndex(uint32 Index) const;
    int32 AppendEdgeSyncData(TArray<FMQCEdgeSyncData>& OutSyncData) const;
    void AddQuadFilter(const FIntPoint& Point, bool bFilterExtrude);

    // -- Corner and Edge Caching

	void PrepareCacheForNextCell();
	void PrepareCacheForNextRow();
	void CacheFirstCorner(const FMQCVoxel& voxel);
	void CacheNextCorner(int32 i, const FMQCVoxel& voxel);
	void CacheEdgeX(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material);
	void CacheEdgeY(const FMQCVoxel& voxel, const FMQCMaterial& Material);
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

    void GenerateEdgeListData();
    void GenerateEdgeListData(int32 EdgeListIndex);

	void AddQuadFace(uint32 a, uint32 b, uint32 c, uint32 d);
	void AddTriangleEdgeFace(uint32 a, uint32 b, uint32 c);
	void AddQuadEdgeFace(uint32 a, uint32 b, uint32 c, uint32 d);
	void AddPentagonEdgeFace(uint32 a, uint32 b, uint32 c, uint32 d, uint32 e);
	void AddHexagonEdgeFace(uint32 a, uint32 b, uint32 c, uint32 d, uint32 e, uint32 f);

    void AddVertex(const FVector2D& Vertex, const FMQCMaterial& Material, bool bIsExtrusion);
    void AddEdge(uint32 a, uint32 b);
    void AddMaterialFace(uint32 a, uint32 b, uint32 c);

    inline uint32 AddVertex2(const FVector2D& Point, const FMQCMaterial& Material)
    {
        // Generate fixed precision integer point vertex
        FIntPoint VertexFixed = UGULMathLibrary::ScaleToIntPoint(FVector2D(ChunkPosition)+Point);

        // Generate position hash
        uint32 Hash = UGULMathLibrary::GetHash(VertexFixed);

        if (uint32* IndexPtr = VertexMap.Find(Hash))
        {
            return *IndexPtr;
        }
        else
        {
            uint32 Index = GetVertexCount();

            // Generate fixed precision vertex position
            FVector2D Vertex = UGULMathLibrary::ScaleToVector2D(VertexFixed);

            AddVertex(Vertex, Material, bExtrusionSurface);

            if (bGenerateExtrusion)
            {
                AddVertex(Vertex, Material, true);
            }

            VertexMap.Emplace(Hash, Index);

            return Index;
        }
    }

    FORCEINLINE uint32 GetVertexHash(uint32 VertexIndex) const
    {
        return UGULMathLibrary::GetHash(GetPositionByIndex(VertexIndex));
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

FORCEINLINE FVector2D FMQCGridSurface::GetPositionByIndex(uint32 Index) const
{
    return !bExtrusionSurface
        ? FVector2D(GetSurfaceSection().Positions[Index])
        : FVector2D(GetExtrudeSection().Positions[Index]);
}

FORCEINLINE int32 FMQCGridSurface::AppendEdgeSyncData(TArray<FMQCEdgeSyncData>& OutSyncData) const
{
    int32 StartIndex = OutSyncData.Num();
    OutSyncData.Append(EdgeSyncList);
    return StartIndex;
}

FORCEINLINE void FMQCGridSurface::AddQuadFilter(const FIntPoint& Point, bool bFilterExtrude)
{
    uint32 Hash = UGULMathLibrary::GetHash(UGULMathLibrary::ScaleInt(Point));

    if (bFilterExtrude)
    {
        ExtrudeMeshData.QuadFilterHashSet.Emplace(Hash);
    }
    else
    {
        SurfaceMeshData.QuadFilterHashSet.Emplace(Hash);
    }
}

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
    cornersMax[0] = AddVertex2(voxel.GetPosition(), voxel.Material);
}

FORCEINLINE void FMQCGridSurface::CacheNextCorner(int32 i, const FMQCVoxel& voxel)
{
    cornersMax[i+1] = AddVertex2(voxel.GetPosition(), voxel.Material);
}

FORCEINLINE void FMQCGridSurface::CacheEdgeX(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
{
    xEdgesMax[i] = AddVertex2(voxel.GetXEdgePoint(), Material);
}

FORCEINLINE void FMQCGridSurface::CacheEdgeY(const FMQCVoxel& voxel, const FMQCMaterial& Material)
{
    yEdgeMax = AddVertex2(voxel.GetYEdgePoint(), Material);
}

FORCEINLINE int32 FMQCGridSurface::CacheFeaturePoint(const FMQCFeaturePoint& f)
{
    check(f.exists);
    return AddVertex2(f.position, f.Material);
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
    AddQuadFace(cornersMin[i], cornersMax[i], cornersMax[i + 1], cornersMin[i + 1]);
}

FORCEINLINE void FMQCGridSurface::AddTriangleA(int32 i, const bool bWall0)
{
    AddTriangleEdgeFace(cornersMin[i], yEdgeMin, xEdgesMin[i]);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddQuadA(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddQuadEdgeFace(FeatureVertexIndex, xEdgesMin[i], cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddTriangleB(int32 i, const bool bWall0)
{
    AddTriangleEdgeFace(cornersMin[i + 1], xEdgesMin[i], yEdgeMax);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddQuadB(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddQuadEdgeFace(FeatureVertexIndex, yEdgeMax, cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddTriangleC(int32 i, const bool bWall0)
{
    AddTriangleEdgeFace(cornersMax[i], xEdgesMax[i], yEdgeMin);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddQuadC(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddQuadEdgeFace(FeatureVertexIndex, yEdgeMin, cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddTriangleD(int32 i, const bool bWall0)
{
    AddTriangleEdgeFace(cornersMax[i + 1], yEdgeMax, xEdgesMax[i]);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddQuadD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddQuadEdgeFace(FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonABC(int32 i, const bool bWall0)
{
    AddPentagonEdgeFace(
        cornersMin[i], cornersMax[i], xEdgesMax[i],
        yEdgeMax, cornersMin[i + 1]);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddHexagonABC(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddHexagonEdgeFace(
        FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
        cornersMin[i], cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMax, FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddPentagonABD(int32 i, const bool bWall0)
{
    AddPentagonEdgeFace(
        cornersMin[i + 1], cornersMin[i], yEdgeMin,
        xEdgesMax[i], cornersMax[i + 1]);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddHexagonABD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddHexagonEdgeFace(
        FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
        cornersMin[i + 1], cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMax[i], FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddPentagonACD(int32 i, const bool bWall0)
{
    AddPentagonEdgeFace(
        cornersMax[i], cornersMax[i + 1], yEdgeMax,
        xEdgesMin[i], cornersMin[i]);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddHexagonACD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddHexagonEdgeFace(
        FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
        cornersMax[i], cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMin[i], FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddPentagonBCD(int32 i, const bool bWall0)
{
    AddPentagonEdgeFace(
        cornersMax[i + 1], cornersMin[i + 1], xEdgesMin[i],
        yEdgeMin, cornersMax[i]);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddHexagonBCD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddHexagonEdgeFace(
        FeatureVertexIndex, yEdgeMin, cornersMax[i],
        cornersMax[i + 1], cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMin, FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddQuadAB(int32 i, const bool bWall0)
{
    AddQuadEdgeFace(cornersMin[i], yEdgeMin, yEdgeMax, cornersMin[i + 1]);
    if (bWall0) AddEdge(yEdgeMin, yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddPentagonAB(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddPentagonEdgeFace(
        FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
        cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddQuadAC(int32 i, const bool bWall0)
{
    AddQuadEdgeFace(cornersMin[i], cornersMax[i], xEdgesMax[i], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMax[i], xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonAC(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddPentagonEdgeFace(
        FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
        cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddQuadBD(int32 i, const bool bWall0)
{
    AddQuadEdgeFace(xEdgesMin[i], xEdgesMax[i], cornersMax[i + 1], cornersMin[i + 1]);
    if (bWall0) AddEdge(xEdgesMin[i], xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonBD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddPentagonEdgeFace(
        FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
        cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddQuadCD(int32 i, const bool bWall0)
{
    AddQuadEdgeFace(yEdgeMin, cornersMax[i], cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddPentagonCD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
{
    AddPentagonEdgeFace(
        FeatureVertexIndex, yEdgeMin, cornersMax[i],
        cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, FeatureVertexIndex);
    if (bWall1) AddEdge(FeatureVertexIndex, yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddQuadBCToA(int32 i, const bool bWall0)
{
    AddQuadEdgeFace(yEdgeMin, cornersMax[i], cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMin);
}

FORCEINLINE void FMQCGridSurface::AddPentagonBCToA(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddPentagonEdgeFace(
        FeatureVertexIndex, yEdgeMin, cornersMax[i],
        cornersMin[i + 1], xEdgesMin[i]);
    if (bWall0) AddEdge(xEdgesMin[i], yEdgeMin, FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddQuadBCToD(int32 i, const bool bWall0)
{
    AddQuadEdgeFace(yEdgeMax, cornersMin[i + 1], cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMax);
}

FORCEINLINE void FMQCGridSurface::AddPentagonBCToD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddPentagonEdgeFace(
        FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
        cornersMax[i], xEdgesMax[i]);
    if (bWall0) AddEdge(xEdgesMax[i], yEdgeMax, FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddQuadADToB(int32 i, const bool bWall0)
{
    AddQuadEdgeFace(xEdgesMin[i], cornersMin[i], cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMin[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonADToB(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddPentagonEdgeFace(
        FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
        cornersMax[i + 1], yEdgeMax);
    if (bWall0) AddEdge(yEdgeMax, xEdgesMin[i], FeatureVertexIndex);
}

FORCEINLINE void FMQCGridSurface::AddQuadADToC(int32 i, const bool bWall0)
{
    AddQuadEdgeFace(xEdgesMax[i], cornersMax[i + 1], cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMax[i]);
}

FORCEINLINE void FMQCGridSurface::AddPentagonADToC(int32 i, int32 FeatureVertexIndex, const bool bWall0)
{
    AddPentagonEdgeFace(
        FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
        cornersMin[i], yEdgeMin);
    if (bWall0) AddEdge(yEdgeMin, xEdgesMax[i], FeatureVertexIndex);
}

// -- Mesh Data

FORCEINLINE void FMQCGridSurface::FMeshData::AddFace(uint32 a, uint32 b, uint32 c)
{
    if (a != b && a != c && b != c)
    {
        Section.Indices.Emplace(a);
        Section.Indices.Emplace(b);
        Section.Indices.Emplace(c);
        //UE_LOG(LogTemp,Warning, TEXT("%u %u %u"), a, b, c);
    }
}

FORCEINLINE void FMQCGridSurface::FMeshData::AddQuad(uint32 a, uint32 b, uint32 c, uint32 d)
{
    // Ensure no duplicate index
    check(a != b);
    check(a != c);
    check(a != d);
    check(b != c);
    check(b != d);
    check(c != d);

    Section.Indices.Emplace(a);
    Section.Indices.Emplace(b);
    Section.Indices.Emplace(c);

    Section.Indices.Emplace(a);
    Section.Indices.Emplace(c);
    Section.Indices.Emplace(d);

    //UE_LOG(LogTemp,Warning, TEXT("%u %u %u %u NO CHECK"), a, b, c, d);
}

FORCEINLINE void FMQCGridSurface::FMeshData::AddQuadInversed(uint32 a, uint32 b, uint32 c, uint32 d)
{
    // Ensure no duplicate index
    check(a != b);
    check(a != c);
    check(a != d);
    check(b != c);
    check(b != d);
    check(c != d);

    Section.Indices.Emplace(c);
    Section.Indices.Emplace(b);
    Section.Indices.Emplace(a);

    Section.Indices.Emplace(d);
    Section.Indices.Emplace(c);
    Section.Indices.Emplace(a);

    //UE_LOG(LogTemp,Warning, TEXT("%u %u %u %u NO CHECK INVERSED"), a, b, c, d);
}

FORCEINLINE bool FMQCGridSurface::FMeshData::IsQuadFiltered(uint32 VertexIndex) const
{
    return QuadFilterHashSet.Contains(UGULMathLibrary::GetHash(FVector2D(Section.Positions[VertexIndex])));
}

FORCEINLINE uint32 FMQCGridSurface::FMeshData::DuplicateVertex(FPMUMeshSection& DstSection, uint32 SourceVertexIndex)
{
    // Ensure valid source vertex index
    check(Section.Positions.IsValidIndex(SourceVertexIndex));

    uint32 OutIndex = DstSection.Positions.Num();

    DstSection.Positions.Emplace(Section.Positions[SourceVertexIndex]);
    DstSection.UVs.Emplace(Section.UVs[SourceVertexIndex]);
    DstSection.Colors.Emplace(Section.Colors[SourceVertexIndex]);
    DstSection.Tangents.Emplace(Section.Tangents[(SourceVertexIndex*2)  ]);
    DstSection.Tangents.Emplace(Section.Tangents[(SourceVertexIndex*2)+1]);
    DstSection.SectionLocalBox += Section.Positions[SourceVertexIndex];

    return OutIndex;
}

FORCEINLINE void FMQCGridSurface::FMeshData::AddMaterialVertex(
    FPMUMeshSection& MaterialSection,
    FIndexMap& VertexIndexMap,
    uint32 VertexIndex,
    uint8 BlendA,
    uint8 BlendB,
    uint8 BlendC
    )
{
    uint32 MappedIndex;

    if (uint32* MappedIndexPtr = VertexIndexMap.Find(VertexIndex))
    {
        MappedIndex = *MappedIndexPtr;
    }
    else
    {
        MappedIndex = DuplicateVertex(MaterialSection, VertexIndex);

        // Assign blend value
        MaterialSection.Colors[MappedIndex].R = BlendA;
        MaterialSection.Colors[MappedIndex].G = BlendB;
        MaterialSection.Colors[MappedIndex].B = BlendC;

        // Map duplicated vertex index
        VertexIndexMap.Emplace(VertexIndex, MappedIndex);
    }

    MaterialSection.Indices.Emplace(MappedIndex);
}

FORCEINLINE void FMQCGridSurface::FMeshData::AddMaterialFace(
    const FMQCMaterialBlend& MaterialBlend,
    uint32 VertexIndexA,
    uint32 VertexIndexB,
    uint32 VertexIndexC,
    uint8 BlendsA[3],
    uint8 BlendsB[3],
    uint8 BlendsC[3]
    )
{
    FPMUMeshSection& MaterialSection(MaterialSectionMap.FindOrAdd(MaterialBlend));
    FIndexMap& VertexIndexMap(MaterialIndexMap.FindOrAdd(MaterialBlend));

    AddMaterialVertex(MaterialSection, VertexIndexMap, VertexIndexA, BlendsA[0], BlendsB[0], BlendsC[0]);
    AddMaterialVertex(MaterialSection, VertexIndexMap, VertexIndexB, BlendsA[1], BlendsB[1], BlendsC[1]);
    AddMaterialVertex(MaterialSection, VertexIndexMap, VertexIndexC, BlendsA[2], BlendsB[2], BlendsC[2]);
}

// -- Edge Link List

FMQCGridSurface::FEdgeLinkList::FEdgeLinkList()
    : ElementCount(0)
    , Head(nullptr)
    , Tail(nullptr)
{
}

FMQCGridSurface::FEdgeLinkList::~FEdgeLinkList()
{
    FEdgeLink* Node = Head;
    while (Node)
    {
        FEdgeLink* Next = Node->Next;
        delete Node;
        Node = Next;
    }
}

FORCEINLINE int32 FMQCGridSurface::FEdgeLinkList::Num() const
{
    return ElementCount;
}

FORCEINLINE bool FMQCGridSurface::FEdgeLinkList::IsEmpty() const
{
    return ElementCount == 0;
}

FORCEINLINE void FMQCGridSurface::FEdgeLinkList::Reset()
{
    check(Head == nullptr);
    check(Tail == nullptr);
    ElementCount = 0;
}

FORCEINLINE void FMQCGridSurface::FEdgeLinkList::AddHead(uint32 Value)
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

FORCEINLINE void FMQCGridSurface::FEdgeLinkList::AddTail(uint32 Value)
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
inline bool FMQCGridSurface::FEdgeLinkList::Connect(uint32 InHeadIndex, uint32 InTailIndex)
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
inline bool FMQCGridSurface::FEdgeLinkList::Merge(FEdgeLinkList& LinkList)
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
