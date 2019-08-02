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
#include "MQCMaterialUtility.h"

void FMQCGridSurface::Configure(const FMQCSurfaceConfig& Config)
{
    // Position & dimension configuration

    VoxelResolution = Config.VoxelResolution;
    VoxelCount = VoxelResolution * VoxelResolution;
    bRequireHash16 = Config.MapSize > 256;
    MapSize = Config.MapSize-1;
    MapSizeInv = MapSize > 0.f ? (1.f/MapSize) : KINDA_SMALL_NUMBER;
    Position = Config.Position;

    // Extrusion configuration

    bGenerateExtrusion = Config.bGenerateExtrusion;
    bExtrusionSurface  = !bGenerateExtrusion && Config.bExtrusionSurface;
    ExtrusionHeight = (FMath::Abs(Config.ExtrusionHeight) > .01f) ? -FMath::Abs(Config.ExtrusionHeight) : -1.f;

    bRemapEdgeUVs = Config.bRemapEdgeUVs;
    MaterialType = Config.MaterialType;
}

void FMQCGridSurface::Initialize()
{
    Clear();
    ReserveGeometry();
}

void FMQCGridSurface::ReserveGeometry()
{
    // Reserve triangulation data containers

    VertexMap.Reserve(VoxelCount*2);
    EdgeMap.Reserve(VoxelResolution*2);

    cornersMinArr.SetNumZeroed(VoxelResolution + 1);
    cornersMaxArr.SetNumZeroed(VoxelResolution + 1);

    xEdgesMinArr.SetNumZeroed(VoxelResolution);
    xEdgesMaxArr.SetNumZeroed(VoxelResolution);

    cornersMin = cornersMinArr.GetData();
    cornersMax = cornersMaxArr.GetData();

    xEdgesMin = xEdgesMinArr.GetData();
    xEdgesMax = xEdgesMaxArr.GetData();

    // Reserve mesh data container

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
    // Shrink vertex map container
    VertexMap.Shrink();
    EdgeMap.Shrink();
    // Shrink mesh data container
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
    MeshData.Section.Indices.Reserve(VoxelCount*6);
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

    CompactGeometry();
}

void FMQCGridSurface::Clear()
{
    // Clear triangulation data
    VertexMap.Empty();
    EdgeMap.Empty();
    cornersMinArr.Empty();
    cornersMaxArr.Empty();
    xEdgesMinArr.Empty();
    xEdgesMaxArr.Empty();
    // Clear geometry data
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
    FVector2D XY((FVector2D(Position)+Vertex));
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

    //UE_LOG(LogTemp,Warning, TEXT("VERT: %s"), *XY.ToString());
}

void FMQCGridSurface::AddEdge(uint32 a, uint32 b)
{
    if ((a == b) || ! bGenerateExtrusion)
    {
        return;
    }

    FEdgeLinkList* Connection = nullptr;

    for (FEdgeLinkList& LinkList : EdgeLinkLists)
    {
        check(! LinkList.IsEmpty());

        if (LinkList.Connect(a, b))
        {
            Connection = &LinkList;
            break;
        }
    }

    // Connection not found, create new edge list
    if (! Connection)
    {
        FEdgeLinkList* LinkList = new FEdgeLinkList;
        LinkList->AddTail(a);
        LinkList->AddTail(b);
        EdgeLinkLists.Add(LinkList);
    }
    // Connection found, merge any edge list connected to the new edge
    else
    {
        check(Connection->Num() > 0);

        int32 It = 0;

        // Iterate existing edge lists
        while (It < EdgeLinkLists.Num())
        {
            FEdgeLinkList& LinkList(EdgeLinkLists[It]);

            if (Connection != &LinkList)
            {
                check(LinkList.Num() > 0);

                // Attempt to merge target list
                bool bMerged = Connection->Merge(LinkList);

                // Merge successful, remove target list and restart iteration
                // to look for another possible connection
                if (bMerged)
                {
                    EdgeLinkLists.RemoveAtSwap(It, 1, false);
                    It = 0;
                    continue;
                }
            }

            ++It;
        }
    }
}

