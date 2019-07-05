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

#include "MQCMaterialUtility.h"

// Globals 

MARCHINGSQUARESCOMPLEX_API const FMQCMaterial FMQCMaterial::Zero(0, 0, 0, 0, 0, 0);
MARCHINGSQUARESCOMPLEX_API const FMQCMaterial FMQCMaterial::Opaque(255, 255, 255, 255, 255, 255);
MARCHINGSQUARESCOMPLEX_API const FMQCMaterial FMQCMaterial::BlendOpaque(255, 255, 255, 0, 0, 0);

// UMQCMaterialUtility

void UMQCMaterialUtility::FindDoubleIndexFaceBlend(
    const FMQCMaterial VertexMaterials[3],
    FMQCMaterialBlend& FaceMaterial,
    uint8 MaterialBlends[3]
    )
{
    const FMQCMaterial& VertexMatA(VertexMaterials[0]);
    const FMQCMaterial& VertexMatB(VertexMaterials[1]);
    const FMQCMaterial& VertexMatC(VertexMaterials[2]);

    // Find material index pair for tri face and blend alphas for each vertex

    FMQCDoubleIndexBlend FaceIndexBlend;
    uint8 BlendA;
    uint8 BlendB;
    uint8 BlendC;

    {
        FMQCDoubleIndexBlend MaterialA(VertexMatA);
        FMQCDoubleIndexBlend MaterialB(VertexMatB);
        FMQCDoubleIndexBlend MaterialC(VertexMatC);

        FaceIndexBlend = FindDoubleIndexBlend(
            MaterialA,
            MaterialB,
            MaterialC
            );

        BlendA = MaterialA.GetBlendFor(FaceIndexBlend);
        BlendB = MaterialB.GetBlendFor(FaceIndexBlend);
        BlendC = MaterialC.GetBlendFor(FaceIndexBlend);
    }

    // Generate blend material

    if (BlendA == 0 && BlendB == 0 && BlendC == 0)
    {
        FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.IndexA);
    }
    else
    if (BlendA == 255 && BlendB == 255 && BlendC == 255)
    {
        FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.IndexB);
    }
    else
    {
        uint8 IndexMin = FaceIndexBlend.IndexA;
        uint8 IndexMax = FaceIndexBlend.IndexB;

        // Make sure index order is not reversed
        check(IndexMin <= IndexMax);

        FaceMaterial = (IndexMin == IndexMax)
            ? FMQCMaterialBlend(IndexMin)
            : FMQCMaterialBlend(IndexMin, IndexMax);
    }

    // Assign material blend

    MaterialBlends[0] = BlendA;
    MaterialBlends[1] = BlendB;
    MaterialBlends[2] = BlendC;
}

