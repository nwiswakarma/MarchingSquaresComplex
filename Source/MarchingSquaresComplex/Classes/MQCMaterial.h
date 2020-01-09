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
struct MARCHINGSQUARESCOMPLEX_API FMQCMaterialId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 Index0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 Index1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 Index2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMQCMaterialType MaterialType;
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
    uint8 R;
    uint8 G;
    uint8 B;
    uint8 Index;
    uint8 Index1;
    uint8 Index2;

	// Global zero material constant */
	static const FMQCMaterial Zero;

	// Global opaque material constant */
	static const FMQCMaterial Opaque;

	// Global opaque material constant */
	static const FMQCMaterial BlendOpaque;

	FORCEINLINE FMQCMaterial()
    {
    }

	FORCEINLINE FMQCMaterial(uint8 R, uint8 G, uint8 B, uint8 Index, uint8 Index1, uint8 Index2)
        : R(R)
        , G(G)
        , B(B)
        , Index(Index)
        , Index1(Index1)
        , Index2(Index2)
    {
    }

	explicit FORCEINLINE FMQCMaterial(EForceInit)
        : R(0)
        , G(0)
        , B(0)
        , Index(0)
        , Index1(0)
        , Index2(0)
    {
    }

	explicit FORCEINLINE FMQCMaterial(ENoInit)
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
        uint8 I0 = GetIndex0();
        uint8 I1 = GetIndex1();
        uint8 I2 = GetIndex2();

        uint8 B0 = GetBlend0();
        uint8 B1 = GetBlend1();
        uint8 B2 = GetBlend2();

        if (I1 > I2)
        {
            Swap(I1, I2);
            Swap(B1, B2);

            SetIndex1(I1);
            SetIndex2(I2);

            SetBlend1(B1);
            SetBlend2(B2);
        }

        if (I0 > I2)
        {
            Swap(I0, I1);
            Swap(I1, I2);
            Swap(B0, B1);
            Swap(B1, B2);

            SetIndex0(I0);
            SetIndex1(I1);
            SetIndex2(I2);

            SetBlend0(B0);
            SetBlend1(B1);
            SetBlend2(B2);
        }
        else
        if (I0 > I1)
        {
            Swap(I0, I1);
            Swap(B0, B1);

            SetIndex0(I0);
            SetIndex1(I1);

            SetBlend0(B0);
            SetBlend1(B1);
        }
    }

    // Getters

    inline uint8 GetR() const { return R; }
    inline uint8 GetG() const { return G; }
    inline uint8 GetB() const { return B; }
    inline uint8 GetA() const { return GetIndex(); }
    inline uint8 GetIndex() const { return Index; }

    inline uint8 GetIndexA() const { return R; }
    inline uint8 GetIndexB() const { return G; }
    inline uint8 GetBlend() const { return B; }

    inline uint8 GetBlend01() const { return B; }
    inline uint8 GetBlend12() const { return GetIndex(); }

    inline uint8 GetIndex0() const { return GetIndex(); }
    inline uint8 GetIndex1() const { return Index1; }
    inline uint8 GetIndex2() const { return Index2; }

    inline uint8 GetBlend0() const { return R; }
    inline uint8 GetBlend1() const { return G; }
    inline uint8 GetBlend2() const { return B; }

    inline FColor ToFColor() const
    {
        return FColor(GetR(), GetG(), GetB(), GetA());
    }

    // Setters

    inline void SetR(uint8 NewR) { R = NewR; }
    inline void SetG(uint8 NewG) { G = NewG; }
    inline void SetB(uint8 NewB) { B = NewB; }
    inline void SetA(uint8 NewA) { SetIndex(NewA); }
    inline void SetIndex(uint8 NewIndex) { Index = NewIndex; }

    inline void SetIndexA(uint8 NewIndexA) { R = NewIndexA; }
    inline void SetIndexB(uint8 NewIndexB) { G = NewIndexB; }
    inline void SetBlend(uint8 NewBlend) { B = NewBlend; }

    inline void SetBlend01(uint8 NewBlend01) { B = NewBlend01; }
    inline void SetBlend12(uint8 NewBlend12) { SetIndex(NewBlend12); }

    inline void SetIndex0(uint8 NewIndex0) { SetIndex(NewIndex0); }
    inline void SetIndex1(uint8 NewIndex1) { Index1 = NewIndex1; }
    inline void SetIndex2(uint8 NewIndex2) { Index2 = NewIndex2; }

    inline void SetBlend0(uint8 NewBlend0) { R = NewBlend0; }
    inline void SetBlend1(uint8 NewBlend1) { G = NewBlend1; }
    inline void SetBlend2(uint8 NewBlend2) { B = NewBlend2; }

    inline void SetColor(const FColor& Color)
    {
        SetR(Color.R);
        SetG(Color.G);
        SetB(Color.B);
        SetA(Color.A);
    }

    // Int Setters

    inline void SetIndex(int32 NewIndex) { SetIndex(CastToUINT8(NewIndex)); }

    inline void SetR(int32 NewR) { SetR(CastToUINT8(NewR)); }
    inline void SetG(int32 NewG) { SetG(CastToUINT8(NewG)); }
    inline void SetB(int32 NewB) { SetB(CastToUINT8(NewB)); }
    inline void SetA(int32 NewA) { SetA(CastToUINT8(NewA)); }

    inline void SetIndexA(int32 NewIndexA) { SetIndexA(CastToUINT8(NewIndexA)); }
    inline void SetIndexB(int32 NewIndexB) { SetIndexB(CastToUINT8(NewIndexB)); }
    inline void SetBlend(int32 NewBlend) { SetBlend(CastToUINT8(NewBlend)); }

    // Delete conversion setters

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

    // Debug

	inline FString ToString() const
	{
        return FString::Printf(TEXT("Index0: %u (%u), Index1: %u (%u), Index2: %u (%u)"),
            Index , R,
            Index1, G,
            Index2, B
            );
	}

    // Serialization

    friend inline FArchive& operator<<(FArchive &Ar, FMQCMaterial& Material)
    {
        Ar << Material.R;
        Ar << Material.G;
        Ar << Material.B;
        Ar << Material.Index;
        Ar << Material.Index1;
        Ar << Material.Index2;
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
    int32 IndexCount;
    uint8 Index0;
    uint8 Index1;
    uint8 Index2;

    FMQCTripleIndex() = default;

    explicit FMQCTripleIndex(uint8 Index0)
        : IndexCount(1)
        , Index0(Index0)
        , Index1(Index0)
        , Index2(Index0)
    {
        check(Index0 <= Index1);
    }

    explicit FMQCTripleIndex(uint8 Index0, uint8 Index1)
        : IndexCount(2)
        , Index0(Index0)
        , Index1(Index1)
        , Index2(Index1)
    {
        check(Index0 <= Index1);
    }

    explicit FMQCTripleIndex(uint8 Index0, uint8 Index1, uint8 Index2)
        : IndexCount(3)
        , Index0(Index0)
        , Index1(Index1)
        , Index2(Index2)
    {
        check(Index0 <= Index1);
        check(Index0 <= Index2);
        check(Index1 <= Index2);
    }

    explicit FMQCTripleIndex(const FMQCMaterial& Material)
    {
        // Assign index
        Index0 = Material.GetIndex0();
        Index1 = Material.GetIndex1();
        Index2 = Material.GetIndex2();

        if (Index1 != Index2)
        {
            IndexCount = 3;
        }
        else
        if (Index0 != Index1)
        {
            IndexCount = 2;
        }
        else
        {
            IndexCount = 1;
        }

        check(Index0 <= Index1);
        check(Index0 <= Index2);
        check(Index1 <= Index2);
        // Make sure Index1 equals Index2 if Index0 equals Index1
        check(Index0 != Index1 || Index1 == Index2);
    }

    inline bool HasAnyIndex(uint8 Index) const
    {
        return Index == Index0 || Index == Index1 || Index == Index2;
    }

    inline bool HasAnyIndexAsDouble(uint8 Index) const
    {
        return Index == Index0 || Index == Index1;
    }

    inline uint8 GetIndexCount() const
    {
        return IndexCount;
    }

    FORCEINLINE bool HasEqualIndexCount(const FMQCTripleIndex& Other) const
    {
        return IndexCount == Other.IndexCount;
    }

    inline bool HasEqualIndex(int32 InIndexCount, uint8 InIndex0, uint8 InIndex1, uint8 InIndex2) const
    {
        return Index0 == InIndex0
            && (InIndexCount > 1 && Index1 == InIndex1)
            && (InIndexCount > 2 && Index2 == InIndex2);
    }

    FORCEINLINE bool HasEqualIndex(const FMQCTripleIndex& Other) const
    {
        return HasEqualIndexCount(Other)
            ? HasEqualIndex(IndexCount, Other.Index0, Other.Index1, Other.Index2)
            : false;
    }
};

