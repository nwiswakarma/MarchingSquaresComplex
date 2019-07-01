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

FMQCTripleIndexBlend UMQCMaterialUtility::FindTripleIndexBlend(
    const FMQCTripleIndexBlend& A,
    const FMQCTripleIndexBlend& B,
    const FMQCTripleIndexBlend& C
    )
{
    typedef TArray<uint8, TFixedAllocator<9>> FIndexList;

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

        static void AddTripleIndexBlend(
            const FMQCTripleIndexBlend& A,
            uint8 Strengths[256],
            FIndexList& UsedIds
            )
        {
            AddIndex(A.Index0, A.GetBlend0(), Strengths, UsedIds);
            AddIndex(A.Index1, A.GetBlend1(), Strengths, UsedIds);

            if (A.bIsTriple)
            {
                AddIndex(A.Index2, A.GetBlend2(), Strengths, UsedIds);
            }
        }
    };

    FHelper::AddTripleIndexBlend(A, Strengths, UsedIds);
    FHelper::AddTripleIndexBlend(B, Strengths, UsedIds);
    FHelper::AddTripleIndexBlend(C, Strengths, UsedIds);

    uint8 MaxIndex0 = 0;
    int32 MaxIndex0Strength = -1;
    uint8 MaxIndex1 = 0;
    int32 MaxIndex1Strength = -1;
    uint8 MaxIndex2 = 0;
    int32 MaxIndex2Strength = -1;

    for (uint8 Id : UsedIds)
    {
        uint8 Strength = Strengths[Id];

        // Assign index by strength, swap order if necessary
        if (Strength >= MaxIndex0Strength)
        {
            MaxIndex2 = MaxIndex1;
            MaxIndex2Strength = MaxIndex1Strength;
            MaxIndex1 = MaxIndex0;
            MaxIndex1Strength = MaxIndex0Strength;
            MaxIndex0 = Id;
            MaxIndex0Strength = Strength;
        }
        else
        if (Strength > MaxIndex1Strength)
        {
            MaxIndex2 = MaxIndex1;
            MaxIndex2Strength = MaxIndex1Strength;
            MaxIndex1 = Id;
            MaxIndex1Strength = Strength;
        }
        else
        if (Strength > MaxIndex2Strength)
        {
            MaxIndex2 = Id;
            MaxIndex2Strength = Strength;
        }
    }

    // Make sure Index 0 have any strength
    check(MaxIndex0Strength >= 0);

    // Index set blend output
    FMQCTripleIndexBlend Blend;

    // Max index 1 is invalid, use single index
    if (MaxIndex1Strength < 0)
    {
        // Make sure Index 2 is also invalid
        check(MaxIndex2Strength < 0);

        Blend = FMQCTripleIndexBlend(MaxIndex0, MaxIndex0, 0);
    }
    // Max index 2 is invalid, use double index
    else
    if (MaxIndex2Strength < 0)
    {
        // Find blend strength average
        int32 Strength = ((255-MaxIndex0Strength) + MaxIndex1Strength) / 2;

        // Make sure strength is withing byte range
        check(0 <= Strength && Strength < 256);

        // Index order is reversed, invert order
        if (MaxIndex0 > MaxIndex1)
        {
            Swap(MaxIndex0, MaxIndex1);
            Strength = 255-Strength;
        }

        Blend = FMQCTripleIndexBlend(MaxIndex0, MaxIndex1, Strength);
    }
    // Otherwise, all index is valid, use triple index
    else
    {
        // Sort index order

        if (MaxIndex1 > MaxIndex2)
        {
            // 1 <-> 2
            Swap(MaxIndex1, MaxIndex2);
            Swap(MaxIndex1Strength, MaxIndex2Strength);
        }

        if (MaxIndex0 > MaxIndex2)
        {
            // 0 <-> 1
            Swap(MaxIndex0, MaxIndex1);
            Swap(MaxIndex0Strength, MaxIndex1Strength);

            // 1 <-> 2
            Swap(MaxIndex1, MaxIndex2);
            Swap(MaxIndex1Strength, MaxIndex2Strength);
        }
        else
        if (MaxIndex0 > MaxIndex1)
        {
            // 0 <-> 1
            Swap(MaxIndex0, MaxIndex1);
            Swap(MaxIndex0Strength, MaxIndex1Strength);
        }

        check(MaxIndex0 <= MaxIndex1);
        check(MaxIndex0 <= MaxIndex2);
        check(MaxIndex1 <= MaxIndex2);

        float Sum3 = MaxIndex0Strength + MaxIndex1Strength + MaxIndex2Strength;

        check(Sum3 > 0.f);

        float Sum3Inv = 1.f / Sum3;

        float Blend0F = MaxIndex0Strength * Sum3Inv;
        float Blend1F = MaxIndex1Strength * Sum3Inv;
        float Blend2F = MaxIndex2Strength * Sum3Inv;

        uint8 Blend0 = AlphaToUINT8(Blend0F);
        uint8 Blend1 = AlphaToUINT8(Blend1F);
        uint8 Blend2 = AlphaToUINT8(Blend2F);

        //float Blend01F = (Blend0>0.f || Blend1>0.f) ? (Blend1/(Blend0+Blend1)) : 0.f;
        //float Blend12F = Blend2;

        //uint8 Blend01 = AlphaToUINT8(Blend01F);
        //uint8 Blend12 = AlphaToUINT8(Blend12F);

        //int32 Blend01I = ((255-MaxIndex0Strength) + MaxIndex1Strength) / 2;
        //int32 Blend12I = ((255-MaxIndex1Strength) + MaxIndex2Strength) / 2;
          
        //uint8 Blend01 = FMath::Clamp<uint8>(Blend01I, 0, 255);
        //uint8 Blend12 = FMath::Clamp<uint8>(Blend12I, 0, 255);

        //Blend = FMQCTripleIndexBlend(MaxIndex0, MaxIndex1, MaxIndex2, Blend01, Blend12);
        Blend = FMQCTripleIndexBlend(MaxIndex0, MaxIndex1, MaxIndex2, Blend0, Blend1, Blend2);
    }

    return Blend;
}

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

        //Blends0[0] = BlendA.Blend01;
        //Blends0[1] = BlendB.Blend01;
        //Blends0[2] = BlendC.Blend01;

        //Blends1[0] = BlendA.Blend12;
        //Blends1[1] = BlendB.Blend12;
        //Blends1[2] = BlendC.Blend12;

        //Blends2[0] = 0;
        //Blends2[1] = 0;
        //Blends2[2] = 0;

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

    // Generate face material for triple index set
    if (FaceIndexBlend.bIsTriple)
    {
        // Make sure there are three unique material index
        check(FaceIndexBlend.Index0 != FaceIndexBlend.Index1);
        check(FaceIndexBlend.Index0 != FaceIndexBlend.Index2);
        check(FaceIndexBlend.Index1 != FaceIndexBlend.Index2);

#if 0
        // Single index with material Index2
        if (IsBlendsEqual(Blends12, 255))
        {
            FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index2);
        }
        else
        if (IsBlendsEqual(Blends01, 255))
        {
            // Single index with material Index1
            if (IsBlendsEqual(Blends12, 0))
            {
                FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index1);
            }
            // Double index with material Index1 and Index2
            else
            {
                FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index1, FaceIndexBlend.Index2);
            }
        }
        else
        if (IsBlendsEqual(Blends01, 0))
        {
            // Single index with material Index0
            if (IsBlendsEqual(Blends12, 0))
            {
                FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index1);
            }
            // Double index with material Index1 and Index2
            else
            {
                FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index1, FaceIndexBlend.Index2);
            }
        }
        // Triple index
        else
        {
            FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index0, FaceIndexBlend.Index1, FaceIndexBlend.Index2);
        }
