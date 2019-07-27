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

#include "MQCMap.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Components/BillboardComponent.h"
#include "Mesh/PMUMeshComponent.h"
#include "Mesh/PMUMeshUtility.h"
#include "Mesh/Simplifier/PMUMeshSimplifier.h"

#include "MQCGridChunk.h"

bool FMQCMap::HasChunk(int32 ChunkIndex) const
{
    return chunks.IsValidIndex(ChunkIndex);
}

int32 FMQCMap::GetChunkCount() const
{
    return chunks.Num();
}

const FMQCGridChunk& FMQCMap::GetChunk(int32 ChunkIndex) const
{
    return *chunks[ChunkIndex];
}

FMQCGridChunk& FMQCMap::GetChunk(int32 ChunkIndex)
{
    return *chunks[ChunkIndex];
}

void FMQCMap::Triangulate()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        Chunk->Triangulate();
    }

    ResolveChunkEdgeData();

    //TSet<FMQCMaterialBlend> MaterialSet;
    //for (FMQCGridChunk* Chunk : chunks)
    //{
    //    Chunk->GetMaterialSet(MaterialSet);
    //}
    //for (const FMQCMaterialBlend& M : MaterialSet)
    //{
    //    UE_LOG(LogTemp,Warning, TEXT("M: %s"), *M.ToString());
    //}
}

void FMQCMap::TriangulateAsync()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        Chunk->TriangulateAsync();
    }

    bRequireFinalizeAsync = true;
}

void FMQCMap::WaitForAsyncTask()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        Chunk->WaitForAsyncTask();
    }
}

void FMQCMap::FinalizeAsync()
{
    if (bRequireFinalizeAsync)
    {
        WaitForAsyncTask();
        ResolveChunkEdgeData();
        bRequireFinalizeAsync = false;
    }
}

void FMQCMap::ResolveChunkEdgeData()
{
    EdgeSyncGroups.SetNum(SurfaceStates.Num()+1, false);

    for (int32 i=0; i<SurfaceStates.Num(); ++i)
    {
        if (SurfaceStates[i].bRemapEdgeUVs)
        {
            ResolveChunkEdgeData(i+1);
        }
    }
}

