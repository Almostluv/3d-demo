/**
 * @file
 * @brief enemy_jump_attack.cpp 実装を定義する。
 */

#include "enemy.h"
#include "enemy_attack_range_marker.h"
#include "audio_manager.h"
#include "sound_resource.h"
#include "DxLib.h"
#include <cmath>

namespace
{
constexpr float JumpAttackGroundRayTop = 160.0f; ///< ジャンプ攻撃の着地点検索レイ開始高さ。
constexpr float JumpAttackGroundRayBottom = 260.0f; ///< ジャンプ攻撃の着地点検索レイを下方向へ伸ばす距離。
constexpr float JumpAttackLandingVisualOffsetY = -1.2f; ///< 着地表示を地面へなじませる Y オフセット。

bool getGroundHit(int stageCollisionModelHandle, VECTOR pos, VECTOR* hitPos)
{
    if (stageCollisionModelHandle == -1 || hitPos == nullptr)
    {
        return false;
    }

    /// 着地予定点の上下にレイを通し、ステージ衝突モデル上の床高さを取得する。
    VECTOR start = VAdd(pos, VGet(0.0f, JumpAttackGroundRayTop, 0.0f));
    VECTOR end = VAdd(pos, VGet(0.0f, -JumpAttackGroundRayBottom, 0.0f));
    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(stageCollisionModelHandle, -1, start, end);

    if (!hit.HitFlag)
    {
        return false;
    }

    *hitPos = hit.HitPosition;
    return true;
}
}
VECTOR Enemy::getJumpAttackHitCenter() const
{
    if (mModelHandle != -1 && mJumpAttackToeFrameIndex != -1)
    {
        /// 実際の着地判定はつま先フレームを優先し、アニメーションの見た目に寄せる。
        VECTOR toePos = MV1GetFramePosition(mModelHandle, mJumpAttackToeFrameIndex);
        toePos.y = mJumpAttackGroundY;
        return toePos;
    }

    /// フレームが取れない場合は、ジャンプ開始時に決めた着地点を使う。
    VECTOR center = mJumpAttackTargetPos;
    center.y = mJumpAttackGroundY;
    return center;
}
VECTOR Enemy::getJumpAttackBodyGroundPosition() const
{
    if (mModelHandle != -1 && mJumpAttackBodyFrameIndex != -1)
    {
        /// 着地後の位置補正に使うため、腰フレームを地面高さへ投影する。
        VECTOR bodyPos = MV1GetFramePosition(mModelHandle, mJumpAttackBodyFrameIndex);
        bodyPos.y = mJumpAttackGroundY;
        return bodyPos;
    }

    VECTOR bodyPos = mPos;
    bodyPos.y = mJumpAttackGroundY;
    return bodyPos;
}