#else
            FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index0, FaceIndexBlend.Index1, FaceIndexBlend.Index2);
#endif
    }
    // Generate face material for single or double index set
    else
    {
#if 0
        // Single index with material Index0
        if (IsBlendsEqual(Blends01, 0))
        {
            FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index0);
        }
        // Single index with material Index1
        else
        if (IsBlendsEqual(Blends01, 255))
        {
            FaceMaterial = FMQCMaterialBlend(FaceIndexBlend.Index1);
        }
        // Double or single index
        else
#endif
        {
            uint8 IndexMin = FaceIndexBlend.Index0;
            uint8 IndexMax = FaceIndexBlend.Index1;

            // Make sure index order is not reversed
            check(IndexMin <= IndexMax);

            FaceMaterial = (IndexMin == IndexMax)
                ? FMQCMaterialBlend(IndexMin)
                : FMQCMaterialBlend(IndexMin, IndexMax);
        }
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

FMQCTripleIndexBlend FMQCTripleIndexBlend::GetBlendFor(const FMQCTripleIndexBlend& Other) const
{
    return bIsTriple
        ? GetBlendAsTripleIndexFor(Other)
        : GetBlendAsDoubleIndexFor(Other);
}

FMQCTripleIndexBlend FMQCTripleIndexBlend::GetBlendAsDoubleIndexFor(const FMQCTripleIndexBlend& Other) const
{
    // Make sure index order is sorted on both index set
    check(!bIsTriple);
    check(Index0 <= Index1);
    check(Other.Index0 <= Other.Index1);
    check(!Other.bIsTriple || Other.Index0 <= Other.Index2);
    check(!Other.bIsTriple || Other.Index1 <= Other.Index2);

    FMQCTripleIndexBlend OutBlend = Other;
    bool bMatchIndex0 = Other.HasAnyIndex(Index0);
    bool bMatchIndex1 = Other.HasAnyIndex(Index1);
    int32 SingleIndex = GetSignificantSingleIndex();

#if 0
    // Double index with only single matching index, treat as single index
    if (SingleIndex < 0 && ((bMatchIndex0 && !bMatchIndex1) || (!bMatchIndex0 && bMatchIndex1)))
    {
        SingleIndex = bMatchIndex0 ? 0 : 1;
    }

    // Single index blend
    if (SingleIndex >= 0)
    {
        uint8 Index = GetIndex(SingleIndex);

        if (Index == Other.Index0)
        {
            OutBlend.Blend01 = 0;
            OutBlend.Blend12 = 0;
        }
        else
        if (Index == Other.Index1)
        {
            OutBlend.Blend01 = 255;
            OutBlend.Blend12 = 0;
        }
        else
        if (Index == Other.Index2)
        {
            OutBlend.Blend01 = 0;
            OutBlend.Blend12 = 255;
        }
    }
    // Double index blend
    else
    if (bMatchIndex0 && bMatchIndex1)
    {
        if (! HasAnyIndexAsDouble(Other.Index0))
        {
            OutBlend.Blend01 = 255;
            OutBlend.Blend12 = Blend01;
        }
        else
        if (! HasAnyIndexAsDouble(Other.Index1))
        {
            OutBlend.Blend01 = 0;
            OutBlend.Blend12 = Blend01;
        }
        else
        if (! HasAnyIndexAsDouble(Other.Index2))
        {
            OutBlend.Blend01 = Blend01;
            OutBlend.Blend12 = 0;
        }
    }
#else
    // Double index with only single matching index, treat as single index
    if (SingleIndex < 0 && ((bMatchIndex0 && !bMatchIndex1) || (!bMatchIndex0 && bMatchIndex1)))
    {
        SingleIndex = bMatchIndex0 ? 0 : 1;
    }

    // Single index blend
    if (SingleIndex >= 0)
    {
        uint8 Index = GetIndex(SingleIndex);

        if (Index == Other.Index0)
        {
            OutBlend.Blend0 = 255;
            OutBlend.Blend1 = 0;
            OutBlend.Blend2 = 0;
        }
        else
        if (Index == Other.Index1)
        {
            OutBlend.Blend0 = 0;
            OutBlend.Blend1 = 255;
            OutBlend.Blend2 = 0;
        }
        else
        if (Index == Other.Index2)
        {
            OutBlend.Blend0 = 0;
            OutBlend.Blend1 = 0;
            OutBlend.Blend2 = 255;
        }
    }
    // Double index blend
    else
    if (bMatchIndex0 && bMatchIndex1)
    {
        if (! HasAnyIndexAsDouble(Other.Index0))
        {
            OutBlend.Blend0 = 0;
            OutBlend.Blend1 = 255-Blend0;
            OutBlend.Blend2 = Blend0;
        }
        else
        if (! HasAnyIndexAsDouble(Other.Index1))
        {
            OutBlend.Blend0 = 255-Blend0;
            OutBlend.Blend1 = 0;
            OutBlend.Blend2 = Blend0;
        }
        else
        if (! HasAnyIndexAsDouble(Other.Index2))
        {
            OutBlend.Blend0 = 255-Blend0;
            OutBlend.Blend1 = Blend0;
            OutBlend.Blend2 = 0;
        }
    }
#endif

    return OutBlend;
}

