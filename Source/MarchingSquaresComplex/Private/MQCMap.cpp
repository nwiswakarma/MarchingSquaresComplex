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
#include "MQCMaterialUtility.h"

FMQCMap::FMQCMap()
    : VoxelResolution(8)
    , ChunkResolution(2)
    , MaxFeatureAngle(135.f)
    , MaxParallelAngle(8.f)
    , ExtrusionHeight(-1.f)
    , MaterialType(EMQCMaterialType::MT_COLOR)
{
}

FMQCMap::~FMQCMap()
{
    Clear();
}

FMQCMaterial FMQCMap::GetTypedMaterial(uint8 MaterialIndex, const FLinearColor& MaterialColor)
{
    return UMQCMaterialUtility::GetTypedInputMaterial(MaterialType, MaterialIndex, MaterialColor);
}

void FMQCMap::Triangulate()
{
    for (FMQCGridChunk* Chunk : Chunks)
    {
        Chunk->Triangulate();
    }

    ResolveChunkEdgeData();
}

void FMQCMap::TriangulateAsync()
{
    for (FMQCGridChunk* Chunk : Chunks)
    {
        Chunk->TriangulateAsync();
    }

    bRequireFinalizeAsync = true;
}

void FMQCMap::WaitForAsyncTask()
{
    for (FMQCGridChunk* Chunk : Chunks)
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
    for (int32 ChunkIndex=0; ChunkIndex<Chunks.Num(); ++ChunkIndex)
    {
        FMQCGridChunk& Chunk(*Chunks[ChunkIndex]);

        // Get chunk edge sync data
        int32 SyncOffetIndex = Chunk.AppendEdgeSyncData(SyncCandidates, StateIndex);

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
        EdgeSyncLists.AddDefaulted(1);
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

    // Generate edge sync array from linked list

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
}

void FMQCMap::InitializeSettings(const FMQCMapConfig& MapConfig)
{
    // Invalid resolution, abort
    if (MapConfig.ChunkResolution < 1 || MapConfig.VoxelResolution < 1)
    {
        return;
    }

    // Initialize map settings

    VoxelResolution = MapConfig.VoxelResolution;
    ChunkResolution = MapConfig.ChunkResolution;
    MaxFeatureAngle = MapConfig.MaxFeatureAngle;
    MaxParallelAngle = MapConfig.MaxParallelAngle;
    ExtrusionHeight = MapConfig.ExtrusionHeight;
    MaterialType = MapConfig.MaterialType;
    SurfaceStates = MapConfig.States;

    check(ChunkResolution > 0);
    check(VoxelResolution > 0);
    check(! bRequireFinalizeAsync);

    // Create uninitialized chunks

    Clear();

    Chunks.SetNumUninitialized(ChunkResolution * ChunkResolution);

    for (int32 i=0; i<Chunks.Num(); ++i)
    {
        Chunks[i] = new FMQCGridChunk;
    }
}

void FMQCMap::InitializeChunk(int32 i, int32 x, int32 y)
{
    check(Chunks.IsValidIndex(i));

    // Initialize chunk

    FIntPoint ChunkPosition(x * VoxelResolution, y * VoxelResolution);

    FMQCChunkConfig ChunkConfig;
    ChunkConfig.States = SurfaceStates;
    ChunkConfig.Position = ChunkPosition;
    ChunkConfig.MapSize = GetVoxelDimension();
    ChunkConfig.VoxelResolution = VoxelResolution;
    ChunkConfig.MaxFeatureAngle = MaxFeatureAngle;
    ChunkConfig.MaxParallelAngle = MaxParallelAngle;
    ChunkConfig.ExtrusionHeight = ExtrusionHeight;
    ChunkConfig.MaterialType = MaterialType;

    // Link chunk neighbours

    FMQCGridChunk& Chunk(*Chunks[i]);

    if (x > 0)
    {
        Chunks[i - 1]->SetNeighbourX(&Chunk);
    }

    if (y > 0)
    {
        Chunks[i - ChunkResolution]->SetNeighbourY(&Chunk);

        if (x > 0)
        {
            Chunks[i - ChunkResolution - 1]->SetNeighbourXY(&Chunk);
        }
    }

    Chunk.Configure(ChunkConfig);
}

void FMQCMap::InitializeChunks()
{
    check(Chunks.Num() == (ChunkResolution * ChunkResolution));

    for (int32 y=0, i=0; y<ChunkResolution; y++)
    for (int32 x=0     ; x<ChunkResolution; x++, i++)
    {
        InitializeChunk(i, x, y);
    }
}

void FMQCMap::Initialize(const FMQCMapConfig& MapConfig)
{
    InitializeSettings(MapConfig);
    InitializeChunks();
}

void FMQCMap::Clear()
{
    for (FMQCGridChunk* Chunk : Chunks)
    {
        delete Chunk;
    }

    Chunks.Empty();
}

void FMQCMap::ResetChunkStates(const TArray<int32>& ChunkIndices)
{
    for (int32 i : ChunkIndices)
    {
        if (Chunks.IsValidIndex(i))
        {
            Chunks[i]->ResetVoxels();
        }
    }
}

void FMQCMap::ResetAllChunkStates()
{
    for (FMQCGridChunk* Chunk : Chunks)
    {
        Chunk->ResetVoxels();
    }
}

void FMQCMap::AddQuadFilter(const FIntPoint& Point, int32 StateIndex, bool bExtrudeFilter)
{
    if (IsWithinDimension(Point.X, Point.Y))
    {
        int32 ChunkIndex = GetChunkIndexByPoint(Point.X, Point.Y);
        GetChunk(ChunkIndex).AddQuadFilter(Point, StateIndex, bExtrudeFilter);
    }
}

int32 FMQCMap::GetEdgePointListCount(int32 StateIndex) const
{
    return EdgeSyncGroups.IsValidIndex(StateIndex)
        ? EdgeSyncGroups[StateIndex].Num()
        : 0;
}

void FMQCMap::GetEdgePoints(TArray<FMQCEdgePointList>& OutPointList, int32 StateIndex) const
{
    // Invalid state, abort
    if (! EdgeSyncGroups.IsValidIndex(StateIndex))
    {
        return;
    }

    const FStateEdgeSyncList& EdgeSyncs(EdgeSyncGroups[StateIndex]);

    OutPointList.Reset();
    OutPointList.SetNum(EdgeSyncs.Num(), true);

    for (int32 i=0; i<EdgeSyncs.Num(); ++i)
    {
        const FEdgeSyncList& SyncList(EdgeSyncs[i]);
        FMQCEdgePointList& Points(OutPointList[i]);

        // Generate connected edge point list
        for (const FMQCEdgeSyncData& SyncData : SyncList)
        {
            const FMQCGridChunk& Chunk(GetChunk(SyncData.ChunkIndex));
            Chunk.AppendConnectedEdgePoints(Points, StateIndex, SyncData.EdgeListIndex);
        }
    }
}

void FMQCMap::GetEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListIndex) const
{
    // Invalid edge list index, abort
    if (! EdgeSyncGroups.IsValidIndex(StateIndex) ||
        ! EdgeSyncGroups[StateIndex].IsValidIndex(EdgeListIndex)
        )
    {
        return;
    }

    const FEdgeSyncList& SyncList(EdgeSyncGroups[StateIndex][EdgeListIndex]);

    // Generate connected edge points
    for (const FMQCEdgeSyncData& SyncData : SyncList)
    {
        const FMQCGridChunk& Chunk(GetChunk(SyncData.ChunkIndex));
        Chunk.AppendConnectedEdgePoints(OutPoints, StateIndex, SyncData.EdgeListIndex);
    }
}

