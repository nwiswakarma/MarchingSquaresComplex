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

    bRemapEdgeUVs = Config.bRemapEdgeUVs;

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
    if (! bGenerateExtrusion)
    {
        return;
    }

    int32 ConnectionIndex = -1;

    for (int32 i=0; i<EdgeLists.Num(); ++i)
    {
        FEdgeList& EdgeList(EdgeLists[i].EdgeList);
        int32 HeadIndex = EdgeList.GetHead()->GetValue();
        int32 TailIndex = EdgeList.GetTail()->GetValue();

        bool bHasConnection = false;

        if (a == HeadIndex)
        {
            EdgeList.AddHead(b);
            bHasConnection = true;
        }
        else
        if (a == TailIndex)
        {
            EdgeList.AddTail(b);
            bHasConnection = true;
        }
        else
        if (b == HeadIndex)
        {
            EdgeList.AddHead(a);
            bHasConnection = true;
        }
        else
        if (b == TailIndex)
        {
            EdgeList.AddTail(a);
            bHasConnection = true;
        }

        if (bHasConnection)
        {
            ConnectionIndex = i;
            break;
        }
    }

    if (ConnectionIndex < 0)
    {
        EdgeLists.SetNum(EdgeLists.Num()+1);
        FEdgeListData& EdgeListData(EdgeLists.Last());
        EdgeListData.Id = FGuid::NewGuid();
        EdgeListData.EdgeList.AddTail(a);
        EdgeListData.EdgeList.AddTail(b);
    }
    else
    if (EdgeLists.Num() > 1)
    {
        FEdgeListData& ResolveData(EdgeLists[ConnectionIndex]);
        FEdgeList& ResolveList(ResolveData.EdgeList);
        int32 HeadIndex = ResolveList.GetHead()->GetValue();
        int32 TailIndex = ResolveList.GetTail()->GetValue();
        int32 It = 0;

        while (It < EdgeLists.Num())
        {
            if (ResolveData.Id != EdgeLists[It].Id)
            {
                const FEdgeList& EdgeList(EdgeLists[It].EdgeList);
                auto* h = EdgeList.GetHead();
                auto* t = EdgeList.GetTail();
                int32 hv = h->GetValue();
                int32 tv = t->GetValue();
                bool bHasConnection = false;

                if (hv == HeadIndex)
                {
                    auto* n = h;
                    while (n)
                    {
                        ResolveList.AddHead(n->GetValue());
                        n = n->GetNextNode();
                    }
                    bHasConnection = true;
                }
                else
                if (hv == TailIndex)
                {
                    auto* n = h;
                    while (n)
                    {
                        ResolveList.AddTail(n->GetValue());
                        n = n->GetNextNode();
                    }
                    bHasConnection = true;
                }
                else
                if (tv == HeadIndex)
                {
                    auto* n = t;
                    while (n)
                    {
                        ResolveList.AddHead(n->GetValue());
                        n = n->GetPrevNode();
                    }
                    bHasConnection = true;
                }
                else
                if (tv == TailIndex)
                {
                    auto* n = t;
                    while (n)
                    {
                        ResolveList.AddTail(n->GetValue());
                        n = n->GetPrevNode();
                    }
                    bHasConnection = true;
                }

                if (bHasConnection)
                {
                    EdgeLists.RemoveAtSwap(It, 1, false);
                    It = 0;
                    continue;
                }
            }

            ++It;
        }
    }
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

void FMQCGridSurface::GenerateEdgeVertex(TArray<int32>& EdgeIndices, int32 SourceIndex)
{
    if (EdgeIndices.Num() != 4)
    {
        EdgeIndices.SetNumUninitialized(4);
    }

    int32 esi = DuplicateVertex(EdgeSection, SurfaceSection, SourceIndex);
    int32 eai = DuplicateVertex(EdgeSection, SurfaceSection, SourceIndex);
    int32 ebi = DuplicateVertex(EdgeSection, ExtrudeSection, SourceIndex);
    int32 eei = DuplicateVertex(EdgeSection, ExtrudeSection, SourceIndex);

    const float Z1 = EdgeSection.Positions[eei].Z;
    const float A1 = EdgeSection.Colors[esi].A;

    EdgeSection.Positions[eai].Z = Z1*.33333f;
    EdgeSection.Positions[ebi].Z = Z1*.66667f;
    EdgeSection.Colors[eai].A = A1*.66667f;
    EdgeSection.Colors[ebi].A = A1*.33333f;

    EdgeIndices[0] = esi;
    EdgeIndices[1] = eai;
    EdgeIndices[2] = ebi;
    EdgeIndices[3] = eei;
}

