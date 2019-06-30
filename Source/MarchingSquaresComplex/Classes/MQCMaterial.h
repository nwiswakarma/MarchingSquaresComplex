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

#pragma once

#include "CoreMinimal.h"
#include "MQCMaterial.generated.h"

inline uint8 CastToUINT8(int32 Value)
{
    ensureAlwaysMsgf(0 <= Value && Value < 256, TEXT("Invalid uint8 value: %d"), Value);
    return Value;
}

// MATERIAL

UENUM(BlueprintType)
enum class EMQCMaterialType : uint8
{
    MT_COLOR,
    MT_SINGLE_INDEX,
    MT_DOUBLE_INDEX,
    MT_TRIPLE_INDEX
};

UENUM(BlueprintType)
enum class EMQCMaterialBlendType : uint8
{
    MBT_DEFAULT,
    MBT_MAX,
    MBT_COPY,
    MBT_LERP
};

USTRUCT(BlueprintType)
struct MARCHINGSQUARESCOMPLEX_API FMQCMaterialInput
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMQCMaterialType Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 Index;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Color;
};

USTRUCT(BlueprintType)
struct MARCHINGSQUARESCOMPLEX_API FMQCMaterialPointInput
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Point;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMQCMaterialInput Material;
};

struct MARCHINGSQUARESCOMPLEX_API FMQCMaterial
{
private:

    uint8 Index;
    uint8 R;
    uint8 G;
    uint8 B;
    
public:

    FMQCMaterial()
    {
        Index = 0;
        R = 0;
        G = 0;
        B = 0;
    }

    FMQCMaterial(ENoInit NoInit)
    {
    }

    inline bool HasOpaqueBlend() const
    {
        return (GetBlend() == 0 || GetBlend() == 255);
    }

    inline bool HasIndexAsDouble(uint8 InIndex) const
    {
        return (InIndex == GetIndexA())
            || (InIndex == GetIndexB());
    }

    inline bool HasIndexAsTriple(uint8 InIndex) const
    {
        return (InIndex == GetIndex0())
            || (InIndex == GetIndex1())
            || (InIndex == GetIndex2());
    }

    inline int32 IsTripleIndexRequiredFor(uint8 InIndex) const
    {
        if (! IsMarkedAsTripledIndex())
        {
            return GetIndexA() != GetIndexB()
                && ! HasOpaqueBlend()
                && ! HasIndexAsDouble(InIndex);
        }

        return true;
    }

    inline bool IsDoubleIndexSortRequired() const
    {
        return GetIndexA() > GetIndexB();
    }

    inline bool IsTripleIndexSortRequired() const
    {
        return GetIndex0() > GetIndex1()
            || GetIndex0() > GetIndex2()
            || GetIndex1() > GetIndex2();
    }

    inline void SortDoubleIndex()
    {
        if (IsDoubleIndexSortRequired())
        {
            uint8 IndexA = GetIndexA();
            uint8 IndexB = GetIndexB();

            SetIndexA(IndexB);
            SetIndexB(IndexA);
            SetBlend(255-GetBlend());
        }
    }

    inline void SortTripleIndex()
    {
        if (IsTripleIndexSortRequired())
        {
            uint8 Index0 = GetIndex0();
            uint8 Index1 = GetIndex1();
            uint8 Index2 = GetIndex2();

            uint8 Blend01 = GetBlend01();
            uint8 Blend12 = GetBlend12();

            if (Index1 > Index2)
            {
                SetIndex1(Index2);
                SetIndex2(Index1);
                SetBlend01(Blend12);
                SetBlend12(Blend01);
            }

            if (Index0 > Index2)
            {
                SetIndex0(Index1);
                SetIndex1(Index2);
                SetIndex2(Index0);
                SetBlend01(Blend12);
                SetBlend12(255-Blend01);
            }
            else
            if (Index0 > Index1)
            {
                SetIndex0(Index1);
                SetIndex1(Index0);
                SetBlend01(255-Blend01);
            }
        }
    }

    inline bool IsMarkedAsTripledIndex() const
    {
        return (G&0xF0) == 0xF0;
    }

    inline void MarkAsTripleIndex()
    {
        G |= 0xF0;
    }

public:

    inline uint8 GetIndex() const { return Index; }

    inline uint8 GetR() const { return R; }
    inline uint8 GetG() const { return G; }
    inline uint8 GetB() const { return B; }
    inline uint8 GetA() const { return GetIndex(); }

    inline uint8 GetIndexA() const { return R; }
    inline uint8 GetIndexB() const { return G; }
    inline uint8 GetBlend() const { return B; }

    inline uint8 GetIndex0() const { return  R    &0x0F; }
    inline uint8 GetIndex1() const { return (R>>4)&0x0F; }
    inline uint8 GetIndex2() const { return  G    &0x0F; }
    inline uint8 GetBlend01() const { return B; }
    inline uint8 GetBlend12() const { return GetIndex(); }

