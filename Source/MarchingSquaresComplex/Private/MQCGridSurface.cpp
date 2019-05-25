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

#include "MQCGridSurface.h"

void FMQCGridSurface::Initialize(const FMQCSurfaceConfig& Config)
{
    // Position & dimension configuration

    position = Config.Position;
    gridSize = Config.ChunkSize;
    mapSize  = Config.MapSize;
    voxelResolution = Config.VoxelResolution;
    voxelCount      = voxelResolution * voxelResolution;
    voxelSize       = gridSize / voxelResolution;
    voxelSizeHalf   = voxelSize / 2.f;
    voxelSizeInv    = 1.f / voxelSize;

    // Extrusion configuration

    bGenerateExtrusion = Config.bGenerateExtrusion;
    bExtrusionSurface  = (! bGenerateExtrusion && Config.bExtrusionSurface);
    extrusionHeight = (FMath::Abs(Config.ExtrusionHeight) > 0.01f) ? -FMath::Abs(Config.ExtrusionHeight) : -1.f;

    {
        bHasHeightMap = false;
        HeightMapType = 0;
        ShapeHeightMapId   = -1;
        SurfaceHeightMapId = -1;
        ExtrudeHeightMapId = -1;
        GridRangeMin = FVector2D::ZeroVector;
        GridRangeMax = FVector2D::ZeroVector;
    }

    // Resize vertex cache containers
    
    cornersMin.SetNum(voxelResolution + 1);
    cornersMax.SetNum(voxelResolution + 1);

    xEdgesMin.SetNum(voxelResolution);
    xEdgesMax.SetNum(voxelResolution);

    // Reserves geometry container spaces

    Section.Positions.Reserve(voxelCount);
    Section.Tangents.Reserve(voxelCount*2);
    Section.Indices.Reserve(voxelCount * 6);
}

void FMQCGridSurface::CopyFrom(const FMQCGridSurface& Surface)
{
    position = Surface.position;
    gridSize = Surface.gridSize;
    mapSize  = Surface.mapSize;

    voxelResolution = Surface.voxelResolution;
    voxelCount      = Surface.voxelCount;
    voxelSize       = Surface.voxelSize;
    voxelSizeHalf   = Surface.voxelSizeHalf;
    voxelSizeInv    = Surface.voxelSizeInv;

    // Extrusion configuration

    bGenerateExtrusion = Surface.bGenerateExtrusion;
    bExtrusionSurface  = Surface.bExtrusionSurface;
    extrusionHeight    = Surface.extrusionHeight;

    // Grid data height map configuration

    bHasHeightMap      = Surface.bHasHeightMap;
    ShapeHeightMapId   = Surface.ShapeHeightMapId;
    SurfaceHeightMapId = Surface.SurfaceHeightMapId;
    ExtrudeHeightMapId = Surface.ExtrudeHeightMapId;
    HeightMapType      = Surface.HeightMapType;
    GridRangeMin       = Surface.GridRangeMin;
    GridRangeMax       = Surface.GridRangeMax;

    // Resize vertex cache containers
    
    cornersMin.SetNum(voxelResolution + 1);
    cornersMax.SetNum(voxelResolution + 1);

    xEdgesMin.SetNum(voxelResolution);
    xEdgesMax.SetNum(voxelResolution);

    // Reserves geometry container spaces

    Section.Positions.Reserve(voxelCount);
    Section.Tangents.Reserve(voxelCount*2);
    Section.Indices.Reserve(voxelCount * 6);
}

void FMQCGridSurface::GenerateEdgeNormals()
{
#if 0
    if (bGenerateExtrusion)
    {
        for (const TPair<int32, int32>& EdgePair : EdgePairs)
        {
            const int32& a(EdgePair.Key);
            const int32& b(EdgePair.Value);

            const FVector& v0(Section.Positions[a]);
            const FVector& v1(Section.Positions[b]);

            const FVector EdgeDirection = (v0-v1).GetSafeNormal();
            const FVector EdgeCross = EdgeDirection ^ FVector::UpVector;

            //Section.VertexBuffer[a  ].Normal += EdgeCross;
            //Section.VertexBuffer[b  ].Normal += EdgeCross;
            //Section.VertexBuffer[a+1].Normal += EdgeCross;
            //Section.VertexBuffer[b+1].Normal += EdgeCross;
        }

        for (int32 i : EdgeIndexSet)
        {
            //Section.VertexBuffer[i  ].Normal.Normalize();
            //Section.VertexBuffer[i+1].Normal.Normalize();
        }
    }
#endif
}

void FMQCGridSurface::ApplyVertex()
{
    if (bGenerateExtrusion)
    {
        GenerateEdgeNormals();
    }
}