FMQCTripleIndexBlend FMQCTripleIndexBlend::GetBlendAsTripleIndexFor(const FMQCTripleIndexBlend& Other) const
{
    // Make sure index order is sorted on both index set
    check(bIsTriple);
    check(Index0 <= Index1);
    check(Index0 <= Index2);
    check(Index1 <= Index2);
    check(Other.Index0 <= Other.Index1);
    check(!Other.bIsTriple || Other.Index0 <= Other.Index2);
    check(!Other.bIsTriple || Other.Index1 <= Other.Index2);

    FMQCTripleIndexBlend OutBlend(*this);

    if (HasEqualIndex(Other))
    {
        return OutBlend;
    }
    else
    {
        return Other;
    }

#if 0
    FMQCTripleIndex IndexMatch = GetMatchingIndex(Other);
    int32 MatchingIndexCount = IndexMatch.GetNonZeroIndexCount();

    if (MatchingIndexCount == 2)
    {
        //if (! IndexMatch.Index0)
        //{
        //    OutBlend.Blend01 = 255;
        //}
        //else
        //if (! IndexMatch.Index1)
        //{
        //    OutBlend.Blend01 = 0;
        //}
        //else
        //{
        //    OutBlend.Blend12 = 0;
        //}

        return OutBlend;
    }
    //else
    //if (MatchingIndexCount == 1)
    //{
    //    if (IndexMatch.Index0)
    //    {
    //        OutBlend.Blend01 = 0;
    //        OutBlend.Blend12 = 0;
    //    }
    //    else
    //    if (IndexMatch.Index1)
    //    {
    //        OutBlend.Blend01 = 255;
    //        OutBlend.Blend12 = 0;
    //    }
    //    else
    //    {
    //        OutBlend.Blend01 = 255;
    //        OutBlend.Blend12 = 255;
    //    }

    //    return OutBlend;
    //}
#endif

#if 0
    // Check equals
    if (HasEqualIndex(Other))
    {
        return OutBlend;
    }

    int32 SingleIndex = GetSignificantSingleIndex();

    // Check for single significant index
    if (SingleIndex >= 0)
    {
        uint8 Index = GetIndex(SingleIndex);

        if (Index == Other.Index0)
        {
            OutBlend.Blend01 = 0;
            OutBlend.Blend12 = 0;
            return OutBlend;
        }
        else
        if (Index == Other.Index1)
        {
            OutBlend.Blend01 = 255;
            OutBlend.Blend12 = 0;
            return OutBlend;
        }
        else
        if (Other.bIsTriple && Index == Other.Index2)
        {
            OutBlend.Blend01 = 0;
            OutBlend.Blend12 = 255;
            return OutBlend;
        }
    }
#endif

    // Otherwise, return other blend
    //return Other;
}
