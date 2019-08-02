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
#include "MQCMaterial.h"
#include "MQCVoxelTypes.generated.h"

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
    FIntPoint Position;
    int32 MapSize;
    int32 VoxelResolution;
    float ExtrusionHeight;
    bool bGenerateExtrusion;
    bool bExtrusionSurface;
    bool bRemapEdgeUVs;
    EMQCMaterialType MaterialType;
};

struct FMQCChunkConfig
{
    FIntPoint Position;
    int32 MapSize;
    int32 VoxelResolution;
    float MaxFeatureAngle;
    float MaxParallelAngle;
    float ExtrusionHeight;
    EMQCMaterialType MaterialType;
    TArray<FMQCSurfaceState> States;
};

USTRUCT(BlueprintType)
struct MARCHINGSQUARESCOMPLEX_API FMQCMapConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 VoxelResolution = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ChunkResolution = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxFeatureAngle = 135.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxParallelAngle = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ExtrusionHeight = -1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMQCMaterialType MaterialType = EMQCMaterialType::MT_COLOR;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMQCSurfaceState> States;
};
