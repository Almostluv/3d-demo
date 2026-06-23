/**
 * @file
 * @brief player_combat.cpp 実装を定義する。
 */

#include "player.h"
#include "player_param.h"
#include "audio_manager.h"
#include "sound_resource.h"
#include "DxLib.h"
#include <cmath>

namespace
{
    constexpr float DelayedCharaSeTime = 0.5f; ///< 攻撃モーションに合わせてキャラクター SE を遅らせる時間。単位は seconds。
    constexpr float MagicCircleSeDelay = 1.0f; ///< 魔法陣 SE を発生タイミングに合わせるための遅延時間。単位は seconds。
}

void Player::updateAttack(float deltaTime)
{
    mStateTimer += deltaTime;

    if (mIsAiControlled && mCharacterType != PlayerCharacterType::Wizard)
    {
        /// AI 操作の騎士は防御系入力を攻撃中でも優先し、相棒として被弾を避けやすくする。
        if (mInput.counter)
        {
            startCounter();
            return;
        }

        if (mInput.dodge)
        {
            startDodge();
            return;
        }

        if (mInput.guard)
        {
            startGuard();
            return;
        }
    }

    if (mCharacterType == PlayerCharacterType::Wizard && mAttackStep == 2)
    {
        /// Wizard のビームはボタン保持中だけ継続し、再生時間に比例してスタミナを消費する。
        float beamDuration = mAnimator.getAnimTotalTime() > 0.0f ? mAnimator.getAnimTotalTime() / 40.0f : mAttackDuration;
        float beamCost = PlayerParam::Stamina * deltaTime / beamDuration;
        if (!consumeStamina(beamCost))
        {
            mState = PlayerState::Idle;
            mStateTimer = 0.0f;
            mAttackStep = 0;
            stopWizardBeamSe();
            return;
        }

        mWizardMagicHitTimer += deltaTime;
        if (mWizardMagicHitTimer >= PlayerParam::WizardMagicAttack2Interval)
        {
            /// 初回ヒット発生時にループ SE へ切り替え、以降は一定間隔でヒット要求を出す。
            if (!mWizardBeamFireSePlayed)
            {
                mWizardBeamFireSePlayed = true;
                AudioManager::instance()->playSe(SoundResource::Se::Chara::WizardBeamFire);
                mWizardBeamLoopSeHandle = AudioManager::instance()->playLoopSe(SoundResource::Se::Chara::WizardBeamLoop);
            }
            mWizardMagicHitTimer = 0.0f;
            mAttackHitRequest = true;
        }

        if (!mInput.guard)
        {
            mState = PlayerState::Idle;
            mStateTimer = 0.0f;
            mAttackStep = 0;
            stopWizardBeamSe();
        }
        return;
    }
    tryQueueNextAttack();

    if (!mAttackHitDone && mAnimator.getAnimTotalTime() > 0.0f)
    {
        /// 近接攻撃の実ヒットはモーション序盤ではなく、振りが入った 35% 付近で発生させる。
        float hitTiming = mAnimator.getAnimTotalTime() * 0.35f;
        if (mAnimator.getAnimTime() >= hitTiming)
        {
            mAttackHitRequest = true;
            mAttackHitDone = true;
        }
    }

    if (mAnimator.getAnimTotalTime() > 0.0f)
    {
        /// 入力先行受付はモーション後半から有効にし、連打で即座に次段へ飛ばないようにする。
        float comboWindow = mAnimator.getAnimTotalTime() * 0.65f;

        if (mAnimator.getAnimTime() >= comboWindow && mNextAttackQueued)
        {
            if (mAttackStep == 1)
            {
                startAttackStep(2);
                return;
            }

            if (mAttackStep == 2)
            {
                startAttackStep(3);
                return;
            }
        }
    }

    if (mAnimator.isFinished() || mStateTimer >= mAttackDuration)
    {
        mState = PlayerState::Idle;
        mStateTimer = 0.0f;
        mAttackStep = 0;
        mNextAttackQueued = false;
        mAttackHitDone = false;
    }
}

void Player::startAttack()
{
    startAttackStep(1);
}

void Player::startWizardMagicAttack2()
{
    if (mStamina < PlayerParam::WizardMagicAttack2StartStamina)
    {
        mState = PlayerState::Idle;
        mStateTimer = 0.0f;
        return;
    }

    startAttackStep(2);
}