void FMQCMap::ResolveChunkEdgeData(int32 StateIndex)
{
    TArray<FMQCEdgeSyncData> SyncCandidates;
    TArray<TDoubleLinkedList<FMQCEdgeSyncData>> EdgeSyncLists;

    // Gather edge sync data from all chunks
    for (int32 ChunkIndex=0; ChunkIndex<chunks.Num(); ++ChunkIndex)
    {
        FMQCGridChunk& Chunk(*chunks[ChunkIndex]);

        // Get chunk edge sync data
        int32 SyncOffetIndex = Chunk.AppendEdgeSyncData(StateIndex, SyncCandidates);

        // Assign edge sync chunk index
        for (int32 i=SyncOffetIndex; i<SyncCandidates.Num(); ++i)
        {
            FMQCEdgeSyncData& SyncData(SyncCandidates[i]);
            SyncData.ChunkIndex = ChunkIndex;
        }
    }

    // Construct edge sync data connection list
    while (SyncCandidates.Num() > 0)
    {
        EdgeSyncLists.SetNum(EdgeSyncLists.Num()+1);
        TDoubleLinkedList<FMQCEdgeSyncData>& SyncList(EdgeSyncLists.Last());

        SyncList.AddTail(SyncCandidates[0]);

        SyncCandidates.RemoveAtSwap(0, 1, false);

        int32 It = 0;

        // Iterate over edge sync list
        while (It < SyncCandidates.Num())
        {
            const FMQCEdgeSyncData& HeadSyncData(SyncList.GetHead()->GetValue());
            const FMQCEdgeSyncData& TailSyncData(SyncList.GetTail()->GetValue());
            uint32 HeadHash = HeadSyncData.HeadHash;
            uint32 TailHash = TailSyncData.TailHash;

            const FMQCEdgeSyncData& SyncData(SyncCandidates[It]);
            uint32 a = SyncData.HeadHash;
            uint32 b = SyncData.TailHash;

            bool bHasConnection = false;

            // Find edge list connection
            if (a == TailHash)
            {
                SyncList.AddTail(SyncData);
                bHasConnection = true;
            }
            else
            if (b == HeadHash)
            {
                SyncList.AddHead(SyncData);
                bHasConnection = true;
            }

            // Connection found remove current edge list and start over iteration
            if (bHasConnection)
            {
                SyncCandidates.RemoveAtSwap(It, 1, false);
                It = 0;
                continue;
            }

            ++It;
        }
    }

    FStateEdgeSyncList& EdgeSyncGroup(EdgeSyncGroups[StateIndex]);

    EdgeSyncGroup.Reset();
    EdgeSyncGroup.SetNum(EdgeSyncLists.Num(), true);

    for (int32 i=0; i<EdgeSyncLists.Num(); ++i)
    {
        const TDoubleLinkedList<FMQCEdgeSyncData>& List(EdgeSyncLists[i]);
        FEdgeSyncList& EdgeSyncArr(EdgeSyncGroup[i]);

        EdgeSyncArr.Reserve(List.Num());

        for (const FMQCEdgeSyncData& SyncData : List)
        {
            EdgeSyncArr.Emplace(SyncData);
        }
    }

#if 0
    // Remap connected edge distances and uvs
    for (int32 ListIndex=0; ListIndex<EdgeSyncLists.Num(); ++ListIndex)
    {
        const FEdgeSyncList& SyncList(EdgeSyncLists[ListIndex]);

        // Skip edge list that does not have any connection (remap not required)
        if (SyncList.Num() < 2)
        {
            continue;
        }

        int32 j=0;
        float Length = 0.f;
        float LengthInv = 0.f;

        // Find edge list total length

        for (const FMQCEdgeSyncData& SyncData : SyncList)
        {
            Length += SyncData.Length;
        }

        // Calculate inversed edge total length

        if (Length > 0.f)
        {
            LengthInv = 1.f/Length;
        }

        // Remap edge list uv

        float UV0 = 0.f;
        float UV1 = 0.f;

        // Remap edge uvs
        for (const FMQCEdgeSyncData& SyncData : SyncList)
        {
            UV0 = UV1;
            UV1 = UV0 + SyncData.Length * LengthInv;

            FMQCGridChunk& Chunk(*chunks[SyncData.ChunkIndex]);
            Chunk.RemapEdgeUVs(StateIndex, SyncData.EdgeListIndex, UV0, UV1);
        }
    }
#endif
}

void FMQCMap::InitializeSettings()
{
    // Invalid resolution, abort
    if (chunkResolution < 1 || voxelResolution < 1)
    {
        return;
    }

    // Initialize map settings

    check(chunkResolution > 0);
    check(voxelResolution > 0);
    check(! bRequireFinalizeAsync);

    // Create uninitialized chunks

    Clear();

    chunks.SetNumUninitialized(chunkResolution * chunkResolution);

    for (int32 i=0; i<chunks.Num(); ++i)
    {
        chunks[i] = new FMQCGridChunk;
    }
}

void FMQCMap::InitializeChunkSettings(int32 i, int32 x, int32 y, FMQCChunkConfig& ChunkConfig)
{
    check(chunks.IsValidIndex(i));

    // Initialize chunk

    FIntPoint chunkPosition(x * voxelResolution, y * voxelResolution);

    ChunkConfig.States = SurfaceStates;
    ChunkConfig.Position = chunkPosition;
    ChunkConfig.MapSize = GetVoxelCount();
    ChunkConfig.VoxelResolution = voxelResolution;
    ChunkConfig.MaxFeatureAngle = MaxFeatureAngle;
    ChunkConfig.MaxParallelAngle = MaxParallelAngle;
    ChunkConfig.ExtrusionHeight = extrusionHeight;
    ChunkConfig.MaterialType = MaterialType;

    // Link chunk neighbours

    FMQCGridChunk& Chunk(*chunks[i]);

    if (x > 0)
    {
        chunks[i - 1]->xNeighbor = &Chunk;
    }

    if (y > 0)
    {
        chunks[i - chunkResolution]->yNeighbor = &Chunk;

        if (x > 0)
        {
            chunks[i - chunkResolution - 1]->xyNeighbor = &Chunk;
        }
    }
}

void FMQCMap::InitializeChunk(int32 i, const FMQCChunkConfig& ChunkConfig)
{
    check(chunks.IsValidIndex(i));
    chunks[i]->Configure(ChunkConfig);
}

