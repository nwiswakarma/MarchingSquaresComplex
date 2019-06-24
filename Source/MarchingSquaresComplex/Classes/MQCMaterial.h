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

FORCEINLINE uint8 CastToUINT8(int32 Value)
{
    ensureAlwaysMsgf(0 <= Value && Value < 256, TEXT("Invalid uint8 value: %d"), Value);
    return Value;
}

USTRUCT(BlueprintType)
struct MARCHINGSQUARESCOMPLEX_API FMQCMaterial
{
    GENERATED_BODY()

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

    void SortIndexOrder()
    {
        uint8 IndexA = GetIndexA();
        uint8 IndexB = GetIndexB();

        if (IndexA > IndexB)
        {
            SetIndexA(IndexB);
            SetIndexB(IndexA);
            SetBlend(255-GetBlend());
        }
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

template <>
struct TTypeTraits<FMQCMaterial> : public TTypeTraitsBase<FMQCMaterial>
{
    enum
    {
        IsBytewiseComparable = true
    };
};

template<>
struct TStructOpsTypeTraits<FMQCMaterial> : public TStructOpsTypeTraitsBase2<FMQCMaterial>
{
    enum 
    {
        WithSerializer = true,
        WithIdenticalViaEquality = true
    };
};
