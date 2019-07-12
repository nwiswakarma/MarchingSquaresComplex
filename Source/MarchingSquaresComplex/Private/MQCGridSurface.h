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
#include "MQCMaterial.h"

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
    typedef TPair<int32, float> FEdgeSegment;
    typedef TDoubleLinkedList<int32> FEdgeList;
    typedef TMap<int32, int32> FIndexMap;

    struct FEdgeListData
    {
        FEdgeList EdgeList;
        FGuid Id;
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

	const FName SHAPE_HEIGHT_MAP_NAME   = TEXT("PMU_VOXEL_SHAPE_HEIGHT_MAP");
	const FName SURFACE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_SURFACE_HEIGHT_MAP");
	const FName EXTRUDE_HEIGHT_MAP_NAME = TEXT("PMU_VOXEL_EXTRUDE_HEIGHT_MAP");

    bool bGenerateExtrusion;
    bool bExtrusionSurface;
    bool bRemapEdgeUVs;
    EMQCMaterialType MaterialType;
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
	TArray<float> xEdgesMinValues;
	TArray<float> xEdgesMaxValues;

	int32 yEdgeMin;
	int32 yEdgeMax;
	int32 yEdgeMinValue;
	int32 yEdgeMaxValue;

    TArray<FEdgeListData> EdgeLists;
    TArray<FEdgeSyncData> EdgeSyncList;

    FMeshData SurfaceMeshData;
    FMeshData ExtrudeMeshData;
    FMeshData EdgeMeshData;

    TMap<FMQCMaterialBlend, FIndexMap> MaterialIndexMaps;
    TMap<FMQCMaterialBlend, FPMUMeshSection> MaterialSectionMap;

    void ReserveGeometry();
    void CompactGeometry();

    void ReserveGeometry(FMeshData& MeshData);
    void CompactGeometry(FMeshData& MeshData);

    void GenerateEdgeGeometry();

public:

    void Initialize(const FMQCSurfaceConfig& Config);
	void Finalize();
	void Clear();

    void GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const;
    void RemapEdgeUVs(int32 EdgeListId, float UVStart, float UVEnd);

    FORCEINLINE int32 GetVertexCount() const
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

    FORCEINLINE int32 AppendEdgeSyncData(TArray<FEdgeSyncData>& OutSyncData) const
    {
        int32 StartIndex = OutSyncData.Num();
        OutSyncData.Append(EdgeSyncList);
        return StartIndex;
    }

    // Corner and Edge Caching

	FORCEINLINE void PrepareCacheForNextCell()
    {
		yEdgeMin = yEdgeMax;
        yEdgeMinValue = yEdgeMaxValue;
	}

	FORCEINLINE void PrepareCacheForNextRow()
    {
        cornersMin = cornersMax;
        xEdgesMin = xEdgesMax;
        xEdgesMinValues = xEdgesMaxValues;
	}

	FORCEINLINE void CacheFirstCorner(const FMQCVoxel& voxel)
    {
		cornersMax[0] = AddVertex2(voxel.position, voxel.Material);
	}

	FORCEINLINE void CacheNextCorner(int32 i, const FMQCVoxel& voxel)
    {
		cornersMax[i+1] = AddVertex2(voxel.position, voxel.Material);
	}

	inline void CacheEdgeXMinToMax(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
#if 1
        const float EdgeX = voxel.GetXEdge();
        if (EdgeX > 0.f)
        {
            FVector2D EdgePoint(voxel.GetXEdgePoint());
            xEdgesMax[i] = AddVertex2(EdgePoint, Material);
        }
        else
        {
            xEdgesMax[i] = cornersMax[i];
        }
        xEdgesMaxValues[i] = EdgeX;
#else
        FVector2D EdgePoint(voxel.GetXEdgePoint());
        xEdgesMax[i] = AddVertex2(EdgePoint, Material);
#endif
    }

	inline void CacheEdgeXMaxToMin(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
#if 1
        const float EdgeX = voxel.GetXEdge();
        if (EdgeX < 1.f)
        {
            int32 ix = (i < 1) ? 0 : i-1;
            //UE_LOG(LogTemp,Warning, TEXT("xEdgesMaxValues[%d]: %f"), ix, xEdgesMaxValues[ix]);
            // Whether individual edge is required from previous edge
            if (i < 1 || EdgeX > 0.f || xEdgesMaxValues[i-1] < 1.f)
            {
                FVector2D EdgePoint(voxel.GetXEdgePoint());
                xEdgesMax[i] = AddVertex2(EdgePoint, Material);
            }
            // Merge possible, merge with previous edge
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("MERGE XMAXTOMIN"));
                xEdgesMax[i] = xEdgesMax[i-1];
            }
        }
        else
        {
            xEdgesMax[i] = cornersMax[i+1];
        }
        xEdgesMaxValues[i] = EdgeX;