void FMQCMap::InitializeChunks()
{
    check(chunks.Num() == (chunkResolution * chunkResolution));

    for (int32 y=0, i=0; y<chunkResolution; y++)
    for (int32 x=0     ; x<chunkResolution; x++, i++)
    {
        FMQCChunkConfig ChunkConfig;
        InitializeChunkSettings(i, x, y, ChunkConfig);
        InitializeChunk(i, ChunkConfig);
    }
}

void FMQCMap::Initialize()
{
    InitializeSettings();
    InitializeChunks();
}

void FMQCMap::Clear()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        delete Chunk;
    }

    chunks.Empty();
}

void FMQCMap::ResetChunkStates(const TArray<int32>& ChunkIndices)
{
    for (int32 i : ChunkIndices)
    {
        if (chunks.IsValidIndex(i))
        {
            chunks[i]->ResetVoxels();
        }
    }
}

void FMQCMap::ResetAllChunkStates()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        Chunk->ResetVoxels();
    }
}

int32 FMQCMap::GetEdgeListCount(int32 StateIndex) const
{
    return EdgeSyncGroups.IsValidIndex(StateIndex)
        ? EdgeSyncGroups[StateIndex].Num()
        : 0;
}

void FMQCMap::GetEdgeList(TArray<FMQCEdgePointList>& OutLists, int32 StateIndex) const
{
    // Invalid state, abort
    if (! EdgeSyncGroups.IsValidIndex(StateIndex))
    {
        return;
    }

    const FStateEdgeSyncList& EdgeSyncs(EdgeSyncGroups[StateIndex]);

    OutLists.Reset();
    OutLists.SetNum(EdgeSyncs.Num(), true);

    for (int32 i=0; i<EdgeSyncs.Num(); ++i)
    {
        const FEdgeSyncList& SyncList(EdgeSyncs[i]);
        TArray<FMQCEdgePoint>& Points(OutLists[i].Points);
        float Distance = 0.f;

        Points.Reset(SyncList.Num());

        // Generate connected edge point list
        for (const FMQCEdgeSyncData& SyncData : SyncList)
        {
            const FMQCGridChunk& Chunk(GetChunk(SyncData.ChunkIndex));
            const FMQCGridSurface& Surface(Chunk.GetSurface(StateIndex));

            //UE_LOG(LogTemp,Warning, TEXT("GetEdgeList() SyncData: %s"), *SyncData.ToString());

            Surface.GetConnectedEdgePoints(Points, SyncData, Distance);

            Distance += SyncData.Length;
        }

        // Check for circular edge list
        if (Points.Num() > 2)
        {
            const FMQCEdgePoint& P0(Points[0]);
            const FMQCEdgePoint& PN(Points.Last());

            // Copy first point normal to the last if circular
            if (P0.Position.Equals(PN.Position))
            {
                Points.Last().Normal = Points[0].Normal;
            }
        }
    }
}

// ----------------------------------------------------------------------------

UMQCMapRef::UMQCMapRef()
    : VoxelResolution(8)
    , ChunkResolution(2)
    , MaterialType(EMQCMaterialType::MT_COLOR)
    , ExtrusionHeight(-1.f)
    , MaxFeatureAngle(135.f)
    , MaxParallelAngle(8.f)
{
}

UMQCMapRef::~UMQCMapRef()
{
}

// MAP SETTINGS FUNCTIONS

void UMQCMapRef::ApplyMapSettings()
{
    VoxelMap.voxelResolution = VoxelResolution;
    VoxelMap.chunkResolution = ChunkResolution;
    VoxelMap.extrusionHeight = ExtrusionHeight;

    VoxelMap.MaterialType = MaterialType;

    VoxelMap.MaxFeatureAngle = MaxFeatureAngle;
    VoxelMap.MaxParallelAngle = MaxParallelAngle;

    VoxelMap.SurfaceStates = SurfaceStates;
}

void UMQCMapRef::InitializeVoxelMap()
{
    ApplyMapSettings();
    VoxelMap.Initialize();
}

FMQCMaterial UMQCMapRef::GetTypedMaterial(uint8 MaterialIndex, const FLinearColor& MaterialColor)
{
    return UMQCMaterialUtility::GetTypedInputMaterial(MaterialType, MaterialIndex, MaterialColor);
}

// TRIANGULATION FUNCTIONS

void UMQCMapRef::ClearVoxelMap()
{
    VoxelMap.Clear();
}

void UMQCMapRef::Triangulate()
{
    VoxelMap.Triangulate();
}