void Player::startWizardAreaPowerUp()
{
    startAttackStep(3);
}
void Player::startAttackStep(int step)
{
    float staminaCost = getAttackStepStaminaCost(step);
    if (staminaCost > 0.0f && !consumeStamina(staminaCost))
    {
        return;
    }

    mState = PlayerState::Attack;
    mStateTimer = 0.0f;
    mAttackStep = step;
    mNextAttackQueued = false;
    mAttackHitDone = false;
    mWizardMagicHitTimer = PlayerParam::WizardMagicAttack2Interval;

    mWizardBeamFireSePlayed = false;
    mWizardBeamLoopSeHandle = -1;
    if (mCharacterType == PlayerCharacterType::Wizard && mAttackStep == 2)
    {
        /// ビームは開始時だけヒット間隔を短くし、チャージ後すぐに攻撃判定を出す。
        mWizardMagicHitTimer = PlayerParam::WizardMagicAttack2Interval - PlayerParam::WizardMagicAttack2StartDelay;
    }

    if (mCharacterType == PlayerCharacterType::Wizard)
    {
        /// Wizard の各攻撃はエフェクト発生位置に合わせて SE の再生タイミングを分ける。
        if (mAttackStep == 1)
        {
            mMagicMissileSeTimer = DelayedCharaSeTime;
        }
        else if (mAttackStep == 2)
        {
            AudioManager::instance()->playSe(SoundResource::Se::Chara::WizardBeamCharging);
        }
        else if (mAttackStep == 3)
        {
            mMagicCircleSeTimer = MagicCircleSeDelay;
        }
    }
    else
    {
        AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightSlash);
    }

    switch (mAttackStep)
    {
    case 1:
        if (mAttack1AnimSourceHandle != -1)
        {
            mAnimator.play(mAttack1AnimIndex, mAttack1AnimSourceHandle, false);
        }
        break;

    case 2:
        if (mAttack2AnimSourceHandle != -1)
        {
            mAnimator.play(mAttack2AnimIndex, mAttack2AnimSourceHandle, mCharacterType == PlayerCharacterType::Wizard);
        }
        break;

    case 3:
        if (mAttack3AnimSourceHandle != -1)
        {
            mAnimator.play(mAttack3AnimIndex, mAttack3AnimSourceHandle, false);
        }
        break;
    }
}

void Player::tryQueueNextAttack()
{
    if (mCharacterType == PlayerCharacterType::Wizard)
    {
        return;
    }

    if (mInput.attack)
    {
        mNextAttackQueued = true;
    }
}

bool Player::isAreaPowerUp() const
{
    return mCharacterType == PlayerCharacterType::Wizard && mAttackStep == 3;
}

bool Player::isPowerUpActive() const
{
    return mPowerUpTimer > 0.0f;
}

float Player::getAreaPowerUpRadius() const
{
    return PlayerParam::WizardAreaPowerUpRadius;
}

void Player::applyAreaPowerUp()
{
    /// 回復と攻撃力上昇を同時に適用し、効果時間終了時に攻撃力だけ戻す。
    mHp += PlayerParam::WizardAreaPowerUpHeal;
    if (mHp > PlayerParam::Hp)
    {
        mHp = PlayerParam::Hp;
    }

    mAttackPower = PlayerParam::WizardAreaPowerUpAttackPower;
    mPowerUpTimer = PlayerParam::WizardAreaPowerUpDuration;
}

float Player::getAttackStepStaminaCost(int step) const
{
    if (mCharacterType == PlayerCharacterType::Wizard)
    {
        if (step == 1) return PlayerParam::WizardMagicAttack1Stamina;
        if (step == 3) return PlayerParam::WizardAreaPowerUpStamina;
        return 0.0f;
    }

    if (step == 1) return PlayerParam::KnightAttack1Stamina;
    if (step == 2) return PlayerParam::KnightAttack2Stamina;
    if (step == 3) return PlayerParam::KnightAttack3Stamina;
    return 0.0f;
}