    inline FColor ToFColor() const
    {
        return FColor(GetR(), GetG(), GetB(), GetA());
    }

public:

    inline void SetIndex(uint8 NewIndex) { Index = NewIndex; }

    inline void SetR(uint8 NewR) { R = NewR; }
    inline void SetG(uint8 NewG) { G = NewG; }
    inline void SetB(uint8 NewB) { B = NewB; }
    inline void SetA(uint8 NewA) { SetIndex(NewA); }

    inline void SetIndexA(uint8 NewIndexA) { R = NewIndexA; }
    inline void SetIndexB(uint8 NewIndexB) { G = NewIndexB; }
    inline void SetBlend(uint8 NewBlend) { B = NewBlend; }

    inline void SetIndex0(uint8 NewI0) { R = (R&0xF0)| (NewI0&0x0F);     }
    inline void SetIndex1(uint8 NewI1) { R = (R&0x0F)|((NewI1&0x0F)<<4); }
    inline void SetIndex2(uint8 NewI2) { G = (G&0xF0)| (NewI2&0x0F);     }
    inline void SetBlend01(uint8 NewBlend01) { B = NewBlend01; }
    inline void SetBlend12(uint8 NewBlend12) { SetIndex(NewBlend12); }

    inline void SetColor(const FColor& Color)
    {
        SetR(Color.R);
        SetG(Color.G);
        SetB(Color.B);
        SetA(Color.A);
    }

public:

    inline void SetIndex(int32 NewIndex) { SetIndex(CastToUINT8(NewIndex)); }

    inline void SetR(int32 NewR) { SetR(CastToUINT8(NewR)); }
    inline void SetG(int32 NewG) { SetG(CastToUINT8(NewG)); }
    inline void SetB(int32 NewB) { SetB(CastToUINT8(NewB)); }
    inline void SetA(int32 NewA) { SetA(CastToUINT8(NewA)); }

    inline void SetIndexA(int32 NewIndexA) { SetIndexA(CastToUINT8(NewIndexA)); }
    inline void SetIndexB(int32 NewIndexB) { SetIndexB(CastToUINT8(NewIndexB)); }
    inline void SetBlend(int32 NewBlend) { SetBlend(CastToUINT8(NewBlend)); }

public:

    template<typename T>
    inline void SetIndex(T) = delete;

    template<typename T>
    inline void SetR(T) = delete;
    template<typename T>
    inline void SetG(T) = delete;
    template<typename T>
    inline void SetB(T) = delete;
    template<typename T>
    inline void SetA(T) = delete;
    
    template<typename T>
    inline void SetIndexA(T) = delete;
    template<typename T>
    inline void SetIndexB(T) = delete;
    template<typename T>
    inline void SetBlend(T) = delete;

public:

    inline bool operator==(const FMQCMaterial& Other) const
    {
        return Index == Other.Index
            && R     == Other.R
            && G     == Other.G
            && B     == Other.B
            ;
    }

    inline bool operator!=(const FMQCMaterial& Other) const
    {
        return Index != Other.Index
            || R     != Other.R
            || G     != Other.G
            || B     != Other.B
            ;
    }

public:

    friend inline FArchive& operator<<(FArchive &Ar, FMQCMaterial& Material)
    {
        Ar << Material.Index;
        Ar << Material.R;
        Ar << Material.G;
        Ar << Material.B;
        return Ar;
    }

    bool Serialize(FArchive& Ar)
    {
        Ar << *this;
        return true;
    }
};

template <> struct TIsPODType<FMQCMaterial> { enum { Value = true }; };

// MATERIAL BLEND

struct FMQCMaterialBlend
{
	enum EKind
	{
		Single,
		Double,
		Triple,
		Invalid
	};

	FMQCMaterialBlend() = default;

	explicit FMQCMaterialBlend(uint8 Index)
		: Index0(Index)
		, Kind(Single)
	{
	}

	explicit FMQCMaterialBlend(uint8 Index0, uint8 Index1)
		: Index0(Index0)
		, Index1(Index1)
		, Kind(Double)
	{
		check(Index0 < Index1);
	}

	explicit FMQCMaterialBlend(uint8 Index0, uint8 Index1, uint8 Index2)
		: Index0(Index0)
		, Index1(Index1)
		, Index2(Index2)
		, Kind(Triple)
	{
		check(Index0 < Index1 && Index1 < Index2);
	}

    inline bool IsDouble() const
    {
        return Kind == Double;
    }

    inline bool IsTriple() const
    {
        return Kind == Triple;
    }

	inline bool operator==(const FMQCMaterialBlend& Other) const
	{
		return Kind == Other.Kind && Index0 == Other.Index0 && Index1 == Other.Index1 && Index2 == Other.Index2;
	}

