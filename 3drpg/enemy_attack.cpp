/**
 * @file
 * @brief Enemy の攻撃状態、予告、攻撃軌跡を定義する。
 */

#include "enemy.h"
#include "enemy_param.h"
#include "DxLib.h"

#include <cmath>

namespace
{
constexpr float EnemyLeftHandOuterOffset = 0.0f; ///< 左手軌跡先端の追加オフセット。
constexpr float EnemyLeftHandInnerOffsetX = 0.0f; ///< 左手軌跡内側点の X オフセット。
constexpr float EnemyLeftHandInnerOffsetY = 0.0f; ///< 左手軌跡内側点の Y オフセット。
constexpr float EnemyLeftHandInnerOffsetZ = 0.0f; ///< 左手軌跡内側点の Z オフセット。
constexpr float EnemyLeftHandOuterOffsetX = 59.826f; ///< 左手軌跡外側点の X オフセット。
constexpr float EnemyLeftHandOuterOffsetY = -5.52f; ///< 左手軌跡外側点の Y オフセット。
constexpr float EnemyLeftHandOuterOffsetZ = 19.98f; ///< 左手軌跡外側点の Z オフセット。
constexpr float EnemyLeftHandTipDirectionSideAdjust = 0.2f; ///< 左手先端方向を横へ補正する量。
constexpr float EnemyLeftHandTipDirectionUpAdjust = 0.0f; ///< 左手先端方向を上へ補正する量。
constexpr float EnemyAttackTrailDuration = 0.28f; ///< 攻撃軌跡の表示時間。単位は seconds。

void drawEnemyTrailBand(VECTOR innerA, VECTOR outerA, VECTOR innerB, VECTOR outerB, unsigned int color)
{
    /// 手の内側点と外側点を帯状の 2 三角形にして攻撃軌跡を描く。
    DrawTriangle3D(innerA, outerA, outerB, color, TRUE);
    DrawTriangle3D(innerA, outerB, innerB, color, TRUE);
}
}

