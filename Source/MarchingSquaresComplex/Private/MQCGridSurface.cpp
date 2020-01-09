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

FMQCGridSurface::FMQCGridSurface()
{
}

FMQCGridSurface::FMQCGridSurface(const FMQCSurfaceConfig& Config)
{
    Configure(Config);
}

FMQCGridSurface::~FMQCGridSurface()
{
}

void FMQCGridSurface::Configure(const FMQCSurfaceConfig& Config)
{
    // Position & dimension configuration

    VoxelResolution = Config.VoxelResolution;
    VoxelCount = VoxelResolution * VoxelResolution;
    MapSize = Config.MapSize-1;
    MapSizeInv = MapSize > 0.f ? (1.f/MapSize) : KINDA_SMALL_NUMBER;
    ChunkPosition = Config.Position;

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
    // Shrink mesh data container
    CompactGeometry(SurfaceMeshData);
    CompactGeometry(ExtrudeMeshData);
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
        GenerateEdgeListData();
    }

    CompactGeometry();
}

void FMQCGridSurface::Clear()
{
    // Clear triangulation data
    VertexMap.Empty();
    cornersMinArr.Empty();
    cornersMaxArr.Empty();
    xEdgesMinArr.Empty();
    xEdgesMaxArr.Empty();
    // Clear geometry data
    GetSurfaceSection().Reset();
    GetExtrudeSection().Reset();
}

void FMQCGridSurface::GetMaterialSet(TSet<FMQCMaterialBlend>& MaterialSet) const
{
    for (const auto& MaterialSectionPair : SurfaceMeshData.MaterialSectionMap)
    {
        MaterialSet.Emplace(MaterialSectionPair.Key);
    }
}

void FMQCGridSurface::AddVertex(const FVector2D& Point, const FMQCMaterial& Material, bool bIsExtrusion)
{
    FVector2D UV(Point*MapSizeInv - MapSizeInv*.5f);

    float Height;
    float FaceSign;

    FMeshData* MeshData;

    if (bIsExtrusion)
    {
        MeshData = &ExtrudeMeshData;
        Height = ExtrusionHeight;
        FaceSign = -1.f;
    }
    else
    {
        MeshData = &SurfaceMeshData;
        Height = 0.f;
        FaceSign = 1.f;
    }

    FPMUMeshSection& Section(MeshData->Section);
    FVector Position(Point, Height);
    FPackedNormal TangentX(FVector(1,0,0));
    FPackedNormal TangentZ(FVector4(0,0,FaceSign,FaceSign));

    // Assign vertex and bounds

    const uint32 VertexIndex = Section.Positions.Num();

    Section.Positions.Emplace(Position);
    Section.UVs.Emplace(UV);
    Section.Colors.Emplace(Material.ToFColor());
    Section.Tangents.Emplace(TangentX.Vector.Packed);
    Section.Tangents.Emplace(TangentZ.Vector.Packed);
    Section.SectionLocalBox += Position;

    MeshData->Materials.Emplace(Material);

    //UE_LOG(LogTemp,Warning, TEXT("VERT: %s"), *XY.ToString());
}

void FMQCGridSurface::AddEdge(uint32 a, uint32 b)
{
    if ((a == b) || ! bGenerateExtrusion)
    {
        return;
    }

    FEdgeLinkList* Connection = nullptr;

    // Find edge connection with existing edge links
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

    FMeshData& MeshData(SurfaceMeshData);

    // Find material blend

    check(MeshData.Materials.IsValidIndex(a));
    check(MeshData.Materials.IsValidIndex(b));
    check(MeshData.Materials.IsValidIndex(c));

    FMQCMaterial Materials[3];
    Materials[0] = MeshData.Materials[a];
    Materials[1] = MeshData.Materials[b];
    Materials[2] = MeshData.Materials[c];

    uint8 BlendsA[3];
    uint8 BlendsB[3];
    uint8 BlendsC[3];

    FMQCMaterialBlend MaterialBlend;

    UMQCMaterialUtility::FindTripleIndexFaceBlend(
        Materials,
        MaterialBlend,
        BlendsA,
        BlendsB,
        BlendsC
        );

    // Add material blend face

    MeshData.AddMaterialFace(MaterialBlend, a, b, c, BlendsA, BlendsB, BlendsC);
}