void Player::stopWizardBeamSe()
{
    bool playedBeamSe = mWizardBeamLoopSeHandle != -1 || mWizardBeamFireSePlayed;
    if (mWizardBeamLoopSeHandle != -1)
    {
        AudioManager::instance()->stopSe(mWizardBeamLoopSeHandle);
        mWizardBeamLoopSeHandle = -1;
    }

    if (playedBeamSe)
    {
        AudioManager::instance()->playSe(SoundResource::Se::Chara::WizardBeamEnd);
    }
    mWizardBeamFireSePlayed = false;
}
void Player::updateDelayedSe(float deltaTime)
{
    if (mMagicMissileSeTimer > 0.0f)
    {
        mMagicMissileSeTimer -= deltaTime;
        if (mMagicMissileSeTimer <= 0.0f)
        {
            mMagicMissileSeTimer = 0.0f;
            AudioManager::instance()->playSe(SoundResource::Se::Chara::WizardMagicMissile);
        }
    }


    if (mMagicCircleSeTimer > 0.0f)
    {
        mMagicCircleSeTimer -= deltaTime;
        if (mMagicCircleSeTimer <= 0.0f)
        {
            mMagicCircleSeTimer = 0.0f;
            AudioManager::instance()->playSe(SoundResource::Se::Chara::WizardMagicCircle);
        }
    }
    if (mKnightParrySeTimer > 0.0f)
    {
        mKnightParrySeTimer -= deltaTime;
        if (mKnightParrySeTimer <= 0.0f)
        {
            mKnightParrySeTimer = 0.0f;
            AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightParry);
        }
    }
}

bool Player::consumeStamina(float value)
{
    if (value <= 0.0f)
    {
        return true;
    }

    if (mStamina < value)
    {
        return false;
    }

    /// スタミナを使った直後は自然回復を少し止め、連続行動のコストを見せる。
    mStamina -= value;
    if (mStamina < 0.0f)
    {
        mStamina = 0.0f;
    }

    mStaminaRegenDelayTimer = PlayerParam::StaminaRegenDelay;
    return true;
}

void Player::updateStamina(float deltaTime)
{
    if (mStaminaRegenDelayTimer > 0.0f)
    {
        mStaminaRegenDelayTimer -= deltaTime;
        return;
    }

    if (mStamina < PlayerParam::Stamina)
    {
        mStamina += PlayerParam::StaminaRegenSpeed * deltaTime;
        if (mStamina > PlayerParam::Stamina)
        {
            mStamina = PlayerParam::Stamina;
        }
    }
}
void Player::updatePowerUp(float deltaTime)
{
    if (mPowerUpTimer <= 0.0f)
    {
        return;
    }

    mPowerUpTimer -= deltaTime;
    if (mPowerUpTimer <= 0.0f)
    {
        /// 強化終了時は通常攻撃力へ戻し、タイマーを 0 にそろえる。
        mAttackPower = PlayerParam::AttackPower;
        mPowerUpTimer = 0.0f;
    }
}

bool Player::consumeAttackHitRequest()
{
    if (!mAttackHitRequest)
    {
        return false;
    }

    mAttackHitRequest = false;
    return true;
}

void Player::startHitStop(float duration)
{
    if (mCharacterType == PlayerCharacterType::Wizard || (mState != PlayerState::Attack && mState != PlayerState::Counter))
    {
        return;
    }
    if (duration > mHitStopTimer)
    {
        mHitStopTimer = duration;
    }
}
void Player::startGuard()
{
    mState = PlayerState::Guard;
    mStateTimer = 0.0f;

    if (mGuardAnimSourceHandle != -1)
    {
        mAnimator.play(mGuardAnimIndex, mGuardAnimSourceHandle, true);
    }
}

void Player::updateGuard(float deltaTime)
{
    (void)deltaTime;

    if (mInput.counter)
    {
        startCounter();
        return;
    }

    if (!mInput.guard)
    {
        mState = VSize(mInput.move) > 0.0f ? PlayerState::Move : PlayerState::Idle;
        return;
    }

}

void Player::updateGuardImpact(float deltaTime)
{
    mStateTimer += deltaTime;

    if (mAnimator.isFinished() && mStateTimer >= PlayerParam::GuardImpactDuration)
    {
        if (mInput.guard)
        {
            startGuard();
            return;
        }

        mState = VSize(mInput.move) > 0.0f ? PlayerState::Move : PlayerState::Idle;
    }
}

void Player::startCounter()
{
    if (!consumeStamina(PlayerParam::CounterStamina))
    {
        return;
    }

    mState = PlayerState::Counter;
    mStateTimer = 0.0f;
    mKnightParrySeTimer = DelayedCharaSeTime;

    if (mCounterAnimSourceHandle != -1)
    {
        mAnimator.play(mCounterAnimIndex, mCounterAnimSourceHandle, false, 0.05f);
    }
}

