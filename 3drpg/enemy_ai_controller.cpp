/**
 * @file
 * @brief enemy_ai_controller.cpp 実装を定義する。
 */

#include "enemy.h"
#include "audio_manager.h"
#include "sound_resource.h"

#include <cmath>

namespace
{
    constexpr float MutantDashFootstepInterval = 0.28f; ///< 突進中の足音を再生する間隔。単位は seconds。
}

void Enemy::updateIdle(float deltaTime, float distanceToPlayer)
{
    (void)deltaTime;
    (void)distanceToPlayer;

    /// Idle は初期状態として使い、実戦中はすぐ Chase に移行する。
    if (mIdleAnimSourceHandle != -1)
    {
        mAnimator.play(mIdleAnimIndex, mIdleAnimSourceHandle, true);
    }

    mState = EnemyState::Chase;
}

void Enemy::updateChase(float deltaTime, VECTOR toPlayer, float distanceToPlayer)
{
    if (mWalkAnimSourceHandle != -1)
    {
        mAnimator.play(mWalkAnimIndex, mWalkAnimSourceHandle, true);
    }
    else if (mIdleAnimSourceHandle != -1)
    {
        mAnimator.play(mIdleAnimIndex, mIdleAnimSourceHandle, true);
    }

    if (distanceToPlayer <= EnemyParam::CloseContactDistance)
    {
        /// 密着が続いた場合だけ 360 度攻撃を出し、接触直後の理不尽な反撃を避ける。
        mCloseContactTimer += deltaTime;
    }
    else
    {
        mCloseContactTimer = 0.0f;
    }

    if (mAttackCooldownTimer <= 0.0f &&
        mCloseContactTimer >= EnemyParam::CloseContactDuration &&
        mSpinAttackCooldownTimer <= 0.0f)
    {
        startAttack360();
        return;
    }

    if (mAttackCooldownTimer <= 0.0f &&
        distanceToPlayer >= EnemyParam::RamTriggerMinDistance &&
        distanceToPlayer <= EnemyParam::RamTriggerMaxDistance &&
        mRamCooldownTimer <= 0.0f)
    {
        /// 中距離では通常攻撃より突進を優先し、距離を詰める行動に変化を出す。
        startCharging(toPlayer, distanceToPlayer);
        return;
    }

    if (distanceToPlayer <= EnemyParam::AttackStartDistance && mAttackCooldownTimer <= 0.0f)
    {
        startAttack();
        return;
    }

    if (distanceToPlayer <= 0.0001f)
    {
        return;
    }

    VECTOR moveDir = VNorm(toPlayer);
    mRot.y = atan2f(moveDir.x, moveDir.z);
    if (mMovementEnabled)
    {
        mPos = VAdd(mPos, VScale(moveDir, mMoveSpeed * deltaTime));
    }
}
void Enemy::startAttack()
{
    mState = EnemyState::Attack;
    mStateTimer = 0.0f;
    mAttackHitDone = false;
    clearAttackTrail();
    mAttackStep = mNextAttackStep;
    mNextAttackStep = mNextAttackStep >= 3 ? 1 : mNextAttackStep + 1;

    /// 通常攻撃は 3 段を順番に使い、同じモーションだけが続かないようにする。
    int animSourceHandle = mAttackAnimSourceHandle;
    int animIndex = mAttackAnimIndex;
    if (mAttackStep == 2 && mAttack2AnimSourceHandle != -1)
    {
        animSourceHandle = mAttack2AnimSourceHandle;
        animIndex = mAttack2AnimIndex;
    }
    else if (mAttackStep == 3 && mAttack3AnimSourceHandle != -1)
    {
        animSourceHandle = mAttack3AnimSourceHandle;
        animIndex = mAttack3AnimIndex;
    }

    if (animSourceHandle != -1)
    {
        mAnimator.play(
            animIndex,
            animSourceHandle,
            false,
            0.08f
        );
    }
}
void Enemy::updateAttack(float deltaTime)
{
    (void)deltaTime;

    if (!mAttackHitDone && mAnimator.getAnimTotalTime() > 0.0f)
    {
        /// 攻撃判定はモーション中盤だけ有効にし、予備動作と戻りには当たりを出さない。
        float activeStart = mAnimator.getAnimTotalTime() * 0.30f;
        float activeEnd = mAnimator.getAnimTotalTime() * 0.75f;
        float animTime = mAnimator.getAnimTime();
        mAttackHitRequest = animTime >= activeStart && animTime <= activeEnd;
    }
    else
    {
        mAttackHitRequest = false;
    }

    if (mAnimator.isFinished())
    {
        mState = EnemyState::Chase;
        mStateTimer = 0.0f;

        mAttackCooldownTimer = getRandomAttackRecovery();
    }
}
void Enemy::startCharging(VECTOR toTarget, float distanceToTarget)
{
    if (VSize(toTarget) <= 0.0001f)
    {
        return;
    }

    mState = EnemyState::Charging;
    mStateTimer = 0.0f;
    mAttackHitDone = false;
    mAttackHitRequest = false;
    clearAttackTrail();
    mRamDirection = VNorm(toTarget);
    mRamDirection.y = 0.0f;
    mRamDirection = VNorm(mRamDirection);
    /// 目標より少し奥まで走らせ、プレイヤー直前で止まる見た目を避ける。
    mRamLength = distanceToTarget + 12.0f;
    if (mRamLength < EnemyParam::RamLength)
    {
        mRamLength = EnemyParam::RamLength;
    }
    mRot.y = atan2f(mRamDirection.x, mRamDirection.z);
    AudioManager::instance()->playSe3D(SoundResource::Se::Enemy::MutantDashRoar, getHeadPosition());

    if (mChargingAnimSourceHandle != -1)
    {
        mAnimator.play(mChargingAnimIndex, mChargingAnimSourceHandle, false, 0.08f);
    }
}

