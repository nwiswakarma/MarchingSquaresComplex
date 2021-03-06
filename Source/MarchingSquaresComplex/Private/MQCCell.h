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
#include "MQCFeaturePoint.h"

class FMQCCell
{
public:

    FMQCVoxel a;
    FMQCVoxel b;
    FMQCVoxel c;
    FMQCVoxel d;
    
    int32 i;
    
    float sharpFeatureLimit;
    float parallelLimit;

    FORCEINLINE FVector2D GetAverageNESW() const
    {
        return (a.GetXEdgePoint() + a.GetYEdgePoint() + b.GetYEdgePoint() + c.GetXEdgePoint()) / 4.f;
    }

    FORCEINLINE FMQCFeaturePoint GetFeatureSW() const
    {
        return GetSharpFeature(a.GetXEdgePoint(), a.NormalX.ToFVector2D(), a.GetYEdgePoint(), a.NormalY.ToFVector2D());
    }
    
    FORCEINLINE FMQCFeaturePoint GetFeatureSE() const
    {
        return GetSharpFeature(a.GetXEdgePoint(), a.NormalX.ToFVector2D(), b.GetYEdgePoint(), b.NormalY.ToFVector2D());
    }
    
    FORCEINLINE FMQCFeaturePoint GetFeatureNW() const
    {
        return GetSharpFeature(a.GetYEdgePoint(), a.NormalY.ToFVector2D(), c.GetXEdgePoint(), c.NormalX.ToFVector2D());
    }
    
    FORCEINLINE FMQCFeaturePoint GetFeatureNE() const
    {
        return GetSharpFeature(c.GetXEdgePoint(), c.NormalX.ToFVector2D(), b.GetYEdgePoint(), b.NormalY.ToFVector2D());
    }
    
    FORCEINLINE FMQCFeaturePoint GetFeatureNS() const
    {
        return GetSharpFeature(a.GetXEdgePoint(), a.NormalX.ToFVector2D(), c.GetXEdgePoint(), c.NormalX.ToFVector2D());
    }
    
    FORCEINLINE FMQCFeaturePoint GetFeatureEW() const
    {
        return GetSharpFeature(a.GetYEdgePoint(), a.NormalY.ToFVector2D(), b.GetYEdgePoint(), b.NormalY.ToFVector2D());
    }

    FORCEINLINE FMQCFeaturePoint GetFeatureNEW() const
    {
        FMQCFeaturePoint f = FMQCFeaturePoint::Average(GetFeatureEW(), GetFeatureNE(), GetFeatureNW());
        if (! f.exists)
        {
            f.position = (a.GetYEdgePoint() + b.GetYEdgePoint() + c.GetXEdgePoint()) / 3.f;
            f.exists = true;
        }
        return f;
    }

    FORCEINLINE FMQCFeaturePoint GetFeatureNSE() const
    {
        FMQCFeaturePoint f = FMQCFeaturePoint::Average(GetFeatureNS(), GetFeatureSE(), GetFeatureNE());
        if (! f.exists)
        {
            f.position = (a.GetXEdgePoint() + b.GetYEdgePoint() + c.GetXEdgePoint()) / 3.f;
            f.exists = true;
        }
        return f;
    }
    
    FORCEINLINE FMQCFeaturePoint GetFeatureNSW() const
    {
        FMQCFeaturePoint f = FMQCFeaturePoint::Average(GetFeatureNS(), GetFeatureNW(), GetFeatureSW());
        if (! f.exists)
        {
            f.position = (a.GetXEdgePoint() + a.GetYEdgePoint() + c.GetXEdgePoint()) / 3.f;
            f.exists = true;
        }
        return f;
    }
    
    FORCEINLINE FMQCFeaturePoint GetFeatureSEW() const
    {
        FMQCFeaturePoint f = FMQCFeaturePoint::Average(GetFeatureEW(), GetFeatureSE(), GetFeatureSW());
        if (! f.exists)
        {
            f.position = (a.GetXEdgePoint() + a.GetYEdgePoint() + b.GetYEdgePoint()) / 3.f;
            f.exists = true;
        }
        return f;
    }
    
    FMQCFeaturePoint GetFeatureAverage(
        const FMQCFeaturePoint& fA,
        const FMQCFeaturePoint& fB,
        const FMQCFeaturePoint& fC,
        const FMQCFeaturePoint& fD
        ) const
    {
        FMQCFeaturePoint f = FMQCFeaturePoint::Average(fA, fB, fC, fD);
        if (! f.exists)
        {
            f.position = GetAverageNESW();
            f.Material = GetMaterial(f.position);
            f.exists = true;
        }
        return f;
    }

