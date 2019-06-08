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

struct FEdgeSyncData
{
    int32 ChunkIndex;
    int32 EdgeListIndex;
    FVector2D HeadPos;
    FVector2D TailPos;
    int32 HeadIndex;
    int32 TailIndex;
    float Length;

    FORCEINLINE FString ToString() const
    {
        return FString::Printf(TEXT("(ChunkIndex: %d, EdgeListIndex: %d, HeadPos: %s, TailPos: %s, HeadIndex: %d, TailIndex: %d, Length: %f)"),
            ChunkIndex,
            EdgeListIndex,
            *HeadPos.ToString(),
            *TailPos.ToString(),
            HeadIndex,
            TailIndex,
            Length
            );
    }
};

class FMQCGridSurface
{
private:

    friend class FMQCGridChunk;
    friend class FMQCGridRenderer;

    typedef TPair<int32, int32> FEdgePair;
    typedef TDoubleLinkedList<int32> FEdgeList;

    struct FEdgeListData
    {
        FEdgeList EdgeList;
        bool bIsCircular = false;
    };

	const FName SHAPE_HEIGHT_MAP_NAME   = TEXT("PMU_VOXEL_SHAPE_HEIGHT_MAP");
	const FName SURFACE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_SURFACE_HEIGHT_MAP");
	const FName EXTRUDE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_EXTRUDE_HEIGHT_MAP");

    bool bGenerateExtrusion;
    bool bExtrusionSurface;
	float ExtrusionHeight;

	int32 VoxelResolution;
    int32 VoxelCount;
    float MapSize;
    float MapSizeInv;
    FVector2D position;

	TArray<int32> cornersMin;
	TArray<int32> cornersMax;

	TArray<int32> xEdgesMin;
	TArray<int32> xEdgesMax;

	int32 yEdgeMin;
	int32 yEdgeMax;

    TSet<int32> EdgeIndexSet;
    TMap<int32, int32> EdgeIndexMap;
    TArray<FEdgePair> EdgePairs;

    TArray<FEdgeListData> EdgeLists;
    TArray<FEdgeSyncData> EdgeSyncList;

    FPMUMeshSection SurfaceSection;
    FPMUMeshSection ExtrudeSection;
    FPMUMeshSection EdgeSection;

    void ReserveGeometry();
    void CompactGeometry();

    void ReserveGeometry(FPMUMeshSection& Section);
    void CompactGeometry(FPMUMeshSection& Section);

    void GenerateEdgeGeometry();

public:

    void Initialize(const FMQCSurfaceConfig& Config);
    void CopyFrom(const FMQCGridSurface& Surface);
	void Finalize();
	void Clear();

    FORCEINLINE int32 GetVertexCount() const
    {
        return !bExtrusionSurface
            ? SurfaceSection.Positions.Num()
            : ExtrudeSection.Positions.Num();
    }

    FORCEINLINE FPMUMeshSection& GetSurfaceSection()
    {
        return SurfaceSection;
    }

    FORCEINLINE FPMUMeshSection& GetExtrudeSection()
    {
        return ExtrudeSection;
    }

    FORCEINLINE FPMUMeshSection& GetEdgeSection()
    {
        return EdgeSection;
    }

    FORCEINLINE int32 AppendEdgeSyncData(TArray<FEdgeSyncData>& OutSyncData) const
    {
        int32 StartIndex = OutSyncData.Num();
        OutSyncData.Append(EdgeSyncList);
        return StartIndex;
    }

    void RemapEdgeUVs(int32 EdgeListId, float UVStart, float UVEnd);

    // Corner and Edge Caching

	FORCEINLINE void CacheFirstCorner(const FMQCVoxel& voxel)
    {
		cornersMax[0] = AddVertex2(voxel.position);
	}

	FORCEINLINE void CacheNextCorner(int32 i, const FMQCVoxel& voxel)
    {
		cornersMax[i + 1] = AddVertex2(voxel.position);
	}

	FORCEINLINE void CacheXEdge(int32 i, const FMQCVoxel& voxel)
    {
        FVector2D EdgePoint(voxel.GetXEdgePoint());
		xEdgesMax[i] = AddVertex4(EdgePoint);
	}

