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
    VoxelResolution = Config.VoxelResolution;
    VoxelCount      = VoxelResolution * VoxelResolution;
    MapSize = Config.MapSize-1;
    MapSizeInv = MapSize > 0.f ? (1.f/MapSize) : KINDA_SMALL_NUMBER;

    // Extrusion configuration

    bGenerateExtrusion = Config.bGenerateExtrusion;
    bExtrusionSurface  = !bGenerateExtrusion && Config.bExtrusionSurface;
    ExtrusionHeight = (FMath::Abs(Config.ExtrusionHeight) > 0.01f) ? -FMath::Abs(Config.ExtrusionHeight) : -1.f;

    // Resize vertex cache containers
    
    cornersMin.SetNum(VoxelResolution + 1);
    cornersMax.SetNum(VoxelResolution + 1);

    xEdgesMin.SetNum(VoxelResolution);
    xEdgesMax.SetNum(VoxelResolution);

    // Reserves geometry container spaces

    ReserveGeometry();
}

void FMQCGridSurface::CopyFrom(const FMQCGridSurface& Surface)
{
    position = Surface.position;

    VoxelResolution = Surface.VoxelResolution;
    VoxelCount      = Surface.VoxelCount;

    // Extrusion configuration

    bGenerateExtrusion = Surface.bGenerateExtrusion;
    bExtrusionSurface  = Surface.bExtrusionSurface;
    ExtrusionHeight    = Surface.ExtrusionHeight;

    // Resize vertex cache containers
    
    cornersMin.SetNum(VoxelResolution + 1);
    cornersMax.SetNum(VoxelResolution + 1);

    xEdgesMin.SetNum(VoxelResolution);
    xEdgesMax.SetNum(VoxelResolution);

    // Reserves geometry container spaces

    SurfaceSection = Surface.SurfaceSection;
    ExtrudeSection = Surface.ExtrudeSection;
}

void FMQCGridSurface::ReserveGeometry()
{
    if (bGenerateExtrusion)
    {
        ReserveGeometry(SurfaceSection);
        ReserveGeometry(ExtrudeSection);
    }
    else
    if (bExtrusionSurface)
    {
        ReserveGeometry(ExtrudeSection);
    }
    else
    {
        ReserveGeometry(SurfaceSection);
    }
}

void FMQCGridSurface::CompactGeometry()
{
    CompactGeometry(SurfaceSection);
    CompactGeometry(ExtrudeSection);
}

void FMQCGridSurface::ReserveGeometry(FPMUMeshSection& Section)
{
    Section.Positions.Reserve(VoxelCount);
    Section.UVs.Reserve(VoxelCount);
    Section.Colors.Reserve(VoxelCount);
    if (bGenerateExtrusion)
    {
        Section.TangentsX.Reserve(VoxelCount);
        Section.TangentsZ.Reserve(VoxelCount);
    }
    Section.Tangents.Reserve(VoxelCount*2);
    Section.Indices.Reserve(VoxelCount * 6);
}

void FMQCGridSurface::CompactGeometry(FPMUMeshSection& Section)
{
    Section.Positions.Shrink();
    Section.UVs.Shrink();
    Section.Colors.Shrink();
    Section.TangentsX.Reset();
    Section.TangentsZ.Reset();
    Section.Tangents.Shrink();
    Section.Indices.Shrink();
}

void FMQCGridSurface::Finalize()
{
    if (bGenerateExtrusion)
    {
        GenerateEdgeGeometry();
    }

    CompactGeometry();
}

void FMQCGridSurface::Clear()
{
    SurfaceSection.Reset();
    ExtrudeSection.Reset();
    EdgeIndexSet.Empty();
    EdgeIndexMap.Empty();
    EdgePairs.Empty();
}