#else
        FVector2D EdgePoint(voxel.GetXEdgePoint());
        xEdgesMax[i] = AddVertex2(EdgePoint, Material);
#endif
    }

	inline void CacheEdgeYMinToMax(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
#if 1
        const float EdgeY = voxel.GetYEdge();
        if (EdgeY > 0.f)
        {
#if 1
            int32 ix = (i < 1) ? 0 : i;
            UE_LOG(LogTemp,Warning, TEXT("xEdgesMaxValues[%d]: %f (%i)"), ix, xEdgesMaxValues[ix], i);
            // Check whether either edge x or edge y is not a corner
            if (i < 0 || EdgeY < 1.f || xEdgesMaxValues[ix] < 1.f)
            {
                FVector2D EdgePoint(voxel.GetYEdgePoint());
                yEdgeMax = AddVertex2(EdgePoint, Material);
            }
            // Otherwise both edges overlap the same corner, merge with edge x
            else
            {
                UE_LOG(LogTemp,Warning, TEXT("MERGE YMINTOMAX: %s"),
                    *GetSurfaceSection().Positions[xEdgesMax[ix]].ToString());
                yEdgeMax = xEdgesMax[ix];
            }
#else
            FVector2D EdgePoint(voxel.GetYEdgePoint());
            yEdgeMax = AddVertex2(EdgePoint, Material);
#endif
        }
        else
        {
            yEdgeMax = cornersMin[i+1];
        }
        yEdgeMaxValue = EdgeY;
#else
        FVector2D EdgePoint(voxel.GetYEdgePoint());
        yEdgeMax = AddVertex2(EdgePoint, Material);
#endif
    }

	inline void CacheEdgeYMaxToMin(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
#if 1
        const float EdgeY = voxel.GetYEdge();
        if (EdgeY < 1.f)
        {
            int32 ix = (i < 0) ? 0 : i;
            bool bMinXIsEdge = (i < 0)
                ? xEdgesMinValues[ix] > 0.f
                : xEdgesMinValues[ix] < 1.f;
            //UE_LOG(LogTemp,Warning, TEXT("xEdgesMinValues[%d]: %f"), ix, xEdgesMinValues[ix]);
            // Check whether either edge x or edge y is not a corner
            if (EdgeY > 0.f || bMinXIsEdge)
            {
                FVector2D EdgePoint(voxel.GetYEdgePoint());
                yEdgeMax = AddVertex2(EdgePoint, Material);
            }
            // Otherwise both edges overlap the same corner, merge with edge x
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("MERGE YMAXTOMIN"));
                yEdgeMax = xEdgesMin[ix];
            }
        }
        else
        {
            yEdgeMax = cornersMax[i+1];
        }
        yEdgeMaxValue = EdgeY;
#else
        FVector2D EdgePoint(voxel.GetYEdgePoint());
        yEdgeMax = AddVertex2(EdgePoint, Material);