void Enemy::startRam()
{
    mState = EnemyState::Ram;
    mStateTimer = 0.0f;
    mAttackHitDone = false;
    mAttackHitRequest = false;
    mRamStartPos = mPos;
    mRamFootstepSeTimer = 0.0f;

    if (mRamAnimSourceHandle != -1)
    {
        mAnimator.play(mRamAnimIndex, mRamAnimSourceHandle, false, 0.08f);
    }
}

void Enemy::startAttack360()
{
    mState = EnemyState::Attack360;
    mStateTimer = 0.0f;
    mAttackHitDone = false;
    mAttackHitRequest = false;
    clearAttackTrail();
    mCloseContactTimer = 0.0f;

    if (mAttack360AnimSourceHandle != -1)
    {
        mAnimator.play(mAttack360AnimIndex, mAttack360AnimSourceHandle, false, 0.08f);
    }
}

void Enemy::updateCharging(float deltaTime, VECTOR toTarget, float distanceToTarget)
{
    (void)deltaTime;

    bool tracking = mStateTimer < EnemyParam::RamChargeDuration * 0.8f;
    if (mAnimator.getAnimTotalTime() > 0.0f)
    {
        /// チャージ終盤は方向を固定し、予告を見て避けられる時間を残す。
        tracking = mAnimator.getAnimTime() < mAnimator.getAnimTotalTime() * 0.8f;
    }

    if (tracking && VSize(toTarget) > 0.0001f)
    {
        mRamDirection = VNorm(toTarget);
        mRamDirection.y = 0.0f;
        if (VSize(mRamDirection) > 0.0001f)
        {
            mRamDirection = VNorm(mRamDirection);
            mRot.y = atan2f(mRamDirection.x, mRamDirection.z);
        }

        mRamLength = distanceToTarget + 12.0f;
        if (mRamLength < EnemyParam::RamLength)
        {
            mRamLength = EnemyParam::RamLength;
        }
    }

    if (mChargingAnimSourceHandle != -1)
    {
        if (mAnimator.isFinished())
        {
            startRam();
        }
        return;
    }

    if (mStateTimer >= EnemyParam::RamChargeDuration)
    {
        startRam();
    }
}

void Enemy::updateRam(float deltaTime)
{
    mPos = VAdd(mPos, VScale(mRamDirection, EnemyParam::RamSpeed * deltaTime));

    mRamFootstepSeTimer -= deltaTime;
    if (mRamFootstepSeTimer <= 0.0f)
    {
        AudioManager::instance()->playSe3D(SoundResource::Se::Enemy::MutantDashFootstep, mPos);
        mRamFootstepSeTimer = MutantDashFootstepInterval;
    }

    if (!mAttackHitDone)
    {
        /// 突進開始直後はまだ当てず、加速してからヒット要求を出す。
        bool hitNow = false;
        if (mAnimator.getAnimTotalTime() > 0.0f)
        {
            hitNow = mAnimator.getAnimTime() >= mAnimator.getAnimTotalTime() * 0.30f;
        }
        else
        {
            hitNow = mStateTimer >= EnemyParam::RamDuration * 0.30f;
        }

        if (hitNow)
        {
            mAttackHitRequest = true;
        }
    }

    if (mStateTimer >= EnemyParam::RamDuration || mAnimator.isFinished())
    {
        mState = EnemyState::Chase;
        mStateTimer = 0.0f;
        mRamCooldownTimer = EnemyParam::RamCooldown;
        mAttackCooldownTimer = getRandomAttackRecovery();
    }
}

void Enemy::updateAttack360(float deltaTime)
{
    (void)deltaTime;
    if (!mAttackHitDone)
    {
        /// 360 度攻撃は広範囲なので、有効時間を明示的に絞って回避余地を作る。
        bool activeNow = false;
        if (mAnimator.getAnimTotalTime() > 0.0f)
        {
            float activeStart = mAnimator.getAnimTotalTime() * 0.25f;
            float activeEnd = mAnimator.getAnimTotalTime() * 0.85f;
            float animTime = mAnimator.getAnimTime();
            activeNow = animTime >= activeStart && animTime <= activeEnd;
        }
        else
        {
            activeNow = mStateTimer >= 0.25f && mStateTimer <= 0.85f;
        }

        mAttackHitRequest = activeNow;
    }
    else
    {
        mAttackHitRequest = false;
    }

    bool finished = mAttack360AnimSourceHandle != -1
        ? mAnimator.isFinished()
        : mStateTimer >= 0.85f;

    if (finished)
    {
        mState = EnemyState::Chase;
        mStateTimer = 0.0f;
        mSpinAttackCooldownTimer = EnemyParam::SpinAttackCooldown;
        mAttackCooldownTimer = getRandomAttackRecovery();
    }
}

