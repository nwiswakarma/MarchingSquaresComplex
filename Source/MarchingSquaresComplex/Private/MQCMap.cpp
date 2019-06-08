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
}

void FMQCMap::TriangulateAsync()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        Chunk->TriangulateAsync();
    }
}

void FMQCMap::ResolveChunkEdgeData()
{
    for (int32 i=0; i<SurfaceStates.Num(); ++i)
    {
        ResolveChunkEdgeData(i+1);
    }
}

void FMQCMap::ResolveChunkEdgeData(int32 StateIndex)
{
    //UE_LOG(LogTemp,Warning, TEXT("StateIndex: %d"), StateIndex);

    TArray<FEdgeSyncData> ChunkSyncList;
    TArray<TDoubleLinkedList<FEdgeSyncData>> SyncLists;

    for (int32 ChunkIndex=0; ChunkIndex<chunks.Num(); ++ChunkIndex)
    {
        FMQCGridChunk& Chunk(*chunks[ChunkIndex]);

        int32 SyncOffetIndex = Chunk.AppendEdgeSyncData(StateIndex, ChunkSyncList);

        for (int32 i=SyncOffetIndex; i<ChunkSyncList.Num(); ++i)
        {
            FEdgeSyncData& SyncData(ChunkSyncList[i]);
            SyncData.ChunkIndex = ChunkIndex;

            //UE_LOG(LogTemp,Warning, TEXT("SyncData[%d]: %s"),
            //    i, *SyncData.ToString());
        }
    }

    while (ChunkSyncList.Num() > 0)
    {
        SyncLists.SetNum(SyncLists.Num()+1);
        TDoubleLinkedList<FEdgeSyncData>& SyncList(SyncLists.Last());

        SyncList.AddTail(ChunkSyncList[0]);

        ChunkSyncList.RemoveAtSwap(0, 1, false);

        int32 i = 0;

        while (i < ChunkSyncList.Num())
        {
            const FEdgeSyncData& HeadSyncData(SyncList.GetHead()->GetValue());
            const FEdgeSyncData& TailSyncData(SyncList.GetTail()->GetValue());
            FVector2D HeadPos = HeadSyncData.HeadPos;
            FVector2D TailPos = TailSyncData.TailPos;

            const FEdgeSyncData& SyncData(ChunkSyncList[i]);
            FVector2D a = SyncData.HeadPos;
            FVector2D b = SyncData.TailPos;

            bool bHasConnection = false;

            if (a.Equals(HeadPos, .001f))
            {
                SyncList.AddHead(SyncData);
                bHasConnection = true;
            }
            else
            if (a.Equals(TailPos, .001f))
            {
                SyncList.AddTail(SyncData);
                bHasConnection = true;
            }
            else
            if (b.Equals(HeadPos, .001f))
            {
                SyncList.AddHead(SyncData);
                bHasConnection = true;
            }
            else
            if (b.Equals(TailPos, .001f))
            {
                SyncList.AddTail(SyncData);
                bHasConnection = true;
            }

            if (bHasConnection)
            {
                ChunkSyncList.RemoveAtSwap(i, 1, false);
                i = 0;
                continue;
            }

            ++i;
        }
    }

    //UE_LOG(LogTemp,Warning, TEXT("SyncLists.Num(): %d"), SyncLists.Num());

    for (int32 ListIndex=0; ListIndex<SyncLists.Num(); ++ListIndex)
    {
        const TDoubleLinkedList<FEdgeSyncData>& SyncList(SyncLists[ListIndex]);

        // Skip edge list that does not have any connection
        if (SyncList.Num() < 2)
        {
            continue;
        }

        int32 j=0;
        float Length = 0.f;
        float LengthInv = 0.f;

        for (const FEdgeSyncData& SyncData : SyncList)
        {
            //UE_LOG(LogTemp,Warning, TEXT("SyncLists[%d] SyncData[%d]: %s"),
            //    ListIndex, j++, *SyncData.ToString());
            Length += SyncData.Length;
        }

        if (Length > 0.f)
        {
            LengthInv = 1.f/Length;
        }

        //UE_LOG(LogTemp,Warning, TEXT("SyncLists[%d] Total Length: %f (%f)"),
        //    ListIndex, Length, LengthInv);

        float UV0 = 0.f;
        float UV1 = 0.f;

        for (const FEdgeSyncData& SyncData : SyncList)
        {
            UV0 = UV1;
            UV1 = UV0 + SyncData.Length * LengthInv;

            FMQCGridChunk& Chunk(*chunks[SyncData.ChunkIndex]);
            Chunk.RemapEdgeUVs(StateIndex, SyncData.EdgeListIndex, UV0, UV1);
        }
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

void FMQCMap::WaitForAsyncTask()
{
    for (FMQCGridChunk* Chunk : chunks)
    {
        Chunk->WaitForAsyncTask();
    }
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

    FVector2D chunkPosition(x * voxelResolution, y * voxelResolution);

    ChunkConfig.States = SurfaceStates;
    ChunkConfig.Position = chunkPosition;
    ChunkConfig.MapSize = GetVoxelCount();
    ChunkConfig.VoxelResolution = voxelResolution;
    ChunkConfig.MaxFeatureAngle = MaxFeatureAngle;
    ChunkConfig.MaxParallelAngle = MaxParallelAngle;
    ChunkConfig.ExtrusionHeight = extrusionHeight;

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
    chunks[i]->Initialize(ChunkConfig);
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

void FMQCMap::CopyFrom(const FMQCMap& VoxelMap)
{
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
    VoxelMap.voxelResolution = VoxelResolution;
    VoxelMap.chunkResolution = ChunkResolution;
    VoxelMap.extrusionHeight = ExtrusionHeight;

    VoxelMap.MaxFeatureAngle = MaxFeatureAngle;
    VoxelMap.MaxParallelAngle = MaxParallelAngle;

    VoxelMap.SurfaceStates = SurfaceStates;
    VoxelMap.meshPrefabs = MeshPrefabs;
}

// TRIANGULATION FUNCTIONS


UMQCMapRef* UMQCMapRef::Copy(UObject* Outer) const
{
    UMQCMapRef* MapCopy = NewObject<UMQCMapRef>(Outer);

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