void UMQCMapRef::TriangulateAsync()
{
    VoxelMap.TriangulateAsync();
}

void UMQCMapRef::WaitForAsyncTask()
{
    VoxelMap.WaitForAsyncTask();
}

void UMQCMapRef::FinalizeAsync()
{
    VoxelMap.FinalizeAsync();
}

void UMQCMapRef::ResetChunkStates(const TArray<int32>& ChunkIndices)
{
    if (IsInitialized())
    {
        VoxelMap.ResetChunkStates(ChunkIndices);
    }
}

void UMQCMapRef::ResetAllChunkStates()
{
    if (IsInitialized())
    {
        VoxelMap.ResetAllChunkStates();
    }
}

// CHUNK & SECTION FUNCTIONS

FVector UMQCMapRef::GetChunkPosition(int32 ChunkIndex) const
{
    return HasChunk(ChunkIndex)
        ? FVector(VoxelMap.GetChunk(ChunkIndex).Position, 0.f)
        : FVector();
}

FPMUMeshSectionRef UMQCMapRef::GetSurfaceSection(int32 ChunkIndex, int32 StateIndex)
{
    if (HasChunk(ChunkIndex))
    {
        FMQCGridChunk& Chunk(VoxelMap.GetChunk(ChunkIndex));
        return FPMUMeshSectionRef(*Chunk.GetSurfaceSection(StateIndex));
    }
    else
    {
        return FPMUMeshSectionRef();
    }
}

FPMUMeshSectionRef UMQCMapRef::GetExtrudeSection(int32 ChunkIndex, int32 StateIndex)
{
    if (HasChunk(ChunkIndex))
    {
        FMQCGridChunk& Chunk(VoxelMap.GetChunk(ChunkIndex));
        return FPMUMeshSectionRef(*Chunk.GetExtrudeSection(StateIndex));
    }
    else
    {
        return FPMUMeshSectionRef();
    }
}

FPMUMeshSectionRef UMQCMapRef::GetEdgeSection(int32 ChunkIndex, int32 StateIndex)
{
    if (HasChunk(ChunkIndex))
    {
        FMQCGridChunk& Chunk(VoxelMap.GetChunk(ChunkIndex));
        return FPMUMeshSectionRef(*Chunk.GetEdgeSection(StateIndex));
    }
    else
    {
        return FPMUMeshSectionRef();
    }
}

void UMQCMapRef::GetEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListId)
{
    if (! IsInitialized())
    {
        return;
    }

    TArray<FMQCEdgePointList> EdgeLists;
    VoxelMap.GetEdgeList(EdgeLists, StateIndex);

    if (EdgeLists.IsValidIndex(EdgeListId))
    {
        const TArray<FMQCEdgePoint>& EdgePoints(EdgeLists[EdgeListId].Points);

        OutPoints.Reset(EdgePoints.Num());

        for (const FMQCEdgePoint& EdgePoint : EdgePoints)
        {
            OutPoints.Emplace(EdgePoint.Position);
        }
    }
}

// MAP ACTOR

AMQCMap::AMQCMap()
    : MapRef(nullptr)
    , VoxelResolution(8)
    , ChunkResolution(2)
    , MaterialType(EMQCMaterialType::MT_COLOR)
    , ExtrusionHeight(-1.f)
    , MaxFeatureAngle(135.f)
    , MaxParallelAngle(8.f)
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

    MeshAnchor = CreateDefaultSubobject<USceneComponent>("MeshAnchor");
    MeshAnchor->SetupAttachment(RootComponent);

#if WITH_EDITOR
    // Add editor sprite component
    auto* SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
    if (SpriteComponent)
    {
        static ConstructorHelpers::FObjectFinder<UTexture2D> SpriteFinder(TEXT("/Engine/EditorResources/S_Terrain"));
        SpriteComponent->Sprite = SpriteFinder.Object;
        SpriteComponent->RelativeScale3D = FVector(0.5f, 0.5f, 0.5f);
        SpriteComponent->bHiddenInGame = true;
        SpriteComponent->bIsScreenSizeScaled = true;
        SpriteComponent->SetupAttachment(RootComponent);
        SpriteComponent->bReceivesDecals = false;
    }
#endif
}

AMQCMap::~AMQCMap()
{
}

