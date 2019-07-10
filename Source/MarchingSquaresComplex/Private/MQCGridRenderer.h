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
#include "MQCVoxel.h"
#include "MQCCell.h"
#include "MQCGridSurface.h"

class FMQCGridRenderer
{
	FMQCGridSurface surface;

public:

    FMQCGridRenderer() = default;

    FMQCGridRenderer(const FMQCSurfaceConfig& Config)
    {
        Initialize(Config);
    }

    void Initialize(const FMQCSurfaceConfig& Config)
    {
        surface.Initialize(Config);
    }

	void Clear()
    {
		surface.Clear();
	}
	
	void Finalize()
    {
		surface.Finalize();
	}

    FORCEINLINE FMQCGridSurface& GetSurface()
    {
        return surface;
    }

    FORCEINLINE const FMQCGridSurface& GetSurface() const
    {
        return surface;
    }
	
	FORCEINLINE void PrepareCacheForNextCell()
    {
		surface.PrepareCacheForNextCell();
	}
	
	FORCEINLINE void PrepareCacheForNextRow()
    {
		surface.PrepareCacheForNextRow();
	}
	
	FORCEINLINE void CacheFirstCorner(const FMQCVoxel& voxel)
    {
		surface.CacheFirstCorner(voxel);
	}
	
	FORCEINLINE void CacheNextCorner(int32 i, const FMQCVoxel& voxel)
    {
		surface.CacheNextCorner(i, voxel);
	}
	
	FORCEINLINE void CacheEdgeXMinToMax(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
		surface.CacheEdgeXMinToMax(i, voxel, Material);
	}
	
	FORCEINLINE void CacheEdgeXMaxToMin(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
		surface.CacheEdgeXMaxToMin(i, voxel, Material);
	}

	FORCEINLINE void CacheEdgeXWallMinToMax(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
		CacheEdgeXMinToMax(i, voxel, Material);
	}
	
	FORCEINLINE void CacheEdgeXWallMaxToMin(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
		CacheEdgeXMaxToMin(i, voxel, Material);
	}

	FORCEINLINE void CacheEdgeYMinToMax(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
		surface.CacheEdgeYMinToMax(i, voxel, Material);
	}

	FORCEINLINE void CacheEdgeYMaxToMin(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
		surface.CacheEdgeYMaxToMin(i, voxel, Material);
	}

	FORCEINLINE void CacheEdgeYWallMinToMax(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
		CacheEdgeYMinToMax(i, voxel, Material);
	}

	FORCEINLINE void CacheEdgeYWallMaxToMin(int32 i, const FMQCVoxel& voxel, const FMQCMaterial& Material)
    {
		CacheEdgeYMaxToMin(i, voxel, Material);
	}

	void FillA(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddQuadA(cell.i, FeaturePointIndex, !cell.c.IsFilled(), !cell.b.IsFilled());
		}
        else
        {
			surface.AddTriangleA(cell.i, !cell.b.IsFilled());
		}
	}

	void FillB(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddQuadB(cell.i, FeaturePointIndex, !cell.a.IsFilled(), !cell.d.IsFilled());
		}
        else
        {
			surface.AddTriangleB(cell.i, !cell.a.IsFilled());
		}
	}
	
	void FillC(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddQuadC(cell.i, FeaturePointIndex, !cell.d.IsFilled(), !cell.a.IsFilled());
		}
        else
        {
			surface.AddTriangleC(cell.i, !cell.a.IsFilled());
		}
	}
	
	void FillD(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddQuadD(cell.i, FeaturePointIndex, !cell.b.IsFilled(), !cell.c.IsFilled());
		}
        else
        {
			surface.AddTriangleD(cell.i, !cell.b.IsFilled());
		}
	}

	void FillABC(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddHexagonABC(cell.i, FeaturePointIndex, !cell.d.IsFilled());
		}
        else
        {
			surface.AddPentagonABC(cell.i, !cell.d.IsFilled());
		}
	}
	
	void FillABD(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddHexagonABD(cell.i, FeaturePointIndex, !cell.c.IsFilled());
		}
        else
        {
			surface.AddPentagonABD(cell.i, !cell.c.IsFilled());
		}
	}
	
	void FillACD(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddHexagonACD(cell.i, FeaturePointIndex, !cell.b.IsFilled());
		}
        else
        {
			surface.AddPentagonACD(cell.i, !cell.b.IsFilled());
		}
	}
	
	void FillBCD(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddHexagonBCD(cell.i, FeaturePointIndex, !cell.a.IsFilled());
		}
        else
        {
			surface.AddPentagonBCD(cell.i, !cell.a.IsFilled());
		}
	}

	void FillAB(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddPentagonAB(cell.i, FeaturePointIndex, !cell.c.IsFilled(), !cell.d.IsFilled());
		}
        else
        {
			surface.AddQuadAB(cell.i, !cell.c.IsFilled());
		}
	}
	
	void FillAC(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddPentagonAC(cell.i, FeaturePointIndex, !cell.d.IsFilled(), !cell.b.IsFilled());
		}
        else
        {
			surface.AddQuadAC(cell.i, !cell.b.IsFilled());
		}
	}
	
	void FillBD(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddPentagonBD(cell.i, FeaturePointIndex, !cell.a.IsFilled(), !cell.c.IsFilled());
		}
        else
        {
			surface.AddQuadBD(cell.i, !cell.a.IsFilled());
		}
	}
	
	void FillCD(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddPentagonCD(cell.i, FeaturePointIndex, !cell.b.IsFilled(), !cell.a.IsFilled());
		}
        else
        {
			surface.AddQuadCD(cell.i, !cell.a.IsFilled());
		}
	}

	void FillADToB(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddPentagonADToB(cell.i, FeaturePointIndex, !cell.b.IsFilled());
		}
        else
        {
			surface.AddQuadADToB(cell.i, !cell.b.IsFilled());
		}
	}
	
	void FillADToC(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddPentagonADToC(cell.i, FeaturePointIndex, !cell.c.IsFilled());
		}
        else
        {
			surface.AddQuadADToC(cell.i, !cell.c.IsFilled());
		}
	}
	
	void FillBCToA(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddPentagonBCToA(cell.i, FeaturePointIndex, !cell.a.IsFilled());
		}
        else
        {
			surface.AddQuadBCToA(cell.i, !cell.a.IsFilled());
		}
	}
	
	void FillBCToD(const FMQCCell& cell, const FMQCFeaturePoint& f)
    {
		if (f.exists)
        {
            int32 FeaturePointIndex = surface.CacheFeaturePoint(f);
			surface.AddPentagonBCToD(cell.i, FeaturePointIndex, !cell.d.IsFilled());
		}
        else
        {
			surface.AddQuadBCToD(cell.i, !cell.d.IsFilled());
		}
	}

	void FillABCD(const FMQCCell& cell)
    {
		surface.AddQuadABCD(cell.i);
	}
};