void FMQCGridSurface::AddMaterialFace(uint32 a, uint32 b, uint32 c)
{
    // Skip non index based material
    if (MaterialType != EMQCMaterialType::MT_TRIPLE_INDEX)
    {
        return;
    }

    // ONLY ALLOW TRIPLE INDEX, DOUBLE INDEX NOT IMPLEMENTED YET!
    check(MaterialType == EMQCMaterialType::MT_TRIPLE_INDEX);

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
            const uint32 Index,
            const uint8 Blend0,
            const uint8 Blend1,
            const uint8 Blend2
            )
        {
            uint32 MappedIndex;

            if (uint32* MappedIndexPtr = IndexMap.Find(Index))
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

uint32 FMQCGridSurface::DuplicateVertex(
    const FMeshData& SrcMeshData,
    FMeshData& DstMeshData,
    uint32 VertexIndex
    )
{
    const FPMUMeshSection& SrcSection(SrcMeshData.Section);
    FPMUMeshSection& DstSection(DstMeshData.Section);

    // Duplicate section geometry
    uint32 OutIndex = DuplicateVertex(SrcSection, DstSection, VertexIndex);

    // Duplicate voxel material
    DstMeshData.Materials.Emplace(SrcMeshData.Materials[VertexIndex]);

    return OutIndex;
}

uint32 FMQCGridSurface::DuplicateVertex(
    const FPMUMeshSection& SrcSection,
    FPMUMeshSection& DstSection,
    uint32 VertexIndex
    )
{
    check(SrcSection.Positions.IsValidIndex(VertexIndex));

    uint32 OutIndex = DstSection.Positions.Num();

    DstSection.Positions.Emplace(SrcSection.Positions[VertexIndex]);
    DstSection.UVs.Emplace(SrcSection.UVs[VertexIndex]);
    DstSection.Colors.Emplace(SrcSection.Colors[VertexIndex]);
    DstSection.Tangents.Emplace(SrcSection.Tangents[(VertexIndex*2)  ]);
    DstSection.Tangents.Emplace(SrcSection.Tangents[(VertexIndex*2)+1]);
    DstSection.SectionLocalBox += SrcSection.Positions[VertexIndex];

    return OutIndex;
}

void FMQCGridSurface::GenerateEdgeVertex(TArray<uint32>& EdgeIndices, uint32 SourceIndex)
{
    if (EdgeIndices.Num() != 4)
    {
        EdgeIndices.SetNumUninitialized(4);
    }

    uint32 esi = DuplicateVertex(ExtrudeMeshData, EdgeMeshData, SourceIndex);
    uint32 eai = DuplicateVertex(ExtrudeMeshData, EdgeMeshData, SourceIndex);
    uint32 ebi = DuplicateVertex(ExtrudeMeshData, EdgeMeshData, SourceIndex);
    uint32 eei = DuplicateVertex(ExtrudeMeshData, EdgeMeshData, SourceIndex);

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

float FMQCGridSurface::GenerateEdgeSegment(TArray<uint32>& EdgeIndices0, TArray<uint32>& EdgeIndices1)
{
    uint32 es0 = EdgeIndices0[0];
    uint32 ea0 = EdgeIndices0[1];
    uint32 eb0 = EdgeIndices0[2];
    uint32 ee0 = EdgeIndices0[3];

    uint32 es1 = EdgeIndices1[0];
    uint32 ea1 = EdgeIndices1[1];
    uint32 eb1 = EdgeIndices1[2];
    uint32 ee1 = EdgeIndices1[3];

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

void FMQCGridSurface::GenerateEdgeGeometry()
{
    // Only generate edge geometry on surface that generate extrusion
    if (! bGenerateExtrusion)
    {
        return;
    }

    EdgePointLists.Reset();
    EdgePointLists.SetNum(EdgeLinkLists.Num());

    for (int32 ListId=0; ListId<EdgeLinkLists.Num(); ++ListId)
    {
        FEdgeLinkList& EdgeList(EdgeLinkLists[ListId]);
        const int32 EdgeCount = EdgeList.Num();

        check(EdgeCount >= 2);

        if (EdgeCount < 2)
        {
            continue;
        }

        FEdgeLink* EdgeNode0 = EdgeList.Head;
        FEdgeLink* EdgeNode1 = EdgeNode0->Next;
        uint32 VertexIndex0 = -1;
        uint32 VertexIndex1 = -1;

        float TotalLength = 0.f;

        FEdgePointList& EdgePoints(EdgePointLists[ListId]);
        EdgePoints.Reset(EdgeCount+1);

        // Generate edge segments
        while (EdgeNode1)
        {
            // Get edge node vertex indices

            VertexIndex0 = EdgeNode0->Value;
            VertexIndex1 = EdgeNode1->Value;

            // Calculate edge direction and length

            const FVector2D EdgePos0 = GetPositionByIndex(VertexIndex0);
            const FVector2D EdgePos1 = GetPositionByIndex(VertexIndex1);
            const FVector2D Edge01 = EdgePos1 - EdgePos0;

            FVector2D Normal;
            float Length;

            Edge01.ToDirectionAndLength(Normal, Length);

            check(Length > 0.f);

            // Generate edge segment data

            FEdgePoint EdgePoint = { VertexIndex0, Normal, TotalLength };
            EdgePoints.Emplace(EdgePoint);

            // Accumulate total edge length

            TotalLength += Length;

            // Advance node iteration

            EdgeNode0 = EdgeNode1;
            EdgeNode1 = EdgeNode0->Next;
        }

        // Add edge segment end point
        {
            VertexIndex0 = EdgeNode0->Value;

            FVector2D Normal = (EdgeNode0->Value == EdgeList.Head->Value)
                ? EdgePoints[0].Normal
                : FVector2D(1.f,0.f);

            FEdgePoint EdgePoint = { VertexIndex0, Normal, TotalLength };
            EdgePoints.Emplace(EdgePoint);
        }

        // Generate edge connection data
        GenerateEdgeSyncData(ListId);
    }
}

void FMQCGridSurface::GenerateEdgeSyncData(int32 EdgeListId)
{
    check(EdgePointLists.IsValidIndex(EdgeListId));

    const FEdgePointList& EdgePoints(EdgePointLists[EdgeListId]);

#if 0
    FPMUMeshSection& EdgeSection(GetEdgeSection());

    //float EdgeLength = EdgeSegments.Last().Get<2>();
    float EdgeLength = EdgeSegments.Last().Length;

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

        //uint32 vi = SegmentData.Get<0>();
        uint32 vi = SegmentData.VertexIndex;

        uint32 i0 = vi;
        uint32 i1 = vi+1;
        uint32 i2 = vi+2;
        uint32 i3 = vi+3;

        uint32 it0 = i0*2;
        uint32 it1 = i1*2;
        uint32 it2 = i2*2;
        uint32 it3 = i3*2;

        //float UVX = SegmentData.Get<2>() * EdgeLengthInv;
        float UVX = SegmentData.Length * EdgeLengthInv;

        EdgeSection.UVs[i0].Set(UVX, 0.f);
        EdgeSection.UVs[i1].Set(UVX, 0.33333f);
        EdgeSection.UVs[i2].Set(UVX, 0.66667f);
        EdgeSection.UVs[i3].Set(UVX, 1.f);
    }

    // Create edge list sync data

    {
        EdgeSyncList.SetNum(EdgeSyncList.Num()+1);
        FMQCEdgeSyncData& SyncData(EdgeSyncList.Last());

        const FEdgeSegment& Segment0(EdgeSegments[0]);
        const FEdgeSegment& SegmentN(EdgeSegments.Last());

        SyncData.EdgeListIndex = EdgeListId;
        //SyncData.HeadHash = EdgeMap.FindChecked(Segment0.Get<1>());
        //SyncData.TailHash = EdgeMap.FindChecked(SegmentN.Get<1>());
        SyncData.HeadHash = EdgeMap.FindChecked(Segment0.VertexIndex);
        SyncData.TailHash = EdgeMap.FindChecked(SegmentN.VertexIndex);
        //SyncData.HeadIndex = Segment0.Get<0>();
        //SyncData.TailIndex = SegmentN.Get<0>();
        SyncData.Length = EdgeLength;
    }
#endif

    // Create edge list sync data

    {
        EdgeSyncList.SetNum(EdgeSyncList.Num()+1);
        FMQCEdgeSyncData& SyncData(EdgeSyncList.Last());

        const FEdgePoint& Point0(EdgePoints[0]);
        const FEdgePoint& PointN(EdgePoints.Last());

        SyncData.EdgeListIndex = EdgeListId;
        SyncData.HeadHash = EdgeMap.FindChecked(Point0.VertexIndex);
        SyncData.TailHash = EdgeMap.FindChecked(PointN.VertexIndex);
        SyncData.Length = PointN.Distance;
    }
}

void FMQCGridSurface::RemapEdgeUVs(int32 EdgeListId, float UVStart, float UVEnd)
{
#if 0
    if (bRemapEdgeUVs)
    {
        FPMUMeshSection& EdgeSection(GetEdgeSection());
        const FMQCEdgeSyncData& SyncData(EdgeSyncList[EdgeListId]);
        uint32 HeadIndex = SyncData.HeadIndex / 4;
        uint32 TailIndex = SyncData.TailIndex / 4;
        float UVRange = UVEnd-UVStart;

        for (uint32 i=HeadIndex; i<=TailIndex; ++i)
        {
            uint32 vi = i*4;

            uint32 i0 = vi;
            uint32 i1 = vi+1;
            uint32 i2 = vi+2;
            uint32 i3 = vi+3;

            float UVX = UVStart + EdgeSection.UVs[vi].X * UVRange;

            EdgeSection.UVs[i0].X = UVX;
            EdgeSection.UVs[i1].X = UVX;
            EdgeSection.UVs[i2].X = UVX;
            EdgeSection.UVs[i3].X = UVX;
        }
    }
#endif
}

void FMQCGridSurface::GetEdgePoint(FMQCEdgePoint& EdgePoint, const FEdgePoint& SourcePoint, float DistanceOffset) const
{
    EdgePoint.Position = GetPositionByIndex(SourcePoint.VertexIndex);
    EdgePoint.Normal = SourcePoint.Normal;
    EdgePoint.Distance = SourcePoint.Distance + DistanceOffset;
}

void FMQCGridSurface::GetConnectedEdgePoints(TArray<FMQCEdgePoint>& OutPoints, const FMQCEdgeSyncData& SyncData, float DistanceOffset) const
{
    check(EdgePointLists.IsValidIndex(SyncData.EdgeListIndex));

    const FEdgePointList& Points(EdgePointLists[SyncData.EdgeListIndex]);

    if (Points.Num() < 1)
    {
        return;
    }

    OutPoints.Reserve(OutPoints.Num()+Points.Num());

    if (OutPoints.Num() > 0)
    {
        GetEdgePoint(OutPoints.Last(), Points[0], DistanceOffset);
    }
    else
    {
        FMQCEdgePoint FirstPoint;
        GetEdgePoint(FirstPoint, Points[0], DistanceOffset);
        OutPoints.Emplace(FirstPoint);
    }

    for (int32 i=1; i<Points.Num(); ++i)
    {
        FMQCEdgePoint Point;
        GetEdgePoint(Point, Points[i], DistanceOffset);
        OutPoints.Emplace(Point);
    }
}

void FMQCGridSurface::AddTriangle(uint32 a, uint32 b, uint32 c)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        ExtrudeMeshData.AddFace(c, b, a);
    }
    else
    {
        // Generate surface
        {
            SurfaceMeshData.AddFace(a, b, c);
            AddMaterialFaceSafe(a, b, c);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            ExtrudeMeshData.AddFace(c, b, a);
        }
    }
}