VECTOR Enemy::getJumpAttackMarkerCenter() const
{
    VECTOR center = mJumpAttackTargetPos;
    center.y = mJumpAttackGroundY;

    if (mJumpAttackStarted && mModelHandle != -1 && mJumpAttackBodyFrameIndex != -1)
    {
        /// ジャンプ中は体の横ずれをマーカーにも反映し、予告範囲と着地位置を近づける。
        VECTOR bodyPos = MV1GetFramePosition(mModelHandle, mJumpAttackBodyFrameIndex);
        VECTOR bodyOffset = VSub(bodyPos, mPos);
        bodyOffset.y = 0.0f;
        center = VAdd(center, bodyOffset);
        center.y = mJumpAttackGroundY;
    }

    return center;
}
void Enemy::updateJumpAttack(float deltaTime)
{
    (void)deltaTime;

    if (!mJumpAttackStarted)
    {
        if (mStateTimer < mJumpAttackStartupDuration)
        {
            return;
        }

        mJumpAttackStarted = true;
        mStateTimer = 0.0f;

        if (mJumpAttackAnimSourceHandle != -1)
        {
            /// ため時間が終わってからジャンプアニメーションを開始する。
            mAnimator.play(mJumpAttackAnimIndex, mJumpAttackAnimSourceHandle, false, 0.05f);
        }
    }

    if (mJumpAttackLanded)
    {
        /// 着地後は地面高さへ固定し、硬直時間が終わるまで次の行動へ移らない。
        mPos.y = mJumpAttackGroundY;

        if (mStateTimer < mJumpAttackRecoveryDuration)
        {
            return;
        }

        VECTOR landingBodyPos = mJumpAttackLandingBodyPos;
        mState = mIsIntroLanding ? EnemyState::Idle : EnemyState::Chase;
        mIsIntroLanding = false;
        mStateTimer = 0.0f;
        mAttackCooldownTimer = getRandomAttackRecovery();
        mJumpAttackHitDone = false;
        mJumpAttackStarted = false;
        mJumpAttackLanded = false;

        if (mWalkAnimSourceHandle != -1)
        {
            /// 着地硬直が終わってから追跡アニメーションへ戻す。
            mAnimator.play(mWalkAnimIndex, mWalkAnimSourceHandle, true, 0.0f);
        }
        else if (mIdleAnimSourceHandle != -1)
        {
            mAnimator.play(mIdleAnimIndex, mIdleAnimSourceHandle, true, 0.0f);
        }

        if (mModelHandle != -1)
        {
            /// アニメーションの腰位置と論理座標の差を補正し、着地後の滑りを抑える。
            syncModelTransform();
            VECTOR currentBodyPos = getJumpAttackBodyGroundPosition();
            VECTOR bodyDiff = VSub(landingBodyPos, currentBodyPos);
            bodyDiff.y = 0.0f;
            mPos = VAdd(mPos, bodyDiff);
            mPos.y = mJumpAttackGroundY;
            syncModelTransform();
        }
        return;
    }

    float moveProgress = mJumpAttackDuration > 0.0f ? mStateTimer / mJumpAttackDuration : 1.0f;
    if (moveProgress > 1.0f)
    {
        moveProgress = 1.0f;
    }

    float ease = moveProgress * moveProgress * (3.0f - (2.0f * moveProgress));
    VECTOR jumpMove = VSub(mJumpAttackTargetPos, mJumpAttackStartPos);

    /// アニメーションは見た目用にし、放物線移動はコード側で固定して制御する。
    mPos = VAdd(mJumpAttackStartPos, VScale(jumpMove, ease));
    if (mIsIntroLanding)
    {
        mPos.y = mJumpAttackStartPos.y + ((mJumpAttackGroundY - mJumpAttackStartPos.y) * ease);
    }
    else
    {
        mPos.y = mJumpAttackStartPos.y + ((mJumpAttackGroundY - mJumpAttackStartPos.y) * ease) + sinf(moveProgress * DX_PI_F) * mJumpAttackHeight;
    }

    if (moveProgress >= 1.0f)
    {
        mPos.y = mJumpAttackGroundY;
    }

    if (!mJumpAttackHitDone && moveProgress >= 1.0f)
    {
        /// 実際の攻撃判定は着地フレームで一度だけ発生させる。
        if (mModelHandle != -1)
        {
            syncModelTransform();
        }
        mJumpAttackMarkerCenter = getJumpAttackMarkerCenter();
        /// 着地時点のマーカー位置を保存し、描画と攻撃判定の中心を一致させる。
        mJumpAttackHitCenter = mJumpAttackMarkerCenter;
        mJumpAttackLandingBodyPos = mJumpAttackMarkerCenter;
        mPos = mJumpAttackTargetPos;
        mPos.y = mJumpAttackGroundY;
        mCurrentAttackRadius = mJumpAttackRadius;
        mAttackHitRequest = !mIsIntroLanding;
        AudioManager::instance()->playSe3D(SoundResource::Se::Enemy::MutantDashFootstep, mJumpAttackMarkerCenter);
        mJumpAttackHitDone = true;
        mJumpAttackLanded = true;
        mStateTimer = 0.0f;
        return;
    }
}


void Enemy::resolveJumpAttackStageCollision(int stageCollisionModelHandle)
{
    /// ステージ衝突モデルを保持し、描画マーカーと着地高さの両方で使う。
    mJumpAttackStageCollisionModelHandle = stageCollisionModelHandle;

    if (mState != EnemyState::JumpAttack || stageCollisionModelHandle == -1)
    {
        return;
    }

    VECTOR targetGroundPos;
    if (getGroundHit(stageCollisionModelHandle, mJumpAttackTargetPos, &targetGroundPos))
    {
        /// 地面が見つかった場合は着地点を床高さへ更新する。
        mJumpAttackGroundY = targetGroundPos.y + JumpAttackLandingVisualOffsetY;
        mJumpAttackTargetPos.y = mJumpAttackGroundY;
    }

    if (mJumpAttackLanded)
    {
        mPos.y = mJumpAttackGroundY;
        return;
    }

}
void Enemy::drawJumpAttackMarker() const
{
    if (mState != EnemyState::JumpAttack || mJumpAttackLanded || mIsIntroLanding)
    {
        return;
    }

    /// 着地が近いほど赤点滅にし、プレイヤーが回避タイミングを判断できるようにする。
    float moveProgress = mJumpAttackStarted && mJumpAttackDuration > 0.0f ? mStateTimer / mJumpAttackDuration : 0.0f;
    bool landingSoon = mJumpAttackStarted && moveProgress >= 0.78f;
    VECTOR center = mJumpAttackHitDone ? mJumpAttackHitCenter : getJumpAttackMarkerCenter();
    center.y = mJumpAttackGroundY + 0.25f;

    drawEnemyAttackRangeMarker(center, mJumpAttackRadius, landingSoon, mStateTimer, mJumpAttackStageCollisionModelHandle);
}