void UMQCMaterialUtility::FindTripleIndexFaceBlend(
    const FMQCMaterial VertexMaterials[3],
    FMQCMaterialBlend& FaceMaterial,
    uint8 MaterialBlends0[3],
    uint8 MaterialBlends1[3],
    uint8 MaterialBlends2[3]
    )
{
    const FMQCMaterial& VertexMatA(VertexMaterials[0]);
    const FMQCMaterial& VertexMatB(VertexMaterials[1]);
    const FMQCMaterial& VertexMatC(VertexMaterials[2]);

    // Find material index set for tri face and blend alphas for each vertex

    FMQCTripleIndexBlend FaceIndexBlend;
    uint8 Blends0[3];
    uint8 Blends1[3];
    uint8 Blends2[3];

    {
        FMQCTripleIndexBlend MaterialA(VertexMatA);
        FMQCTripleIndexBlend MaterialB(VertexMatB);
        FMQCTripleIndexBlend MaterialC(VertexMatC);

        // Generate face material index blend

        FaceIndexBlend = FindTripleIndexBlend(
            MaterialA,
            MaterialB,
            MaterialC
            );

        // Generate vertex blend amounts

        FMQCTripleIndexBlend BlendA = MaterialA.GetBlendFor(FaceIndexBlend);
        FMQCTripleIndexBlend BlendB = MaterialB.GetBlendFor(FaceIndexBlend);
        FMQCTripleIndexBlend BlendC = MaterialC.GetBlendFor(FaceIndexBlend);

        Blends0[0] = BlendA.Blend0;
        Blends0[1] = BlendB.Blend0;
        Blends0[2] = BlendC.Blend0;

        Blends1[0] = BlendA.Blend1;
        Blends1[1] = BlendB.Blend1;
        Blends1[2] = BlendC.Blend1;

        Blends2[0] = BlendA.Blend2;
        Blends2[1] = BlendB.Blend2;
        Blends2[2] = BlendC.Blend2;
    }

    int32 FaceIndexCount = FaceIndexBlend.GetIndexCount();

    // Triple index material id
    if (FaceIndexCount == 3)
    {
        // Make sure all material index are unique
        check(FaceIndexBlend.Index0 != FaceIndexBlend.Index1);
        check(FaceIndexBlend.Index0 != FaceIndexBlend.Index2);
        check(FaceIndexBlend.Index1 != FaceIndexBlend.Index2);

        FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index0, FaceIndexBlend.Index1, FaceIndexBlend.Index2);
    }
    // Double index material id
    else
    if (FaceIndexCount == 2)
    {
        // Make sure all material index are unique
        check(FaceIndexBlend.Index0 != FaceIndexBlend.Index1);

        FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index0, FaceIndexBlend.Index1);
    }
    // Single index material id
    else
    {
        FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index0);
    }

    // Assign output

    MaterialBlends0[0] = Blends0[0];
    MaterialBlends0[1] = Blends0[1];
    MaterialBlends0[2] = Blends0[2];

    MaterialBlends1[0] = Blends1[0];
    MaterialBlends1[1] = Blends1[1];
    MaterialBlends1[2] = Blends1[2];

    MaterialBlends2[0] = Blends2[0];
    MaterialBlends2[1] = Blends2[1];
    MaterialBlends2[2] = Blends2[2];
}

void UMQCMaterialUtility::ClearZeroInfluence(FMQCMaterial& Material)
{
    uint8 Index0 = Material.GetIndex0();
    uint8 Index1 = Material.GetIndex1();
    uint8 Index2 = Material.GetIndex2();

    // Skip single index material
    if (Index0 == Index1)
    {
        return;
    }

    uint8 Blend0 = Material.GetBlend0();
    uint8 Blend1 = Material.GetBlend1();
    uint8 Blend2 = Material.GetBlend2();

    // Double Index
    if (Index1 == Index2)
    {
        check(Blend2 == 0);

        if (Blend1 == 0)
        {
            Material.SetIndex1(Index0);
            Material.SetIndex2(Index0);

            Material.SetBlend1(0);
        }
        else
        if (Blend0 == 0)
        {
            Material.SetIndex0(Index1);
            Material.SetBlend0(Blend1);

            Material.SetBlend1(0);
        }
    }
    // Triple Index
    else
    {
        // Zero influence Index2
        if (Blend2 == 0)
        {
            // Zero influence Index2 and Index1, replace all index with Index0
            if (Blend1 == 0)
            {
                Material.SetIndex1(Index0);
                Material.SetIndex2(Index0);

                Material.SetBlend1(0);
                Material.SetBlend2(0);
            }
            // Zero influence Index2 and Index0, replace all index with Index1
            else
            if (Blend0 == 0)
            {
                Material.SetIndex0(Index1);
                Material.SetIndex2(Index1);

                Material.SetBlend0(Blend1);
                Material.SetBlend1(0);
                Material.SetBlend2(0);
            }
            // Replace Index2 with Index1
            else
            {
                Material.SetIndex2(Index1);

                Material.SetBlend2(0);
            }
        }
        // Zero influence Index1
        else
        if (Blend1 == 0)
        {
            // Zero influence Index1 and Index0, replace all index with Index2
            if (Blend0 == 0)
            {
                Material.SetIndex0(Index2);
                Material.SetIndex1(Index2);

                Material.SetBlend0(Blend2);
                Material.SetBlend1(0);
                Material.SetBlend2(0);
            }
            // Replace Index1 with Index2
            else
            {
                Material.SetIndex1(Index2);

                Material.SetBlend1(Blend2);
                Material.SetBlend2(0);
            }
        }
    }
}

