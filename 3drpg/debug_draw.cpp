/**
 * @file
 * @brief debug_draw.cpp 実装を定義する。
 */

#include "debug_draw.h"

#include <cmath>

namespace
{
    void drawDebugRingXZ(VECTOR center, float radius, unsigned int color)
    {
        constexpr int SegmentNum = 40;
        /// XZ 平面の円周を線分で近似し、床上の判定範囲を可視化する。
        for (int i = 0; i < SegmentNum; ++i)
        {
            float angle1 = DX_TWO_PI_F * static_cast<float>(i) / static_cast<float>(SegmentNum);
            float angle2 = DX_TWO_PI_F * static_cast<float>(i + 1) / static_cast<float>(SegmentNum);
            VECTOR p1 = VGet(center.x + cosf(angle1) * radius, center.y, center.z + sinf(angle1) * radius);
            VECTOR p2 = VGet(center.x + cosf(angle2) * radius, center.y, center.z + sinf(angle2) * radius);
            DrawLine3D(p1, p2, color);
        }
    }

    void drawDebugArcXY(VECTOR center, float radius, float startAngle, float endAngle, unsigned int color)
    {
        constexpr int SegmentNum = 20;
        /// カプセルの側面を表すため、XY 平面上の半円弧を描く。
        VECTOR prev = VGet(center.x + cosf(startAngle) * radius, center.y + sinf(startAngle) * radius, center.z);
        for (int i = 1; i <= SegmentNum; ++i)
        {
            float rate = static_cast<float>(i) / static_cast<float>(SegmentNum);
            float angle = startAngle + (endAngle - startAngle) * rate;
            VECTOR p = VGet(center.x + cosf(angle) * radius, center.y + sinf(angle) * radius, center.z);
            DrawLine3D(prev, p, color);
            prev = p;
        }
    }

    void drawDebugArcZY(VECTOR center, float radius, float startAngle, float endAngle, unsigned int color)
    {
        constexpr int SegmentNum = 20;
        /// カプセルを別方向から読めるよう、ZY 平面上の半円弧も描く。
        VECTOR prev = VGet(center.x, center.y + sinf(startAngle) * radius, center.z + cosf(startAngle) * radius);
        for (int i = 1; i <= SegmentNum; ++i)
        {
            float rate = static_cast<float>(i) / static_cast<float>(SegmentNum);
            float angle = startAngle + (endAngle - startAngle) * rate;
            VECTOR p = VGet(center.x, center.y + sinf(angle) * radius, center.z + cosf(angle) * radius);
            DrawLine3D(prev, p, color);
            prev = p;
        }
    }
}

void drawDebugCircleXZ(VECTOR center, float radius, unsigned int color, float yOffset)
{
    constexpr int SegmentNum = 40;
    center.y += yOffset;

    /// デバッグ線は地形に隠れないよう、Z 判定と Z 書き込みを一時的に無効にする。
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    for (int i = 0; i < SegmentNum; ++i)
    {
        float angle1 = DX_TWO_PI_F * static_cast<float>(i) / static_cast<float>(SegmentNum);
        float angle2 = DX_TWO_PI_F * static_cast<float>(i + 1) / static_cast<float>(SegmentNum);
        VECTOR p1 = VGet(center.x + cosf(angle1) * radius, center.y, center.z + sinf(angle1) * radius);
        VECTOR p2 = VGet(center.x + cosf(angle2) * radius, center.y, center.z + sinf(angle2) * radius);
        DrawLine3D(p1, p2, color);
    }
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
}

void drawDebugCapsuleY(VECTOR baseCenter, float radius, float height, unsigned int color)
{
    /// キャラクターの縦向きカプセル判定をリングと側面線で表現する。
    float sphereOffset = radius;
    float topOffset = height - radius;
    if (topOffset < sphereOffset)
    {
        topOffset = sphereOffset;
    }

    VECTOR bottom = VGet(baseCenter.x, baseCenter.y + sphereOffset, baseCenter.z);
    VECTOR top = VGet(baseCenter.x, baseCenter.y + topOffset, baseCenter.z);

    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    drawDebugRingXZ(bottom, radius, color);
    drawDebugRingXZ(top, radius, color);

    DrawLine3D(VGet(bottom.x + radius, bottom.y, bottom.z), VGet(top.x + radius, top.y, top.z), color);
    DrawLine3D(VGet(bottom.x - radius, bottom.y, bottom.z), VGet(top.x - radius, top.y, top.z), color);
    DrawLine3D(VGet(bottom.x, bottom.y, bottom.z + radius), VGet(top.x, top.y, top.z + radius), color);
    DrawLine3D(VGet(bottom.x, bottom.y, bottom.z - radius), VGet(top.x, top.y, top.z - radius), color);

    drawDebugArcXY(top, radius, 0.0f, DX_PI_F, color);
    drawDebugArcXY(bottom, radius, DX_PI_F, DX_TWO_PI_F, color);
    drawDebugArcZY(top, radius, 0.0f, DX_PI_F, color);
    drawDebugArcZY(bottom, radius, DX_PI_F, DX_TWO_PI_F, color);
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
}

void drawDebugForwardRect(VECTOR start, VECTOR forward, float length, float halfWidth, unsigned int color, float yOffset)
{
    /// 突進攻撃などの前方矩形範囲を XZ 平面に描く。
    forward.y = 0.0f;
    if (VSize(forward) <= 0.0001f)
    {
        return;
    }

    forward = VNorm(forward);
    VECTOR right = VGet(forward.z, 0.0f, -forward.x);
    start.y += yOffset;
    VECTOR end = VAdd(start, VScale(forward, length));

    VECTOR leftStart = VAdd(start, VScale(right, -halfWidth));
    VECTOR rightStart = VAdd(start, VScale(right, halfWidth));
    VECTOR leftEnd = VAdd(end, VScale(right, -halfWidth));
    VECTOR rightEnd = VAdd(end, VScale(right, halfWidth));

    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    DrawLine3D(leftStart, leftEnd, color);
    DrawLine3D(rightStart, rightEnd, color);
    DrawLine3D(leftStart, rightStart, color);
    DrawLine3D(leftEnd, rightEnd, color);
    DrawLine3D(start, end, color);
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
}
void drawDebugFrontHalfCircleXZ(VECTOR center, VECTOR forward, float radius, unsigned int color, float yOffset)
{
    constexpr int SegmentNum = 24;
    /// 近接攻撃の前方半円を、中心からの放射線と外周線で可視化する。
    forward.y = 0.0f;
    if (VSize(forward) <= 0.0001f)
    {
        return;
    }

    forward = VNorm(forward);
    float baseAngle = atan2f(forward.z, forward.x);
    center.y += yOffset;

    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    VECTOR prev = center;
    for (int i = 0; i <= SegmentNum; ++i)
    {
        float rate = static_cast<float>(i) / static_cast<float>(SegmentNum);
        float angle = baseAngle - (DX_PI_F * 0.5f) + (DX_PI_F * rate);
        VECTOR p = VGet(center.x + cosf(angle) * radius, center.y, center.z + sinf(angle) * radius);
        DrawLine3D(center, p, color);
        if (i > 0)
        {
            DrawLine3D(prev, p, color);
        }
        prev = p;
    }
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
}