void AMQCMap::InitializeMeshComponents(TArray<UPMUMeshComponent*>& MeshComponents)
{
    if (HasValidMap())
    {
        MeshComponents.SetNumZeroed(MapRef->GetChunkCount());
    }
}

UPMUMeshComponent* AMQCMap::GetOrAddMeshComponent(TArray<UPMUMeshComponent*>& MeshComponents, int32 MeshIndex)
{
    UPMUMeshComponent* MeshComponent = nullptr;

    if (MeshComponents.IsValidIndex(MeshIndex))
    {
        MeshComponent = MeshComponents[MeshIndex];

        if (! IsValid(MeshComponent))
        {
            FName MeshName(*FString::Printf(TEXT("MaterialMesh_%d"), MeshIndex));
            MeshComponent = NewObject<UPMUMeshComponent>(this, MeshName);
            MeshComponent->bEditableWhenInherited = true;
            MeshComponent->SetupAttachment(MeshAnchor);
            MeshComponent->RegisterComponent();
            MeshComponents[MeshIndex] = MeshComponent;
        }
    }

    return IsValid(MeshComponent) ? MeshComponent : nullptr;
}

UPMUMeshComponent* AMQCMap::GetSurfaceMesh(int32 MeshIndex)
{
    return GetOrAddMeshComponent(SurfaceMeshComponents, MeshIndex);
}

void AMQCMap::Initialize()
{
    // Create new 
    if (! HasValidMap())
    {
        MapRef = NewObject<UMQCMapRef>(this);
    }

    // Apply settings and initialize map
    MapRef->VoxelResolution = VoxelResolution;
    MapRef->ChunkResolution = ChunkResolution;
    MapRef->MaterialType = MaterialType;
    MapRef->SurfaceStates = SurfaceStates;
    MapRef->ExtrusionHeight = ExtrusionHeight;
    MapRef->MaxFeatureAngle = MaxFeatureAngle;
    MapRef->MaxParallelAngle = MaxParallelAngle;
    MapRef->InitializeVoxelMap();

    // Set mesh anchor offset
    MeshAnchor->SetRelativeLocation(FVector(-MapRef->GetCenter(), 0.f));
}

void AMQCMap::Triangulate(bool bAsync, bool bWaitForAsyncToFinish)
{
    if (HasValidMap())
    {
        if (bAsync)
        {
            MapRef->TriangulateAsync();

            if (bWaitForAsyncToFinish)
            {
                MapRef->FinalizeAsync();
            }
        }
        else
        {
            MapRef->Triangulate();
        }
    }
    else
    {
    }
}

void AMQCMap::GenerateMapMesh()
{
    if (! HasValidMap())
    {
        return;
    }

    FMQCMap& Map(MapRef->GetMap());
    const int32 StateCount = Map.GetStateCount();
    const int32 ChunkCount = Map.GetChunkCount();

    InitializeMeshComponents(SurfaceMeshComponents);

    for (int32 ChunkIndex=0; ChunkIndex<ChunkCount; ++ChunkIndex)
    {
        const int32 StateIndex = 1;

        FMQCGridChunk& Chunk(Map.GetChunk(ChunkIndex));
        FPMUMeshSectionRef SurfaceRef(*Chunk.GetSurfaceSection(StateIndex));
        FPMUMeshSectionRef ExtrudeRef(*Chunk.GetExtrudeSection(StateIndex));
        FPMUMeshSectionRef EdgeRef(*Chunk.GetEdgeSection(StateIndex));

        //UPMUMeshComponent* Mesh;
        UPMUMeshComponent* Mesh = GetSurfaceMesh(ChunkIndex);
        //FName MeshName(*FString::Printf(TEXT("SurfaceMesh_%d"), ChunkIndex));

        //Mesh = NewObject<UPMUMeshComponent>(this, MeshName);
        //Mesh->bEditableWhenInherited = true;
        //Mesh->SetupAttachment(MeshAnchor);
        //Mesh->RegisterComponent();

        Mesh->CreateNewSection(SurfaceRef, MGI_SURFACE);
        //Mesh->CreateNewSection(ExtrudeRef, MGI_EXTRUDE);
        //Mesh->CreateNewSection(EdgeRef, MGI_EDGE);
        Mesh->UpdateRenderState();
    }
}