    bool HasConnectionAD(const FMQCFeaturePoint& fA, const FMQCFeaturePoint& fD)
    {
        bool flip = (a.voxelState < b.voxelState) == (a.voxelState < c.voxelState);
        if (IsParallel(a.NormalX.ToFVector2D(), a.NormalY.ToFVector2D(), flip) ||
            IsParallel(c.NormalX.ToFVector2D(), b.NormalY.ToFVector2D(), flip))
        {
            return true;
        }
        if (fA.exists)
        {
            if (fD.exists)
            {
                if (IsBelowLine(fA.position, b.GetYEdgePoint(), fD.position))
                {
                    if (IsBelowLine(fA.position, fD.position, c.GetXEdgePoint()) ||
                        IsBelowLine(fD.position, fA.position, a.GetXEdgePoint()))
                    {
                        return true;
                    }
                }
                else if (IsBelowLine(fA.position, fD.position, c.GetXEdgePoint()) &&
                         IsBelowLine(fD.position, a.GetYEdgePoint(), fA.position))
                {
                    return true;
                }
                return false;
            }
            return IsBelowLine(fA.position, b.GetYEdgePoint(), c.GetXEdgePoint());
        }
        if (fD.exists)
        {
            return IsBelowLine(fD.position, a.GetYEdgePoint(), a.GetXEdgePoint());
        }
        return (a.pointState == a.voxelState) && (a.pointState == d.voxelState);
    }
    
    bool HasConnectionBC(const FMQCFeaturePoint& fB, const FMQCFeaturePoint& fC)
    {
        bool flip = (b.voxelState < a.voxelState) == (b.voxelState < d.voxelState);
        if (IsParallel(a.NormalX.ToFVector2D(), b.NormalY.ToFVector2D(), flip) ||
            IsParallel(c.NormalX.ToFVector2D(), a.NormalY.ToFVector2D(), flip))
        {
            return true;
        }
        if (fB.exists)
        {
            if (fC.exists)
            {
                if (IsBelowLine(fC.position, a.GetXEdgePoint(), fB.position))
                {
                    if (IsBelowLine(fC.position, fB.position, b.GetYEdgePoint()) ||
                        IsBelowLine(fB.position, fC.position, a.GetYEdgePoint()))
                    {
                        return true;
                    }
                }
                else if (IsBelowLine(fC.position, fB.position, b.GetYEdgePoint()) &&
                         IsBelowLine(fB.position, c.GetXEdgePoint(), fC.position))
                {
                    return true;
                }
                return false;
            }
            return IsBelowLine(fB.position, c.GetXEdgePoint(), a.GetYEdgePoint());
        }
        if (fC.exists)
        {
            return IsBelowLine(fC.position, a.GetXEdgePoint(), b.GetYEdgePoint());
        }
        return (a.pointState == b.voxelState) && (a.pointState == c.voxelState);
    }

    FORCEINLINE bool IsInsideABD(const FVector2D& point)
    {
        return IsBelowLine(point, a.GetPosition(), d.GetPosition());
    }
    
    FORCEINLINE bool IsInsideACD(const FVector2D& point)
    {
        return IsBelowLine(point, d.GetPosition(), a.GetPosition());
    }

    FORCEINLINE bool IsInsideABC(const FVector2D& point)
    {
        return IsBelowLine(point, c.GetPosition(), b.GetPosition());
    }

    FORCEINLINE bool IsInsideBCD(const FVector2D& point)
    {
        return IsBelowLine(point, b.GetPosition(), c.GetPosition());
    }

    FORCEINLINE static bool IsBelowLine(const FVector2D& p, const FVector2D& start, const FVector2D& end)
    {
        float determinant = (end.X - start.X) * (p.Y - start.Y) - (end.Y - start.Y) * (p.X - start.X);
        return determinant < 0.f;
    }

    FORCEINLINE static FVector2D GetIntersection(const FVector2D& p1, const FVector2D& n1, const FVector2D& p2, const FVector2D& n2)
    {
        FVector2D d2(-n2.Y, n2.X);
        float u2 = -FVector2D::DotProduct(n1, p2 - p1) / FVector2D::DotProduct(n1, d2);
        return p2 + d2 * u2;
    }

    FORCEINLINE bool IsSharpFeature(const FVector2D& n1, const FVector2D& n2) const
    {
        float dot = FVector2D::DotProduct(n1, -n2);
        return dot >= sharpFeatureLimit && dot < 0.999f;
    }

    FORCEINLINE bool IsParallel(const FVector2D& n1, const FVector2D& n2, bool flip) const
    {
        return FVector2D::DotProduct(n1, flip ? -n2 : n2) > parallelLimit;
    }

    FORCEINLINE bool IsInsideCell(const FVector2D& point) const
    {
        return (point >= a.GetPosition()) && (point <= d.GetPosition());
    }

    inline FMQCFeaturePoint GetSharpFeature(const FVector2D& p1, const FVector2D& n1, const FVector2D& p2, const FVector2D& n2) const
    {
        FMQCFeaturePoint f;
        if (IsSharpFeature(n1, n2))
        {
            f.position = GetIntersection(p1, n1, p2, n2);
            f.exists = IsInsideCell(f.position);

            // Assign material if feature point is valid
            if (f.exists)
            {
                f.Material = GetMaterial(f.position);
            }
        }
        else
        {
            f.position = FVector2D::ZeroVector;
            f.exists = false;
        }
        return f;
    }

    inline FMQCMaterial GetMaterial(const FVector2D& Position) const
    {
        FVector2D PointToCenter = Position - ((d.GetPosition()-a.GetPosition()) / 2.f);
        FMQCMaterial Material;

        if (Position.X > 0.f)
        {
            if (Position.Y > 0.f)
            {
                Material = d.Material;
            }
            else
            {
                Material = b.Material;
            }
        }
        else
        {
            if (Position.Y > 0.f)
            {
                Material = c.Material;
            }
            else
            {
                Material = a.Material;
            }
        }

        return Material;
    }
};