// FMQCDoubleIndexBlend

FMQCDoubleIndexBlend UMQCMaterialUtility::FindDoubleIndexBlend(
    const FMQCDoubleIndexBlend& A,
    const FMQCDoubleIndexBlend& B,
    const FMQCDoubleIndexBlend& C
    )
{
    typedef TArray<uint8, TFixedAllocator<6>> FIndexList;

    uint8 Strengths[256];
    FIndexList UsedIds;

    struct FHelper
    {
        static void AddIndex(
            uint8 Index,
            uint8 Strength,
            uint8 Strengths[256],
            FIndexList& UsedIds
            )
        {
            if (! UsedIds.Contains(Index))
            {
                UsedIds.Add(Index);
                Strengths[Index] = 0;
            }
            Strengths[Index] = FMath::Max(Strengths[Index], Strength);
        }

        static void AddDoubleIndexBlend(
            const FMQCDoubleIndexBlend& A,
            uint8 Strengths[256],
            FIndexList& UsedIds
            )
        {
            AddIndex(A.IndexA, 255 - A.Blend, Strengths, UsedIds);
            AddIndex(A.IndexB, A.Blend, Strengths, UsedIds);
        }
    };

    FHelper::AddDoubleIndexBlend(A, Strengths, UsedIds);
    FHelper::AddDoubleIndexBlend(B, Strengths, UsedIds);
    FHelper::AddDoubleIndexBlend(C, Strengths, UsedIds);

    uint8 MaxIndexA = 0;
    int32 MaxIndexAStrength = -1;
    uint8 MaxIndexB = 0;
    int32 MaxIndexBStrength = -1;

    for (uint8 Id : UsedIds)
    {
        uint8 Strength = Strengths[Id];
        if (Strength >= MaxIndexAStrength)
        {
            MaxIndexB = MaxIndexA;
            MaxIndexBStrength = MaxIndexAStrength;
            MaxIndexA = Id;
            MaxIndexAStrength = Strength;
        }
        else
        if (Strength > MaxIndexBStrength)
        {
            MaxIndexB = Id;
            MaxIndexBStrength = Strength;
        }
    }

    check(MaxIndexAStrength >= 0);

    if (MaxIndexBStrength < 0)
    {
        MaxIndexB = MaxIndexA;
        MaxIndexBStrength = 0;
    }

    int32 Strength = ((255 - MaxIndexAStrength) + MaxIndexBStrength) / 2;
    check(0 <= Strength && Strength < 256);

    if (MaxIndexA > MaxIndexB)
    {
        Swap(MaxIndexA, MaxIndexB);
        Strength = 255-Strength;
    }

    return FMQCDoubleIndexBlend(MaxIndexA, MaxIndexB, Strength);
}

// FMQCTripleIndexBlend