float Enemy::getAttackWidth() const
{
    return EnemyParam::AttackWidth;
}
VECTOR Enemy::getAttackHitPosition() const
{
    /// 攻撃段と 360 度攻撃で参照する手のフレームを切り替える。
    int frameIndex = mState == EnemyState::Attack360 ? mRightHandFrameIndex : mAttackStep == 1 ? mLeftHandFrameIndex : mRightHandIndex3FrameIndex;
    if (frameIndex == -1 && (mState == EnemyState::Attack360 || mAttackStep != 1))
    {
        frameIndex = mRightHandFrameIndex;
    }
    if (mModelHandle != -1 && frameIndex != -1)
    {
        return MV1GetFramePosition(mModelHandle, frameIndex);
    }
    return mPos;
}
float Enemy::getAttackHitRadius() const
{
    return EnemyParam::AttackWidth;
}
VECTOR Enemy::getAttackTrailInnerPosition() const
{
    /// 攻撃軌跡の内側点は攻撃段に応じて左手・右手フレームを切り替える。
    bool useLeftHand = mState != EnemyState::Attack360 && mAttackStep == 1;
    int frameIndex = mState == EnemyState::Attack360 ? mRightHandFrameIndex : useLeftHand ? mLeftHandFrameIndex : mRightHandIndex3FrameIndex;
    if (frameIndex == -1 && !useLeftHand)
    {
        frameIndex = mRightHandFrameIndex;
    }
    if (mModelHandle != -1 && frameIndex != -1)
    {
        VECTOR pos = MV1GetFramePosition(mModelHandle, frameIndex);
        if (useLeftHand)
        {
            pos = VAdd(pos, VGet(EnemyLeftHandInnerOffsetX, EnemyLeftHandInnerOffsetY, EnemyLeftHandInnerOffsetZ));
        }
        return pos;
    }
    return getAttackHitPosition();
}
VECTOR Enemy::getAttackTrailOuterPosition() const
{
    if (mState == EnemyState::Attack360 || mAttackStep != 1)
    {
        /// 右手攻撃では指先フレームを優先して、軌跡の外側点を取る。
        int frameIndex = mRightHandIndex4FrameIndex != -1 ? mRightHandIndex4FrameIndex : mRightHandIndex3FrameIndex;
        if (mModelHandle != -1 && frameIndex != -1)
        {
            return MV1GetFramePosition(mModelHandle, frameIndex);
        }
        return getAttackTrailInnerPosition();
    }

    return getLeftHandTrailTipPosition();
}
VECTOR Enemy::getLeftHandTrailTipPosition() const
{
    if (mModelHandle == -1 || mLeftHandFrameIndex == -1)
    {
        return getAttackTrailInnerPosition();
    }

    /// 左手の根元から先端方向を推定し、モデルに先端フレームがない場合でも軌跡幅を作る。
    VECTOR hand = MV1GetFramePosition(mModelHandle, mLeftHandFrameIndex);
    hand = VAdd(hand, VGet(EnemyLeftHandInnerOffsetX, EnemyLeftHandInnerOffsetY, EnemyLeftHandInnerOffsetZ));
    VECTOR root = mLeftForeArmFrameIndex != -1 ? MV1GetFramePosition(mModelHandle, mLeftForeArmFrameIndex) : hand;
    VECTOR direction = VSub(hand, root);
    if (VSize(direction) <= 0.0001f)
    {
        direction = getForward();
    }

    direction = VNorm(direction);
    VECTOR side = VCross(VGet(0.0f, 1.0f, 0.0f), direction);
    if (VSize(side) <= 0.0001f)
    {
        side = VGet(1.0f, 0.0f, 0.0f);
    }
    side = VNorm(side);
    VECTOR up = VNorm(VCross(direction, side));
    direction = VNorm(VAdd(direction, VAdd(VScale(side, EnemyLeftHandTipDirectionSideAdjust), VScale(up, EnemyLeftHandTipDirectionUpAdjust))));

    float rawLength = VSize(VGet(EnemyLeftHandOuterOffset + EnemyLeftHandOuterOffsetX, EnemyLeftHandOuterOffsetY, EnemyLeftHandOuterOffsetZ));
    return VAdd(hand, VScale(direction, rawLength * EnemyParam::Scale));
}
float Enemy::getRandomAttackRecovery() const
{
    /// 攻撃後硬直を少し揺らし、再攻撃タイミングが完全固定にならないようにする。
    float rate = static_cast<float>(GetRand(1000)) / 1000.0f;
    return EnemyParam::AttackRecoveryMin + (EnemyParam::AttackRecoveryMax - EnemyParam::AttackRecoveryMin) * rate;
}
void Enemy::clearAttackTrail()
{
    /// 攻撃開始や状態リセット時に、古い腕軌跡が残らないよう消す。
    mAttackTrailPointCount = 0;
    mAttackTrailFadeTimer = 0.0f;
}
void Enemy::updateAttackTrail(float deltaTime)
{
    if (mState != EnemyState::Attack && mState != EnemyState::Attack360)
    {
        if (mAttackTrailFadeTimer > 0.0f)
        {
            mAttackTrailFadeTimer -= deltaTime;
        }
        else
        {
            mAttackTrailPointCount = 0;
        }
        return;
    }

    VECTOR innerPos = getAttackTrailInnerPosition();
    VECTOR outerPos = getAttackTrailOuterPosition();
    if (mAttackTrailPointCount > 0)
    {
        /// 手先がほとんど動いていないフレームは点を増やさず、軌跡の密集を避ける。
        VECTOR diff = VSub(outerPos, mAttackTrailOuterPoints[mAttackTrailPointCount - 1]);
        if (VSize(diff) < 0.35f)
        {
            mAttackTrailFadeTimer = EnemyAttackTrailDuration;
            return;
        }
    }

    if (mAttackTrailPointCount >= AttackTrailPointMax)
    {
        /// 固定長配列をリング風に使い、古い軌跡から順に捨てる。
        for (int i = 1; i < AttackTrailPointMax; ++i)
        {
            mAttackTrailInnerPoints[i - 1] = mAttackTrailInnerPoints[i];
            mAttackTrailOuterPoints[i - 1] = mAttackTrailOuterPoints[i];
        }
        mAttackTrailPointCount = AttackTrailPointMax - 1;
    }

    mAttackTrailInnerPoints[mAttackTrailPointCount] = innerPos;
    mAttackTrailOuterPoints[mAttackTrailPointCount] = outerPos;
    ++mAttackTrailPointCount;
    mAttackTrailFadeTimer = EnemyAttackTrailDuration;
}
void Enemy::drawRamMarker() const
{
    if (mState != EnemyState::Charging)
    {
        return;
    }

    VECTOR forward = mRamDirection;
    forward.y = 0.0f;
    if (VSize(forward) <= 0.0001f)
    {
        return;
    }
    forward = VNorm(forward);

    VECTOR side = VGet(forward.z, 0.0f, -forward.x);
    float halfWidth = EnemyParam::AttackWidth;
    VECTOR start = VAdd(mPos, VGet(0.0f, 0.06f, 0.0f));
    VECTOR end = VAdd(start, VScale(forward, mRamLength));
    VECTOR p1 = VAdd(start, VScale(side, halfWidth));
    VECTOR p2 = VAdd(start, VScale(side, -halfWidth));
    VECTOR p3 = VAdd(end, VScale(side, -halfWidth));
    VECTOR p4 = VAdd(end, VScale(side, halfWidth));

    bool tracking = mStateTimer < EnemyParam::RamChargeDuration * 0.8f;
    if (mAnimator.getAnimTotalTime() > 0.0f)
    {
        /// 追尾中は黄色、方向固定後は赤にして、回避すべきタイミングを見せる。
        tracking = mAnimator.getAnimTime() < mAnimator.getAnimTotalTime() * 0.8f;
    }
    unsigned int color = tracking ? GetColor(255, 220, 70) : GetColor(255, 45, 35);

    SetUseBackCulling(FALSE);
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, tracking ? 70 : 100);
    DrawTriangle3D(p1, p2, p3, color, TRUE);
    DrawTriangle3D(p1, p3, p4, color, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    DrawLine3D(p1, p2, color);
    DrawLine3D(p2, p3, color);
    DrawLine3D(p3, p4, color);
    DrawLine3D(p4, p1, color);

    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(FALSE);
    SetUseBackCulling(TRUE);
}
void Enemy::drawAttackTrail() const
{
    if (mAttackTrailPointCount <= 1 || mAttackTrailFadeTimer <= 0.0f)
    {
        return;
    }

    int alpha = static_cast<int>(190.0f * (mAttackTrailFadeTimer / EnemyAttackTrailDuration));
    if (mState == EnemyState::Attack)
    {
        alpha = 190;
    }
    if (alpha < 0) alpha = 0;
    if (alpha > 190) alpha = 190;

    unsigned int bloodColor = GetColor(210, 18, 12);
    unsigned int hotEdgeColor = GetColor(255, 145, 65);
    unsigned int glowColor = GetColor(255, 55, 35);

    SetUseBackCulling(FALSE);
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);

    int maxSegment = mAttackTrailPointCount - 1;
    for (int i = 1; i < mAttackTrailPointCount; ++i)
    {
        /// 新しい軌跡ほど強く光らせ、攻撃方向を読みやすくする。
        float segmentRate = maxSegment > 0 ? static_cast<float>(i) / static_cast<float>(maxSegment) : 1.0f;
        int segmentAlpha = static_cast<int>(alpha * (0.25f + segmentRate * 0.75f));
        SetDrawBlendMode(DX_BLENDMODE_ADD, segmentAlpha);

        VECTOR innerA = mAttackTrailInnerPoints[i - 1];
        VECTOR outerA = mAttackTrailOuterPoints[i - 1];
        VECTOR innerB = mAttackTrailInnerPoints[i];
        VECTOR outerB = mAttackTrailOuterPoints[i];
        drawEnemyTrailBand(innerA, outerA, innerB, outerB, bloodColor);

        VECTOR hotInnerA = VAdd(innerA, VScale(VSub(outerA, innerA), 0.58f));
        VECTOR hotInnerB = VAdd(innerB, VScale(VSub(outerB, innerB), 0.58f));
        drawEnemyTrailBand(hotInnerA, outerA, hotInnerB, outerB, hotEdgeColor);

        VECTOR moveDir = VSub(outerB, outerA);
        if (VSize(moveDir) > 0.0001f)
        {
            moveDir = VNorm(moveDir);
            VECTOR trailBack = VScale(moveDir, -1.0f);
            VECTOR lift = VGet(0.0f, 0.8f + 2.4f * segmentRate, 0.0f);
            VECTOR glowOuterA = VAdd(outerA, VAdd(lift, VScale(trailBack, 6.0f)));
            VECTOR glowOuterB = VAdd(outerB, VAdd(lift, VScale(trailBack, 6.0f)));
            VECTOR glowInnerA = VAdd(outerA, VAdd(lift, VScale(trailBack, 1.5f)));
            VECTOR glowInnerB = VAdd(outerB, VAdd(lift, VScale(trailBack, 1.5f)));
            SetDrawBlendMode(DX_BLENDMODE_ADD, segmentAlpha / 2);
            drawEnemyTrailBand(glowInnerA, glowOuterA, glowInnerB, glowOuterB, glowColor);
        }
    }


    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(FALSE);
    SetUseBackCulling(TRUE);
}
void Enemy::drawLeftHandTrailDebugPoints() const
{
    if (mModelHandle == -1 || mLeftHandFrameIndex == -1)
    {
        return;
    }

    VECTOR inner = VAdd(MV1GetFramePosition(mModelHandle, mLeftHandFrameIndex), VGet(EnemyLeftHandInnerOffsetX, EnemyLeftHandInnerOffsetY, EnemyLeftHandInnerOffsetZ));
    VECTOR outer = getLeftHandTrailTipPosition();

    unsigned int innerColor = GetColor(255, 255, 0);
    unsigned int outerColor = GetColor(0, 255, 255);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawSphere3D(inner, 0.35f, 8, innerColor, innerColor, TRUE);
    DrawSphere3D(outer, 0.35f, 8, outerColor, outerColor, TRUE);
    DrawLine3D(inner, outer, GetColor(255, 255, 255));
}
bool Enemy::isAttackThreatening() const
{
    /// 相棒 AI が防御判断に使うため、危険な攻撃状態だけ true にする。
    if (mState == EnemyState::JumpAttack)
    {
        return !mJumpAttackLanded;
    }

    if (mState == EnemyState::Charging || mState == EnemyState::Ram || mState == EnemyState::Attack360)
    {
        return true;
    }

    if (mState != EnemyState::Attack || mAnimator.getAnimTotalTime() <= 0.0f)
    {
        return false;
    }

    float rate = mAnimator.getAnimTime() / mAnimator.getAnimTotalTime();
    return rate >= 0.25f && rate <= 0.58f;
}
bool Enemy::isCounterTiming() const
{
    /// カウンター可能時間は攻撃種類ごとに読みやすい窓へ限定する。
    if (mState == EnemyState::JumpAttack)
    {
        return !mJumpAttackLanded;
    }

    if (mState == EnemyState::Ram || mState == EnemyState::Attack360)
    {
        return !mAttackHitDone;
    }

    if (mState != EnemyState::Attack || mAnimator.getAnimTotalTime() <= 0.0f)
    {
        return false;
    }

    float rate = mAnimator.getAnimTime() / mAnimator.getAnimTotalTime();
    return rate >= 0.28f && rate <= 0.58f;
}
bool Enemy::isDodgeTiming() const
{
    /// 回避判断はカウンターより少し早いタイミングを許可する。
    if (mState == EnemyState::JumpAttack)
    {
        return !mJumpAttackLanded;
    }

    if (mState == EnemyState::Charging || mState == EnemyState::Ram || mState == EnemyState::Attack360)
    {
        return !mAttackHitDone;
    }

    if (mState != EnemyState::Attack || mAnimator.getAnimTotalTime() <= 0.0f)
    {
        return false;
    }

    float rate = mAnimator.getAnimTime() / mAnimator.getAnimTotalTime();
    return rate >= 0.18f && rate <= 0.42f;
}