void AMQCMap::GenerateMaterialMesh(
    int32 StateIndex,
    uint8 MaterialIndex0,
    uint8 MaterialIndex1,
    uint8 MaterialIndex2,
    bool bUseTripleIndex,
    UMaterialInterface* Material
    )
{
    if (! HasValidMap())
    {
        return;
    }

    FMQCMaterialBlend MaterialId;

    if (bUseTripleIndex)
    {
        if (MaterialIndex1 > MaterialIndex2)
        {
            Swap(MaterialIndex1, MaterialIndex2);
        }
        if (MaterialIndex0 > MaterialIndex2)
        {
            Swap(MaterialIndex0, MaterialIndex1);
            Swap(MaterialIndex1, MaterialIndex2);
        }
        else
        if (MaterialIndex0 > MaterialIndex1)
        {
            Swap(MaterialIndex0, MaterialIndex1);
        }

        MaterialId = FMQCMaterialBlend(
            MaterialIndex0,
            MaterialIndex1,
            MaterialIndex2
            );
    }
    else
    {
        if (MaterialIndex0 > MaterialIndex1)
        {
            Swap(MaterialIndex0, MaterialIndex1);
        }

        if (MaterialIndex0 == MaterialIndex1)
        {
            MaterialId = FMQCMaterialBlend(MaterialIndex0);
        }
        else
        {
            MaterialId = FMQCMaterialBlend(MaterialIndex0, MaterialIndex1);
        }
    }

    FMQCMap& Map(MapRef->GetMap());
    const int32 ChunkCount = Map.GetChunkCount();

    InitializeMeshComponents(SurfaceMeshComponents);

    for (int32 ChunkIndex=0; ChunkIndex<ChunkCount; ++ChunkIndex)
    {
        UPMUMeshComponent* MeshComponent = GetOrAddMeshComponent(SurfaceMeshComponents, ChunkIndex);

        FMQCGridChunk& Chunk(Map.GetChunk(ChunkIndex));
        FPMUMeshSection* SectionPtr;

        SectionPtr = Chunk.GetMaterialSection(StateIndex, MaterialId);

        if (SectionPtr)
        {
            FPMUMeshSectionRef SectionRef(*SectionPtr);
            int32 SectionIndex = MeshComponent->CreateNewSection(SectionRef);
            MeshComponent->SetMaterial(SectionIndex, Material);
            MeshComponent->UpdateRenderState();
        }
    }
}

void AMQCMap::ApplyHeightMap(
    int32 StateIndex,
    UTexture* HeightTexture,
    bool bGenerateTangents,
    float HeightScale
    )
{
    if (! HasValidMap())
    {
        return;
    }

    FMQCMap& Map(MapRef->GetMap());
    TArray<FPMUMeshSectionRef> SectionRefs;
    const int32 ChunkCount = Map.GetChunkCount();

    InitializeMeshComponents(SurfaceMeshComponents);

    // Find mesh sections
    for (int32 ChunkIndex=0; ChunkIndex<ChunkCount; ++ChunkIndex)
    {
        UPMUMeshComponent* Mesh = GetSurfaceMesh(ChunkIndex);

        if (! IsValid(Mesh))
        {
            continue;
        }

        TArray<int32> SectionIndices;
        Mesh->GetAllNonEmptySectionIndices(SectionIndices);

        for (int32 SectionIndex : SectionIndices)
        {
            FPMUMeshSectionRef SectionRef(Mesh->GetSectionRef(SectionIndex));
            FPMUMeshSection* SectionPtr(SectionRef.SectionPtr);

            if (SectionPtr && SectionPtr->HasGeometry())
            {
                SectionRefs.Emplace(SectionRef);
            }
        }
    }

    // Apply height map
    if (SectionRefs.Num() > 0)
    {
        FVector2D PosScale = MapRef->GetMeshInverseScale();

        UPMUMeshUtility::ApplyHeightMapToMeshSectionMulti(
            this,               // WorldContextObject
            SectionRefs,        // SectionRefs
            HeightTexture,      // HeightTexture
            HeightScale,        // HeightScale
            false,              // bUseUV
            PosScale.X,         // PositionToUVScaleX
            PosScale.Y,         // PositionToUVScaleX
            false,              // bMaskByColor
            false,              // bInverseColorMask
            bGenerateTangents,  // bAssignTangents
            nullptr             // CallbackEvent
            );
    }
}

