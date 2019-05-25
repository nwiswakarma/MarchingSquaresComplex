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

void FMQCMap::RefreshAllChunks()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        Chunk->Refresh();
    }
}

void FMQCMap::RefreshAllChunksAsync(FGWTAsyncTaskRef& TaskRef)
{
    if (! TaskRef.IsValid())
    {
        //FGWTAsyncTaskRef::Init(
        //    TaskRef,
        //    IProceduralMeshUtility::Get().GetThreadPool());
    }

    check(TaskRef.IsValid());

    FPSGWTAsyncTask& Task(TaskRef.Task);

    for (int32 i=0; i<chunks.Num(); ++i)
    {
        FMQCGridChunk& Chunk(*chunks[i]);
        Task->AddTask([&, i](){ Chunk.Refresh(); });
    }
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

void FMQCMap::Clear()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        delete Chunk;
    }

    chunks.Empty();
}

TSharedPtr<FGWTAsyncThreadPool> FMQCMap::GetThreadPool()
{
    if (! ThreadPool.IsValid())
    {
        //ThreadPool = IProceduralMeshUtility::Get().GetThreadPool();
    }
    return ThreadPool;
}

void FMQCMap::InitializeSettings()
{
    // Invalid resolution, abort
    if (mapSize < 1 || chunkResolution < 1 || voxelResolution < 1)
    {
        return;
    }

    // Initialize map settings

    check(mapSize > 0);
    check(chunkResolution > 0);
    check(voxelResolution > 0);

    chunkSize = mapSize / chunkResolution;
    voxelSize = chunkSize / voxelResolution;

    bHasGridData = false;

    // Create uninitialized chunks

    Clear();

    chunks.SetNumUninitialized(chunkResolution * chunkResolution);

    for (int32 i=0; i<chunks.Num(); ++i)
    {
        chunks[i] = new FMQCGridChunk;
    }
}

