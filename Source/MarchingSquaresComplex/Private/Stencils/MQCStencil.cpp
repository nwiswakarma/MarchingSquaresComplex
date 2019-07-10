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

#include "MQCStencil.h"
#include "MQCGridChunk.h"
#include "MQCVoxel.h"

void FMQCStencil::ValidateNormalX(FMQCVoxel& xMin, const FMQCVoxel& xMax)
{
    if (xMin.voxelState < xMax.voxelState)
    {
        if (xMin.xNormal.X > 0.f)
        {
            xMin.xNormal = -xMin.xNormal;
        }
    }
    else if (xMin.xNormal.X < 0.f)
    {
        xMin.xNormal = -xMin.xNormal;
    }
}

void FMQCStencil::ValidateNormalY(FMQCVoxel& yMin, const FMQCVoxel& yMax)
{
    if (yMin.voxelState < yMax.voxelState)
    {
        if (yMin.yNormal.Y > 0.f)
        {
            yMin.yNormal = -yMin.yNormal;
        }
    }
    else if (yMin.yNormal.Y < 0.f)
    {
        yMin.yNormal = -yMin.yNormal;
    }
}

void FMQCStencil::GetOffsetBounds(float& X0, float& X1, float& Y0, float& Y1, const FVector2D& Offset) const
{
    X0 = GetXStart() - Offset.X;
    X1 = GetXEnd()   - Offset.X;
    Y0 = GetYStart() - Offset.Y;
    Y1 = GetYEnd()   - Offset.Y;
}

void FMQCStencil::GetMapRange(int32& x0, int32& x1, int32& y0, int32& y1, const int32 voxelResolution, const int32 chunkResolution) const
{
    x0 = (int32)(GetXStart()) / voxelResolution;
    if (x0 < 0)
    {
        x0 = 0;
    }

    x1 = (int32)(GetXEnd()) / voxelResolution;
    if (x1 >= chunkResolution)
    {
        x1 = chunkResolution - 1;
    }

    y0 = (int32)(GetYStart()) / voxelResolution;
    if (y0 < 0)
    {
        y0 = 0;
    }

    y1 = (int32)(GetYEnd()) / voxelResolution;
    if (y1 >= chunkResolution)
    {
        y1 = chunkResolution - 1;
    }
}

void FMQCStencil::GetMapRange(TArray<int32>& ChunkIndices, const int32 voxelResolution, const int32 chunkResolution) const
{
    int32 X0, X1, Y0, Y1;
    GetMapRange(X0, X1, Y0, Y1, voxelResolution, chunkResolution);

    for (int32 y=Y1; y>=Y0; y--)
    {
        int32 i = y * chunkResolution + X1;

        for (int32 x=X1; x>=X0; x--, i--)
        {
            ChunkIndices.Emplace(i);
        }
    }
}

void FMQCStencil::GetChunkRange(int32& x0, int32& x1, int32& y0, int32& y1, const FMQCGridChunk& Chunk) const
{
    const int32 resolution = Chunk.voxelResolution;
    const FIntPoint Offset = Chunk.position;

    x0 = (int32)(GetXStart()) - Offset.X;
    if (x0 < 0)
    {
        x0 = 0;
    }

    x1 = (int32)(GetXEnd()) - Offset.X;
    if (x1 >= resolution)
    {
        x1 = resolution - 1;
    }

    y0 = (int32)(GetYStart()) - Offset.Y;
    if (y0 < 0)
    {
        y0 = 0;
    }

    y1 = (int32)(GetYEnd()) - Offset.Y;
    if (y1 >= resolution)
    {
        y1 = resolution - 1;
    }
}

void FMQCStencil::GetChunks(TArray<FMQCGridChunk*>& Chunks, FMQCMap& Map, const TArray<int32>& ChunkIndices) const
{
    Chunks.Reset(ChunkIndices.Num());

    for (int32 i : ChunkIndices)
    {
        Chunks.Emplace(Map.chunks[i]);
    }
}

void FMQCStencil::SetCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FIntPoint& ChunkOffset) const
{
    if (xMin.voxelState != xMax.voxelState)
    {
        FindCrossingX(xMin, xMax, ChunkOffset);
    }
    else
    {
        xMin.InvalidateEdgeX();
    }
}

void FMQCStencil::SetCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FIntPoint& ChunkOffset) const
{
    if (yMin.voxelState != yMax.voxelState)
    {
        FindCrossingY(yMin, yMax, ChunkOffset);
    }
    else
    {
        yMin.InvalidateEdgeY();
    }
}

