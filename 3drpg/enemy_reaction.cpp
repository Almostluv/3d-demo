/**
 * @file
 * @brief enemy_reaction.cpp 実装を定義する。
 */

#include "enemy.h"
#include "DxLib.h"

void Enemy::updateHit(float deltaTime, VECTOR playerPos)
{
    (void)deltaTime;

    if (mKnockbackActive)
    {
        /// ノックバック距離は時間またはヒットアニメーション進行率に合わせて段階的に適用する。
        float progress = mKnockbackDuration > 0.0f ? mStateTimer / mKnockbackDuration : 1.0f;

        if (mHitAnimPlaying && mAnimator.getAnimTotalTime() > 0.0f)
        {
            progress = mAnimator.getAnimTime() / (mAnimator.getAnimTotalTime() * 0.85f);
        }

        if (progress > 1.0f)
        {
            progress = 1.0f;
        }

        /// ease-out にして、吹き飛びの終わりが急に止まりすぎないようにする。
        float easeOut = 1.0f - ((1.0f - progress) * (1.0f - progress));
        float targetDistance = mKnockbackTotalDistance * easeOut;
        float moveDistance = targetDistance - mKnockbackAppliedDistance;

        if (moveDistance > 0.0f)
        {
            mPos = VAdd(mPos, VScale(mKnockbackDirection, moveDistance));
            mKnockbackAppliedDistance = targetDistance;
        }
    }

    bool hitFinished = mHitAnimPlaying ? mAnimator.isFinished() : mStateTimer >= mHitDuration;
    if (hitFinished)
    {
        if (mKnockbackActive)
        {
            /// アニメーション差分で不足しやすい最終距離を補正する。
            mPos = VAdd(mPos, VScale(mKnockbackDirection, mKnockbackEndCompensationDistance));
        }

        if (mHit2ReactionActive && shouldStartJumpAttack(playerPos))
        {
            /// 強いリアクション後に条件を満たす場合は、反撃としてジャンプ攻撃へ移る。
            startJumpAttack(playerPos);
            return;
        }

        /// ヒット硬直後は移動アニメーションへ戻し、通常追跡状態に復帰する。
        if (mWalkAnimSourceHandle != -1)
        {
            mAnimator.play(mWalkAnimIndex, mWalkAnimSourceHandle, true, 0.0f);
        }
        else if (mIdleAnimSourceHandle != -1)
        {
            mAnimator.play(mIdleAnimIndex, mIdleAnimSourceHandle, true, 0.0f);
        }

        mState = EnemyState::Chase;
        mStateTimer = 0.0f;
        mHitAnimPlaying = false;
        mHit2ReactionActive = false;
        mKnockbackActive = false;
        mKnockbackAppliedDistance = 0.0f;
    }
}

void Enemy::updateDying(float deltaTime)
{
    (void)deltaTime;
}

bool Enemy::counterReact(VECTOR attackerPos)
{
    if (mState == EnemyState::Dying)
    {
        return false;
    }

    /// カウンター成功時は強リアクションと短いヒットストップを発生させる。
    startHit(3, attackerPos);
    mHitStopTimer = EnemyParam::CounterHitStopDuration;
    return true;
}
EnemyDamageResult Enemy::damage(int value, int playerAttackStep, VECTOR attackerPos, ThreatSource source, bool shieldBreaker)
{
    if (mState == EnemyState::Dying)
    {
        return EnemyDamageResult::None;
    }

    /// ダメージ量を脅威値へ反映し、敵 AI のターゲット選択に使う。
    mThreat.addDamageThreat(source, value);

    if (mBossShieldEnabled && mBossShieldHp > 0)
    {
        if (value > 0)
        {
            /// シールドブレイク攻撃以外はシールドへのダメージを軽減する。
            int shieldDamage = shieldBreaker ? value : value / 2;
            if (shieldDamage <= 0)
            {
                shieldDamage = 1;
            }
            mBossShieldHp -= shieldDamage;
            if (mBossShieldHp <= 0)
            {
                /// シールドが尽きたらブレイク演出用タイマーを開始する。
                mBossShieldHp = 0;
                mBossShieldBreakTimer = EnemyParam::BossShieldBreakDuration;
            }
        }
        return EnemyDamageResult::Shield;
    }

    /// シールドがない場合のみ本体 HP を減らす。
    mHp -= value;
    if (mHp < 0)
    {
        mHp = 0;
    }

    if (mHp <= 0)
    {
        /// HP が 0 になったら死亡状態へ移り、以後の通常リアクションは行わない。
        startDying();
        return EnemyDamageResult::Hp;
    }

    if (shouldReactToHit(playerAttackStep))
    {
        /// 攻撃段数と敵状態に応じて、のけぞりを開始する。
        startHit(playerAttackStep, attackerPos);
    }

    return EnemyDamageResult::Hp;
}

bool Enemy::shouldReactToHit(int playerAttackStep) const
{
    if (!mIsBoss)
    {
        return true;
    }

    if (mState == EnemyState::JumpAttack)
    {
        /// ボスのジャンプ攻撃中は攻撃を中断させない。
        return false;
    }

    /// ボスは通常攻撃では怯まず、コンボ最終段だけリアクションする。
    return playerAttackStep == 3;
}

bool Enemy::consumeAttackHitRequest()
{
    if (!mAttackHitRequest)
    {
        return false;
    }

    mAttackHitRequest = false;
    return true;
}

void Enemy::startHit(int playerAttackStep, VECTOR attackerPos)
{
    /// ヒット状態へ入る前に、攻撃中の一時フラグをリセットする。
    mState = EnemyState::Hit;
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

    if (playerAttackStep == 3 && mHit2AnimSourceHandle != -1)
    {
        /// コンボ最終段は攻撃者から離れる方向へノックバックを設定する。
        VECTOR knockbackDir = VSub(mPos, attackerPos);
        knockbackDir.y = 0.0f;
        if (VSize(knockbackDir) > 0.0001f)
        {
            mKnockbackDirection = VNorm(knockbackDir);
            mKnockbackActive = true;
        }

        /// 強リアクション用アニメーションを優先し、終了後の反撃判定に使う。
        mAnimator.play(mHit2AnimIndex, mHit2AnimSourceHandle, false, 0.05f);
        mHit2ReactionActive = true;
        mHitAnimPlaying = true;
        return;
    }

    if (mHit1AnimSourceHandle != -1)
    {
        /// 通常ヒットは短いリアクションアニメーションだけを再生する。
        mAnimator.play(mHit1AnimIndex, mHit1AnimSourceHandle, false, 0.05f);
        mHitAnimPlaying = true;
    }
}

void Enemy::startDying()
{
    /// 死亡状態へ入る時に攻撃・ヒット関連の一時フラグをすべて落とす。
    mState = EnemyState::Dying;
    mStateTimer = 0.0f;
    mAttackHitRequest = false;
    mAttackHitDone = false;
    mJumpAttackStarted = false;
    mJumpAttackLanded = false;
    mHitAnimPlaying = false;
    mDyingAnimPlaying = false;
    mKnockbackActive = false;

    if (mDyingAnimSourceHandle != -1)
    {
        /// 死亡アニメーションがある場合は一度だけ再生する。
        mAnimator.play(mDyingAnimIndex, mDyingAnimSourceHandle, false, 0.08f);
        mDyingAnimPlaying = true;
    }
}