void Enemy::startJumpAttack(VECTOR playerPos)
{
    /// ジャンプ攻撃へ入る時は通常攻撃・被弾・ノックバック系の一時状態を初期化する。
    mState = EnemyState::JumpAttack;
    mStateTimer = 0.0f;
    mAttackHitRequest = false;
    mAttackHitDone = false;
    mJumpAttackHitDone = false;
    mJumpAttackStarted = false;
    mJumpAttackLanded = false;
    mHit2ReactionActive = false;
    mHitAnimPlaying = false;
    mKnockbackActive = false;
    mKnockbackAppliedDistance = 0.0f;
    mIsIntroLanding = false;

    mJumpAttackStartPos = mPos;
    mJumpAttackHitCenter = mPos;
    mJumpAttackMarkerCenter = mPos;
    mJumpAttackLandingBodyPos = mPos;
    mJumpAttackGroundY = mPos.y;
    mJumpAttackCooldownTimer = mJumpAttackCooldown;

    VECTOR jumpDir = VSub(playerPos, mJumpAttackStartPos);
    jumpDir.y = 0.0f;
    if (VSize(jumpDir) > 0.0001f)
    {
        /// プレイヤーの少し手前へ着地させ、密着しすぎる着地点を避ける。
        jumpDir = VNorm(jumpDir);
        mRot.y = atan2f(jumpDir.x, jumpDir.z);
        mJumpAttackTargetPos = VSub(playerPos, VScale(jumpDir, mJumpAttackLandingOffsetDistance));
    }
    else
    {
        mJumpAttackTargetPos = playerPos;
    }
    mJumpAttackTargetPos.y = mJumpAttackGroundY;
    mJumpAttackMarkerCenter = mJumpAttackTargetPos;

    VECTOR jumpMove = VSub(mJumpAttackTargetPos, mJumpAttackStartPos);
    jumpMove.y = 0.0f;
    float jumpDistance = VSize(jumpMove);

    /// 距離が長いほど移動時間と高さを増やすが、上限下限で読みやすい範囲に収める。
    mJumpAttackDuration = 0.75f + (jumpDistance * 0.012f);
    if (mJumpAttackDuration < mJumpAttackMinDuration)
    {
        mJumpAttackDuration = mJumpAttackMinDuration;
    }
    if (mJumpAttackDuration > mJumpAttackMaxDuration)
    {
        mJumpAttackDuration = mJumpAttackMaxDuration;
    }

    mJumpAttackHeight = 12.0f + (jumpDistance * 0.12f);
    if (mJumpAttackHeight < mJumpAttackMinHeight)
    {
        mJumpAttackHeight = mJumpAttackMinHeight;
    }
    if (mJumpAttackHeight > mJumpAttackMaxHeight)
    {
        mJumpAttackHeight = mJumpAttackMaxHeight;
    }
}

bool Enemy::shouldStartJumpAttack(VECTOR playerPos) const
{
    if (mJumpAttackAnimSourceHandle == -1 || mJumpAttackCooldownTimer > 0.0f || mAttackCooldownTimer > 0.0f)
    {
        return false;
    }

    /// 遠距離では確定でジャンプ攻撃を選び、近距離では確率で混ぜる。
    VECTOR toPlayer = VSub(playerPos, mPos);
    toPlayer.y = 0.0f;
    float distanceToPlayer = VSize(toPlayer);

    if (distanceToPlayer > mJumpAttackEffectiveDistance)
    {
        return true;
    }

    return GetRand(99) < 50;
}

void Enemy::startIntroLanding(VECTOR landingPos)
{
    /// 登場演出用の着地は攻撃判定を出さず、ジャンプ状態の見た目だけを利用する。
    mState = EnemyState::JumpAttack;
    mStateTimer = 0.0f;
    mAttackHitRequest = false;
    mAttackHitDone = false;
    mJumpAttackHitDone = false;
    mJumpAttackStarted = true;
    mJumpAttackLanded = false;
    mHit2ReactionActive = false;
    mHitAnimPlaying = false;
    mKnockbackActive = false;
    mKnockbackAppliedDistance = 0.0f;
    mIsIntroLanding = true;

    mJumpAttackStartPos = mPos;
    mJumpAttackHitCenter = landingPos;
    mJumpAttackMarkerCenter = landingPos;
    mJumpAttackLandingBodyPos = landingPos;
    mJumpAttackTargetPos = landingPos;
    mJumpAttackGroundY = landingPos.y + JumpAttackLandingVisualOffsetY;
    mJumpAttackDuration = 1.6f;
    mJumpAttackHeight = 0.0f;
    mJumpAttackCooldownTimer = mJumpAttackCooldown;

    if (mJumpAttackAnimSourceHandle != -1)
    {
        mAnimator.play(mJumpAttackAnimIndex, mJumpAttackAnimSourceHandle, false, 0.0f);
    }
}
