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
#include "MQCVoxel.h"

struct FMQCFeaturePoint
{
	FVector2D position;
    FMQCMaterial Material;
	bool exists;

	static FMQCFeaturePoint Average(const FMQCFeaturePoint& a, const FMQCFeaturePoint& b, const FMQCFeaturePoint& c)
    {
		FVector2D Avg(ForceInitToZero);
		float FeatureCount = 0.f;
        bool bExists;
        // Add feature positions
		if (a.exists)
        {
			Avg += a.position;
			FeatureCount += 1.f;
		}
		if (b.exists)
        {
			Avg += b.position;
			FeatureCount += 1.f;
		}
		if (c.exists)
        {
			Avg += c.position;
			FeatureCount += 1.f;
		}
        // Calculate average position
		if (FeatureCount > 0.f)
        {
			Avg /= FeatureCount;
			bExists = true;
		}
		else
        {
			bExists = false;
		}
        FMQCFeaturePoint f;
        f.position = Avg;
        f.exists = bExists;
		return f;
	}

	static FMQCFeaturePoint Average(const FMQCFeaturePoint& a, const FMQCFeaturePoint& b, const FMQCFeaturePoint& c, const FMQCFeaturePoint& d)
    {
		FVector2D Avg(ForceInitToZero);
		float FeatureCount = 0.f;
        bool bExists;
        // Add feature positions
		if (a.exists)
        {
			Avg += a.position;
			FeatureCount += 1.f;
		}
		if (b.exists)
        {
			Avg += b.position;
			FeatureCount += 1.f;
		}
		if (c.exists)
        {
			Avg += c.position;
			FeatureCount += 1.f;
		}
		if (d.exists)
        {
			Avg += d.position;
			FeatureCount += 1.f;
		}
        // Calculate average position
		if (FeatureCount > 0.f)
        {
			Avg /= FeatureCount;
			bExists = true;
		}
		else
        {
			bExists = false;
		}
        FMQCFeaturePoint f;
        f.position = Avg;
        f.exists = bExists;
		return f;
	}

	FORCEINLINE static uint32 GetHash(const FMQCFeaturePoint& f)
    {
        FIntPoint Position(f.position.X, f.position.Y);
        uint8 EdgeX = FMQCVoxel::EncodeEdge(f.position.X-Position.X);
        uint8 EdgeY = FMQCVoxel::EncodeEdge(f.position.Y-Position.Y);
        return FMQCVoxel::GetPositionHashPacked(Position, EdgeX, EdgeY);
    }

	FORCEINLINE static uint32 GetHash8(const FMQCFeaturePoint& f)
    {
        FIntPoint Position(f.position.X, f.position.Y);
        uint8 EdgeX = FMQCVoxel::EncodeEdge(f.position.X-Position.X);
        uint8 EdgeY = FMQCVoxel::EncodeEdge(f.position.Y-Position.Y);
        return FMQCVoxel::GetPositionHashPacked8(Position, EdgeX, EdgeY);
    }

	FORCEINLINE uint32 GetHash() const
    {
        return GetHash(*this);
    }

	FORCEINLINE uint32 GetHash8() const
    {
        return GetHash8(*this);
    }
};
