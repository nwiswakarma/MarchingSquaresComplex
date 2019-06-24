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
#include "MQCVoxelTypes.generated.h"

UENUM(BlueprintType)
enum class EMQCMaterialType : uint8
{
    MT_COLOR,
    MT_SINGLE_INDEX,
    MT_DOUBLE_INDEX
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
struct MARCHINGSQUARESCOMPLEX_API FMQCSurfaceState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bGenerateExtrusion = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bExtrusionSurface  = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRemapEdgeUVs = false;
};

struct FMQCSurfaceConfig
{
    FVector2D Position;
    int32 MapSize;
    int32 VoxelResolution;
    float ExtrusionHeight;
    bool bGenerateExtrusion;
    bool bExtrusionSurface;
    bool bRemapEdgeUVs;
};

struct FMQCChunkConfig
{
    FVector2D Position;
    int32 MapSize;
    int32 VoxelResolution;
    float MaxFeatureAngle;
    float MaxParallelAngle;
    float ExtrusionHeight;
    TArray<FMQCSurfaceState> States;
};
