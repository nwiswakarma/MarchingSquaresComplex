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
    MaterialType = Config.MaterialType;

    // Resize vertex cache containers
    
    cornersMin.SetNum(VoxelResolution + 1);
    cornersMax.SetNum(VoxelResolution + 1);

    xEdgesMin.SetNum(VoxelResolution);
    xEdgesMax.SetNum(VoxelResolution);

    // Reserves geometry container spaces

    ReserveGeometry();
}

void FMQCGridSurface::ReserveGeometry()
{
    if (bGenerateExtrusion)
    {
        ReserveGeometry(SurfaceMeshData);
        ReserveGeometry(ExtrudeMeshData);
    }
    else
    if (bExtrusionSurface)
    {
        ReserveGeometry(ExtrudeMeshData);
    }
    else
    {
        ReserveGeometry(SurfaceMeshData);
    }
}

void FMQCGridSurface::CompactGeometry()
{
    CompactGeometry(SurfaceMeshData);
    CompactGeometry(ExtrudeMeshData);
    CompactGeometry(EdgeMeshData);
}

void FMQCGridSurface::ReserveGeometry(FMeshData& MeshData)
{
    MeshData.Section.Positions.Reserve(VoxelCount);
    MeshData.Section.UVs.Reserve(VoxelCount);
    MeshData.Section.Colors.Reserve(VoxelCount);
    MeshData.Section.Tangents.Reserve(VoxelCount*2);
    MeshData.Section.Indices.Reserve(VoxelCount * 6);
    MeshData.Materials.Reserve(VoxelCount);
}

void FMQCGridSurface::CompactGeometry(FMeshData& MeshData)
{
    MeshData.Section.Positions.Shrink();
    MeshData.Section.UVs.Shrink();
    MeshData.Section.Colors.Shrink();
    MeshData.Section.Tangents.Shrink();
    MeshData.Section.Indices.Shrink();
    MeshData.Materials.Shrink();
}

void FMQCGridSurface::Finalize()
{
    if (bGenerateExtrusion)
    {
        GenerateEdgeGeometry();
    }

#if 0
    for (auto& Pair : MaterialSectionMap)
    {
        FMQCMaterialBlend Id(Pair.Get<0>());

        if (! Id.IsTriple())
        {
            continue;
        }

        FIndexMap& IndexMap012(MaterialIndexMaps.FindChecked(Id));
        FPMUMeshSection& Section012(Pair.Get<1>());

        FMQCMaterialBlend Id01(Id.Index0, Id.Index1);
        FIndexMap* IndexMap01(MaterialIndexMaps.Find(Id01));
        FPMUMeshSection* Section01(MaterialSectionMap.Find(Id01));

        FMQCMaterialBlend Id02(Id.Index0, Id.Index2);
        FIndexMap* IndexMap02(MaterialIndexMaps.Find(Id02));
        FPMUMeshSection* Section02(MaterialSectionMap.Find(Id02));

        FMQCMaterialBlend Id12(Id.Index1, Id.Index2);
        FIndexMap* IndexMap12(MaterialIndexMaps.Find(Id12));
        FPMUMeshSection* Section12(MaterialSectionMap.Find(Id12));

        for (const auto& IndexPair012 : IndexMap012)
        {
            const int32 VId = IndexPair012.Key;
            FColor& Color(Section012.Colors[IndexPair012.Value]);

            if (IndexMap01)
            {
                if (int32* i01 = IndexMap01->Find(VId))
                {
                    Color.R = Section01->Colors[*i01].R;
                    Color.G = 0;
                }
            }
            if (IndexMap02)
            {
                if (int32* i02 = IndexMap02->Find(VId))
                {
                    Color.R = 0;
                    Color.G = Section02->Colors[*i02].R;
                }
            }
            if (IndexMap12)
            {
                if (int32* i12 = IndexMap12->Find(VId))
                {
                    Color.R = 255;
                    Color.G = Section12->Colors[*i12].R;
                }
            }
        }
    }
#endif

    CompactGeometry();
}