void FMQCGridSurface::AddVertex(float X, float Y, bool bIsExtrusion)
{
    const float PX = (position.X + X) - .5f;
    const float PY = (position.Y + Y) - .5f;
    FVector2D XY(PX, PY);
    FVector2D UV(XY*MapSizeInv - MapSizeInv*.5f);

    float Height;
    float FaceSign;
    uint8 SurfaceMask = 0;

    FPMUMeshSection* SectionPtr;

    if (bIsExtrusion)
    {
        SectionPtr = &ExtrudeSection;
        Height = ExtrusionHeight;
        SurfaceMask = 0;
        FaceSign = -1.f;
    }
    else
    {
        SectionPtr = &SurfaceSection;
        Height = 0.f;
        SurfaceMask = 255;
        FaceSign = 1.f;
    }

    FPMUMeshSection& Section(*SectionPtr);
    FVector Pos(XY, Height);
    FPackedNormal TangentX(FVector(1,0,0));
    FPackedNormal TangentZ(FVector4(0,0,FaceSign,FaceSign));

    // Assign vertex and bounds

    Section.Positions.Emplace(Pos);
    Section.UVs.Emplace(UV);
    Section.Colors.Emplace(0,0,0,SurfaceMask);
    if (bGenerateExtrusion)
    {
        Section.TangentsX.Emplace(0);
        Section.TangentsZ.Emplace(0);
    }
    Section.Tangents.Emplace(TangentX.Vector.Packed);
    Section.Tangents.Emplace(TangentZ.Vector.Packed);
    Section.SectionLocalBox += Pos;
}

void FMQCGridSurface::AddSection(int32 a, int32 b)
{
    // Surface does not require edge section, abort
    if (! bGenerateExtrusion)
    {
        return;
    }

#if 0
    int32 ea0 = a;
    int32 eb0 = b;
    int32 ea1 = a+1;
    int32 eb1 = b+1;

    TArray<uint32>& IndexBuffer(SurfaceSection.Indices);
    IndexBuffer.Emplace(eb1);
    IndexBuffer.Emplace(eb0);
    IndexBuffer.Emplace(ea0);
    IndexBuffer.Emplace(ea1);
    IndexBuffer.Emplace(eb1);
    IndexBuffer.Emplace(ea0);

    EdgeIndexSet.Emplace(a);
    EdgeIndexSet.Emplace(b);
#endif

    EdgePairs.Emplace(a, b);
}

int32 FMQCGridSurface::DuplicateVertex(FPMUMeshSection& DstSection, const FPMUMeshSection& SrcSection, int32 VertexIndex)
{
    check(SrcSection.Positions.IsValidIndex(VertexIndex));

    int32 OutIndex = DstSection.Positions.Num();

    DstSection.Positions.Emplace(SrcSection.Positions[VertexIndex]);
    DstSection.UVs.Emplace(SrcSection.UVs[VertexIndex]);
    DstSection.Colors.Emplace(SrcSection.Colors[VertexIndex]);
    if (bGenerateExtrusion)
    {
        DstSection.TangentsX.Emplace(0);
        DstSection.TangentsZ.Emplace(0);
    }
    DstSection.Tangents.Emplace(SrcSection.Tangents[(VertexIndex*2)  ]);
    DstSection.Tangents.Emplace(SrcSection.Tangents[(VertexIndex*2)+1]);
    DstSection.SectionLocalBox += SrcSection.Positions[VertexIndex];

    return OutIndex;
}

int32 FMQCGridSurface::FindOrAddEdgeVertex(int32 VertexIndex)
{
    // Only allow call if generating an extrusion and is a valid surface vertex
    check(bGenerateExtrusion);
    check(SurfaceSection.Positions.IsValidIndex(VertexIndex));

    int32* MappedIndex = EdgeIndexMap.Find(VertexIndex);

    // Vertex index found, return mapped index
    if (MappedIndex)
    {
        return *MappedIndex;
    }
    // Vertex index not found, create and map a vertex duplicate
    // and return the mapped index
    else
    {
        int32 i = DuplicateVertex(EdgeSection, SurfaceSection, VertexIndex);
        DuplicateVertex(EdgeSection, ExtrudeSection, VertexIndex);

        EdgeIndexMap.Emplace(VertexIndex, i);

        return i;
    }
}