void Player::updateCounter(float deltaTime)
{
    mStateTimer += deltaTime;

    if (mAnimator.isFinished() || mStateTimer >= mCounterDuration)
    {
        mState = PlayerState::Idle;
        mStateTimer = 0.0f;
    }
}

void Player::updateHit(float deltaTime)
{
    mStateTimer += deltaTime;

    if (mHitKnockbackDistance > 0.0f)
    {
        /// ノックバック移動はイージングで進め、被弾直後の移動量を大きく見せる。
        float progress = mHitDuration > 0.0f ? mStateTimer / mHitDuration : 1.0f;
        if (mHitKnockbackActive && mKnockbackAnimSourceHandle != -1 && mAnimator.getAnimTotalTime() > 0.0f)
        {
            progress = mAnimator.getAnimTime() / mAnimator.getAnimTotalTime();
        }
        if (progress > 1.0f)
        {
            progress = 1.0f;
        }

        float easedProgress = 1.0f - (1.0f - progress) * (1.0f - progress);
        float deltaProgress = easedProgress - mPrevHitKnockbackProgress;
        mPrevHitKnockbackProgress = easedProgress;
        if (deltaProgress > 0.0f)
        {
            mPos = VAdd(mPos, VScale(mHitKnockbackDirection, mHitKnockbackDistance * deltaProgress));
        }
    }

    if (mHitKnockbackActive && mKnockbackAnimSourceHandle != -1)
    {
        mAnimator.play(mKnockbackAnimIndex, mKnockbackAnimSourceHandle, false);
    }
    else if (mHitAnimSourceHandle != -1)
    {
        mAnimator.play(mHitAnimIndex, mHitAnimSourceHandle, false);
    }

    if (mAnimator.isFinished() || mStateTimer >= mHitDuration)
    {
        mState = PlayerState::Idle;
        mStateTimer = 0.0f;
        mHitKnockbackDistance = 0.0f;
        mPrevHitKnockbackProgress = 0.0f;
        mHitKnockbackActive = false;
        mHitKnockbackStartBodyOffset = VGet(0.0f, 0.0f, 0.0f);
    }
}

void Player::damage(int value, VECTOR attackerPos)
{
    damage(value, attackerPos, VGet(0.0f, 0.0f, 0.0f), 0.0f);
}

bool Player::consumeGuardImpactEffectRequest()
{
    bool requested = mGuardImpactEffectRequest;
    mGuardImpactEffectRequest = false;
    return requested;
}