void FMQCGridSurface::AddQuad(uint32 a, uint32 b, uint32 c, uint32 d)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        ExtrudeMeshData.AddFace(c, b, a);
        ExtrudeMeshData.AddFace(d, c, a);
    }
    else
    {
        // Generate surface
        {
            SurfaceMeshData.AddFace(a, b, c);
            SurfaceMeshData.AddFace(a, c, d);
            AddMaterialFaceSafe(a, b, c);
            AddMaterialFaceSafe(a, c, d);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            ExtrudeMeshData.AddFace(c, b, a);
            ExtrudeMeshData.AddFace(d, c, a);
        }
    }
}

void FMQCGridSurface::AddQuadCorners(uint32 a, uint32 b, uint32 c, uint32 d)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        ExtrudeMeshData.AddFaceNoCheck(c, b, a);
        ExtrudeMeshData.AddFaceNoCheck(d, c, a);
    }
    else
    {
        // Generate surface
        {
            SurfaceMeshData.AddFaceNoCheck(a, b, c);
            SurfaceMeshData.AddFaceNoCheck(a, c, d);
            AddMaterialFace(a, b, c);
            AddMaterialFace(a, c, d);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            ExtrudeMeshData.AddFaceNoCheck(c, b, a);
            ExtrudeMeshData.AddFaceNoCheck(d, c, a);
        }
    }
}