	FORCEINLINE void CacheYEdge(const FMQCVoxel& voxel)
    {
        FVector2D EdgePoint(voxel.GetYEdgePoint());
		yEdgeMax = AddVertex4(EdgePoint);
	}

	FORCEINLINE void PrepareCacheForNextCell()
    {
		yEdgeMin = yEdgeMax;
	}

	FORCEINLINE void PrepareCacheForNextRow()
    {
        Swap(cornersMin, cornersMax);
        Swap(xEdgesMin, xEdgesMax);
	}

public:

    // Triangulation Functions

	FORCEINLINE void AddQuadABCD(int32 i)
    {
		AddQuad(cornersMin[i], cornersMax[i], cornersMax[i + 1], cornersMin[i + 1]);
	}
	
	FORCEINLINE void AddTriangleA(int32 i, const bool bWall0)
    {
		AddTriangle(cornersMin[i], yEdgeMin, xEdgesMin[i]);
        if (bWall0) AddSection(yEdgeMin, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddQuadA(int32 i, const FVector2D& extraVertex, const bool bWall0, const bool bWall1)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddQuad(ExtraVertexIndex, xEdgesMin[i], cornersMin[i], yEdgeMin);
        if (bWall0) AddSection(yEdgeMin, ExtraVertexIndex);
        if (bWall1) AddSection(ExtraVertexIndex, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddTriangleB(int32 i, const bool bWall0)
    {
		AddTriangle(cornersMin[i + 1], xEdgesMin[i], yEdgeMax);
		if (bWall0) AddSection(xEdgesMin[i], yEdgeMax);
	}
	
	FORCEINLINE void AddQuadB(int32 i, const FVector2D& extraVertex, const bool bWall0, const bool bWall1)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddQuad(ExtraVertexIndex, yEdgeMax, cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddSection(xEdgesMin[i], ExtraVertexIndex);
		if (bWall1) AddSection(ExtraVertexIndex, yEdgeMax);
	}
	
	FORCEINLINE void AddTriangleC(int32 i, const bool bWall0)
    {
		AddTriangle(cornersMax[i], xEdgesMax[i], yEdgeMin);
		if (bWall0) AddSection(xEdgesMax[i], yEdgeMin);
	}
	
	FORCEINLINE void AddQuadC(int32 i, const FVector2D& extraVertex, const bool bWall0, const bool bWall1)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddQuad(ExtraVertexIndex, yEdgeMin, cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddSection(xEdgesMax[i], ExtraVertexIndex);
		if (bWall1) AddSection(ExtraVertexIndex, yEdgeMin);
	}
	
	FORCEINLINE void AddTriangleD(int32 i, const bool bWall0)
    {
		AddTriangle(cornersMax[i + 1], yEdgeMax, xEdgesMax[i]);
		if (bWall0) AddSection(yEdgeMax, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddQuadD(int32 i, const FVector2D& extraVertex, const bool bWall0, const bool bWall1)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddQuad(ExtraVertexIndex, xEdgesMax[i], cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddSection(yEdgeMax, ExtraVertexIndex);
		if (bWall1) AddSection(ExtraVertexIndex, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddPentagonABC(int32 i, const bool bWall0)
    {
		AddPentagon(
			cornersMin[i], cornersMax[i], xEdgesMax[i],
			yEdgeMax, cornersMin[i + 1]);
		if (bWall0) AddSection(xEdgesMax[i], yEdgeMax);
	}
	
	FORCEINLINE void AddHexagonABC(int32 i, const FVector2D& extraVertex, const bool bWall0)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddHexagon(
			ExtraVertexIndex, yEdgeMax, cornersMin[i + 1],
			cornersMin[i], cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddSection(xEdgesMax[i], yEdgeMax, ExtraVertexIndex);
	}
	
	FORCEINLINE void AddPentagonABD(int32 i, const bool bWall0)
    {
		AddPentagon(
			cornersMin[i + 1], cornersMin[i], yEdgeMin,
			xEdgesMax[i], cornersMax[i + 1]);
		if (bWall0) AddSection(yEdgeMin, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddHexagonABD(int32 i, const FVector2D& extraVertex, const bool bWall0)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddHexagon(
			ExtraVertexIndex, xEdgesMax[i], cornersMax[i + 1],
			cornersMin[i + 1], cornersMin[i], yEdgeMin);
		if (bWall0) AddSection(yEdgeMin, xEdgesMax[i], ExtraVertexIndex);
	}
	
	FORCEINLINE void AddPentagonACD(int32 i, const bool bWall0)
    {
		AddPentagon(
			cornersMax[i], cornersMax[i + 1], yEdgeMax,
			xEdgesMin[i], cornersMin[i]);
		if (bWall0) AddSection(yEdgeMax, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddHexagonACD(int32 i, const FVector2D& extraVertex, const bool bWall0)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddHexagon(
			ExtraVertexIndex, xEdgesMin[i], cornersMin[i],
			cornersMax[i], cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddSection(yEdgeMax, xEdgesMin[i], ExtraVertexIndex);
	}
	
	FORCEINLINE void AddPentagonBCD(int32 i, const bool bWall0)
    {
		AddPentagon(
			cornersMax[i + 1], cornersMin[i + 1], xEdgesMin[i],
			yEdgeMin, cornersMax[i]);
		if (bWall0) AddSection(xEdgesMin[i], yEdgeMin);
	}
	
	FORCEINLINE void AddHexagonBCD(int32 i, const FVector2D& extraVertex, const bool bWall0)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddHexagon(
			ExtraVertexIndex, yEdgeMin, cornersMax[i],
			cornersMax[i + 1], cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddSection(xEdgesMin[i], yEdgeMin, ExtraVertexIndex);
	}
	
	FORCEINLINE void AddQuadAB(int32 i, const bool bWall0)
    {
		AddQuad(cornersMin[i], yEdgeMin, yEdgeMax, cornersMin[i + 1]);
		if (bWall0) AddSection(yEdgeMin, yEdgeMax);
	}
	
	FORCEINLINE void AddPentagonAB(int32 i, const FVector2D& extraVertex, const bool bWall0, const bool bWall1)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddPentagon(
			ExtraVertexIndex, yEdgeMax, cornersMin[i + 1],
			cornersMin[i], yEdgeMin);
		if (bWall0) AddSection(yEdgeMin, ExtraVertexIndex);
		if (bWall1) AddSection(ExtraVertexIndex, yEdgeMax);
	}
	
	FORCEINLINE void AddQuadAC(int32 i, const bool bWall0)
    {
		AddQuad(cornersMin[i], cornersMax[i], xEdgesMax[i], xEdgesMin[i]);
		if (bWall0) AddSection(xEdgesMax[i], xEdgesMin[i]);
	}
	
	FORCEINLINE void AddPentagonAC(int32 i, const FVector2D& extraVertex, const bool bWall0, const bool bWall1)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddPentagon(
			ExtraVertexIndex, xEdgesMin[i], cornersMin[i],
			cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddSection(xEdgesMax[i], ExtraVertexIndex);
		if (bWall1) AddSection(ExtraVertexIndex, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddQuadBD(int32 i, const bool bWall0)
    {
		AddQuad(xEdgesMin[i], xEdgesMax[i], cornersMax[i + 1], cornersMin[i + 1]);
		if (bWall0) AddSection(xEdgesMin[i], xEdgesMax[i]);
	}
	
	FORCEINLINE void AddPentagonBD(int32 i, const FVector2D& extraVertex, const bool bWall0, const bool bWall1)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddPentagon(
			ExtraVertexIndex, xEdgesMax[i], cornersMax[i + 1],
			cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddSection(xEdgesMin[i], ExtraVertexIndex);
		if (bWall1) AddSection(ExtraVertexIndex, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddQuadCD(int32 i, const bool bWall0)
    {
		AddQuad(yEdgeMin, cornersMax[i], cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddSection(yEdgeMax, yEdgeMin);
	}
	
	FORCEINLINE void AddPentagonCD(int32 i, const FVector2D& extraVertex, const bool bWall0, const bool bWall1)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddPentagon(
			ExtraVertexIndex, yEdgeMin, cornersMax[i],
			cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddSection(yEdgeMax, ExtraVertexIndex);
		if (bWall1) AddSection(ExtraVertexIndex, yEdgeMin);
	}
	
	FORCEINLINE void AddQuadBCToA(int32 i, const bool bWall0)
    {
		AddQuad(yEdgeMin, cornersMax[i], cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddSection(xEdgesMin[i], yEdgeMin);
	}
	
	FORCEINLINE void AddPentagonBCToA(int32 i, const FVector2D& extraVertex, const bool bWall0)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddPentagon(
			ExtraVertexIndex, yEdgeMin, cornersMax[i],
			cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddSection(xEdgesMin[i], yEdgeMin, ExtraVertexIndex);
	}
	
	FORCEINLINE void AddQuadBCToD(int32 i, const bool bWall0)
    {
		AddQuad(yEdgeMax, cornersMin[i + 1], cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddSection(xEdgesMax[i], yEdgeMax);
	}
	
	FORCEINLINE void AddPentagonBCToD(int32 i, const FVector2D& extraVertex, const bool bWall0)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddPentagon(
			ExtraVertexIndex, yEdgeMax, cornersMin[i + 1],
			cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddSection(xEdgesMax[i], yEdgeMax, ExtraVertexIndex);
	}
	
	FORCEINLINE void AddQuadADToB(int32 i, const bool bWall0)
    {
		AddQuad(xEdgesMin[i], cornersMin[i], cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddSection(yEdgeMax, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddPentagonADToB(int32 i, const FVector2D& extraVertex, const bool bWall0)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddPentagon(
			ExtraVertexIndex, xEdgesMin[i], cornersMin[i],
			cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddSection(yEdgeMax, xEdgesMin[i], ExtraVertexIndex);
	}
	
	FORCEINLINE void AddQuadADToC(int32 i, const bool bWall0)
    {
		AddQuad(xEdgesMax[i], cornersMax[i + 1], cornersMin[i], yEdgeMin);
		if (bWall0) AddSection(yEdgeMin, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddPentagonADToC(int32 i, const FVector2D& extraVertex, const bool bWall0)
    {
        int32 ExtraVertexIndex = AddVertex4(extraVertex);
		AddPentagon(
			ExtraVertexIndex, xEdgesMax[i], cornersMax[i + 1],
			cornersMin[i], yEdgeMin);
		if (bWall0) AddSection(yEdgeMin, xEdgesMax[i], ExtraVertexIndex);
	}

private:

    // Geometry Generation Functions

	void AddTriangle(int32 a, int32 b, int32 c);
	void AddQuad(int32 a, int32 b, int32 c, int32 d);
	void AddPentagon(int32 a, int32 b, int32 c, int32 d, int32 e);
	void AddHexagon(int32 a, int32 b, int32 c, int32 d, int32 e, int32 f);

    void AddVertex(float X, float Y, bool bIsExtrusion);
    void AddSection(int32 a, int32 b);

    int32 DuplicateVertex(FPMUMeshSection& DstSection, const FPMUMeshSection& SrcSection, int32 VertexIndex);
    int32 FindOrAddEdgeVertex(int32 VertexIndex);

    FORCEINLINE int32 AddVertex2(const FVector2D& Vertex)
    {
        int32 Index = GetVertexCount();

        AddVertex(Vertex.X, Vertex.Y, bExtrusionSurface);

        if (bGenerateExtrusion)
        {
            AddVertex(Vertex.X, Vertex.Y, true);
        }

        return Index;
    }

    FORCEINLINE int32 AddVertex4(const FVector2D& Vertex)
    {
        const int32 Index = GetVertexCount();

        AddVertex(Vertex.X, Vertex.Y, bExtrusionSurface);

        if (bGenerateExtrusion)
        {
            AddVertex(Vertex.X, Vertex.Y, true);
        }

        return Index;
    }

    FORCEINLINE void AddSection(int32 a, int32 b, int32 c)
    {
        if (bGenerateExtrusion)
        {
            AddSection(a, c);
            AddSection(c, b);
        }
    }
};
