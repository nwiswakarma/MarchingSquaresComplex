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
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MQCMaterial.h"
#include "MQCMaterialUtility.generated.h"

UCLASS()
class MARCHINGSQUARESCOMPLEX_API UMQCMaterialUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	inline static uint8 LerpUINT8(uint8 A, uint8 B, float Alpha)
	{
		float LerpResult = FMath::Lerp<float>(A, B, Alpha);
		// Do special rounding to not get stuck, eg Lerp(251, 255, 0.1) = 251
		int32 RoundedResult = Alpha > 0.f
            ? FMath::CeilToInt(LerpResult)
            : FMath::FloorToInt(LerpResult);
		return FMath::Clamp(RoundedResult, 0, 255);
	}

	FORCEINLINE static uint8 AlphaToUINT8(float Alpha)
    {
        return LerpUINT8(0, 255, FMath::Clamp(Alpha, 0.f, 1.f));
    }

    inline static FMQCMaterial GetTypedMaterial(EMQCMaterialType MaterialType, uint8 MaterialIndex, const FColor& MaterialColor)
    {
        FMQCMaterial Material;

        if (MaterialType == EMQCMaterialType::MT_COLOR)
        {
            Material.SetColor(MaterialColor);
        }
        else
        {
            Material.SetIndex(MaterialIndex);
        }

        return Material;
    }

    FORCEINLINE static FMQCMaterial GetTypedMaterial(EMQCMaterialType MaterialType, uint8 MaterialIndex, const FLinearColor& MaterialColor)
    {
        return GetTypedMaterial(MaterialType, MaterialIndex, MaterialColor.ToFColor(true));
    }

    FORCEINLINE static FMQCMaterial GetTypedMaterial(FMQCMaterialInput Input)
    {
        return GetTypedMaterial(Input.Type, Input.Index, Input.Color);
    }

    inline static bool IsBlendsEqual(uint8 Blends[3], uint8 Blend)
    {
        return Blends[0] == Blend && Blends[1] == Blend && Blends[2] == Blend;
    }

    static FMQCDoubleIndexBlend FindDoubleIndexBlend(
        const FMQCDoubleIndexBlend& A,
        const FMQCDoubleIndexBlend& B,
        const FMQCDoubleIndexBlend& C
        );

    static FMQCTripleIndexBlend FindTripleIndexBlend(
        const FMQCTripleIndexBlend& A,
        const FMQCTripleIndexBlend& B,
        const FMQCTripleIndexBlend& C
        );

    static void FindDoubleIndexFaceBlend(
        const FMQCMaterial VertexMaterials[3],
        FMQCMaterialBlend& FaceMaterial,
        uint8 MaterialBlends[3]
        );

    static void FindTripleIndexFaceBlend(
        const FMQCMaterial VertexMaterials[3],
        FMQCMaterialBlend& FaceMaterial,
        uint8 MaterialBlends0[3],
        uint8 MaterialBlends1[3],
        uint8 MaterialBlends2[3]
        );
};