void FMQCGridSurface::GenerateEdgeListData()
{
    // Only generate edge geometry on surface that generate extrusion
    if (! bGenerateExtrusion)
    {
        return;
    }

    EdgePointIndexList.Reset();
    EdgePointIndexList.SetNum(EdgeLinkLists.Num());

    for (int32 ListId=0; ListId<EdgeLinkLists.Num(); ++ListId)
    {
        FEdgeLinkList& EdgeList(EdgeLinkLists[ListId]);
        const int32 EdgeCount = EdgeList.Num();

        // Ensure valid edge count
        check(EdgeCount >= 2);

        if (EdgeCount < 2)
        {
            continue;
        }

        FIndexArray& PointIndices(EdgePointIndexList[ListId]);
        PointIndices.Reset(EdgeCount+1);

        // Generate point indices

        FEdgeLink* EdgeNode = EdgeList.Head;

        while (EdgeNode)
        {
            PointIndices.Emplace(EdgeNode->Value);

            EdgeNode = EdgeNode->Next;
        }

        // Generate edge connection data
        GenerateEdgeListData(ListId);
    }
}

void FMQCGridSurface::GenerateEdgeListData(int32 EdgeListIndex)
{
    check(EdgePointIndexList.IsValidIndex(EdgeListIndex));

    const FIndexArray& PointIndices(EdgePointIndexList[EdgeListIndex]);

    // Create edge list sync data

    EdgeSyncList.AddDefaulted(1);
    FMQCEdgeSyncData& SyncData(EdgeSyncList.Last());

    SyncData.EdgeListIndex = EdgeListIndex;
    SyncData.HeadHash = GetVertexHash(PointIndices[0]);
    SyncData.TailHash = GetVertexHash(PointIndices.Last());
}

void FMQCGridSurface::GetEdgePoints(TArray<FMQCEdgePointData>& OutPointList) const
{
    OutPointList.SetNum(EdgePointIndexList.Num());

    for (int32 li=0; li<EdgePointIndexList.Num(); ++li)
    {
        const FIndexArray& EdgePointIndices(EdgePointIndexList[li]);
        TArray<FVector2D>& OutPoints(OutPointList[li].Points);

        OutPoints.SetNumUninitialized(EdgePointIndices.Num());

        for (int32 i=0; i<EdgePointIndices.Num(); ++i)
        {
            OutPoints[i] = GetPositionByIndex(EdgePointIndices[i]);
        }
    }
}

void FMQCGridSurface::GetEdgePoints(TArray<FVector2D>& OutPoints, int32 EdgeListIndex) const
{
    if (! EdgePointIndexList.IsValidIndex(EdgeListIndex))
    {
        return;
    }

    const FIndexArray& Points(EdgePointIndexList[EdgeListIndex]);

    OutPoints.Reserve(Points.Num());

    for (int32 i=1; i<Points.Num(); ++i)
    {
        OutPoints.Emplace(GetPositionByIndex(Points[i]));
    }
}

void FMQCGridSurface::AppendConnectedEdgePoints(TArray<FVector2D>& OutPoints, int32 EdgeListIndex) const
{
    check(EdgePointIndexList.IsValidIndex(EdgeListIndex));

    if (! EdgePointIndexList.IsValidIndex(EdgeListIndex))
    {
        return;
    }

    const FIndexArray& Points(EdgePointIndexList[EdgeListIndex]);

    if (Points.Num() < 1)
    {
        return;
    }

    OutPoints.Reserve(OutPoints.Num()+Points.Num());

    // Assign first position from current sync data

    if (OutPoints.Num() > 0)
    {
        OutPoints.Last() = GetPositionByIndex(Points[0]);
    }
    else
    {
        OutPoints.Emplace(GetPositionByIndex(Points[0]));
    }

    // Assign the rest of the edge sync points

    for (int32 i=1; i<Points.Num(); ++i)
    {
        OutPoints.Emplace(GetPositionByIndex(Points[i]));
    }
}

void FMQCGridSurface::AddQuadFace(uint32 a, uint32 b, uint32 c, uint32 d)
{
    // Generate extrude only
    if (bExtrusionSurface)
    {
        ExtrudeMeshData.AddQuadInversed(a, b, c, d);
    }
    else
    {
        // Generate surface
        if (! SurfaceMeshData.IsQuadFiltered(a))
        {
            SurfaceMeshData.AddQuad(a, b, c, d);
            AddMaterialFace(a, b, c);
            AddMaterialFace(a, c, d);
        }

        // Generate extrude
        if (bGenerateExtrusion && ! ExtrudeMeshData.IsQuadFiltered(a))
        {
            ExtrudeMeshData.AddQuadInversed(a, b, c, d);
        }
    }
}

void FMQCGridSurface::AddTriangleEdgeFace(uint32 a, uint32 b, uint32 c)
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

void FMQCGridSurface::AddQuadEdgeFace(uint32 a, uint32 b, uint32 c, uint32 d)
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

void FMQCGridSurface::AddPentagonEdgeFace(uint32 a, uint32 b, uint32 c, uint32 d, uint32 e)
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

void FMQCGridSurface::AddHexagonEdgeFace(uint32 a, uint32 b, uint32 c, uint32 d, uint32 e, uint32 f)
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