void FMQCGridSurface::Clear()
{
    GetSurfaceSection().Reset();
    GetExtrudeSection().Reset();
    GetEdgeSection().Reset();
}

void FMQCGridSurface::GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const
{
    for (const auto& MaterialSectionPair : MaterialSectionMap)
    {
        MaterialSet.Emplace(MaterialSectionPair.Key);
    }
}

void FMQCGridSurface::AddVertex(const FVector2D& Vertex, const FMQCMaterial& Material, bool bIsExtrusion)
{
    FVector2D XY((position+Vertex)-.5f);
    FVector2D UV(XY*MapSizeInv - MapSizeInv*.5f);

    float Height;
    float FaceSign;

    FMeshData* TargetMeshData;

    if (bIsExtrusion)
    {
        TargetMeshData = &ExtrudeMeshData;
        Height = ExtrusionHeight;
        FaceSign = -1.f;
    }
    else
    {
        TargetMeshData = &SurfaceMeshData;
        Height = 0.f;
        FaceSign = 1.f;
    }

    FPMUMeshSection& Section(TargetMeshData->Section);
    FVector Pos(XY, Height);
    FPackedNormal TangentX(FVector(1,0,0));
    FPackedNormal TangentZ(FVector4(0,0,FaceSign,FaceSign));

    // Assign vertex and bounds

    Section.Positions.Emplace(Pos);
    Section.UVs.Emplace(UV);
    Section.Colors.Emplace(Material.ToFColor());
    Section.Tangents.Emplace(TangentX.Vector.Packed);
    Section.Tangents.Emplace(TangentZ.Vector.Packed);
    Section.SectionLocalBox += Pos;

    TargetMeshData->Materials.Emplace(Material);
}

void FMQCGridSurface::AddEdgeFace(int32 a, int32 b)
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

void FMQCGridSurface::AddMaterialFace(int32 a, int32 b, int32 c)
{
    const FMeshData& SrcMeshData(SurfaceMeshData);

    check(SrcMeshData.Materials.IsValidIndex(a));
    check(SrcMeshData.Materials.IsValidIndex(b));
    check(SrcMeshData.Materials.IsValidIndex(c));

    FMQCMaterial Materials[3];
    Materials[0] = SrcMeshData.Materials[a];
    Materials[1] = SrcMeshData.Materials[b];
    Materials[2] = SrcMeshData.Materials[c];

    uint8 Blends0[3];
    uint8 Blends1[3];
    uint8 Blends2[3];

    FMQCMaterialBlend Material;

    UMQCMaterialUtility::FindTripleIndexFaceBlend(
        Materials,
        Material,
        Blends0,
        Blends1,
        Blends2
        );

    struct FVertexHelper
    {
        const FPMUMeshSection& SrcSection;
        FPMUMeshSection& DstSection;
        FIndexMap& IndexMap;

        FVertexHelper(
            const FPMUMeshSection& SrcSection,
            FPMUMeshSection& DstSection,
            FIndexMap& IndexMap
            )
            : SrcSection(SrcSection)
            , DstSection(DstSection)
            , IndexMap(IndexMap)
        {
        }

        inline void AddVertex(
            const int32 Index,
            const uint8 Blend0,
            const uint8 Blend1,
            const uint8 Blend2
            )
        {
            int32 MappedIndex;

            if (int32* MappedIndexPtr = IndexMap.Find(Index))
            {
                MappedIndex = *MappedIndexPtr;
            }
            else
            {
                MappedIndex = FMQCGridSurface::DuplicateVertex(
                    SrcSection,
                    DstSection,
                    Index
                    );

                // Assign blend value
                DstSection.Colors[MappedIndex].R = Blend0;
                DstSection.Colors[MappedIndex].G = Blend1;
                DstSection.Colors[MappedIndex].B = Blend2;

                // Map duplicated vertex index
                IndexMap.Add(Index, MappedIndex);
            }

            DstSection.Indices.Emplace(MappedIndex);
        }
    };

    FPMUMeshSection& DstSection(MaterialSectionMap.FindOrAdd(Material));
    FIndexMap& IndexMap(MaterialIndexMaps.FindOrAdd(Material));

    FVertexHelper VertexHelper(SrcMeshData.Section, DstSection, IndexMap);
    VertexHelper.AddVertex(a, Blends0[0], Blends1[0], Blends2[0]);
    VertexHelper.AddVertex(b, Blends0[1], Blends1[1], Blends2[1]);
    VertexHelper.AddVertex(c, Blends0[2], Blends1[2], Blends2[2]);
}