void FMQCGridSurface::GenerateEdgeGeometry()
{
    if (! bGenerateExtrusion)
    {
        return;
    }

#if 0
    for (const TPair<int32, int32>& EdgePair : EdgePairs)
    {
        const int32& a(EdgePair.Key);
        const int32& b(EdgePair.Value);

        const FVector& v0(SurfaceSection.Positions[a]);
        const FVector& v1(SurfaceSection.Positions[b]);

        const FVector EdgeTangent = (v1-v0).GetSafeNormal();
        const FVector EdgeNormal(-EdgeTangent.Y, EdgeTangent.X, 0.f);

        SurfaceSection.TangentsX[a  ] += EdgeTangent;
        SurfaceSection.TangentsX[b  ] += EdgeTangent;
        SurfaceSection.TangentsX[a+1] += EdgeTangent;
        SurfaceSection.TangentsX[b+1] += EdgeTangent;

        SurfaceSection.TangentsZ[a  ] += EdgeNormal;
        SurfaceSection.TangentsZ[b  ] += EdgeNormal;
        SurfaceSection.TangentsZ[a+1] += EdgeNormal;
        SurfaceSection.TangentsZ[b+1] += EdgeNormal;
    }

    for (int32 i : EdgeIndexSet)
    {
        int32 i0 = i;
        int32 i1 = i+1;

        int32 it0 = i0*2;
        int32 it1 = i1*2;

        FVector& TangentX0(SurfaceSection.TangentsX[i0]);
        FVector& TangentZ0(SurfaceSection.TangentsZ[i0]);

        FVector& TangentX1(SurfaceSection.TangentsX[i1]);
        FVector& TangentZ1(SurfaceSection.TangentsZ[i1]);

        // Normalize tangents

        TangentX0.Normalize();
        TangentX1.Normalize();

        TangentZ0.Normalize();
        TangentZ1.Normalize();

        // Use Gram-Schmidt orthogonalization to make sure X is orth with Z

        TangentX0 -= TangentZ0 * (TangentZ0 | TangentX0);
        TangentX0.Normalize();

        TangentX1 -= TangentZ1 * (TangentZ1 | TangentX1);
        TangentX1.Normalize();

        FPackedNormal TX0(TangentX0);
        FPackedNormal TX1(TangentX1);

        FPackedNormal TZ0(FVector4(TangentZ0, 1));
        FPackedNormal TZ1(FVector4(TangentZ1, 1));

        SurfaceSection.Tangents[it0] = TX0.Vector.Packed;
        SurfaceSection.Tangents[it1] = TX1.Vector.Packed;

        SurfaceSection.Tangents[it0+1] = TZ0.Vector.Packed;
        SurfaceSection.Tangents[it1+1] = TZ1.Vector.Packed;
    }
#else
    TArray<uint32>& IndexBuffer(EdgeSection.Indices);

    // Generate edge tangents and faces

    for (const TPair<int32, int32>& EdgePair : EdgePairs)
    {
        const int32& a(EdgePair.Get<0>());
        const int32& b(EdgePair.Get<1>());

        // Find or add edge vertices

        int32 ea0 = FindOrAddEdgeVertex(a);
        int32 eb0 = FindOrAddEdgeVertex(b);
        int32 ea1 = ea0 + 1;
        int32 eb1 = eb0 + 1;

        // Add edge section face

        IndexBuffer.Emplace(eb1);
        IndexBuffer.Emplace(eb0);
        IndexBuffer.Emplace(ea0);
        IndexBuffer.Emplace(ea1);
        IndexBuffer.Emplace(eb1);
        IndexBuffer.Emplace(ea0);

        // Assign edge tangents

        const FVector& v0(EdgeSection.Positions[ea0]);
        const FVector& v1(EdgeSection.Positions[eb0]);

        const FVector EdgeTangent = (v1-v0).GetSafeNormal();
        const FVector EdgeNormal(-EdgeTangent.Y, EdgeTangent.X, 0.f);

        EdgeSection.TangentsX[ea0] += EdgeTangent;
        EdgeSection.TangentsX[eb0] += EdgeTangent;

        EdgeSection.TangentsZ[ea0] += EdgeNormal;
        EdgeSection.TangentsZ[eb0] += EdgeNormal;
    }

    // Generate packed tangent data

    for (const auto& MappedIndex : EdgeIndexMap)
    {
        int32 i0 = MappedIndex.Value;
        int32 i1 = i0+1;

        int32 it0 = i0*2;
        int32 it1 = i1*2;

        FVector& TangentX(EdgeSection.TangentsX[i0]);
        FVector& TangentZ(EdgeSection.TangentsZ[i0]);

        // Normalize tangents

        TangentX.Normalize();
        TangentZ.Normalize();

        // Use Gram-Schmidt orthogonalization to make sure X is orth with Z

        TangentX -= TangentZ * (TangentZ | TangentX);
        TangentX.Normalize();

        // Assign packed tangents

        FPackedNormal TX(TangentX);
        FPackedNormal TZ(FVector4(TangentZ, 1));

        EdgeSection.Tangents[it0  ] = TX.Vector.Packed;
        EdgeSection.Tangents[it0+1] = TZ.Vector.Packed;

        EdgeSection.Tangents[it1  ] = TX.Vector.Packed;
        EdgeSection.Tangents[it1+1] = TZ.Vector.Packed;
    }
#endif
}