	inline FString ToString() const
	{
		switch (Kind)
		{
            case Single: return FString::Printf(TEXT("Single %u"), Index0);
            case Double: return FString::Printf(TEXT("Double %u %u"), Index0, Index1);
            case Triple: return FString::Printf(TEXT("Triple %u %u %u"), Index0, Index1, Index2);

            case Invalid:
            default:
                return "Invalid";
		}
	}

	inline FString KindToString() const
	{
		switch (Kind)
		{
            case Single: return "Single";
            case Double: return "Double";
            case Triple: return "Triple";

            case Invalid:
            default:
                return "Invalid";
		}
	}

	inline TArray<uint8, TFixedAllocator<3>> GetElements() const
	{
		switch (Kind)
		{
            case Single: return { Index0 };
            case Double: return { Index0, Index1 };
            case Triple: return { Index0, Index1, Index2 };

            case Invalid:
            default:
                return {};
		}
	}

	uint8 Index0 = 255;
	uint8 Index1 = 255;
	uint8 Index2 = 255;
	EKind Kind = Invalid;
};

inline uint32 GetTypeHash(const FMQCMaterialBlend& O)
{
	return GetTypeHash(O.Index0) ^ GetTypeHash(O.Index1) ^ GetTypeHash(O.Index2) ^ GetTypeHash(O.Kind);
}

// INDEX BLEND GENERATION

struct FMQCDoubleIndex
{
    uint8 IndexA;
    uint8 IndexB;

    FMQCDoubleIndex() = default;

    explicit FMQCDoubleIndex(uint8 IndexA, uint8 IndexB)
        : IndexA(IndexA)
        , IndexB(IndexB)
    {
        check(IndexA <= IndexB);
    }

    explicit FMQCDoubleIndex(const FMQCMaterial& Material)
        : IndexA(Material.GetIndexA())
        , IndexB(Material.GetIndexB())
    {
        check(IndexA <= IndexB);
    }

    inline bool HasEqualIndex(uint8 InIndexA, uint8 InIndexB) const
    {
        return IndexA == InIndexA && IndexB == InIndexB;
    }

    FORCEINLINE bool HasEqualIndex(const FMQCDoubleIndex& Other) const
    {
        return HasEqualIndex(Other.IndexA, Other.IndexB);
    }
};

struct FMQCDoubleIndexBlend : public FMQCDoubleIndex
{
    uint8 Blend;

    FMQCDoubleIndexBlend() = default;

    explicit FMQCDoubleIndexBlend(uint8 IndexA, uint8 IndexB, uint8 Blend)
        : FMQCDoubleIndex(IndexA, IndexB)
        , Blend(Blend)
    {
    }

    explicit FMQCDoubleIndexBlend(const FMQCMaterial& Material)
        : FMQCDoubleIndex(Material)
        , Blend(Material.GetBlend())
    {
    }

    inline uint8 GetBlendA() { return 255-Blend; }
    inline uint8 GetBlendB() { return Blend; }

    inline int32 GetSignificantSingleIndex() const
    {
        // Single index or Significant IndexA
        if (IndexA == IndexB || Blend == 0)
        {
            return 0;
        }
        // Significant IndexB
        else
        if (Blend == 255)
        {
            return 1;
        }
        // Does not have single significant index
        else
        {
            return -1;
        }
    }

    inline uint8 GetBlendFor(const FMQCDoubleIndexBlend& Other) const
    {
        // Make sure index order is sorted on both index set
        check(IndexA <= IndexB);
        check(Other.IndexA <= Other.IndexB);

        // Check equals, return blend
        if (HasEqualIndex(Other))
        {
            return Blend;
        }

        // Check for single significant index, blend with single index set
        // or double index with only single significant index

        int32 SingleIndex = GetSignificantSingleIndex();

        if (SingleIndex >= 0)
        {
            uint8 Index = (SingleIndex == 0) ? IndexA : IndexB;

            if (Index == Other.IndexA)
            {
                return 0;
            }
            else
            if (Index == Other.IndexB)
            {
                return 255;
            }
        }

        // Otherwise, return target blend
        return Other.Blend;
    }
};

struct FMQCTripleIndex
{
    bool bIsTriple;
    uint8 Index0;
    uint8 Index1;
    uint8 Index2;

    FMQCTripleIndex() = default;

    explicit FMQCTripleIndex(uint8 Index0, uint8 Index1)
        : bIsTriple(false)
        , Index0(Index0)
        , Index1(Index1)
        , Index2(255)
    {
        check(Index0 <= Index1);
    }