float FMQCGridSurface::GenerateEdgeSegment(TArray<int32>& EdgeIndices0, TArray<int32>& EdgeIndices1)
{
    int32 es0 = EdgeIndices0[0];
    int32 ea0 = EdgeIndices0[1];
    int32 eb0 = EdgeIndices0[2];
    int32 ee0 = EdgeIndices0[3];

    int32 es1 = EdgeIndices1[0];
    int32 ea1 = EdgeIndices1[1];
    int32 eb1 = EdgeIndices1[2];
    int32 ee1 = EdgeIndices1[3];

    // Construct edge geometry

    TArray<uint32>& IndexBuffer(EdgeSection.Indices);

    IndexBuffer.Emplace(ee1);
    IndexBuffer.Emplace(eb1);
    IndexBuffer.Emplace(eb0);
    IndexBuffer.Emplace(ee0);
    IndexBuffer.Emplace(ee1);
    IndexBuffer.Emplace(eb0);

    IndexBuffer.Emplace(eb1);
    IndexBuffer.Emplace(ea1);
    IndexBuffer.Emplace(ea0);
    IndexBuffer.Emplace(eb0);
    IndexBuffer.Emplace(eb1);
    IndexBuffer.Emplace(ea0);

    IndexBuffer.Emplace(ea1);
    IndexBuffer.Emplace(es1);
    IndexBuffer.Emplace(es0);
    IndexBuffer.Emplace(ea0);
    IndexBuffer.Emplace(ea1);
    IndexBuffer.Emplace(es0);

    // Calculate edge length

    FVector2D v0(EdgeSection.Positions[es0]);
    FVector2D v1(EdgeSection.Positions[es1]);
    FVector2D E01 = v1 - v0;

    // Assign edge tangents

    const FVector EdgeTangent(E01.GetSafeNormal(), 0.f);
    const FVector EdgeNormal(-EdgeTangent.Y, EdgeTangent.X, 0.f);

    EdgeSection.TangentsX[es0] += EdgeTangent;
    EdgeSection.TangentsX[es1] += EdgeTangent;

    EdgeSection.TangentsZ[es0] += EdgeNormal;
    EdgeSection.TangentsZ[es1] += EdgeNormal;

    return E01.Size();
}