void FMQCGridSurface::AddTriangle(int32 a, int32 b, int32 c)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        TArray<uint32>& IndexBuffer(ExtrudeSection.Indices);
        IndexBuffer.Emplace(c);
        IndexBuffer.Emplace(b);
        IndexBuffer.Emplace(a);
    }
    else
    {
        // Generate surface
        {
            TArray<uint32>& IndexBuffer(SurfaceSection.Indices);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            TArray<uint32>& IndexBuffer(ExtrudeSection.Indices);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(a);
        }
    }
}

void FMQCGridSurface::AddQuad(int32 a, int32 b, int32 c, int32 d)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        TArray<uint32>& IndexBuffer(ExtrudeSection.Indices);
        IndexBuffer.Emplace(c);
        IndexBuffer.Emplace(b);
        IndexBuffer.Emplace(a);
        IndexBuffer.Emplace(d);
        IndexBuffer.Emplace(c);
        IndexBuffer.Emplace(a);
    }
    else
    {
        // Generate surface
        {
            TArray<uint32>& IndexBuffer(SurfaceSection.Indices);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(d);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            TArray<uint32>& IndexBuffer(ExtrudeSection.Indices);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
        }
    }
}

void FMQCGridSurface::AddPentagon(int32 a, int32 b, int32 c, int32 d, int32 e)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        TArray<uint32>& IndexBuffer(ExtrudeSection.Indices);
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
        // Generate surface
        {
            TArray<uint32>& IndexBuffer(SurfaceSection.Indices);
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

        // Generate extrude
        if (bGenerateExtrusion)
        {
            TArray<uint32>& IndexBuffer(ExtrudeSection.Indices);
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
    }
}

void FMQCGridSurface::AddHexagon(int32 a, int32 b, int32 c, int32 d, int32 e, int32 f)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        TArray<uint32>& IndexBuffer(ExtrudeSection.Indices);
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
        // Generate surface
        {
            TArray<uint32>& IndexBuffer(SurfaceSection.Indices);
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

        // Generate extrude
        if (bGenerateExtrusion)
        {
            TArray<uint32>& IndexBuffer(ExtrudeSection.Indices);
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
    }
}