    explicit FMQCTripleIndex(uint8 Index0, uint8 Index1, uint8 Index2)
        : bIsTriple(true)
        , Index0(Index0)
        , Index1(Index1)
        , Index2(Index2)
    {
        check(Index0 <= Index1);
        check(Index0 <= Index2);
        check(Index1 <= Index2);
    }

    explicit FMQCTripleIndex(const FMQCMaterial& Material)
        : bIsTriple(Material.IsMarkedAsTripledIndex())
    {
        if (bIsTriple)
        {
            Index0 = Material.GetIndex0();
            Index1 = Material.GetIndex1();
            Index2 = Material.GetIndex2();
            check(Index0 <= Index1);
            check(Index0 <= Index2);
            check(Index1 <= Index2);
        }
        else
        {
            Index0 = Material.GetIndexA();
            Index1 = Material.GetIndexB();
            check(Index0 <= Index1);
        }
    }

    inline bool HasAnyIndex(uint8 Index) const
    {
        return Index0 == Index || Index1 == Index || Index2 == Index;
    }

    inline bool HasAnyIndexAsDouble(uint8 Index) const
    {
        return Index0 == Index || Index == Index1;
    }

    inline bool HasEqualIndex(bool bInIsTriple, uint8 InIndex0, uint8 InIndex1, uint8 InIndex2) const
    {
        return bIsTriple == bInIsTriple
            && Index0 == InIndex0
            && Index1 == InIndex1
            && Index2 == InIndex2;
    }

    FORCEINLINE bool HasEqualIndex(const FMQCTripleIndex& Other) const
    {
        return HasEqualIndex(
            Other.bIsTriple,
            Other.Index0,
            Other.Index1,
            Other.Index2
            );
    }

    inline uint8 GetIndex(int32 Index) const
    {
        switch (Index)
        {
            case 0: return Index0;
            case 1: return Index1;
            case 2: return Index2;

            default:
                return 0xFF;
        }
    }

    inline FMQCTripleIndex GetMatchingIndex(const FMQCTripleIndex& Other) const
    {
        FMQCTripleIndex Match;
        Match.Index0 = Index0 == Other.Index0;
        Match.Index1 = Index1 == Other.Index1;
        Match.Index2 = Index2 == Other.Index2;
        return Match;
    }

    inline bool HasNonZeroIndex() const
    {
        return Index0 || Index1 || Index2;
    }

    inline int32 GetNonZeroIndexCount() const
    {
        return Index0 + Index1 + Index2;
    }
};

struct FMQCTripleIndexBlend : public FMQCTripleIndex
{
    uint8 Blend01;
    uint8 Blend12;

    FMQCTripleIndexBlend() = default;

    explicit FMQCTripleIndexBlend(uint8 Index0, uint8 Index1, uint8 Blend01)
        : FMQCTripleIndex(Index0, Index1)
        , Blend01(Blend01)
        , Blend12(0)
    {
    }

    explicit FMQCTripleIndexBlend(uint8 Index0, uint8 Index1, uint8 Index2, uint8 Blend01, uint8 Blend12)
        : FMQCTripleIndex(Index0, Index1, Index2)
        , Blend01(Blend01)
        , Blend12(Blend12)
    {
    }

    explicit FMQCTripleIndexBlend(const FMQCMaterial& Material)
        : FMQCTripleIndex(Material)
    {
        if (bIsTriple)
        {
            Blend01 = Material.GetBlend01();
            Blend12 = Material.GetBlend12();
        }
        else
        {
            Blend01 = Material.GetBlend();
        }
    }

    inline uint8 GetBlend0() const { return 255-Blend01; }
    inline uint8 GetBlend1() const { return Blend01; }
    inline uint8 GetBlend2() const { return Blend12; }

    inline int32 GetSignificantSingleIndex() const
    {
        if (bIsTriple)
        {
            // Significant Index0
            if (Blend01 == 0 && Blend12 == 0)
            {
                return 0;
            }
            // Significant Index1
            else
            if (Blend01 == 255 && Blend12 == 0)
            {
                return 1;
            }
            // Significant Index2
            else
            if (Blend01 == 255 && Blend12 == 255)
            {
                return 2;
            }
            // Does not have single significant index
            else
            {
                return -1;
            }
        }
        else
        {
            // Single index or Significant Index0
            if (Index0 == Index1 || Blend01 == 0)
            {
                return 0;
            }
            // Significant Index1
            else
            if (Blend01 == 255)
            {
                return 1;
            }
            // Does not have single significant index
            else
            {
                return -1;
            }
        }
    }

    FMQCTripleIndexBlend GetBlendAsDoubleIndexFor(const FMQCTripleIndexBlend& Other) const;
    FMQCTripleIndexBlend GetBlendAsTripleIndexFor(const FMQCTripleIndexBlend& Other) const;
    FMQCTripleIndexBlend GetBlendFor(const FMQCTripleIndexBlend& Other) const;
};