FMQCMaterial FMQCStencil::GetMaterialFor(const FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
{
    return FMQCMaterial::Zero;
}

void FMQCStencil::GetMaterialBlendTyped(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const
{
    switch (MaterialType)
    {
        case EMQCMaterialType::MT_COLOR:
            GetMaterialBlendColor(OutMaterial, BaseMaterial, BlendAlpha);
            break;

        case EMQCMaterialType::MT_SINGLE_INDEX:
            GetMaterialBlendSingleIndex(OutMaterial, BaseMaterial, BlendAlpha);
            break;

        case EMQCMaterialType::MT_DOUBLE_INDEX:
            GetMaterialBlendDoubleIndex(OutMaterial, BaseMaterial, BlendAlpha);
            break;

        case EMQCMaterialType::MT_TRIPLE_INDEX:
            GetMaterialBlendTripleIndex(OutMaterial, BaseMaterial, BlendAlpha);
            break;
    }
}

void FMQCStencil::GetMaterialBlendColor(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const
{
    FLinearColor BaseColor = BaseMaterial.ToFColor().ReinterpretAsLinear();
    FLinearColor StencilColor = Material.ToFColor().ReinterpretAsLinear();
    FLinearColor LinearBlend = FMath::Lerp(BaseColor, StencilColor, BlendAlpha);
    FColor ColorBlend = LinearBlend.ToFColor(false);
    OutMaterial.SetR(ColorBlend.R);
    OutMaterial.SetG(ColorBlend.G);
    OutMaterial.SetB(ColorBlend.B);
    OutMaterial.SetA(ColorBlend.A);
}

void FMQCStencil::GetMaterialBlendSingleIndex(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const
{
}

void FMQCStencil::GetMaterialBlendDoubleIndex(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const
{
    uint8 TargetIndex = Material.GetIndex();
    uint8 TargetBlend = UMQCMaterialUtility::LerpUINT8(0, 255, BlendAlpha);
    uint8 BaseBlend = BaseMaterial.GetBlend();

    OutMaterial = BaseMaterial;

    // Zero target blend, skip further material assignment
    if (TargetBlend == 0)
    {
        return;
    }

    uint8 IndexA = BaseMaterial.GetIndexA();
    uint8 IndexB = BaseMaterial.GetIndexB();

    // Base material does not have the target index, assign target index
    if (TargetIndex != IndexA && TargetIndex != IndexB)
    {
        if (TargetBlend < 255)
        {
            // Index A dominant, assign target index as material index B
            if (BaseBlend < 128)
            {
                OutMaterial.SetIndexB(TargetIndex);
                OutMaterial.SetBlend(TargetBlend);
            }
            // Index B dominant, assign target index as material index A
            else
            {
                OutMaterial.SetIndexA(TargetIndex);
                OutMaterial.SetBlend(255-TargetBlend);
            }
        }
        // Max target blend, assign single index material
        else
        {
            OutMaterial.SetIndexA(TargetIndex);
            OutMaterial.SetIndexB(TargetIndex);
            OutMaterial.SetBlend(0);
        }
    }
    // Base material have the target index, blend target index
    else
    {
        bool bInverseBlendAlpha = (TargetIndex == IndexA);

        if (bInverseBlendAlpha)
        {
            TargetBlend = 255-TargetBlend;
        }

        switch (MaterialBlendType)
        {
            case EMQCMaterialBlendType::MBT_DEFAULT:
            case EMQCMaterialBlendType::MBT_MAX:
                TargetBlend = bInverseBlendAlpha
                    ? FMath::Min(BaseBlend, TargetBlend)
                    : FMath::Max(BaseBlend, TargetBlend);
                break;

            case EMQCMaterialBlendType::MBT_LERP:
                TargetBlend = UMQCMaterialUtility::LerpUINT8(BaseBlend, TargetBlend, BlendAlpha);
                break;

            case EMQCMaterialBlendType::MBT_COPY:
            default:
                break;
        }

        OutMaterial.SetBlend(TargetBlend);
    }

    OutMaterial.SortDoubleIndex();
}

void FMQCStencil::GetMaterialBlendTripleIndex(FMQCMaterial& OutMaterial, const FMQCMaterial& BaseMaterial, float BlendAlpha) const
{
    uint8 TargetIndex = Material.GetIndex();
    uint8 TargetBlend = UMQCMaterialUtility::LerpUINT8(0, 255, BlendAlpha);

    uint8 Index0 = BaseMaterial.GetIndex0();
    uint8 Index1 = BaseMaterial.GetIndex1();
    uint8 Index2 = BaseMaterial.GetIndex2();

    uint8 Blend0 = BaseMaterial.GetBlend0();
    uint8 Blend1 = BaseMaterial.GetBlend1();
    uint8 Blend2 = BaseMaterial.GetBlend2();

    OutMaterial = BaseMaterial;

    // Update existing index
    if (BaseMaterial.HasIndexAsTriple(TargetIndex))
    {
        uint8 MatchingIndex = 255;
        uint8 OldBlend = 255;

        // Find matching index and old blend

        if (Index0 == TargetIndex)
        {
            MatchingIndex = 0;
            OldBlend = Blend0;
        }
        else
        if (Index1 == TargetIndex)
        {
            MatchingIndex = 1;
            OldBlend = Blend1;
        }
        else
        {
            MatchingIndex = 2;
            OldBlend = Blend2;
        }

        // Calculate target blending
        switch (MaterialBlendType)
        {
            case EMQCMaterialBlendType::MBT_DEFAULT:
            case EMQCMaterialBlendType::MBT_MAX:
                TargetBlend = FMath::Max(OldBlend, TargetBlend);
                break;

            case EMQCMaterialBlendType::MBT_LERP:
                TargetBlend = UMQCMaterialUtility::LerpUINT8(
                    OldBlend,
                    TargetBlend,
                    BlendAlpha
                    );
                break;

            case EMQCMaterialBlendType::MBT_COPY:
            default:
                break;
        }

        // Assign target blend
        switch (MatchingIndex)
        {
            case 0: OutMaterial.SetBlend0(TargetBlend); break;
            case 1: OutMaterial.SetBlend1(TargetBlend); break;
            case 2: OutMaterial.SetBlend2(TargetBlend); break;

            default:
                // Check no-entry
                check(false);
                break;
        }
    }
    // Add new index
    else
    if (TargetBlend > 0)
    {
        // Add to single index material
        if (Index0 == Index1)
        {
            check(Index1 == Index2);

            // Replace zero influence Index0
            if (Blend0 == 0)
            {
                Index0 = TargetIndex;
                Index1 = TargetIndex;
                Index2 = TargetIndex;

                Blend0 = TargetBlend;
                Blend1 = 0;
                Blend2 = 0;
            }
            // Add depending on index order
            else
            if (Index0 > TargetIndex)
            {
                Index1 = Index0;
                Index2 = Index0;
                Blend1 = Blend0;
                Blend2 = 0;

                Index0 = TargetIndex;
                Blend0 = TargetBlend;
            }
            else
            {
                Index1 = TargetIndex;
                Index2 = TargetIndex;
                Blend1 = TargetBlend;
                Blend2 = 0;
            }
        }
        // Add to double index material
        else
        if (Index1 == Index2)
        {
            Index2 = TargetIndex;
            Blend2 = TargetBlend;
        }
        // Triple index material, replace least significant index
        else
        {
            // Replace Index2
            if (Blend2 <= Blend1 && Blend2 <= Blend0)
            {
                Index2 = TargetIndex;
                Blend2 = TargetBlend;
            }
            // Replace Index1
            else
            if (Blend1 <= Blend2 && Blend1 <= Blend0)
            {
                Index1 = TargetIndex;
                Blend1 = TargetBlend;
            }
            // Replace Index0
            else
            if (Blend0 <= Blend2 && Blend0 <= Blend1)
            {
                Index0 = TargetIndex;
                Blend0 = TargetBlend;
            }
            // Otherwise, replace last index (Index2)
            else
            {
                Index2 = TargetIndex;
                Blend2 = TargetBlend;
            }
        }

        // Assign output
        OutMaterial.SetIndex0(Index0);
        OutMaterial.SetIndex1(Index1);
        OutMaterial.SetIndex2(Index2);
        OutMaterial.SetBlend0(Blend0);
        OutMaterial.SetBlend1(Blend1);
        OutMaterial.SetBlend2(Blend2);

        // Clear zero influence index
        UMQCMaterialUtility::ClearZeroInfluence(OutMaterial);
    }

    // Sort index order if required
    if (OutMaterial.IsTripleIndexSortRequired())
    {
        OutMaterial.SortTripleIndex();
    }
}

void FMQCStencil::Initialize(const FMQCMap& VoxelMap)
{
    fillType = FillTypeSetting;
    Material = MaterialSetting;
    MaterialBlendType = MaterialBlendSetting;
    MaterialType = VoxelMap.GetMaterialType();
}

void FMQCStencil::EditMap(FMQCMap& Map, const FVector2D& center)
{
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;

    TArray<int32> ChunkIndices;
    TArray<FMQCGridChunk*> Chunks;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(ChunkIndices, voxelResolution, chunkResolution);
    GetChunks(Chunks, Map, ChunkIndices);

    SetVoxels(Chunks);
    SetCrossings(Chunks);
}

void FMQCStencil::EditMaterial(FMQCMap& Map, const FVector2D& center)
{
    const int32 voxelResolution = Map.voxelResolution;
    const int32 chunkResolution = Map.chunkResolution;

    TArray<int32> ChunkIndices;
    TArray<FMQCGridChunk*> Chunks;

    Initialize(Map);
    SetCenter(center.X, center.Y);
    GetMapRange(ChunkIndices, voxelResolution, chunkResolution);
    GetChunks(Chunks, Map, ChunkIndices);

    SetMaterials(Chunks);
}

void FMQCStencil::SetVoxels(FMQCGridChunk& Chunk)
{
    int32 X0, X1, Y0, Y1;
    GetChunkRange(X0, X1, Y0, Y1, Chunk);
    Chunk.SetStates(*this, X0, X1, Y0, Y1);
}

void FMQCStencil::SetVoxels(const TArray<FMQCGridChunk*>& Chunks)
{
    if (bEnableAsync)
    {
        FMQCStencil* Stencil(this);
        // ParallelFor() is used instead of SetStatesAsync() because
        // most other stencil operations rely on synchronized states result
        ParallelFor(
            Chunks.Num(),
            [Stencil, &Chunks](int32 ThreadIndex)
            {
                FMQCGridChunk* Chunk(Chunks[ThreadIndex]);
                int32 X0, X1, Y0, Y1;
                Stencil->GetChunkRange(X0, X1, Y0, Y1, *Chunk);
                Chunk->SetStates(*Stencil, X0, X1, Y0, Y1);
            } );
    }
    else
    {
        for (FMQCGridChunk* Chunk : Chunks)
        {
            int32 X0, X1, Y0, Y1;
            GetChunkRange(X0, X1, Y0, Y1, *Chunk);
            Chunk->SetStates(*this, X0, X1, Y0, Y1);
        }
    }
}

void FMQCStencil::SetCrossings(FMQCGridChunk& Chunk)
{
    int32 X0, X1, Y0, Y1;
    GetChunkRange(X0, X1, Y0, Y1, Chunk);
    Chunk.SetCrossings(*this, X0, X1, Y0, Y1);
}

void FMQCStencil::SetCrossings(const TArray<FMQCGridChunk*>& Chunks)
{
    for (FMQCGridChunk* Chunk : Chunks)
    {
        int32 X0, X1, Y0, Y1;
        GetChunkRange(X0, X1, Y0, Y1, *Chunk);

        if (bEnableAsync)
        {
            Chunk->SetCrossingsAsync(*this, X0, X1, Y0, Y1);
        }
        else
        {
            Chunk->SetCrossings(*this, X0, X1, Y0, Y1);
        }
    }
}

void FMQCStencil::SetMaterials(FMQCGridChunk& Chunk)
{
    int32 X0, X1, Y0, Y1;
    GetChunkRange(X0, X1, Y0, Y1, Chunk);
    Chunk.SetMaterials(*this, X0, X1, Y0, Y1);
}

void FMQCStencil::SetMaterials(const TArray<FMQCGridChunk*>& Chunks)
{
    for (FMQCGridChunk* Chunk : Chunks)
    {
        int32 X0, X1, Y0, Y1;
        GetChunkRange(X0, X1, Y0, Y1, *Chunk);

        if (bEnableAsync)
        {
            Chunk->SetMaterialsAsync(*this, X0, X1, Y0, Y1);
        }
        else
        {
            Chunk->SetMaterials(*this, X0, X1, Y0, Y1);
        }
    }
}

void FMQCStencil::ApplyVoxel(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
{
    const FVector2D& p(Voxel.position);

    float X0, X1, Y0, Y1;
    GetOffsetBounds(X0, X1, Y0, Y1, ChunkOffset);

    if (p.X >= X0 && p.X <= X1 && p.Y >= Y0 && p.Y <= Y1)
    {
        Voxel.voxelState = fillType;
    }
}

void FMQCStencil::ApplyMaterial(FMQCVoxel& Voxel, const FIntPoint& ChunkOffset) const
{
    const FVector2D& p(Voxel.position);

    float X0, X1, Y0, Y1;
    GetOffsetBounds(X0, X1, Y0, Y1, ChunkOffset);

    if (p.X >= X0 && p.X <= X1 && p.Y >= Y0 && p.Y <= Y1)
    {
        Voxel.Material = Material;
    }
}

void UMQCStencilRef::EditMap(UMQCMapRef* MapRef)
{
    if (IsValid(MapRef))
    {
        EditMapAt(MapRef, MapRef->GetCenter());
    }
}

void UMQCStencilRef::EditMaterial(UMQCMapRef* MapRef)
{
    if (IsValid(MapRef))
    {
        EditMaterialAt(MapRef, MapRef->GetCenter());
    }
}
