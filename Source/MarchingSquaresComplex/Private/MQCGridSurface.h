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

class FMQCGridSurface
{
private:

    friend class FMQCGridChunk;
    friend class FMQCGridRenderer;

	const FName SHAPE_HEIGHT_MAP_NAME   = TEXT("PMU_VOXEL_SHAPE_HEIGHT_MAP");
	const FName SURFACE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_SURFACE_HEIGHT_MAP");
	const FName EXTRUDE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_EXTRUDE_HEIGHT_MAP");

	float voxelSize;
	float voxelSizeInv;
	float voxelSizeHalf;
	float gridSize;
    float mapSize;

    bool bGenerateExtrusion;
    bool bExtrusionSurface;
	float extrusionHeight;

	int32 voxelResolution;
    int32 voxelCount;
    FVector2D position;

	TArray<int32> cornersMin;
	TArray<int32> cornersMax;

	TArray<int32> xEdgesMin;
	TArray<int32> xEdgesMax;

	int32 yEdgeMin;
	int32 yEdgeMax;

    TSet<int32> EdgeIndexSet;
    TArray<TPair<int32, int32>> EdgePairs;

    FPMUMeshSection Section;

    // Height map grid data

    FVector2D GridRangeMin;
    FVector2D GridRangeMax;

    bool bHasHeightMap = false;
    uint8 HeightMapType = 0;
    int32 ShapeHeightMapId = -1;
    int32 SurfaceHeightMapId = -1;
    int32 ExtrudeHeightMapId = -1;

public:

    void Initialize(const FMQCSurfaceConfig& Config);
    void CopyFrom(const FMQCGridSurface& Surface);

	void Clear()
    {
        Section.Reset();
        EdgeIndexSet.Empty();
        EdgePairs.Empty();
	}

	void Apply()
    {
        ApplyVertex();

        // Compact geometry containers
        Section.Positions.Shrink();
        Section.UVs.Shrink();
        Section.TangentsX.Reset();
        Section.TangentsZ.Reset();
        Section.Tangents.Shrink();
        Section.Indices.Shrink();
	}

    FORCEINLINE int32 GetVertexCount() const
    {
        return Section.Positions.Num();
    }

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

// Voxel Triangulation Functions
public:

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

// Geometry Generation Functions
private:

	FORCEINLINE void AddTriangle(int32 a, int32 b, int32 c)
    {
        TArray<uint32>& IndexBuffer(Section.Indices);

        if (bExtrusionSurface)
        {
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(a);
        }
        else
        {
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
        }

        if (bGenerateExtrusion)
        {
            IndexBuffer.Emplace(c+1);
            IndexBuffer.Emplace(b+1);
            IndexBuffer.Emplace(a+1);
        }
	}
	
	FORCEINLINE void AddQuad(int32 a, int32 b, int32 c, int32 d)
    {
        TArray<uint32>& IndexBuffer(Section.Indices);

        if (bExtrusionSurface)
        {
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
        }
        else
        {
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(d);
        }

        if (bGenerateExtrusion)
        {
            IndexBuffer.Emplace(c+1);
            IndexBuffer.Emplace(b+1);
            IndexBuffer.Emplace(a+1);
            IndexBuffer.Emplace(d+1);
            IndexBuffer.Emplace(c+1);
            IndexBuffer.Emplace(a+1);
        }
	}
	
	FORCEINLINE void AddPentagon(int32 a, int32 b, int32 c, int32 d, int32 e)
    {
        TArray<uint32>& IndexBuffer(Section.Indices);

        if (bExtrusionSurface)
        {
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(e);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(a);
        }
        else
        {
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(e);
        }

        if (bGenerateExtrusion)
        {
            IndexBuffer.Emplace(c+1);
            IndexBuffer.Emplace(b+1);
            IndexBuffer.Emplace(a+1);
            IndexBuffer.Emplace(d+1);
            IndexBuffer.Emplace(c+1);
            IndexBuffer.Emplace(a+1);
            IndexBuffer.Emplace(e+1);
            IndexBuffer.Emplace(d+1);
            IndexBuffer.Emplace(a+1);
        }
	}
	
