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

#include "MQCStencilBox.h"

void FMQCStencilBox::FindHorizontalCrossing(FMQCVoxel& xMin, const FMQCVoxel& xMax) const
{
    if (xMin.position.Y < GetYStart() || xMin.position.Y > GetYEnd())
    {
        return;
    }

    if (xMin.state == fillType)
    {
        if (xMin.position.X <= GetXEnd() && xMax.position.X >= GetXEnd())
        {
            if (xMin.xEdge < 0.f || xMin.xEdge < GetXEnd())
            {
                xMin.xEdge = GetXEnd()-xMin.position.X;
                xMin.xNormal = FVector2D(fillType ? 1.f : -1.f, 0.f);
            }
            else
            {
                ValidateHorizontalNormal(xMin, xMax);
            }
        }
    }
    else if (xMax.state == fillType)
    {
        if (xMin.position.X <= GetXStart() && xMax.position.X >= GetXStart())
        {
            if (xMin.xEdge < 0.f || xMin.xEdge > GetXStart())
            {
                xMin.xEdge = 1.f - (xMax.position.X-GetXStart());
                xMin.xNormal = FVector2D(fillType ? -1.f : 1.f, 0.f);
            }
            else
            {
                ValidateHorizontalNormal(xMin, xMax);
            }
        }
    }
}

void FMQCStencilBox::FindVerticalCrossing(FMQCVoxel& yMin, const FMQCVoxel& yMax) const
{
    if (yMin.position.X < GetXStart() || yMin.position.X > GetXEnd())
    {
        return;
    }

    if (yMin.state == fillType)
    {
        if (yMin.position.Y <= GetYEnd() && yMax.position.Y >= GetYEnd())
        {
            if (yMin.yEdge < 0.f || yMin.yEdge < GetYEnd())
            {
                yMin.yEdge = GetYEnd()-yMin.position.Y;
                yMin.yNormal = FVector2D(0.f, fillType ? 1.f : -1.f);
            }
            else
            {
                ValidateVerticalNormal(yMin, yMax);
            }
        }
    }
    else if (yMax.state == fillType)
    {
        if (yMin.position.Y <= GetYStart() && yMax.position.Y >= GetYStart())
        {
            if (yMin.yEdge < 0.f || yMin.yEdge > GetYStart())
            {
                yMin.yEdge = 1.f - (yMax.position.Y-GetYStart());
                yMin.yNormal = FVector2D(0.f, fillType ? -1.f : 1.f);
            }
            else
            {
                ValidateVerticalNormal(yMin, yMax);
            }
        }
    }
}