#endif
    }

	FORCEINLINE int32 CacheFeaturePoint(const FMQCFeaturePoint& f)
    {
        check(f.exists);
        return AddVertex2(f.position, f.Material);
    }

	inline int32 CacheFeaturePoint(int32 i, const FMQCFeaturePoint& f, uint8 Mask)
    {
        check(f.exists);
        check((Mask & 0x0F) != 0x00);
        check((Mask & 0x0F) != 0x0F);

        uint8 fm = f.CornerMask;

#if 1
        //UE_LOG(LogTemp,Warning, TEXT("fm: %d %u (%u %u %u %u) %u (%u %u %u %u)"),
        //    i,
        //    fm,
        //    Mask & 0x01,
        //    Mask & 0x02,
        //    Mask & 0x04,
        //    Mask & 0x08,
        //    (~Mask&0x0F),
        //    (~Mask&0x0F) & 0x01,
        //    (~Mask&0x0F) & 0x02,
        //    (~Mask&0x0F) & 0x04,
        //    (~Mask&0x0F) & 0x08
        //    );

        if (fm == 0)
        {
            return AddVertex2(f.position, f.Material);
        }

        // Check for corner overlaps
        if ((fm & Mask) & 0x01)
        {
            //UE_LOG(LogTemp,Warning, TEXT("((fm & Mask) & 0x01)"));
            return cornersMin[i];
        }
        else
        if ((fm & Mask) & 0x02)
        {
            //UE_LOG(LogTemp,Warning, TEXT("((fm & Mask) & 0x02)"));
            return cornersMin[i+1];
        }
        else
        if ((fm & Mask) & 0x04)
        {
            //UE_LOG(LogTemp,Warning, TEXT("((fm & Mask) & 0x04)"));
            return cornersMax[i];
        }
        else
        if ((fm & Mask) & 0x08)
        {
            //UE_LOG(LogTemp,Warning, TEXT("((fm & Mask) & 0x08)"));
            return cornersMax[i+1];
        }

        // Check for edge overlaps
        uint8 InvMask = ~Mask;
        if ((fm & InvMask) & 0x01)
        {
            float EdgeValX = xEdgesMinValues[i];
            float EdgeValY = yEdgeMinValue;
            //UE_LOG(LogTemp,Warning, TEXT("((fm & InvMask) & 0x01): %f %f"),
            //    EdgeValX,
            //    EdgeValY);
            if (EdgeValX < KINDA_SMALL_NUMBER)
            {
                return xEdgesMin[i];
            }
            else
            if (EdgeValY < KINDA_SMALL_NUMBER)
            {
                return yEdgeMin;
            }
        }
        if ((fm & InvMask) & 0x02)
        {
            float EdgeValX = xEdgesMinValues[i];
            float EdgeValY = yEdgeMaxValue;
            //UE_LOG(LogTemp,Warning, TEXT("((fm & InvMask) & 0x02): %f %f"),
            //    EdgeValX,
            //    EdgeValY);
            if (EdgeValX > (1.f-KINDA_SMALL_NUMBER))
            {
                return xEdgesMin[i];
            }
            else
            if (EdgeValY < KINDA_SMALL_NUMBER)
            {
                return yEdgeMax;
            }
        }
        if ((fm & InvMask) & 0x04)
        {
            float EdgeValX = xEdgesMaxValues[i];
            float EdgeValY = yEdgeMinValue;
            //UE_LOG(LogTemp,Warning, TEXT("((fm & InvMask) & 0x04): %f %f"),
            //    EdgeValX,
            //    EdgeValY);
            if (EdgeValX < KINDA_SMALL_NUMBER)
            {
                return xEdgesMax[i];
            }
            else
            if (EdgeValY > (1.f-KINDA_SMALL_NUMBER))
            {
                return yEdgeMin;
            }
        }
        if ((fm & InvMask) & 0x08)
        {
            float EdgeValX = xEdgesMaxValues[i];
            float EdgeValY = yEdgeMaxValue;
            //UE_LOG(LogTemp,Warning, TEXT("((fm & InvMask) & 0x08): %f %f"),
            //    EdgeValX,
            //    EdgeValY);
            if (EdgeValX > (1.f-KINDA_SMALL_NUMBER))
            {
                return xEdgesMax[i];
            }
            else
            if (EdgeValY > (1.f-KINDA_SMALL_NUMBER))
            {
                return yEdgeMax;
            }
        }
#endif

        return AddVertex2(f.position, f.Material);
    }