	FORCEINLINE void AddHexagon(int32 a, int32 b, int32 c, int32 d, int32 e, int32 f)
    {
        TArray<uint32>& IndexBuffer(Section.Indices);

        if (bExtrusionSurface)
        {
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(e);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(f);
            IndexBuffer.Emplace(e);
            IndexBuffer.Emplace(a);
        }
        else
        {
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(e);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(e);
            IndexBuffer.Emplace(f);
        }

        if (bGenerateExtrusion)
        {
            IndexBuffer.Emplace(c+1);
            IndexBuffer.Emplace(b+1);
            IndexBuffer.Emplace(a+1);
            IndexBuffer.Emplace(d+1);
            IndexBuffer.Emplace(c+1);
            IndexBuffer.Emplace(a+1);
            IndexBuffer.Emplace(e+1);
            IndexBuffer.Emplace(d+1);
            IndexBuffer.Emplace(a+1);
            IndexBuffer.Emplace(f+1);
            IndexBuffer.Emplace(e+1);
            IndexBuffer.Emplace(a+1);
        }
	}

    void AddSection(int32 a, int32 b)
    {
        // Surface does not require edge section, abort
        if (! bGenerateExtrusion)
        {
            return;
        }

        int32 ea0 = a;
        int32 eb0 = b;
        int32 ea1 = a+1;
        int32 eb1 = b+1;

        TArray<uint32>& IndexBuffer(Section.Indices);
        IndexBuffer.Emplace(eb1);
        IndexBuffer.Emplace(eb0);
        IndexBuffer.Emplace(ea0);
        IndexBuffer.Emplace(ea1);
        IndexBuffer.Emplace(eb1);
        IndexBuffer.Emplace(ea0);

        EdgeIndexSet.Emplace(a);
        EdgeIndexSet.Emplace(b);
        EdgePairs.Emplace(a, b);

#if 0
        const FVector& v0(Section.Positions[a]);
        const FVector& v1(Section.Positions[b]);

        const FVector EdgeDirection = (v0-v1).GetSafeNormal();
        const FVector EdgeCross = EdgeDirection ^ FVector::UpVector;

        Section.TangentsZ[a  ] += EdgeCross;
        Section.TangentsZ[b  ] += EdgeCross;
        Section.TangentsZ[a+1] += EdgeCross;
        Section.TangentsZ[b+1] += EdgeCross;
#endif
    }

    FORCEINLINE void AddSection(int32 a, int32 b, int32 c)
    {
        if (bGenerateExtrusion)
        {
            AddSection(a, c);
            AddSection(c, b);
        }
    }

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

    int32 AddVertex4(const FVector2D& Vertex)
    {
        const int32 Index = GetVertexCount();

        AddVertex(Vertex.X, Vertex.Y, bExtrusionSurface);

        if (bGenerateExtrusion)
        {
            AddVertex(Vertex.X, Vertex.Y, true);
        }

        return Index;
    }

    FORCEINLINE void AddVertex(float X, float Y, bool bIsExtrusion)
    {
        const float PX = (position.X + X) - voxelSizeHalf;
        const float PY = (position.Y + Y) - voxelSizeHalf;

        float Height;
        FVector Normal;
        FColor Color(ForceInitToZero);

        if (bIsExtrusion)
        {
            Height = extrusionHeight;
            Normal.Set(0.f, 0.f, -1.f);
        }
        else
        {
            Height = 0.f;
            Normal.Set(0.f, 0.f, 1.f);
        }

        FVector Pos(PX, PY, Height);
        FPackedNormal TangentX(FVector(1,0,0));
        FPackedNormal TangentZ(FVector4(Normal,1));

        Section.Positions.Emplace(Pos);
        Section.UVs.Emplace(PX, PY);
        if (bGenerateExtrusion)
        {
            Section.TangentsX.Emplace(1,0,0);
            Section.TangentsZ.Emplace(Normal);
        }
        Section.Tangents.Emplace(TangentX.Vector.Packed);
        Section.Tangents.Emplace(TangentZ.Vector.Packed);

        Section.SectionLocalBox += Pos;
    }

    void GenerateEdgeNormals();
    void ApplyVertex();
};