FMQCTripleIndexBlend UMQCMaterialUtility::FindTripleIndexBlend(
    const FMQCTripleIndexBlend& A,
    const FMQCTripleIndexBlend& B,
    const FMQCTripleIndexBlend& C
    )
{
    typedef TArray<uint8, TFixedAllocator<9>> FIndexList;

    uint8 BlendsLUT[256];
    FIndexList UsedIds;

    struct FHelper
    {
        static void AddIndex(
            uint8 Index,
            uint8 Blend,
            uint8 BlendsLUT[256],
            FIndexList& UsedIds
            )
        {
            if (! UsedIds.Contains(Index))
            {
                UsedIds.Add(Index);
                BlendsLUT[Index] = 0;
            }

            BlendsLUT[Index] = FMath::Max(BlendsLUT[Index], Blend);
        }

        static void AddTripleIndexBlend(
            const FMQCTripleIndexBlend& A,
            uint8 BlendsLUT[256],
            FIndexList& UsedIds
            )
        {
            int32 IndexCount = A.GetIndexCount();

            if (IndexCount == 3)
            {
                AddIndex(A.Index0, A.GetBlend0(), BlendsLUT, UsedIds);
                AddIndex(A.Index1, A.GetBlend1(), BlendsLUT, UsedIds);
                AddIndex(A.Index2, A.GetBlend2(), BlendsLUT, UsedIds);
            }
            else
            if (IndexCount == 2)
            {
                AddIndex(A.Index0, A.GetBlend0(), BlendsLUT, UsedIds);
                AddIndex(A.Index1, A.GetBlend1(), BlendsLUT, UsedIds);
            }
            if (IndexCount == 1)
            {
                AddIndex(A.Index0, A.GetBlend0(), BlendsLUT, UsedIds);
            }
        }
    };

    FHelper::AddTripleIndexBlend(A, BlendsLUT, UsedIds);
    FHelper::AddTripleIndexBlend(B, BlendsLUT, UsedIds);
    FHelper::AddTripleIndexBlend(C, BlendsLUT, UsedIds);

    uint8 MaxIndex0 = 0;
    int32 MaxBlend0 = -1;
    uint8 MaxIndex1 = 0;
    int32 MaxBlend1 = -1;
    uint8 MaxIndex2 = 0;
    int32 MaxBlend2 = -1;

    for (uint8 Id : UsedIds)
    {
        uint8 Blend = BlendsLUT[Id];

        // Assign index by strength, swap order if necessary
        if (Blend >= MaxBlend0)
        {
            MaxIndex2 = MaxIndex1;
            MaxBlend2 = MaxBlend1;
            MaxIndex1 = MaxIndex0;
            MaxBlend1 = MaxBlend0;
            MaxIndex0 = Id;
            MaxBlend0 = Blend;
        }
        else
        if (Blend > MaxBlend1)
        {
            MaxIndex2 = MaxIndex1;
            MaxBlend2 = MaxBlend1;
            MaxIndex1 = Id;
            MaxBlend1 = Blend;
        }
        else
        if (Blend > MaxBlend2)
        {
            MaxIndex2 = Id;
            MaxBlend2 = Blend;
        }
    }

    // Make sure Index 0 is valid
    check(MaxBlend0 >= 0);

    // Index set blend output
    FMQCTripleIndexBlend MatBlend;

    // Max index 1 is invalid, use single index
    if (MaxBlend1 < 0)
    {
        // Make sure Index 2 is also invalid
        check(MaxBlend2 < 0);

        MatBlend = FMQCTripleIndexBlend(MaxIndex0, MaxBlend0);
    }
    // Max index 2 is invalid, use double index
    else
    if (MaxBlend2 < 0)
    {
        // Sort index order
        if (MaxIndex0 > MaxIndex1)
        {
            Swap(MaxIndex0, MaxIndex1);
        }

        check(MaxIndex0 <= MaxIndex1);

        MatBlend = FMQCTripleIndexBlend(MaxIndex0, MaxIndex1, MaxBlend0, MaxBlend1);
    }
    // Otherwise, all index is valid, use triple index
    else
    {
        // Sort index order

        if (MaxIndex1 > MaxIndex2)
        {
            // 1 <-> 2
            Swap(MaxIndex1, MaxIndex2);
            Swap(MaxBlend1, MaxBlend2);
        }

        if (MaxIndex0 > MaxIndex2)
        {
            // 0 <-> 1
            Swap(MaxIndex0, MaxIndex1);
            Swap(MaxBlend0, MaxBlend1);

            // 1 <-> 2
            Swap(MaxIndex1, MaxIndex2);
            Swap(MaxBlend1, MaxBlend2);
        }
        else
        if (MaxIndex0 > MaxIndex1)
        {
            // 0 <-> 1
            Swap(MaxIndex0, MaxIndex1);
            Swap(MaxBlend0, MaxBlend1);
        }

        check(MaxIndex0 <= MaxIndex1);
        check(MaxIndex0 <= MaxIndex2);
        check(MaxIndex1 <= MaxIndex2);

        MatBlend = FMQCTripleIndexBlend(MaxIndex0, MaxIndex1, MaxIndex2, MaxBlend0, MaxBlend1, MaxBlend2);
    }

    return MatBlend;
}

