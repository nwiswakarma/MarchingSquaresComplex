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
    if (bGenerateExtrusion)
    {
        EdgePairs.Emplace(a, b);

#if 0
        int32 TailIndex = -1;
        int32 HeadIndex = -1;

        for (int32 i=0; i<EdgeLists.Num(); ++i)
        {
            FEdgeListData& EdgeListData(EdgeLists[i]);
            FEdgeList& EdgeList(EdgeListData.EdgeList);

            if (EdgeListData.bIsCircular)
            {
                continue;
            }

            const int32& Head(EdgeList.GetHead()->GetValue());
            const int32& Tail(EdgeList.GetTail()->GetValue());

            // If list tail equals point 0, add point 1 as list tail
            if (TailIndex < 0 && a == Tail)
            {
                EdgeList.AddTail(b);
                TailIndex = i;
            }

            // If list head equals point 1, add point 0 as list head
            if (HeadIndex < 0 && b == Head)
            {
                EdgeList.AddHead(a);
                HeadIndex = i;
            }

            // Both edge pair points have connection, stop iteration
            if (TailIndex >= 0 && HeadIndex >= 0)
            {
                break;
            }
        }

        bool bLinkedTail = (TailIndex >= 0);
        bool bLinkedHead = (HeadIndex >= 0);

        // Connected list, add head list to tail list
        if (bLinkedTail && bLinkedHead)
        {
            check(! EdgeLists[TailIndex].bIsCircular);
            check(! EdgeLists[HeadIndex].bIsCircular);

            if (TailIndex != HeadIndex)
            {
                FEdgeList& TailList(EdgeLists[TailIndex].EdgeList);
                FEdgeList& HeadList(EdgeLists[HeadIndex].EdgeList);

                auto* HeadNode = HeadList.GetHead();

                for (int32 SafeIt=0; (HeadNode && SafeIt<50); ++SafeIt)
                {
                    UE_LOG(LogTemp,Warning, TEXT("Head[%d]: %d"), HeadIndex, HeadNode->GetValue());
                    TailList.AddTail(HeadNode->GetValue());
                    HeadNode = HeadNode->GetNextNode();
                }

                EdgeLists.RemoveAtSwap(HeadIndex, 1, false);
            }
            else
            {
                EdgeLists[TailIndex].bIsCircular = true;
            }
        }
        // No link found, create new list
        else
        if (!bLinkedTail && !bLinkedHead)
        {
            EdgeLists.SetNum(EdgeLists.Num()+1);
            FEdgeList& EdgeList(EdgeLists.Last().EdgeList);
            EdgeList.AddTail(a);
            EdgeList.AddTail(b);
        }

#if 0
        UE_LOG(LogTemp,Warning, TEXT("EdgeLists.Num(): %d"), EdgeLists.Num());

        for (int32 i=0; i<EdgeLists.Num(); ++i)
        {
            const FEdgeList& EdgeList(EdgeLists[i]);
            UE_LOG(LogTemp,Warning, TEXT("EdgeLists[%d] Head: %d"), i, EdgeList.GetHead()->GetValue());
            UE_LOG(LogTemp,Warning, TEXT("EdgeLists[%d] Tail: %d"), i, EdgeList.GetTail()->GetValue());
            UE_LOG(LogTemp,Warning, TEXT("EdgeLists[%d].Num(): %d"), i, EdgeList.Num());
        }
#endif

#endif
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
    // Only generate edge geometry on surface that generate extrusion
    if (! bGenerateExtrusion)
    {
        return;
    }

    TArray<uint32>& IndexBuffer(EdgeSection.Indices);

#if 0
    // Generate edge tangents and faces

    for (const FEdgePair& EdgePair : EdgePairs)
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
#else

    TArray<FEdgePair> EdgePairsCopy(EdgePairs);

    while (EdgePairsCopy.Num() > 0)
    {
        EdgeLists.SetNum(EdgeLists.Num()+1);
        FEdgeList& EdgeList(EdgeLists.Last().EdgeList);

        EdgeList.AddTail(EdgePairsCopy[0].Get<0>());
        EdgeList.AddTail(EdgePairsCopy[0].Get<1>());

        EdgePairsCopy.RemoveAtSwap(0, 1, false);

        int32 i = 0;

        while (i < EdgePairsCopy.Num())
        {
            int32 HeadIndex = EdgeList.GetHead()->GetValue();
            int32 TailIndex = EdgeList.GetTail()->GetValue();

            const FEdgePair& EdgePair(EdgePairsCopy[i]);
            int32 a = EdgePair.Get<0>();
            int32 b = EdgePair.Get<1>();

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
                EdgePairsCopy.RemoveAtSwap(i, 1, false);
                i = 0;
                continue;
            }

            ++i;
        }
    }

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
        int32 vi = Node0->GetValue();
        int32 es0;
        int32 ee0;
        int32 es1 = DuplicateVertex(EdgeSection, SurfaceSection, vi);
        int32 ee1 = DuplicateVertex(EdgeSection, ExtrudeSection, vi);
        EdgeDistances.Emplace(es1, 0.f);

        // Calculate edge distance data
        do
        {
            vi = Node1->GetValue();

            es0 = es1;
            ee0 = ee1;

            es1 = DuplicateVertex(EdgeSection, SurfaceSection, vi);
            ee1 = DuplicateVertex(EdgeSection, ExtrudeSection, vi);

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
                IndexBuffer.Emplace(es1);
                IndexBuffer.Emplace(es0);
                IndexBuffer.Emplace(ee0);
                IndexBuffer.Emplace(ee1);
                IndexBuffer.Emplace(es0);

                // Assign edge tangents

                const FVector EdgeTangent(E01.GetSafeNormal(), 0.f);
                const FVector EdgeNormal(-EdgeTangent.Y, EdgeTangent.X, 0.f);

                EdgeSection.TangentsX[es0] += EdgeTangent;
                EdgeSection.TangentsX[es1] += EdgeTangent;

                EdgeSection.TangentsZ[es0] += EdgeNormal;
                EdgeSection.TangentsZ[es1] += EdgeNormal;
            }

            Node0 = Node1;
            Node1 = Node1->GetNextNode();
        }
        while (Node1);

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

            int32 i0 = EdgeData.Key;
            int32 i1 = i0 + 1;

            int32 it0 = i0*2;
            int32 it1 = i1*2;

            float UVX = EdgeData.Value * EdgeLengthInv;

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

            EdgeSection.UVs[i0].Set(UVX, 0.f);
            EdgeSection.UVs[i1].Set(UVX, 1.f);
        }

        // Create edge list sync data

        EdgeSyncList.SetNum(EdgeSyncList.Num()+1);
        FEdgeSyncData& SyncData(EdgeSyncList.Last());

        int32 evi0 = EdgeDistances[0].Key;
        int32 evi1 = EdgeDistances.Last().Key;

        SyncData.EdgeListIndex = ListId;
        SyncData.HeadPos = FVector2D(EdgeSection.Positions[evi0]);
        SyncData.TailPos = FVector2D(EdgeSection.Positions[evi1]);
        SyncData.HeadIndex = evi0 / 2;
        SyncData.TailIndex = evi1 / 2;
        SyncData.Length = EdgeLength;
    }