void FMQCGridSurface::AddPentagon(uint32 a, uint32 b, uint32 c, uint32 d, uint32 e)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        ExtrudeMeshData.AddFace(c, b, a);
        ExtrudeMeshData.AddFace(d, c, a);
        ExtrudeMeshData.AddFace(e, d, a);
    }
    else
    {
        // Generate surface
        {
            SurfaceMeshData.AddFace(a, b, c);
            SurfaceMeshData.AddFace(a, c, d);
            SurfaceMeshData.AddFace(a, d, e);
            AddMaterialFaceSafe(a, b, c);
            AddMaterialFaceSafe(a, c, d);
            AddMaterialFaceSafe(a, d, e);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            ExtrudeMeshData.AddFace(c, b, a);
            ExtrudeMeshData.AddFace(d, c, a);
            ExtrudeMeshData.AddFace(e, d, a);
        }
    }
}

void FMQCGridSurface::AddHexagon(uint32 a, uint32 b, uint32 c, uint32 d, uint32 e, uint32 f)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        ExtrudeMeshData.AddFace(c, b, a);
        ExtrudeMeshData.AddFace(d, c, a);
        ExtrudeMeshData.AddFace(e, d, a);
        ExtrudeMeshData.AddFace(f, e, a);
    }
    else
    {
        // Generate surface
        {
            SurfaceMeshData.AddFace(a, b, c);
            SurfaceMeshData.AddFace(a, c, d);
            SurfaceMeshData.AddFace(a, d, e);
            SurfaceMeshData.AddFace(a, e, f);
            AddMaterialFaceSafe(a, b, c);
            AddMaterialFaceSafe(a, c, d);
            AddMaterialFaceSafe(a, d, e);
            AddMaterialFaceSafe(a, e, f);
        }

        // Generate extrude
        if (bGenerateExtrusion)
        {
            ExtrudeMeshData.AddFace(c, b, a);
            ExtrudeMeshData.AddFace(d, c, a);
            ExtrudeMeshData.AddFace(e, d, a);
            ExtrudeMeshData.AddFace(f, e, a);
        }
    }
}