FMQCTripleIndexBlend FMQCTripleIndexBlend::GetBlendFor(const FMQCTripleIndexBlend& Other) const
{
    if (HasEqualIndex(Other))
    {
        return *this;
    }

    FMQCTripleIndexBlend MatBlend(Other);

    if (IndexCount == 3)
    {
        check(Index0 != Index1);
        check(Index0 != Index2);
        check(Index1 != Index2);

        uint8 MatchFlags0 = Other.GetMatchFlags(Index0);
        uint8 MatchFlags1 = Other.GetMatchFlags(Index1);
        uint8 MatchFlags2 = Other.GetMatchFlags(Index2);
        uint8 OutputMask = MatchFlags0 | MatchFlags1 | MatchFlags2;

        if (OutputMask != 0)
        {
            MatBlend.SetBlendMasked(MatchFlags0, Blend0);
            MatBlend.SetBlendMasked(MatchFlags1, Blend1);
            MatBlend.SetBlendMasked(MatchFlags2, Blend2);
            MatBlend.SetBlendMasked(~OutputMask, 0);
        }
    }
    else
    if (IndexCount == 2)
    {
        check(Index0 != Index1);

        uint8 MatchFlags0 = Other.GetMatchFlags(Index0);
        uint8 MatchFlags1 = Other.GetMatchFlags(Index1);
        uint8 OutputMask = MatchFlags0 | MatchFlags1;

        if (OutputMask != 0)
        {
            MatBlend.SetBlendMasked(MatchFlags0, Blend0);
            MatBlend.SetBlendMasked(MatchFlags1, Blend1);
            MatBlend.SetBlendMasked(~OutputMask, 0);
        }
    }
    else
    {
        uint8 OutputMask = Other.GetMatchFlags(Index0);

        if (OutputMask != 0)
        {
            MatBlend.SetBlendMasked(OutputMask, Blend0);
            MatBlend.SetBlendMasked(~OutputMask, 0);
        }
    }

    return MatBlend;
}

uint8 FMQCTripleIndexBlend::GetMatchFlags(uint8 InIndex) const
{
    uint8 OutFlags = 0;

    if (IndexCount == 3)
    {
        check(Index0 != Index1);
        check(Index0 != Index2);
        check(Index1 != Index2);

        if (Index0 == InIndex)
        {
            OutFlags = 0x01;
        }
        else
        if (Index1 == InIndex)
        {
            OutFlags = 0x02;
        }
        else
        if (Index2 == InIndex)
        {
            OutFlags = 0x04;
        }
    }
    else
    if (IndexCount == 2)
    {
        check(Index0 != Index1);

        if (Index0 == InIndex)
        {
            OutFlags = 0x01;
        }
        else
        if (Index1 == InIndex)
        {
            OutFlags = 0x02;
        }
    }
    else
    {
        if (Index0 == InIndex)
        {
            OutFlags = 0x01;
        }
    }

    return OutFlags;
}

void FMQCTripleIndexBlend::SetBlendMasked(uint8 Mask, uint8 InBlend)
{
    if (Mask & 0x01)
    {
        Blend0 = InBlend;
    }
    if (Mask & 0x02)
    {
        Blend1 = InBlend;
    }
    if (Mask & 0x04)
    {
        Blend2 = InBlend;
    }
}