public:

    // Triangulation Functions

	FORCEINLINE void AddQuadABCD(int32 i)
    {
		AddQuadCorners(cornersMin[i], cornersMax[i], cornersMax[i + 1], cornersMin[i + 1]);
	}
	
	FORCEINLINE void AddTriangleA(int32 i, const bool bWall0)
    {
		AddTriangle(cornersMin[i], yEdgeMin, xEdgesMin[i]);
        if (bWall0) AddEdgeFace(yEdgeMin, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddQuadA(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
    {
		AddQuad(FeatureVertexIndex, xEdgesMin[i], cornersMin[i], yEdgeMin);
        if (bWall0) AddEdgeFace(yEdgeMin, FeatureVertexIndex);
        if (bWall1) AddEdgeFace(FeatureVertexIndex, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddTriangleB(int32 i, const bool bWall0)
    {
		AddTriangle(cornersMin[i + 1], xEdgesMin[i], yEdgeMax);
		if (bWall0) AddEdgeFace(xEdgesMin[i], yEdgeMax);
	}
	
	FORCEINLINE void AddQuadB(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
    {
		AddQuad(FeatureVertexIndex, yEdgeMax, cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddEdgeFace(xEdgesMin[i], FeatureVertexIndex);
		if (bWall1) AddEdgeFace(FeatureVertexIndex, yEdgeMax);
	}
	
	FORCEINLINE void AddTriangleC(int32 i, const bool bWall0)
    {
		AddTriangle(cornersMax[i], xEdgesMax[i], yEdgeMin);
		if (bWall0) AddEdgeFace(xEdgesMax[i], yEdgeMin);
	}
	
	FORCEINLINE void AddQuadC(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
    {
		AddQuad(FeatureVertexIndex, yEdgeMin, cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddEdgeFace(xEdgesMax[i], FeatureVertexIndex);
		if (bWall1) AddEdgeFace(FeatureVertexIndex, yEdgeMin);
	}
	
	FORCEINLINE void AddTriangleD(int32 i, const bool bWall0)
    {
		AddTriangle(cornersMax[i + 1], yEdgeMax, xEdgesMax[i]);
		if (bWall0) AddEdgeFace(yEdgeMax, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddQuadD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
    {
		AddQuad(FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddEdgeFace(yEdgeMax, FeatureVertexIndex);
		if (bWall1) AddEdgeFace(FeatureVertexIndex, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddPentagonABC(int32 i, const bool bWall0)
    {
		AddPentagon(
			cornersMin[i], cornersMax[i], xEdgesMax[i],
			yEdgeMax, cornersMin[i + 1]);
		if (bWall0) AddEdgeFace(xEdgesMax[i], yEdgeMax);
	}
	
	FORCEINLINE void AddHexagonABC(int32 i, int32 FeatureVertexIndex, const bool bWall0)
    {
		AddHexagon(
			FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
			cornersMin[i], cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddEdgeFace(xEdgesMax[i], yEdgeMax, FeatureVertexIndex);
	}
	
	FORCEINLINE void AddPentagonABD(int32 i, const bool bWall0)
    {
		AddPentagon(
			cornersMin[i + 1], cornersMin[i], yEdgeMin,
			xEdgesMax[i], cornersMax[i + 1]);
		if (bWall0) AddEdgeFace(yEdgeMin, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddHexagonABD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
    {
		AddHexagon(
			FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
			cornersMin[i + 1], cornersMin[i], yEdgeMin);
		if (bWall0) AddEdgeFace(yEdgeMin, xEdgesMax[i], FeatureVertexIndex);
	}
	
	FORCEINLINE void AddPentagonACD(int32 i, const bool bWall0)
    {
		AddPentagon(
			cornersMax[i], cornersMax[i + 1], yEdgeMax,
			xEdgesMin[i], cornersMin[i]);
		if (bWall0) AddEdgeFace(yEdgeMax, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddHexagonACD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
    {
		AddHexagon(
			FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
			cornersMax[i], cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddEdgeFace(yEdgeMax, xEdgesMin[i], FeatureVertexIndex);
	}
	
	FORCEINLINE void AddPentagonBCD(int32 i, const bool bWall0)
    {
		AddPentagon(
			cornersMax[i + 1], cornersMin[i + 1], xEdgesMin[i],
			yEdgeMin, cornersMax[i]);
		if (bWall0) AddEdgeFace(xEdgesMin[i], yEdgeMin);
	}
	
	FORCEINLINE void AddHexagonBCD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
    {
		AddHexagon(
			FeatureVertexIndex, yEdgeMin, cornersMax[i],
			cornersMax[i + 1], cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddEdgeFace(xEdgesMin[i], yEdgeMin, FeatureVertexIndex);
	}
	
	FORCEINLINE void AddQuadAB(int32 i, const bool bWall0)
    {
		AddQuad(cornersMin[i], yEdgeMin, yEdgeMax, cornersMin[i + 1]);
		if (bWall0) AddEdgeFace(yEdgeMin, yEdgeMax);
	}
	
	FORCEINLINE void AddPentagonAB(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
    {
		AddPentagon(
			FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
			cornersMin[i], yEdgeMin);
		if (bWall0) AddEdgeFace(yEdgeMin, FeatureVertexIndex);
		if (bWall1) AddEdgeFace(FeatureVertexIndex, yEdgeMax);
	}
	
	FORCEINLINE void AddQuadAC(int32 i, const bool bWall0)
    {
		AddQuad(cornersMin[i], cornersMax[i], xEdgesMax[i], xEdgesMin[i]);
		if (bWall0) AddEdgeFace(xEdgesMax[i], xEdgesMin[i]);
	}
	
	FORCEINLINE void AddPentagonAC(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
    {
		AddPentagon(
			FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
			cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddEdgeFace(xEdgesMax[i], FeatureVertexIndex);
		if (bWall1) AddEdgeFace(FeatureVertexIndex, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddQuadBD(int32 i, const bool bWall0)
    {
		AddQuad(xEdgesMin[i], xEdgesMax[i], cornersMax[i + 1], cornersMin[i + 1]);
		if (bWall0) AddEdgeFace(xEdgesMin[i], xEdgesMax[i]);
	}
	
	FORCEINLINE void AddPentagonBD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
    {
		AddPentagon(
			FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
			cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddEdgeFace(xEdgesMin[i], FeatureVertexIndex);
		if (bWall1) AddEdgeFace(FeatureVertexIndex, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddQuadCD(int32 i, const bool bWall0)
    {
		AddQuad(yEdgeMin, cornersMax[i], cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddEdgeFace(yEdgeMax, yEdgeMin);
	}
	
	FORCEINLINE void AddPentagonCD(int32 i, int32 FeatureVertexIndex, const bool bWall0, const bool bWall1)
    {
		AddPentagon(
			FeatureVertexIndex, yEdgeMin, cornersMax[i],
			cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddEdgeFace(yEdgeMax, FeatureVertexIndex);
		if (bWall1) AddEdgeFace(FeatureVertexIndex, yEdgeMin);
	}
	
	FORCEINLINE void AddQuadBCToA(int32 i, const bool bWall0)
    {
		AddQuad(yEdgeMin, cornersMax[i], cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddEdgeFace(xEdgesMin[i], yEdgeMin);
	}
	
	FORCEINLINE void AddPentagonBCToA(int32 i, int32 FeatureVertexIndex, const bool bWall0)
    {
		AddPentagon(
			FeatureVertexIndex, yEdgeMin, cornersMax[i],
			cornersMin[i + 1], xEdgesMin[i]);
		if (bWall0) AddEdgeFace(xEdgesMin[i], yEdgeMin, FeatureVertexIndex);
	}
	
	FORCEINLINE void AddQuadBCToD(int32 i, const bool bWall0)
    {
		AddQuad(yEdgeMax, cornersMin[i + 1], cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddEdgeFace(xEdgesMax[i], yEdgeMax);
	}
	
	FORCEINLINE void AddPentagonBCToD(int32 i, int32 FeatureVertexIndex, const bool bWall0)
    {
		AddPentagon(
			FeatureVertexIndex, yEdgeMax, cornersMin[i + 1],
			cornersMax[i], xEdgesMax[i]);
		if (bWall0) AddEdgeFace(xEdgesMax[i], yEdgeMax, FeatureVertexIndex);
	}
	
	FORCEINLINE void AddQuadADToB(int32 i, const bool bWall0)
    {
		AddQuad(xEdgesMin[i], cornersMin[i], cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddEdgeFace(yEdgeMax, xEdgesMin[i]);
	}
	
	FORCEINLINE void AddPentagonADToB(int32 i, int32 FeatureVertexIndex, const bool bWall0)
    {
		AddPentagon(
			FeatureVertexIndex, xEdgesMin[i], cornersMin[i],
			cornersMax[i + 1], yEdgeMax);
		if (bWall0) AddEdgeFace(yEdgeMax, xEdgesMin[i], FeatureVertexIndex);
	}
	
	FORCEINLINE void AddQuadADToC(int32 i, const bool bWall0)
    {
		AddQuad(xEdgesMax[i], cornersMax[i + 1], cornersMin[i], yEdgeMin);
		if (bWall0) AddEdgeFace(yEdgeMin, xEdgesMax[i]);
	}
	
	FORCEINLINE void AddPentagonADToC(int32 i, int32 FeatureVertexIndex, const bool bWall0)
    {
		AddPentagon(
			FeatureVertexIndex, xEdgesMax[i], cornersMax[i + 1],
			cornersMin[i], yEdgeMin);
		if (bWall0) AddEdgeFace(yEdgeMin, xEdgesMax[i], FeatureVertexIndex);
	}

private:

    // Geometry Generation Functions

	void AddTriangle(int32 a, int32 b, int32 c);
	void AddQuad(int32 a, int32 b, int32 c, int32 d);
	void AddQuadCorners(int32 a, int32 b, int32 c, int32 d);
	void AddPentagon(int32 a, int32 b, int32 c, int32 d, int32 e);
	void AddHexagon(int32 a, int32 b, int32 c, int32 d, int32 e, int32 f);

    static int32 DuplicateVertex(
        const FMeshData& SrcMeshData,
        FMeshData& DstMeshData,
        int32 VertexIndex
        );

    static int32 DuplicateVertex(
        const FPMUMeshSection& SrcSection,
        FPMUMeshSection& DstSection,
        int32 VertexIndex
        );

    void GenerateEdgeVertex(TArray<int32>& EdgeIndices, int32 SourceIndex);
    float GenerateEdgeSegment(TArray<int32>& EdgeIndices0, TArray<int32>& EdgeIndices1);
    void GenerateEdgeSyncData(int32 EdgeListId, const TArray<FEdgeSegment>& EdgeSegments);

    void AddVertex(
        const FVector2D& Vertex,
        const FMQCMaterial& Material,
        bool bIsExtrusion
        );

    void AddEdgeFace(int32 a, int32 b);
    void AddMaterialFace(int32 a, int32 b, int32 c);

    FORCEINLINE void AddEdgeFace(int32 a, int32 b, int32 c)
    {
        if (bGenerateExtrusion)
        {
            AddEdgeFace(a, c);
            AddEdgeFace(c, b);
        }
    }

    FORCEINLINE void AddMaterialFaceSafe(int32 a, int32 b, int32 c)
    {
        if (a != b && a != c && b != c)
        {
            AddMaterialFace(a, b, c);
        }
    }

    inline int32 AddVertex2(const FVector2D& Vertex, const FMQCMaterial& Material)
    {
        int32 Index = GetVertexCount();

        AddVertex(Vertex, Material, bExtrusionSurface);

        if (bGenerateExtrusion)
        {
            AddVertex(Vertex, Material, true);
        }

        return Index;
    }
};