void FMQCMap::InitializeChunkSettings(int32 i, int32 x, int32 y, FMQCGridConfig& ChunkConfig)
{
    check(chunks.IsValidIndex(i));

    // Initialize chunk

    FVector2D chunkPosition(x * chunkSize, y * chunkSize);

    ChunkConfig.States = surfaceStates;
    ChunkConfig.Position = chunkPosition;
    ChunkConfig.ChunkSize = chunkSize;
    ChunkConfig.VoxelResolution = voxelResolution;
    ChunkConfig.MapSize = mapSize;
    ChunkConfig.MaxFeatureAngle = maxFeatureAngle;
    ChunkConfig.MaxParallelAngle = maxParallelAngle;
    ChunkConfig.ExtrusionHeight = extrusionHeight;

    UE_LOG(LogTemp,Warning, TEXT("chunkPosition: %s"), *chunkPosition.ToString());
    UE_LOG(LogTemp,Warning, TEXT("chunkSize: %f"), chunkSize);
    UE_LOG(LogTemp,Warning, TEXT("voxelSize: %f"), voxelSize);

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

void FMQCMap::InitializeChunk(int32 i, const FMQCGridConfig& ChunkConfig)
{
    check(chunks.IsValidIndex(i));
    chunks[i]->Initialize(ChunkConfig);
}

void FMQCMap::InitializeChunkAsync(int32 i, const FMQCGridConfig& ChunkConfig, FGWTAsyncTaskRef& TaskRef)
{
    check(chunks.IsValidIndex(i));

    FMQCGridChunk& chunk(*chunks[i]);
    TaskRef.AddTask([&chunk, ChunkConfig](){ chunk.Initialize(ChunkConfig); });
}

void FMQCMap::InitializeChunks()
{
    check(chunks.Num() == (chunkResolution * chunkResolution));

    for (int32 y=0, i=0; y<chunkResolution; y++)
    for (int32 x=0     ; x<chunkResolution; x++, i++)
    {
        FMQCGridConfig ChunkConfig;
        InitializeChunkSettings(i, x, y, ChunkConfig);
        InitializeChunk(i, ChunkConfig);
    }
}

void FMQCMap::InitializeChunksAsync(FGWTAsyncTaskRef& TaskRef)
{
    check(chunks.Num() == (chunkResolution * chunkResolution));

    for (int32 y=0, i=0; y<chunkResolution; y++)
    for (int32 x=0     ; x<chunkResolution; x++, i++)
    {
        FMQCGridConfig ChunkConfig;
        InitializeChunkSettings(i, x, y, ChunkConfig);
        InitializeChunkAsync(i, ChunkConfig, TaskRef);
    }
}

void FMQCMap::Initialize()
{
    InitializeSettings();
    InitializeChunks();
}

void FMQCMap::InitializeAsync(FGWTAsyncTaskRef& TaskRef)
{
    if (! TaskRef.IsValid())
    {
        //FGWTAsyncTaskRef::Init(
        //    TaskRef,
        //    IProceduralMeshUtility::Get().GetThreadPool());
    }

    check(TaskRef.IsValid());

    InitializeSettings();
    InitializeChunksAsync(TaskRef);
}

void FMQCMap::CopyFrom(const FMQCMap& VoxelMap)
{
    chunkSize = VoxelMap.chunkSize;
    voxelSize = VoxelMap.voxelSize;
    bHasGridData = VoxelMap.bHasGridData;

    // Create uninitialized chunks

    Clear();

    chunks.SetNumUninitialized(chunkResolution * chunkResolution);

    for (int32 y=0, i=0; y<chunkResolution; y++)
    for (int32 x=0     ; x<chunkResolution; x++, i++)
    {
        chunks[i] = new FMQCGridChunk;

        FMQCGridChunk& Chunk(*chunks[i]);

        Chunk.CopyFrom(*VoxelMap.chunks[i]);

        // Link chunk neighbours

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
}

bool FMQCMap::IsPrefabValid(int32 PrefabIndex, int32 LODIndex, int32 SectionIndex) const
{
    if (! HasPrefab(PrefabIndex))
    {
        return false;
    }

    const UStaticMesh* Mesh = meshPrefabs[PrefabIndex];

    if (Mesh->bAllowCPUAccess &&
        Mesh->RenderData      &&
        Mesh->RenderData->LODResources.IsValidIndex(LODIndex) && 
        Mesh->RenderData->LODResources[LODIndex].Sections.IsValidIndex(SectionIndex)
        )
    {
        return true;
    }

    return false;
}

bool FMQCMap::HasIntersectingBounds(const TArray<FBox2D>& Bounds) const
{
    if (Bounds.Num() > 0)
    {
        for (const FPrefabData& PrefabData : AppliedPrefabs)
        {
            const TArray<FBox2D>& AppliedBounds(PrefabData.Bounds);

            for (const FBox2D& Box0 : Bounds)
            for (const FBox2D& Box1 : AppliedBounds)
            {
                if (Box0.Intersect(Box1))
                {
                    // Skip exactly intersecting box

                    if (FMath::IsNearlyEqual(Box0.Min.X, Box1.Max.X, 1.e-3f) ||
                        FMath::IsNearlyEqual(Box1.Min.X, Box0.Max.X, 1.e-3f))
                    {
                        continue;
                    }

                    if (FMath::IsNearlyEqual(Box0.Min.Y, Box1.Max.Y, 1.e-3f) ||
                        FMath::IsNearlyEqual(Box1.Min.Y, Box0.Max.Y, 1.e-3f))
                    {
                        continue;
                    }

                    return true;
                }
            }
        }
    }

    return false;
}

bool FMQCMap::TryPlacePrefabAt(int32 PrefabIndex, const FVector2D& Center)
{
    TArray<FBox2D> Bounds;
    GetPrefabBounds(PrefabIndex, Bounds);

    // Offset prefab bounds towards the specified center location
    for (FBox2D& Box : Bounds)
    {
        Box = Box.ShiftBy(Center);
    }

    if (! HasIntersectingBounds(Bounds))
    {
        AppliedPrefabs.Emplace(Bounds);
        return true;
    }

    return false;
}

void FMQCMap::GetPrefabBounds(int32 PrefabIndex, TArray<FBox2D>& Bounds) const
{
    Bounds.Empty();

    if (! HasPrefab(PrefabIndex))
    {
        return;
    }

    const UStaticMesh& Mesh(*meshPrefabs[PrefabIndex]);
    const TArray<UStaticMeshSocket*>& Sockets(Mesh.Sockets);

    typedef TKeyValuePair<UStaticMeshSocket*, UStaticMeshSocket*> FBoundsPair;
    TArray<FBoundsPair> BoundSockets;

    const FString MIN_PREFIX(TEXT("Bounds_MIN_"));
    const FString MAX_PREFIX(TEXT("Bounds_MAX_"));
    const int32 PREFIX_LEN = MIN_PREFIX.Len();

    for (UStaticMeshSocket* Socket0 : Sockets)
    {
        // Invalid socket, skip
        if (! IsValid(Socket0))
        {
            continue;
        }

        FString MinSocketName(Socket0->SocketName.ToString());
        FString MaxSocketName(MAX_PREFIX);

        // Not a min bounds socket, skip
        if (! MinSocketName.StartsWith(*MIN_PREFIX, ESearchCase::IgnoreCase))
        {
            continue;
        }

        MaxSocketName += MinSocketName.RightChop(PREFIX_LEN);

        for (UStaticMeshSocket* Socket1 : Sockets)
        {
            if (IsValid(Socket1) && Socket1->SocketName.ToString() == MaxSocketName)
            {
                BoundSockets.Emplace(Socket0, Socket1);
                break;
            }
        }
    }

    for (FBoundsPair BoundsPair : BoundSockets)
    {
        FBox2D Box;
        const FVector& Min(BoundsPair.Key->RelativeLocation);
        const FVector& Max(BoundsPair.Value->RelativeLocation);
        const float MinX = FMath::RoundHalfFromZero(Min.X);
        const float MinY = FMath::RoundHalfFromZero(Min.Y);
        const float MaxX = FMath::RoundHalfFromZero(Max.X);
        const float MaxY = FMath::RoundHalfFromZero(Max.Y);
        Box += FVector2D(MinX, MinY);
        Box += FVector2D(MaxX, MaxY);
        Bounds.Emplace(Box);
    }
}

TArray<FBox2D> FMQCMap::GetPrefabBounds(int32 PrefabIndex) const
{
    TArray<FBox2D> Bounds;
    GetPrefabBounds(PrefabIndex, Bounds);
    return Bounds;
}

// ----------------------------------------------------------------------------

// MAP SETTINGS FUNCTIONS

void UMQCMapRef::ApplyMapSettings()
{
    VoxelMap.mapSize = MapSize;
    VoxelMap.voxelResolution = VoxelResolution;
    VoxelMap.chunkResolution = ChunkResolution;
    VoxelMap.extrusionHeight = ExtrusionHeight;

    VoxelMap.maxFeatureAngle = MaxFeatureAngle;
    VoxelMap.maxParallelAngle = MaxParallelAngle;

    VoxelMap.surfaceStates = SurfaceStates;
    VoxelMap.meshPrefabs = MeshPrefabs;
}

// TRIANGULATION FUNCTIONS

void UMQCMapRef::EditMapAsync(FGWTAsyncTaskRef& TaskRef, const TArray<UMQCStencilRef*>& Stencils)
{
    if (! TaskRef.IsValid())
    {
        //FGWTAsyncTaskRef::Init(
        //    TaskRef,
        //    IProceduralMeshUtility::Get().GetThreadPool());
    }

    check(TaskRef.IsValid());

    FPSGWTAsyncTask& Task(TaskRef.Task);

    TArray<UMQCStencilRef*> ValidStencils(
        Stencils.FilterByPredicate(
            [&](UMQCStencilRef* const & Stencil) { return IsValid(Stencil); }
        ) );

    for (int32 i=0; i<ValidStencils.Num(); ++i)
    {
        UMQCStencilRef* Stencil(ValidStencils[i]);

        check(IsValid(Stencil));

        Stencil->EditMapAsync(Task, VoxelMap);
    }
}

void UMQCMapRef::EditStatesAsync(FGWTAsyncTaskRef& TaskRef, const TArray<UMQCStencilRef*>& Stencils)
{
    if (! TaskRef.IsValid())
    {
        //FGWTAsyncTaskRef::Init(
        //    TaskRef,
        //    IProceduralMeshUtility::Get().GetThreadPool());
    }

    check(TaskRef.IsValid());

    FPSGWTAsyncTask& Task(TaskRef.Task);

    TArray<UMQCStencilRef*> ValidStencils(
        Stencils.FilterByPredicate(
            [&](UMQCStencilRef* const & Stencil) { return IsValid(Stencil); }
        ) );

    for (int32 i=0; i<ValidStencils.Num(); ++i)
    {
        UMQCStencilRef* Stencil(ValidStencils[i]);

        check(IsValid(Stencil));

        //Stencil->EditStatesAsync(Task, VoxelMap);

        Task->AddTask(
            [this, Stencil]()
            {
                Stencil->EditStates(VoxelMap);
            } );
    }
}

void UMQCMapRef::EditCrossingsAsync(FGWTAsyncTaskRef& TaskRef, const TArray<UMQCStencilRef*>& Stencils)
{
    if (! TaskRef.IsValid())
    {
        //FGWTAsyncTaskRef::Init(
        //    TaskRef,
        //    IProceduralMeshUtility::Get().GetThreadPool());
    }

    check(TaskRef.IsValid());

    FPSGWTAsyncTask& Task(TaskRef.Task);

    TArray<UMQCStencilRef*> ValidStencils(
        Stencils.FilterByPredicate(
            [&](UMQCStencilRef* const & Stencil) { return IsValid(Stencil); }
        ) );

    for (int32 i=0; i<ValidStencils.Num(); ++i)
    {
        UMQCStencilRef* Stencil(ValidStencils[i]);

        check(IsValid(Stencil));

        //Stencil->EditCrossingsAsync(Task, VoxelMap);

        Task->AddTask(
            [this, Stencil]()
            {
                Stencil->EditCrossings(VoxelMap);
            } );
    }
}

UMQCMapRef* UMQCMapRef::Copy(UObject* Outer) const
{
    UMQCMapRef* MapCopy = NewObject<UMQCMapRef>(Outer);

    MapCopy->MapSize = MapSize;
	MapCopy->VoxelResolution = VoxelResolution;
	MapCopy->ChunkResolution = ChunkResolution;
    MapCopy->SurfaceStates = SurfaceStates;
	MapCopy->ExtrusionHeight = ExtrusionHeight;
	MapCopy->MaxFeatureAngle = MaxFeatureAngle;
	MapCopy->MaxParallelAngle = MaxParallelAngle;
    MapCopy->MeshPrefabs = MeshPrefabs;

    if (IsInitialized())
    {
        MapCopy->ApplyMapSettings();
        MapCopy->VoxelMap.CopyFrom(VoxelMap);
    }

    return MapCopy;
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
        ? FVector(VoxelMap.GetChunk(ChunkIndex).position, 0.f)
        : FVector();
}

FPMUMeshSection UMQCMapRef::GetSection(int32 ChunkIndex, int32 StateIndex) const
{
    if (HasChunk(ChunkIndex))
    {
        const FPMUMeshSection* Section = VoxelMap.GetChunk(ChunkIndex).GetSection(StateIndex);
        return Section ? *Section : FPMUMeshSection();
    }

    return FPMUMeshSection();
}

// PREFAB FUNCTIONS

TArray<FBox2D> UMQCMapRef::GetPrefabBounds(int32 PrefabIndex) const
{
    return IsInitialized()
        ? VoxelMap.GetPrefabBounds(PrefabIndex)
        : TArray<FBox2D>();
}

bool UMQCMapRef::HasPrefab(int32 PrefabIndex) const
{
    return IsInitialized() ? VoxelMap.HasPrefab(PrefabIndex) : false;
}
