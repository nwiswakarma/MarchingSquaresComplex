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
    const FIntPoint Offset = Chunk.GetOffsetId();

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

void FMQCStencil::SetCrossingX(FMQCVoxel& xMin, const FMQCVoxel& xMax, const FVector2D& ChunkOffset) const
{
    if (xMin.voxelState != xMax.voxelState)
    {
        FindCrossingX(xMin, xMax, ChunkOffset);
    }
    else
    {
        xMin.xEdge = -1.f;
    }
}

void FMQCStencil::SetCrossingY(FMQCVoxel& yMin, const FMQCVoxel& yMax, const FVector2D& ChunkOffset) const
{
    if (yMin.voxelState != yMax.voxelState)
    {
        FindCrossingY(yMin, yMax, ChunkOffset);
    }
    else
    {
        yMin.yEdge = -1.f;
    }
}

FMQCMaterial FMQCStencil::GetMaterialFor(const FMQCVoxel& Voxel, const FVector2D& ChunkOffset) const
{
    return FMQCMaterial();
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

    // Triple index is not required or fully opaque target blend, use double index
    if (! BaseMaterial.IsTripleIndexRequiredFor(TargetIndex) || TargetBlend == 255)
    {
        GetMaterialBlendDoubleIndex(OutMaterial, BaseMaterial, BlendAlpha);
        return;
    }

    const bool bBaseIsTriple = BaseMaterial.IsMarkedAsTripledIndex();

    OutMaterial = BaseMaterial;

    // Base material does not have the target index, assign target index
    if (! BaseMaterial.HasIndexAsTriple(TargetIndex))
    {
        if (bBaseIsTriple)
        {
        }
        else
        {
            uint8 IndexA = BaseMaterial.GetIndexA();
            uint8 IndexB = BaseMaterial.GetIndexB();
            uint8 Blend  = BaseMaterial.GetBlend();

            check(IndexA <= IndexB);

            OutMaterial = FMQCMaterial();

            float B0F = (255-Blend);
            float B1F = Blend;
            float B2F = TargetBlend;

            B0F /= 255.f;
            B1F /= 255.f;
            B2F /= 255.f;

            float Sum = B0F + B1F + B2F;
            float SumInv = (Sum > 0.f) ? (1.f/Sum) : 0.f;

            uint8 B0 = UMQCMaterialUtility::AlphaToUINT8(B0F*SumInv);
            uint8 B1 = UMQCMaterialUtility::AlphaToUINT8(B1F*SumInv);
            uint8 B2 = UMQCMaterialUtility::AlphaToUINT8(B2F*SumInv);

            if (TargetIndex > IndexB)
            {
                OutMaterial.SetIndex0(IndexA);
                OutMaterial.SetIndex1(IndexB);
                OutMaterial.SetIndex2(TargetIndex);

                //OutMaterial.SetBlend01(Blend);
                //OutMaterial.SetBlend12(TargetBlend);

                //OutMaterial.SetBlend0(255-Blend);
                //OutMaterial.SetBlend1(Blend);
                //OutMaterial.SetBlend2(TargetBlend);

                OutMaterial.SetBlend0(B0);
                OutMaterial.SetBlend1(B1);
                OutMaterial.SetBlend2(B2);
            }
            else
            if (TargetIndex > IndexA)
            {
                OutMaterial.SetIndex0(IndexA);
                OutMaterial.SetIndex1(TargetIndex);
                OutMaterial.SetIndex2(IndexB);

                //OutMaterial.SetBlend01(TargetBlend);
                //OutMaterial.SetBlend12(Blend);
                //OutMaterial.SetBlend12(UMQCMaterialUtility::LerpUINT8(Blend, 0, TargetBlend/255.f));

                OutMaterial.SetBlend0(B0);
                OutMaterial.SetBlend1(B2);
                OutMaterial.SetBlend2(B1);
            }
            else
            {
                OutMaterial.SetIndex0(TargetIndex);
                OutMaterial.SetIndex1(IndexA);
                OutMaterial.SetIndex2(IndexB);

                //OutMaterial.SetBlend01(255-TargetBlend);
                //OutMaterial.SetBlend12(Blend);

                OutMaterial.SetBlend0(B2);
                OutMaterial.SetBlend1(B0);
                OutMaterial.SetBlend2(B1);
            }

            OutMaterial.MarkAsTripleIndex();
        }
    }
#if 0
    // Base material have the target index, blend target index
    else
    {
        bool bInverseBlendAlpha = (TargetIndex == BaseMaterial.GetIndexA());

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
#endif

    OutMaterial.SortTripleIndex();
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

void FMQCStencil::ApplyVoxel(FMQCVoxel& Voxel, const FVector2D& ChunkOffset) const
{
    const FVector2D& p(Voxel.position);

    float X0, X1, Y0, Y1;
    GetOffsetBounds(X0, X1, Y0, Y1, ChunkOffset);

    if (p.X >= X0 && p.X <= X1 && p.Y >= Y0 && p.Y <= Y1)
    {
        Voxel.voxelState = fillType;
    }
}

void FMQCStencil::ApplyMaterial(FMQCVoxel& Voxel, const FVector2D& ChunkOffset) const
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