void AMQCMap::CalculateMeshNormal(int32 StateIndex)
{
    if (! HasValidMap())
    {
        return;
    }

    FMQCMap& Map(MapRef->GetMap());
    TArray<FPMUMeshSectionRef> SectionRefs;
    const int32 ChunkCount = Map.GetChunkCount();

    InitializeMeshComponents(SurfaceMeshComponents);

    // Find mesh sections
    for (int32 ChunkIndex=0; ChunkIndex<ChunkCount; ++ChunkIndex)
    {
        UPMUMeshComponent* Mesh = GetSurfaceMesh(ChunkIndex);

        if (! IsValid(Mesh))
        {
            continue;
        }

        TArray<int32> SectionIndices;
        Mesh->GetAllNonEmptySectionIndices(SectionIndices);

        for (int32 SectionIndex : SectionIndices)
        {
            FPMUMeshSectionRef SectionRef(Mesh->GetSectionRef(SectionIndex));
            FPMUMeshSection* SectionPtr(SectionRef.SectionPtr);

            if (SectionPtr && SectionPtr->HasGeometry())
            {
                SectionRefs.Emplace(SectionRef);
            }
        }
    }

    for (FPMUMeshSectionRef& SectionRef : SectionRefs)
    {
        check(SectionRef.HasValidSection());
        check(SectionRef.SectionPtr->HasGeometry());

        FPMUMeshSection& Section(*SectionRef.SectionPtr);

        TArray<uint32>& Indices(Section.Indices);
        TArray<FVector>& Positions(Section.Positions);
        TArray<uint32>& Tangents(Section.Tangents);
        TArray<FVector> Normals;

        const int32 VCount = Positions.Num();
        const int32 TCount = Indices.Num() / 3;

        Normals.SetNumZeroed(VCount);

        for (int32 ti=0; ti<TCount; ++ti)
        {
            int32 i0 = ti*3;
            int32 i1 = i0+1;
            int32 i2 = i0+2;

            int32 vi0 = Indices[i0];
            int32 vi1 = Indices[i1];
            int32 vi2 = Indices[i2];

            const FVector& Pos0(Positions[vi0]);
            const FVector& Pos1(Positions[vi1]);
            const FVector& Pos2(Positions[vi2]);

            const FVector Edge21 = Pos1 - Pos2;
            const FVector Edge20 = Pos0 - Pos2;
            const FVector TriNormal = (Edge21 ^ Edge20).GetSafeNormal();

            Normals[vi0] += TriNormal;
            Normals[vi1] += TriNormal;
            Normals[vi2] += TriNormal;
        }

        for (int32 vi=0; vi<VCount; ++vi)
        {
            FVector4 Normal(Normals[vi].GetSafeNormal(), 1.f);
            FPackedNormal PackedNormal(Normal);

            Tangents[vi*2+1] = PackedNormal.Vector.Packed;
        }
    }
}

void AMQCMap::SimplifyMesh(int32 StateIndex, FPMUMeshSimplifierOptions Options)
{
    if (! HasValidMap())
    {
        return;
    }

    FMQCMap& Map(MapRef->GetMap());
    TArray<FPMUMeshSectionRef> SectionRefs;
    const int32 ChunkCount = Map.GetChunkCount();

    InitializeMeshComponents(SurfaceMeshComponents);

    // Find mesh sections
    for (int32 ChunkIndex=0; ChunkIndex<ChunkCount; ++ChunkIndex)
    {
        UPMUMeshComponent* Mesh = GetSurfaceMesh(ChunkIndex);

        if (! IsValid(Mesh))
        {
            continue;
        }

        TArray<int32> SectionIndices;
        Mesh->GetAllNonEmptySectionIndices(SectionIndices);

        for (int32 SectionIndex : SectionIndices)
        {
            FPMUMeshSectionRef SectionRef(Mesh->GetSectionRef(SectionIndex));
            FPMUMeshSection* SectionPtr(SectionRef.SectionPtr);

            if (SectionPtr && SectionPtr->HasGeometry())
            {
                SectionRefs.Emplace(SectionRef);
            }
        }
    }

    for (FPMUMeshSectionRef& SectionRef : SectionRefs)
    {
        FPMUMeshSimplifier::SimplifyMeshSection(SectionRef, Options);
    }
}

UMQCPrefabBuilder* AMQCMap::CreatePrefabBuilder(TSubclassOf<UMQCPrefabBuilder> Type)
{
    return NewObject<UMQCPrefabBuilder>(this, Type);
}