void FMQCMap::GetEdgePointsByChunkSurface(TArray<FMQCEdgePointData>& OutPointList, int32 ChunkIndex, int32 StateIndex) const
{
    if (Chunks.IsValidIndex(ChunkIndex))
    {
        GetChunk(ChunkIndex).GetEdgePoints(OutPointList, StateIndex);
    }
}

// ----------------------------------------------------------------------------

// MAP SETTINGS FUNCTIONS

void UMQCMapRef::InitializeVoxelMap()
{
    VoxelMap.Initialize(MapConfig);
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
        ? FVector(VoxelMap.GetChunk(ChunkIndex).GetOffsetId(), 0.f)
        : FVector(ForceInitToZero);
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

void UMQCMapRef::GetEdgePoints(TArray<FVector2D>& OutPoints, int32 StateIndex, int32 EdgeListIndex)
{
    if (IsInitialized())
    {
        VoxelMap.GetEdgePoints(OutPoints, StateIndex, EdgeListIndex);
    }
}

void UMQCMapRef::GetEdgePointsByChunkSurface(TArray<FMQCEdgePointData>& OutPointList, int32 ChunkIndex, int32 StateIndex)
{
    if (IsInitialized())
    {
        VoxelMap.GetEdgePointsByChunkSurface(OutPointList, ChunkIndex, StateIndex);
    }
}

void UMQCMapRef::AddQuadFilters(const TArray<FIntPoint>& Points, int32 StateIndex, bool bFilterExtrude)
{
    if (IsInitialized() && VoxelMap.HasState(StateIndex))
    {
        for (const FIntPoint& Point : Points)
        {
            VoxelMap.AddQuadFilter(Point, StateIndex, bFilterExtrude);
        }
    }
}

void UMQCMapRef::AddQuadFiltersByBounds(const FIntPoint& BoundsMin, const FIntPoint& BoundsMax, int32 StateIndex, bool bFilterExtrude)
{
    if (IsInitialized() && VoxelMap.HasState(StateIndex))
    {
        FIntPoint ClampedMin;
        FIntPoint ClampedMax;

        ClampedMin.X = FMath::Clamp(BoundsMin.X, 0, GetVoxelDimension()-1);
        ClampedMin.Y = FMath::Clamp(BoundsMin.Y, 0, GetVoxelDimension()-1);
        ClampedMax.X = FMath::Clamp(BoundsMax.X, ClampedMin.X, GetVoxelDimension()-1);
        ClampedMax.Y = FMath::Clamp(BoundsMax.Y, ClampedMin.Y, GetVoxelDimension()-1);

        FIntPoint Point;

        for (int32 y=ClampedMin.Y; y<=ClampedMax.Y; ++y)
        for (int32 x=ClampedMin.X; x<=ClampedMax.X; ++x)
        {
            Point.X = x;
            Point.Y = y;
            VoxelMap.AddQuadFilter(Point, StateIndex, bFilterExtrude);
        }
    }
}

// MAP ACTOR

AMQCMap::AMQCMap()
    : MapRef(nullptr)
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
    //MapRef->VoxelResolution = VoxelResolution;
    //MapRef->ChunkResolution = ChunkResolution;
    //MapRef->MaxFeatureAngle = MaxFeatureAngle;
    //MapRef->MaxParallelAngle = MaxParallelAngle;
    //MapRef->ExtrusionHeight = ExtrusionHeight;
    //MapRef->MaterialType = MaterialType;
    //MapRef->SurfaceStates = SurfaceStates;
    MapRef->MapConfig = MapConfig;
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

        //UPMUMeshComponent* Mesh;
        UPMUMeshComponent* Mesh = GetSurfaceMesh(ChunkIndex);
        //FName MeshName(*FString::Printf(TEXT("SurfaceMesh_%d"), ChunkIndex));

        //Mesh = NewObject<UPMUMeshComponent>(this, MeshName);
        //Mesh->bEditableWhenInherited = true;
        //Mesh->SetupAttachment(MeshAnchor);
        //Mesh->RegisterComponent();

        Mesh->CreateNewSection(SurfaceRef, MGI_SURFACE);
        //Mesh->CreateNewSection(ExtrudeRef, MGI_EXTRUDE);
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

    // Generate material blend identifier

    FMQCMaterialBlend MaterialBlend;

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

        MaterialBlend = FMQCMaterialBlend(
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
            MaterialBlend = FMQCMaterialBlend(MaterialIndex0);
        }
        else
        {
            MaterialBlend = FMQCMaterialBlend(MaterialIndex0, MaterialIndex1);
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

        SectionPtr = Chunk.GetSurfaceMaterialSection(StateIndex, MaterialBlend);

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