struct FMQCTripleIndexBlend : public FMQCTripleIndex
{
    uint8 Blend0;
    uint8 Blend1;
    uint8 Blend2;

    FMQCTripleIndexBlend() = default;

    explicit FMQCTripleIndexBlend(uint8 Index0, uint8 Blend0)
        : FMQCTripleIndex(Index0)
        , Blend0(Blend0)
        , Blend1(0)
        , Blend2(0)
    {
    }

    explicit FMQCTripleIndexBlend(uint8 Index0, uint8 Index1, uint8 Blend0, uint8 Blend1)
        : FMQCTripleIndex(Index0, Index1)
        , Blend0(Blend0)
        , Blend1(Blend1)
        , Blend2(0)
    {
    }

    explicit FMQCTripleIndexBlend(uint8 Index0, uint8 Index1, uint8 Index2, uint8 Blend0, uint8 Blend1, uint8 Blend2)
        : FMQCTripleIndex(Index0, Index1, Index2)
        , Blend0(Blend0)
        , Blend1(Blend1)
        , Blend2(Blend2)
    {
    }

    explicit FMQCTripleIndexBlend(const FMQCMaterial& Material)
        : FMQCTripleIndex(Material)
    {
        Blend0 = Material.GetBlend0();
        Blend1 = Material.GetBlend1();
        Blend2 = Material.GetBlend2();
    }

    inline uint8 GetBlend0() const { return Blend0; }
    inline uint8 GetBlend1() const { return Blend1; }
    inline uint8 GetBlend2() const { return Blend2; }
    uint8 GetMatchFlags(uint8 InIndex) const;

    void SetBlendMasked(uint8 Mask, uint8 InBlend);

    FMQCTripleIndexBlend GetBlendFor(const FMQCTripleIndexBlend& Other) const;
};