int32 FMQCGridSurface::DuplicateVertex(
    const FMeshData& SrcMeshData,
    FMeshData& DstMeshData,
    int32 VertexIndex
    )
{
    const FPMUMeshSection& SrcSection(SrcMeshData.Section);
    FPMUMeshSection& DstSection(DstMeshData.Section);

    // Duplicate section geometry
    int32 OutIndex = DuplicateVertex(SrcSection, DstSection, VertexIndex);

    // Duplicate voxel material
    DstMeshData.Materials.Emplace(SrcMeshData.Materials[VertexIndex]);

    return OutIndex;
}

int32 FMQCGridSurface::DuplicateVertex(
    const FPMUMeshSection& SrcSection,
    FPMUMeshSection& DstSection,
    int32 VertexIndex
    )
{
    check(SrcSection.Positions.IsValidIndex(VertexIndex));

    int32 OutIndex = DstSection.Positions.Num();

    DstSection.Positions.Emplace(SrcSection.Positions[VertexIndex]);
    DstSection.UVs.Emplace(SrcSection.UVs[VertexIndex]);
    DstSection.Colors.Emplace(SrcSection.Colors[VertexIndex]);
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

    int32 esi = DuplicateVertex(ExtrudeMeshData, EdgeMeshData, SourceIndex);
    int32 eai = DuplicateVertex(ExtrudeMeshData, EdgeMeshData, SourceIndex);
    int32 ebi = DuplicateVertex(ExtrudeMeshData, EdgeMeshData, SourceIndex);
    int32 eei = DuplicateVertex(ExtrudeMeshData, EdgeMeshData, SourceIndex);

    FPMUMeshSection& EdgeSection(GetEdgeSection());

    const float Z1 = EdgeSection.Positions[eei].Z;
    EdgeSection.Positions[esi].Z = 0.f;
    EdgeSection.Positions[eai].Z = Z1*.33333f;
    EdgeSection.Positions[ebi].Z = Z1*.66667f;

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

    TArray<uint32>& IndexBuffer(GetEdgeSection().Indices);

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

    FVector2D v0(GetEdgeSection().Positions[es0]);
    FVector2D v1(GetEdgeSection().Positions[es1]);
    FVector2D E01 = v1 - v0;

    return E01.Size();
}

void FMQCGridSurface::GenerateEdgeSyncData(int32 EdgeListId, const TArray<FEdgeSegment>& EdgeSegments)
{
    check(EdgeLists.IsValidIndex(EdgeListId));
    check(EdgeSegments.Num() > 0);

    FPMUMeshSection& EdgeSection(GetEdgeSection());

    float EdgeLength = EdgeSegments.Last().Value;

    // Calculate inverse edge length

    float EdgeLengthInv = 0.f;

    if (EdgeLength > 0.f)
    {
        EdgeLengthInv = 1.f/EdgeLength;
    }

    // Assign edge uvs and normalized tangents
    for (int32 i=0; i<EdgeSegments.Num(); ++i)
    {
        const FEdgeSegment& SegmentData(EdgeSegments[i]);

        int32 vi = SegmentData.Key;

        int32 i0 = vi;
        int32 i1 = vi+1;
        int32 i2 = vi+2;
        int32 i3 = vi+3;

        int32 it0 = i0*2;
        int32 it1 = i1*2;
        int32 it2 = i2*2;
        int32 it3 = i3*2;

        float UVX = SegmentData.Value * EdgeLengthInv;

        EdgeSection.UVs[i0].Set(UVX, 0.f);
        EdgeSection.UVs[i1].Set(UVX, 0.33333f);
        EdgeSection.UVs[i2].Set(UVX, 0.66667f);
        EdgeSection.UVs[i3].Set(UVX, 1.f);
    }

    // Create edge list sync data

    EdgeSyncList.SetNum(EdgeSyncList.Num()+1);
    FEdgeSyncData& SyncData(EdgeSyncList.Last());

    int32 evi0 = EdgeSegments[0].Key;
    int32 evi1 = EdgeSegments.Last().Key;

    SyncData.EdgeListIndex = EdgeListId;
    SyncData.HeadPos = FVector2D(EdgeSection.Positions[evi0]);
    SyncData.TailPos = FVector2D(EdgeSection.Positions[evi1]);
    SyncData.HeadIndex = evi0;
    SyncData.TailIndex = evi1;
    SyncData.Length = EdgeLength;
}

