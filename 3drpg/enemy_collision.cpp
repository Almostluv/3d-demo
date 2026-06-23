/**
 * @file
 * @brief Enemy の攻撃判定と被弾判定を定義する。
 */

#include "enemy.h"
#include "enemy_param.h"
#include "DxLib.h"

#include <cmath>

namespace
{
constexpr float EnemyArmBoxHalfWidth = 6.5f; ///< 腕攻撃判定ボックスの半幅。
constexpr float EnemyArmBoxHalfHeight = 8.0f; ///< 腕攻撃判定ボックスの半高さ。
constexpr float EnemyLeftClawExtension = 10.0f; ///< 左手爪判定を先端方向へ伸ばす距離。

float clampFloat(float value, float minValue, float maxValue)
{
    /// 当たり判定計算用の値を指定範囲へ丸める。
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

float dotVector(VECTOR a, VECTOR b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

VECTOR crossVector(VECTOR a, VECTOR b)
{
    return VGet(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

float pointAabbDistanceSq(VECTOR point, float halfX, float halfY, float halfZ)
{
    /// 点を AABB 内の最近点へ丸め、そこからの二乗距離を返す。
    float closestX = clampFloat(point.x, -halfX, halfX);
    float closestY = clampFloat(point.y, -halfY, halfY);
    float closestZ = clampFloat(point.z, -halfZ, halfZ);
    float dx = point.x - closestX;
    float dy = point.y - closestY;
    float dz = point.z - closestZ;
    return dx * dx + dy * dy + dz * dz;
}

float segmentAabbDistanceSq(VECTOR start, VECTOR end, float halfX, float halfY, float halfZ)
{
    VECTOR diff = VSub(end, start);
    float left = 0.0f;
    float right = 1.0f;
    /// 線分と AABB の最近点を三分探索で近似し、腕判定用の距離計算を軽く済ませる。
    for (int i = 0; i < 18; ++i)
    {
        float t1 = left + (right - left) / 3.0f;
        float t2 = right - (right - left) / 3.0f;
        VECTOR p1 = VAdd(start, VScale(diff, t1));
        VECTOR p2 = VAdd(start, VScale(diff, t2));
        if (pointAabbDistanceSq(p1, halfX, halfY, halfZ) < pointAabbDistanceSq(p2, halfX, halfY, halfZ))
        {
            right = t2;
        }
        else
        {
            left = t1;
        }
    }

    float t = (left + right) * 0.5f;
    VECTOR closest = VAdd(start, VScale(diff, t));
    float distanceSq = pointAabbDistanceSq(closest, halfX, halfY, halfZ);
    float startSq = pointAabbDistanceSq(start, halfX, halfY, halfZ);
    float endSq = pointAabbDistanceSq(end, halfX, halfY, halfZ);
    if (startSq < distanceSq) distanceSq = startSq;
    if (endSq < distanceSq) distanceSq = endSq;
    return distanceSq;
}

float segmentSegmentDistanceSq(VECTOR a0, VECTOR a1, VECTOR b0, VECTOR b1)
{
    /// 2 本の線分の最近点を求め、武器軌跡と対象カプセルの交差判定に使う。
    VECTOR u = VSub(a1, a0);
    VECTOR v = VSub(b1, b0);
    VECTOR w = VSub(a0, b0);
    float a = dotVector(u, u);
    float b = dotVector(u, v);
    float c = dotVector(v, v);
    float d = dotVector(u, w);
    float e = dotVector(v, w);
    float denom = a * c - b * b;
    float s = 0.0f;
    float t = 0.0f;

    if (denom > 0.0001f)
    {
        s = clampFloat((b * e - c * d) / denom, 0.0f, 1.0f);
    }

    if (c > 0.0001f)
    {
        t = clampFloat((b * s + e) / c, 0.0f, 1.0f);
    }
    if (a > 0.0001f)
    {
        s = clampFloat((b * t - d) / a, 0.0f, 1.0f);
    }

    VECTOR p = VAdd(a0, VScale(u, s));
    VECTOR q = VAdd(b0, VScale(v, t));
    VECTOR diff = VSub(p, q);
    return dotVector(diff, diff);
}

float segmentSegmentClosestT(VECTOR a0, VECTOR a1, VECTOR b0, VECTOR b1)
{
    /// 2 線分の最近点計算から、a 側線分上の補間率だけを取り出す。
    VECTOR u = VSub(a1, a0);
    VECTOR v = VSub(b1, b0);
    VECTOR w = VSub(a0, b0);
    float a = dotVector(u, u);
    float b = dotVector(u, v);
    float c = dotVector(v, v);
    float d = dotVector(u, w);
    float e = dotVector(v, w);
    float denom = a * c - b * b;
    float s = 0.0f;
    float t = 0.0f;

    if (denom > 0.0001f)
    {
        s = clampFloat((b * e - c * d) / denom, 0.0f, 1.0f);
    }
    if (c > 0.0001f)
    {
        t = clampFloat((b * s + e) / c, 0.0f, 1.0f);
    }
    if (a > 0.0001f)
    {
        s = clampFloat((b * t - d) / a, 0.0f, 1.0f);
    }
    return s;
}

float segmentAabbClosestT(VECTOR start, VECTOR end, float halfX, float halfY, float halfZ)
{
    VECTOR diff = VSub(end, start);
    float left = 0.0f;
    float right = 1.0f;
    /// 距離だけでなく線分上の位置も必要なため、同じ探索で t 値を返す。
    for (int i = 0; i < 18; ++i)
    {
        float t1 = left + (right - left) / 3.0f;
        float t2 = right - (right - left) / 3.0f;
        VECTOR p1 = VAdd(start, VScale(diff, t1));
        VECTOR p2 = VAdd(start, VScale(diff, t2));
        if (pointAabbDistanceSq(p1, halfX, halfY, halfZ) < pointAabbDistanceSq(p2, halfX, halfY, halfZ))
        {
            right = t2;
        }
        else
        {
            left = t1;
        }
    }
    return (left + right) * 0.5f;
}
}

bool Enemy::getAttackArmBox(int index, VECTOR* start, VECTOR* end, float* halfWidth, float* halfHeight) const
{
    if (mModelHandle == -1 || index < 0 || index > 1 || start == nullptr || end == nullptr)
    {
        return false;
    }

    int rootFrameIndex = index == 0 ? mLeftForeArmFrameIndex : mRightForeArmFrameIndex;
    int endFrameIndex = index == 0 ? mLeftHandFrameIndex : mRightHandIndex4FrameIndex;
    if (endFrameIndex == -1 && index == 1)
    {
        endFrameIndex = mRightHandFrameIndex;
    }
    if (endFrameIndex == -1)
    {
        return false;
    }

    /// 前腕から手先までを箱状の攻撃判定として扱い、振り抜き中の腕全体を拾う。
    *end = MV1GetFramePosition(mModelHandle, endFrameIndex);
    if (rootFrameIndex != -1)
    {
        *start = MV1GetFramePosition(mModelHandle, rootFrameIndex);
    }
    else
    {
        VECTOR center = mSpineFrameIndex != -1 ? MV1GetFramePosition(mModelHandle, mSpineFrameIndex) : mPos;
        VECTOR toCenter = VSub(center, *end);
        if (VSize(toCenter) <= 0.0001f)
        {
            toCenter = VScale(getForward(), -1.0f);
        }
        *start = VAdd(*end, VScale(VNorm(toCenter), 18.0f));
    }

    if (index == 0)
    {
        VECTOR clawDirection = VSub(*end, *start);
        if (VSize(clawDirection) > 0.0001f)
        {
            *end = VAdd(*end, VScale(VNorm(clawDirection), EnemyLeftClawExtension));
        }
    }

    if (halfWidth != nullptr)
    {
        *halfWidth = EnemyArmBoxHalfWidth;
    }
    if (halfHeight != nullptr)
    {
        *halfHeight = EnemyArmBoxHalfHeight;
    }
    return true;
}
VECTOR Enemy::getMagicBeamHitPosition(VECTOR startPos, VECTOR forward, float range) const
{
    if (VSize(forward) <= 0.0001f)
    {
        return getHeadPosition();
    }

    /// ビーム線分に最も近い体または腕の位置を探し、ヒット演出の発生点にする。
    forward = VNorm(forward);
    VECTOR beamEnd = VAdd(startPos, VScale(forward, range));
    VECTOR bestPos = getHeadPosition();
    float bestDistanceSq = 0.0f;
    bool found = false;

    VECTOR bodyStart = VGet(mPos.x, mPos.y + getCollisionRadius(), mPos.z);
    VECTOR bodyEnd = VGet(mPos.x, mPos.y + getCollisionHeight() - getCollisionRadius(), mPos.z);
    float bodyT = segmentSegmentClosestT(startPos, beamEnd, bodyStart, bodyEnd);
    VECTOR bodyPos = VAdd(startPos, VScale(VSub(beamEnd, startPos), bodyT));
    float bodyDistanceSq = segmentSegmentDistanceSq(startPos, beamEnd, bodyStart, bodyEnd);
    bestPos = bodyPos;
    bestDistanceSq = bodyDistanceSq;
    found = true;

    for (int i = 0; i < 2; ++i)
    {
        /// 腕判定も候補に入れ、ビームが腕へ当たった場合の演出位置を近づける。
        VECTOR armStart;
        VECTOR armEnd;
        float halfWidth;
        float halfHeight;
        if (!getAttackArmBox(i, &armStart, &armEnd, &halfWidth, &halfHeight))
        {
            continue;
        }

        VECTOR axis = VSub(armEnd, armStart);
        float length = VSize(axis);
        if (length <= 0.0001f)
        {
            continue;
        }
        axis = VNorm(axis);
        VECTOR right = crossVector(VGet(0.0f, 1.0f, 0.0f), axis);
        if (VSize(right) <= 0.0001f)
        {
            right = VGet(1.0f, 0.0f, 0.0f);
        }
        right = VNorm(right);
        VECTOR up = VNorm(crossVector(axis, right));
        VECTOR center = VScale(VAdd(armStart, armEnd), 0.5f);
        VECTOR localStartOffset = VSub(startPos, center);
        VECTOR localEndOffset = VSub(beamEnd, center);
        VECTOR localStart = VGet(dotVector(localStartOffset, axis), dotVector(localStartOffset, up), dotVector(localStartOffset, right));
        VECTOR localEnd = VGet(dotVector(localEndOffset, axis), dotVector(localEndOffset, up), dotVector(localEndOffset, right));
        float armT = segmentAabbClosestT(localStart, localEnd, length * 0.5f, halfHeight, halfWidth);
        VECTOR localClosest = VAdd(localStart, VScale(VSub(localEnd, localStart), armT));
        float armDistanceSq = pointAabbDistanceSq(localClosest, length * 0.5f, halfHeight, halfWidth);
        if (!found || armDistanceSq < bestDistanceSq)
        {
            bestDistanceSq = armDistanceSq;
            bestPos = VAdd(startPos, VScale(VSub(beamEnd, startPos), armT));
            found = true;
        }
    }

    return bestPos;
}
bool Enemy::isMagicBeamHitting(VECTOR startPos, VECTOR forward, float range, float radius) const
{
    if (VSize(forward) <= 0.0001f)
    {
        return false;
    }

    /// 体カプセルとビーム線分の距離で、まず本体への命中を判定する。
    forward = VNorm(forward);
    VECTOR beamEnd = VAdd(startPos, VScale(forward, range));
    float bodyRadius = radius + getCollisionRadius();
    VECTOR bodyStart = VGet(mPos.x, mPos.y + getCollisionRadius(), mPos.z);
    VECTOR bodyEnd = VGet(mPos.x, mPos.y + getCollisionHeight() - getCollisionRadius(), mPos.z);
    if (segmentSegmentDistanceSq(startPos, beamEnd, bodyStart, bodyEnd) <= bodyRadius * bodyRadius)
    {
        return true;
    }

    for (int i = 0; i < 2; ++i)
    {
        /// 腕は線分をローカル AABB に変換して、ビーム幅との距離で判定する。
        VECTOR armStart;
        VECTOR armEnd;
        float halfWidth;
        float halfHeight;
        if (!getAttackArmBox(i, &armStart, &armEnd, &halfWidth, &halfHeight))
        {
            continue;
        }

        VECTOR axis = VSub(armEnd, armStart);
        float length = VSize(axis);
        if (length <= 0.0001f)
        {
            continue;
        }
        axis = VNorm(axis);
        VECTOR right = crossVector(VGet(0.0f, 1.0f, 0.0f), axis);
        if (VSize(right) <= 0.0001f)
        {
            right = VGet(1.0f, 0.0f, 0.0f);
        }
        right = VNorm(right);
        VECTOR up = VNorm(crossVector(axis, right));
        VECTOR center = VScale(VAdd(armStart, armEnd), 0.5f);

        VECTOR localStartOffset = VSub(startPos, center);
        VECTOR localEndOffset = VSub(beamEnd, center);
        VECTOR localStart = VGet(dotVector(localStartOffset, axis), dotVector(localStartOffset, up), dotVector(localStartOffset, right));
        VECTOR localEnd = VGet(dotVector(localEndOffset, axis), dotVector(localEndOffset, up), dotVector(localEndOffset, right));
        if (segmentAabbDistanceSq(localStart, localEnd, length * 0.5f, halfHeight, halfWidth) <= radius * radius)
        {
            return true;
        }
    }

    return false;
}
bool Enemy::isArmAttackHitting(VECTOR targetPos, float targetRadius, float targetHeight) const
{
    if (mState != EnemyState::Attack && mState != EnemyState::Attack360)
    {
        return false;
    }

    int firstIndex = mState == EnemyState::Attack360 ? 0 : (mAttackStep == 1 ? 0 : 1);
    int lastIndex = mState == EnemyState::Attack360 ? 1 : firstIndex;
    for (int i = firstIndex; i <= lastIndex; ++i)
    {
        /// 対象カプセルを腕ボックスのローカル空間へ写し、近ければ命中とする。
        VECTOR start;
        VECTOR end;
        float halfWidth;
        float halfHeight;
        if (!getAttackArmBox(i, &start, &end, &halfWidth, &halfHeight))
        {
            continue;
        }

        VECTOR axis = VSub(end, start);
        float length = VSize(axis);
        if (length <= 0.0001f)
        {
            continue;
        }

        axis = VNorm(axis);
        VECTOR right = crossVector(VGet(0.0f, 1.0f, 0.0f), axis);
        if (VSize(right) <= 0.0001f)
        {
            right = VGet(1.0f, 0.0f, 0.0f);
        }
        right = VNorm(right);
        VECTOR up = VNorm(crossVector(axis, right));
        VECTOR center = VScale(VAdd(start, end), 0.5f);
        float halfLength = length * 0.5f;

        float capsuleBottom = targetPos.y + targetRadius;
        float capsuleTop = targetPos.y + targetHeight - targetRadius;
        if (capsuleTop < capsuleBottom)
        {
            capsuleTop = capsuleBottom;
        }
        VECTOR capsuleStart = VGet(targetPos.x, capsuleBottom, targetPos.z);
        VECTOR capsuleEnd = VGet(targetPos.x, capsuleTop, targetPos.z);
        VECTOR localStartOffset = VSub(capsuleStart, center);
        VECTOR localEndOffset = VSub(capsuleEnd, center);
        VECTOR localStart = VGet(dotVector(localStartOffset, axis), dotVector(localStartOffset, up), dotVector(localStartOffset, right));
        VECTOR localEnd = VGet(dotVector(localEndOffset, axis), dotVector(localEndOffset, up), dotVector(localEndOffset, right));
        if (segmentAabbDistanceSq(localStart, localEnd, halfLength, halfHeight, halfWidth) <= targetRadius * targetRadius)
        {
            return true;
        }
    }

    return false;
}
void Enemy::drawAttackArmHitBoxes() const
{
    /// 通常攻撃の腕判定ボックスをワールド空間の線で可視化する。
    if (mState != EnemyState::Attack && mState != EnemyState::Attack360)
    {
        return;
    }

    int firstIndex = mState == EnemyState::Attack360 ? 0 : (mAttackStep == 1 ? 0 : 1);
    int lastIndex = mState == EnemyState::Attack360 ? 1 : firstIndex;
    unsigned int color = GetColor(255, 80, 80);

    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    for (int i = firstIndex; i <= lastIndex; ++i)
    {
        VECTOR start;
        VECTOR end;
        float halfWidth;
        float halfHeight;
        if (!getAttackArmBox(i, &start, &end, &halfWidth, &halfHeight))
        {
            continue;
        }

        VECTOR axis = VSub(end, start);
        float length = VSize(axis);
        if (length <= 0.0001f)
        {
            continue;
        }
        axis = VNorm(axis);
        VECTOR right = crossVector(VGet(0.0f, 1.0f, 0.0f), axis);
        if (VSize(right) <= 0.0001f)
        {
            right = VGet(1.0f, 0.0f, 0.0f);
        }
        right = VNorm(right);
        VECTOR up = VNorm(crossVector(axis, right));
        VECTOR center = VScale(VAdd(start, end), 0.5f);
        float halfLength = length * 0.5f;

        VECTOR p[8];
        int n = 0;
        for (int x = 0; x < 2; ++x)
        {
            float lx = x == 0 ? -halfLength : halfLength;
            for (int y = 0; y < 2; ++y)
            {
                float ly = y == 0 ? -halfHeight : halfHeight;
                for (int z = 0; z < 2; ++z)
                {
                    float lz = z == 0 ? -halfWidth : halfWidth;
                    p[n++] = VAdd(VAdd(VAdd(center, VScale(axis, lx)), VScale(up, ly)), VScale(right, lz));
                }
            }
        }

        DrawLine3D(p[0], p[1], color);
        DrawLine3D(p[0], p[2], color);
        DrawLine3D(p[1], p[3], color);
        DrawLine3D(p[2], p[3], color);
        DrawLine3D(p[4], p[5], color);
        DrawLine3D(p[4], p[6], color);
        DrawLine3D(p[5], p[7], color);
        DrawLine3D(p[6], p[7], color);
        DrawLine3D(p[0], p[4], color);
        DrawLine3D(p[1], p[5], color);
        DrawLine3D(p[2], p[6], color);
        DrawLine3D(p[3], p[7], color);
    }
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
}
int Enemy::getHitCircleCount() const
{
    return EnemyParam::HitCircleCount;
}
VECTOR Enemy::getHitCircleCenter(int index) const
{
    /// 被弾判定は複数フレームの小円で構成し、モデル形状へ寄せる。
    int frameIndex = -1;
    switch (index)
    {
    case 0:
        frameIndex = mLeftUpLegFrameIndex;
        break;
    case 1:
        frameIndex = mRightUpLegFrameIndex;
        break;
    case 2:
        frameIndex = mLeftLegFrameIndex;
        break;
    case 3:
        frameIndex = mRightLegFrameIndex;
        break;
    case 4:
        frameIndex = mLeftHandFrameIndex;
        break;
    case 5:
        frameIndex = mRightHandFrameIndex;
        break;
    }

    if (mModelHandle != -1 && frameIndex != -1)
    {
        return MV1GetFramePosition(mModelHandle, frameIndex);
    }

    VECTOR forward = VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
    if (index % 3 == 1)
    {
        return VAdd(mPos, VScale(forward, EnemyParam::HitCircleOffset));
    }

    if (index % 3 == 2)
    {
        return VAdd(mPos, VScale(forward, -EnemyParam::HitCircleOffset));
    }

    return mPos;
}
float Enemy::getHitCircleRadius(int index) const
{
    (void)index;
    return EnemyParam::HitCircleRadius;
}