void Enemy::updateTargetThreat(float deltaTime, const CombatTarget* targets, int targetCount)
{
    if (targets == nullptr || targetCount <= 0)
    {
        return;
    }

    for (int i = 0; i < targetCount; ++i)
    {
        if (!targets[i].active)
        {
            continue;
        }

        VECTOR diff = VSub(targets[i].position, mPos);
        diff.y = 0.0f;
        float distance = VSize(diff);

        if (distance < mDetectRadius)
        {
            /// 近い対象ほど脅威値を上げ、攻撃していない相手にも自然に注意を向ける。
            float nearRate = 1.0f - (distance / mDetectRadius);
            mThreat.addThreat(targets[i].source, nearRate * 5.0f * deltaTime);
        }

        if (targets[i].attacking)
        {
            /// 攻撃中の対象は危険度を上げ、敵が反応しやすくする。
            mThreat.addThreat(targets[i].source, 10.0f * deltaTime);
        }

        if (targets[i].support)
        {
            /// 回復や支援行動は高めの脅威として扱う。
            mThreat.addThreat(targets[i].source, 18.0f * deltaTime);
        }
    }
}
const CombatTarget* Enemy::findTarget(const CombatTarget* targets, int targetCount, ThreatSource source) const
{
    /// 現在ターゲットの ThreatSource に対応する有効な対象を探す。
    if (targets == nullptr)
    {
        return nullptr;
    }

    for (int i = 0; i < targetCount; ++i)
    {
        if (targets[i].active && targets[i].source == source)
        {
            return &targets[i];
        }
    }

    return nullptr;
}
float Enemy::getTargetScore(const CombatTarget& target) const
{
    VECTOR diff = VSub(target.position, mPos);
    diff.y = 0.0f;
    float distance = VSize(diff);

    if (distance > mDetectRadius * EnemyParam::FarTargetInvalidRate)
    {
        /// 遠すぎる対象は脅威値が高くても選ばない。
        return -100000.0f;
    }

    float score = mThreat.getThreat(target.source);
    if (target.source == ThreatSource::Player && mIsBoss)
    {
        score += EnemyParam::BossPlayerThreatBonus;
    }

    float penaltyStart = mDetectRadius * EnemyParam::FarTargetPenaltyStartRate;
    if (distance > penaltyStart)
    {
        /// 有効範囲内でも遠い対象は少し不利にし、ターゲットが頻繁に飛ばないようにする。
        score -= (distance - penaltyStart) * EnemyParam::FarTargetPenaltyScale;
    }

    return score;
}
VECTOR Enemy::chooseTargetPosition(const CombatTarget* targets, int targetCount, VECTOR fallbackPos)
{
    if (targets == nullptr || targetCount <= 0)
    {
        mCurrentTargetSource = ThreatSource::Player;
        return fallbackPos;
    }

    const CombatTarget* currentTarget = findTarget(targets, targetCount, mCurrentTargetSource);
    if (currentTarget != nullptr && mTargetSwitchCooldownTimer > 0.0f)
    {
        /// 一度選んだ対象は短時間固定し、毎フレームのスコア差で視線が揺れないようにする。
        return currentTarget->position;
    }

    const CombatTarget* bestTarget = nullptr;
    float bestScore = -1000000.0f;

    /// 全ターゲットのスコアを比較し、最も脅威が高い対象を選ぶ。
    for (int i = 0; i < targetCount; ++i)
    {
        if (!targets[i].active)
        {
            continue;
        }

        float score = getTargetScore(targets[i]);
        if (bestTarget == nullptr || score > bestScore)
        {
            bestTarget = &targets[i];
            bestScore = score;
        }
    }

    if (bestTarget == nullptr)
    {
        mCurrentTargetSource = ThreatSource::Player;
        return fallbackPos;
    }

    if (mIsBoss && bestTarget->source == ThreatSource::Companion)
    {
        /// ボスは相棒へ偏りすぎないよう、一定確率でプレイヤーを優先する。
        const CombatTarget* playerTarget = findTarget(targets, targetCount, ThreatSource::Player);
        if (playerTarget != nullptr && GetRand(99) >= EnemyParam::BossCompanionTargetChance)
        {
            bestTarget = playerTarget;
        }
    }

    if (mCurrentTargetSource != bestTarget->source)
    {
        mCurrentTargetSource = bestTarget->source;
        mTargetSwitchCooldownTimer = EnemyParam::TargetSwitchCooldown;
    }

    return bestTarget->position;
}
