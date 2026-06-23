/**
 * @file
 * @brief enemy_attack_range_marker.cpp 実装を定義する。
 */

#include "enemy_attack_range_marker.h"

#include <cmath>

namespace
{
constexpr float MarkerGroundRayTop = 160.0f; ///< 地面検出レイの開始高さ。
constexpr float MarkerGroundRayBottom = 260.0f; ///< 地面検出レイを下方向へ伸ばす距離。
constexpr float MarkerGroundOffsetY = 0.20f; ///< マーカーを地面から少し浮かせる Y オフセット。

VECTOR sampleMarkerGroundPosition(int stageCollisionModelHandle, VECTOR pos)
{
    if (stageCollisionModelHandle == -1)
    {
        return pos;
    }

    /// 上から下へレイを飛ばし、傾いた床でもマーカーを地面に沿わせる。
    VECTOR start = VAdd(pos, VGet(0.0f, MarkerGroundRayTop, 0.0f));
    VECTOR end = VAdd(pos, VGet(0.0f, -MarkerGroundRayBottom, 0.0f));
    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(stageCollisionModelHandle, -1, start, end);
    if (hit.HitFlag)
    {
        return VAdd(hit.HitPosition, VGet(0.0f, MarkerGroundOffsetY, 0.0f));
    }

    return pos;
}
}

void drawEnemyAttackRangeMarker(
    VECTOR center,
    float radius,
    bool landingSoon,
    float blinkTimer,
    int stageCollisionModelHandle)
{
    /// 着地直前は点滅させ、プレイヤーへ危険タイミングを伝える。
    bool blinkOff =
        landingSoon &&
        (static_cast<int>(blinkTimer * 20.0f) % 2 == 0);

    if (blinkOff)
    {
        return;
    }

    /// 黄色は予告、赤は着地直前として色で危険度を分ける。
    unsigned int color =
        landingSoon
        ? GetColor(255, 40, 40)
        : GetColor(255, 230, 80);

    const int segmentNum = 40;

    /// 塗りつぶし部分は半透明で描き、床やキャラクターを完全には隠さない。
    unsigned int fillColor = landingSoon ? GetColor(255, 40, 40) : GetColor(255, 230, 80);
    VECTOR fillCenter = sampleMarkerGroundPosition(stageCollisionModelHandle, center);
    SetUseBackCulling(FALSE);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, landingSoon ? 100 : 70);
    for (int i = 0; i < segmentNum; ++i)
    {
        float angle1 = DX_TWO_PI_F * static_cast<float>(i) / static_cast<float>(segmentNum);
        float angle2 = DX_TWO_PI_F * static_cast<float>(i + 1) / static_cast<float>(segmentNum);
        VECTOR p1 = sampleMarkerGroundPosition(stageCollisionModelHandle, VGet(center.x + cosf(angle1) * radius, center.y, center.z + sinf(angle1) * radius));
        VECTOR p2 = sampleMarkerGroundPosition(stageCollisionModelHandle, VGet(center.x + cosf(angle2) * radius, center.y, center.z + sinf(angle2) * radius));
        DrawTriangle3D(fillCenter, p1, p2, fillColor, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    /// 外周線を別に描き、半透明の塗りだけでも範囲境界が読めるようにする。
    for (int i = 0; i < segmentNum; ++i)
    {
        float angle1 =
            DX_TWO_PI_F
            * static_cast<float>(i)
            / static_cast<float>(segmentNum);

        float angle2 =
            DX_TWO_PI_F
            * static_cast<float>(i + 1)
            / static_cast<float>(segmentNum);

        VECTOR p1 = sampleMarkerGroundPosition(
            stageCollisionModelHandle,
            VGet(
                center.x + cosf(angle1) * radius,
                center.y,
                center.z + sinf(angle1) * radius
            )
        );

        VECTOR p2 = sampleMarkerGroundPosition(
            stageCollisionModelHandle,
            VGet(
                center.x + cosf(angle2) * radius,
                center.y,
                center.z + sinf(angle2) * radius
            )
        );

        DrawLine3D(p1, p2, color);
    }

    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
    SetUseBackCulling(TRUE);
}