void FMQCGridSurface::GenerateEdgeGeometry()
{
    // Only generate edge geometry on surface that generate extrusion
    if (! bGenerateExtrusion)
    {
        return;
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

        int32 nvi = Node0->GetValue();
        TArray<int32> EdgeIndices0;
        TArray<int32> EdgeIndices1;

        GenerateEdgeVertex(EdgeIndices1, nvi);

        TArray<FEdgeSegment> EdgeSegments;
        float EdgeLength = 0.f;

        EdgeSegments.Reserve(EdgeList.Num()+1);
        EdgeSegments.Emplace(EdgeIndices1[0], 0.f);

        // Calculate edge distance data
        do
        {
            nvi = Node1->GetValue();

            EdgeIndices0 = EdgeIndices1;

            GenerateEdgeVertex(EdgeIndices1, nvi);
            EdgeLength += GenerateEdgeSegment(EdgeIndices0, EdgeIndices1);
            EdgeSegments.Emplace(EdgeIndices1[0], EdgeLength);

            Node0 = Node1;
            Node1 = Node1->GetNextNode();
        }
        while (Node1);

        if (bRemapEdgeUVs)
        {
            GenerateEdgeSyncData(ListId, EdgeSegments);
        }
    }
}

void FMQCGridSurface::RemapEdgeUVs(int32 EdgeListId, float UVStart, float UVEnd)
{
    if (bRemapEdgeUVs)
    {
        FPMUMeshSection& EdgeSection(GetEdgeSection());
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
}

void FMQCGridSurface::AddTriangle(int32 a, int32 b, int32 c)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        TArray<uint32>& IndexBuffer(GetExtrudeSection().Indices);
        IndexBuffer.Emplace(c);
        IndexBuffer.Emplace(b);
        IndexBuffer.Emplace(a);
    }
    else
    {
        // Generate surface
        {
            TArray<uint32>& IndexBuffer(GetSurfaceSection().Indices);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
            AddMaterialFace(a, b, c);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            TArray<uint32>& IndexBuffer(GetExtrudeSection().Indices);
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
        TArray<uint32>& IndexBuffer(GetExtrudeSection().Indices);
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
            TArray<uint32>& IndexBuffer(GetSurfaceSection().Indices);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(d);
            AddMaterialFace(a, b, c);
            AddMaterialFace(a, c, d);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            TArray<uint32>& IndexBuffer(GetExtrudeSection().Indices);
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
        TArray<uint32>& IndexBuffer(GetExtrudeSection().Indices);
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
            TArray<uint32>& IndexBuffer(GetSurfaceSection().Indices);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(b);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(c);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(a);
            IndexBuffer.Emplace(d);
            IndexBuffer.Emplace(e);
            AddMaterialFace(a, b, c);
            AddMaterialFace(a, c, d);
            AddMaterialFace(a, d, e);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            TArray<uint32>& IndexBuffer(GetExtrudeSection().Indices);
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
        TArray<uint32>& IndexBuffer(GetExtrudeSection().Indices);
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
            TArray<uint32>& IndexBuffer(GetSurfaceSection().Indices);
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
            AddMaterialFace(a, b, c);
            AddMaterialFace(a, c, d);
            AddMaterialFace(a, d, e);
            AddMaterialFace(a, e, f);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            TArray<uint32>& IndexBuffer(GetExtrudeSection().Indices);
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