void Player::damage(int value, VECTOR attackerPos, VECTOR knockbackDirection, float knockbackDistance)
{
    if (mState == PlayerState::Dead)
    {
        return;
    }

    if (isCounterActive())
    {
        return;
    }

    int damageValue = value;
    bool guardBlocked = false;
    if (mState == PlayerState::Guard)
    {
        /// ガードは正面からの攻撃だけ軽減し、スタミナが足りない場合は通常被弾にする。
        VECTOR guardForward = VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
        VECTOR toAttacker = VSub(attackerPos, mPos);
        toAttacker.y = 0.0f;

        if (VSize(toAttacker) > 0.0001f)
        {
            toAttacker = VNorm(toAttacker);
        }

        float guardDot = guardForward.x * toAttacker.x + guardForward.z * toAttacker.z;
        if (guardDot >= PlayerParam::GuardFrontDotThreshold && consumeStamina(PlayerParam::GuardImpactStamina))
        {
            damageValue = value / 2;
            guardBlocked = true;
        }
    }

    if (damageValue <= 0 && !guardBlocked)
    {
        return;
    }

    if (guardBlocked)
    {
        AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightGuard);
    }
    else
    {
        AudioManager::instance()->playSe(SoundResource::Se::Chara::CharacterHit);
    }

    mHp -= damageValue;
    if (mCharacterType == PlayerCharacterType::Wizard && mState == PlayerState::Attack && mAttackStep == 2)
    {
        /// ビーム中に被弾した場合、ループ SE と攻撃判定を必ず止める。
        stopWizardBeamSe();
        mAttackStep = 0;
        mAttackHitRequest = false;
        mAttackHitDone = false;
    }

    if (mHp <= 0)
    {
        mHp = 0;
        mState = PlayerState::Dead;
        mStateTimer = 0.0f;

        if (mDeadAnimSourceHandle != -1)
        {
            mAnimator.play(mDeadAnimIndex, mDeadAnimSourceHandle, false, 0.08f);
        }
        return;
    }

    if (mState == PlayerState::Guard)
    {
        /// ガード成功時は専用リアクションへ入り、通常の被弾ノックバックへ進ませない。
        VECTOR guardForward = VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
        VECTOR toAttacker = VSub(attackerPos, mPos);
        toAttacker.y = 0.0f;

        if (VSize(toAttacker) > 0.0001f)
        {
            toAttacker = VNorm(toAttacker);
        }

        float guardDot = guardForward.x * toAttacker.x + guardForward.z * toAttacker.z;
        if (guardBlocked && guardDot >= PlayerParam::GuardFrontDotThreshold)
        {
            mPos = VAdd(mPos, VScale(guardForward, -PlayerParam::GuardKnockbackDistance));
            mGuardImpactEffectRequest = true;
            mState = PlayerState::GuardImpact;
            mStateTimer = 0.0f;
            if (mGuardImpactAnimSourceHandle != -1)
            {
                mAnimator.play(mGuardImpactAnimIndex, mGuardImpactAnimSourceHandle, false, 0.03f);
            }
            return;
        }
    }

    knockbackDirection.y = 0.0f;
    if (knockbackDistance > 0.0f && VSize(knockbackDirection) > 0.0001f)
    {
        /// ノックバック方向へ背を向けさせ、吹き飛ばされる見た目と移動方向を合わせる。
        mHitKnockbackDirection = VNorm(knockbackDirection);
        mHitKnockbackDistance = knockbackDistance;
        mPrevHitKnockbackProgress = 0.0f;
        mHitKnockbackActive = true;
        mRot.y = atan2f(-mHitKnockbackDirection.x, -mHitKnockbackDirection.z);
        if (mKnockbackAnimSourceHandle != -1)
        {
            mAnimator.play(mKnockbackAnimIndex, mKnockbackAnimSourceHandle, false, 0.05f);
            if (mModelHandle != -1 && mBodyFrameIndex != -1)
            {
                mHitKnockbackStartBodyOffset = VSub(MV1GetFramePosition(mModelHandle, mBodyFrameIndex), mPos);
            }
        }
        else if (mHitAnimSourceHandle != -1)
        {
            mAnimator.play(mHitAnimIndex, mHitAnimSourceHandle, false, 0.05f);
        }
    }
    else
    {
        mHitKnockbackDirection = VGet(0.0f, 0.0f, 0.0f);
        mHitKnockbackDistance = 0.0f;
        mPrevHitKnockbackProgress = 0.0f;
        mHitKnockbackActive = false;
        if (mHitAnimSourceHandle != -1)
        {
            mAnimator.play(mHitAnimIndex, mHitAnimSourceHandle, false, 0.05f);
        }
    }

    mState = PlayerState::Hit;
    mStateTimer = 0.0f;
}

void Player::restoreHpForScene(int hp)
{
    if (hp <= 0)
    {
        forceDead();
        return;
    }

    mHp = hp > PlayerParam::Hp ? PlayerParam::Hp : hp;
}

void Player::forceDead()
{
    if (mState == PlayerState::Dead)
    {
        return;
    }

    mHp = 0;
    mState = PlayerState::Dead;
    mStateTimer = 0.0f;

    if (mDeadAnimSourceHandle != -1)
    {
        mAnimator.play(mDeadAnimIndex, mDeadAnimSourceHandle, false, 0.08f);
    }
}
bool Player::isDeadAnimationFinished() const
{
    if (mState != PlayerState::Dead)
    {
        return false;
    }

    return mDeadAnimSourceHandle != -1
        ? mAnimator.isFinished()
        : mStateTimer >= mDeadDuration;
}
bool Player::isCounterActive() const
{
    if (mState != PlayerState::Counter || mAnimator.getAnimTotalTime() <= 0.0f)
    {
        return false;
    }

    float normalizedTime = mAnimator.getAnimTime() / mAnimator.getAnimTotalTime();
    /// カウンターはモーション全体ではなく、武器を合わせる中盤だけ有効にする。
    return normalizedTime >= 0.18f && normalizedTime <= 0.72f;
}

void Player::updateDead(float deltaTime)
{
    mStateTimer += deltaTime;
    (void)deltaTime;

    if (mDeadAnimSourceHandle != -1)
    {
        mAnimator.play(mDeadAnimIndex, mDeadAnimSourceHandle, false);
    }
}