void FMQCGridSurface::GenerateEdgeGeometry()
{
    // Only generate edge geometry on surface that generate extrusion
    if (! bGenerateExtrusion)
    {
        return;
    }

    //TArray<uint32>& IndexBuffer(EdgeSection.Indices);

    for (int32 ListId=0; ListId<EdgeLists.Num(); ++ListId)
    {
        FEdgeList& EdgeList(EdgeLists[ListId].EdgeList);

        check(EdgeList.Num() >= 2);

        if (EdgeList.Num() < 2)
        {
            continue;
        }

        auto* Node0 = EdgeList.GetHead();
        auto* Node1 = Node0->GetNextNode();

        float EdgeLength = 0.f;
        TArray<TPair<int32, float>> EdgeDistances;

        EdgeDistances.Reserve(EdgeList.Num()+1);

        // Construct initial edge vertex
        int32 nvi = Node0->GetValue();
        TArray<int32> EdgeIndices0;
        TArray<int32> EdgeIndices1;

        GenerateEdgeVertex(EdgeIndices1, nvi);

        //int32 es0;
        //int32 ea0;
        //int32 eb0;
        //int32 ee0;
        //int32 es1;
        //int32 ea1;
        //int32 eb1;
        //int32 ee1;

        //{
        //    es1 = GenerateEdgeVertex(nvi);
        //    ea1 = es1+1;
        //    eb1 = es1+2;
        //    ee1 = es1+3;
        //}

        EdgeDistances.Emplace(EdgeIndices1[0], 0.f);

        // Calculate edge distance data
        do
        {
            nvi = Node1->GetValue();

            EdgeIndices0 = EdgeIndices1;

            GenerateEdgeVertex(EdgeIndices1, nvi);

            EdgeLength += GenerateEdgeSegment(EdgeIndices0, EdgeIndices1);

            // Assign edge vertex index and t distance
            EdgeDistances.Emplace(EdgeIndices1[0], EdgeLength);

#if 0
            es0 = es1;
            ea0 = ea1;
            eb0 = eb1;
            ee0 = ee1;

            es1 = GenerateEdgeVertex(nvi);
            ea1 = es1+1;
            eb1 = es1+2;
            ee1 = es1+3;

            // Calculate edge length
            FVector2D v0(EdgeSection.Positions[es0]);
            FVector2D v1(EdgeSection.Positions[es1]);
            FVector2D E01 = v1 - v0;
            EdgeLength += E01.Size();

            // Assign edge vertex index and t distance
            EdgeDistances.Emplace(es1, EdgeLength);

            // Construct edge geometry
            {
                // Add edge section face

                IndexBuffer.Emplace(ee1);
                IndexBuffer.Emplace(eb1);
                IndexBuffer.Emplace(eb0);
                IndexBuffer.Emplace(ee0);
                IndexBuffer.Emplace(ee1);
                IndexBuffer.Emplace(eb0);

                IndexBuffer.Emplace(eb1);
                IndexBuffer.Emplace(ea1);
                IndexBuffer.Emplace(ea0);
                IndexBuffer.Emplace(eb0);
                IndexBuffer.Emplace(eb1);
                IndexBuffer.Emplace(ea0);

                IndexBuffer.Emplace(ea1);
                IndexBuffer.Emplace(es1);
                IndexBuffer.Emplace(es0);
                IndexBuffer.Emplace(ea0);
                IndexBuffer.Emplace(ea1);
                IndexBuffer.Emplace(es0);

                // Assign edge tangents

                const FVector EdgeTangent(E01.GetSafeNormal(), 0.f);
                const FVector EdgeNormal(-EdgeTangent.Y, EdgeTangent.X, 0.f);

                EdgeSection.TangentsX[es0] += EdgeTangent;
                EdgeSection.TangentsX[es1] += EdgeTangent;

                EdgeSection.TangentsZ[es0] += EdgeNormal;
                EdgeSection.TangentsZ[es1] += EdgeNormal;
            }
#endif

            Node0 = Node1;
            Node1 = Node1->GetNextNode();
        }
        while (Node1);

        if (! bRemapEdgeUVs)
        {
            continue;
        }

        // Calculate inverse edge length

        float EdgeLengthInv = 0.f;

        if (EdgeLength > 0.f)
        {
            EdgeLengthInv = 1.f/EdgeLength;
        }

        // Assign edge uvs and normalized tangents
        for (int32 i=0; i<EdgeDistances.Num(); ++i)
        {
            auto& EdgeData(EdgeDistances[i]);

            int32 vi = EdgeData.Key;

            int32 i0 = vi;
            int32 i1 = vi+1;
            int32 i2 = vi+2;
            int32 i3 = vi+3;

            int32 it0 = i0*2;
            int32 it1 = i1*2;
            int32 it2 = i2*2;
            int32 it3 = i3*2;

            float UVX = EdgeData.Value * EdgeLengthInv;

            FVector& TangentX(EdgeSection.TangentsX[vi]);
            FVector& TangentZ(EdgeSection.TangentsZ[vi]);

            // Normalize tangents

            TangentX.Normalize();
            TangentZ.Normalize();

            // Use Gram-Schmidt orthogonalization to make sure X is orth with Z

            TangentX -= TangentZ * (TangentZ | TangentX);
            TangentX.Normalize();

            // Assign packed tangents

            FPackedNormal TX(TangentX);
            FPackedNormal TZ(FVector4(TangentZ, 1));

            //EdgeSection.Tangents[it0  ] = TX.Vector.Packed;
            //EdgeSection.Tangents[it0+1] = TZ.Vector.Packed;

            EdgeSection.Tangents[it1  ] = TX.Vector.Packed;
            EdgeSection.Tangents[it1+1] = TZ.Vector.Packed;

            EdgeSection.Tangents[it2  ] = TX.Vector.Packed;
            EdgeSection.Tangents[it2+1] = TZ.Vector.Packed;

            //EdgeSection.Tangents[it3  ] = TX.Vector.Packed;
            //EdgeSection.Tangents[it3+1] = TZ.Vector.Packed;

            EdgeSection.UVs[i0].Set(UVX, 1.f);
            EdgeSection.UVs[i1].Set(UVX, 2.f/3.f);
            EdgeSection.UVs[i2].Set(UVX, 1.f/3.f);
            EdgeSection.UVs[i3].Set(UVX, 0.f);
        }

        // Create edge list sync data

        EdgeSyncList.SetNum(EdgeSyncList.Num()+1);
        FEdgeSyncData& SyncData(EdgeSyncList.Last());

        int32 evi0 = EdgeDistances[0].Key;
        int32 evi1 = EdgeDistances.Last().Key;

        SyncData.EdgeListIndex = ListId;
        SyncData.HeadPos = FVector2D(EdgeSection.Positions[evi0]);
        SyncData.TailPos = FVector2D(EdgeSection.Positions[evi1]);
        SyncData.HeadIndex = evi0;
        SyncData.TailIndex = evi1;
        SyncData.Length = EdgeLength;
    }
}

void FMQCGridSurface::RemapEdgeUVs(int32 EdgeListId, float UVStart, float UVEnd)
{
    if (! bRemapEdgeUVs)
    {
        return;
    }

    const FEdgeSyncData& SyncData(EdgeSyncList[EdgeListId]);
    int32 HeadIndex = SyncData.HeadIndex / 4;
    int32 TailIndex = SyncData.TailIndex / 4;
    float UVRange = UVEnd-UVStart;

    for (int32 i=HeadIndex; i<=TailIndex; ++i)
    {
        int32 vi = i*4;

        int32 i0 = vi;
        int32 i1 = vi+1;
        int32 i2 = vi+2;
        int32 i3 = vi+3;

        float UVX = UVStart + EdgeSection.UVs[vi].X * UVRange;

        EdgeSection.UVs[i0].X = UVX;
        EdgeSection.UVs[i1].X = UVX;
        EdgeSection.UVs[i2].X = UVX;
        EdgeSection.UVs[i3].X = UVX;
    }
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