#if 0
    UE_LOG(LogTemp,Warning, TEXT("EdgeLists.Num(): %d"), EdgeLists.Num());
    for (FEdgeListData& EdgeListData : EdgeLists)
    {
        FEdgeList& EdgeList(EdgeListData.EdgeList);

        UE_LOG(LogTemp,Warning, TEXT("bIsCircular: %d"),
            EdgeListData.bIsCircular);

        check(EdgeList.Num() >= 2);

        auto* HeadNode = EdgeList.GetHead();
        auto* Node0 = HeadNode;
        auto* Node1 = Node0->GetNextNode();

        int32 PairIndex0;
        int32 PairIndex1;
        int32 VertexIndex0;
        int32 VertexIndex1;

        TArray<TPair<int32, float>> EdgeDistances;
        float EdgeLength = 0.f;

        EdgeDistances.Reserve(EdgeList.Num());

        // Calculate edge distance data
        int32 SafeIt = 0;
        do
        {
            PairIndex0 = Node0->GetValue();
            PairIndex1 = Node1->GetValue();
            VertexIndex0 = EdgeIndexMap.FindChecked(PairIndex0);
            VertexIndex1 = EdgeIndexMap.FindChecked(PairIndex1);

            FVector2D v0(EdgeSection.Positions[VertexIndex0]);
            FVector2D v1(EdgeSection.Positions[VertexIndex1]);

            EdgeDistances.Emplace(VertexIndex0, EdgeLength);
            EdgeLength += (v1-v0).Size();

            Node0 = Node1;
            Node1 = Node1->GetNextNode();

            ++SafeIt;
        }
        while (Node1 && SafeIt<50);

        // Assign last edge vertex distance data
        EdgeDistances.Emplace(VertexIndex1, EdgeLength);

        // Calculate inverse edge length

        float EdgeLengthInv = 0.f;

        if (EdgeLength > 0.f)
        {
            EdgeLengthInv = 1.f/EdgeLength;
        }

        // Assign edge uvs
        for (int32 i=0; i<EdgeDistances.Num(); ++i)
        {
            auto& EdgeData(EdgeDistances[i]);

            int32 ea0 = EdgeData.Key;
            int32 ea1 = ea0 + 1;

            float Dist = EdgeData.Value * EdgeLengthInv;

            EdgeSection.UVs[ea0].Set(Dist, 0.f);
            EdgeSection.UVs[ea1].Set(Dist, 1.f);

            //FVector2D Pos(EdgeSection.Positions[ea0]);
            //UE_LOG(LogTemp,Warning, TEXT("EdgeDistances[%d]: %f (%f) %s"),
            //    i, Dist, Dist/EdgeLengthInv, *Pos.ToString());
        }
    }
#endif
#endif
}

void FMQCGridSurface::RemapEdgeUVs(int32 EdgeListId, float UVStart, float UVEnd)
{
    const FEdgeSyncData& SyncData(EdgeSyncList[EdgeListId]);
    int32 HeadIndex = SyncData.HeadIndex;
    int32 TailIndex = SyncData.TailIndex;
    float UVRange = UVEnd-UVStart;

    for (int32 i=HeadIndex; i<=TailIndex; ++i)
    {
        int32 i0 = i * 2;
        int32 i1 = i0 + 1;

        float UVX = UVStart + EdgeSection.UVs[i0].X * UVRange;

        EdgeSection.UVs[i0].Set(UVX, 0.f);
        EdgeSection.UVs[i1].Set(UVX, 1.f);
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
