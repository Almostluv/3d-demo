/**
 * @file
 * @brief player_movement.cpp 実装を定義する。
 */

#include "player.h"
#include "camera.h"
#include "player_param.h"
#include "audio_manager.h"
#include "sound_resource.h"
#include "DxLib.h"

#include <cmath>

void Player::playMoveAnimation()
{
    int moveAnimSourceHandle = mMoveAnimSourceHandle;

    if (mIsLockOn)
    {
        /// ロックオン中は入力軸に応じて前後左右の専用移動アニメーションを選ぶ。
        float absX = fabsf(mInput.move.x);
        float absZ = fabsf(mInput.move.z);
        if (absX > absZ)
        {
            moveAnimSourceHandle = mInput.move.x < 0.0f ? mMoveLeftAnimSourceHandle : mMoveRightAnimSourceHandle;
        }
        else if (mInput.move.z < 0.0f)
        {
            moveAnimSourceHandle = mMoveBackAnimSourceHandle;
        }
    }

    if (moveAnimSourceHandle != -1)
    {
        mAnimator.play(mMoveAnimIndex, moveAnimSourceHandle, true);
    }
    else if (mMoveAnimSourceHandle != -1)
    {
        mAnimator.play(mMoveAnimIndex, mMoveAnimSourceHandle, true);
    }
}
void Player::updateMove(float deltaTime)
{
    playMoveAnimation();

    /// 移動中のアクション入力は移動処理より先に評価し、状態遷移の遅れを防ぐ。
    if (mInput.attack)
    {
        startAttack();
        return;
    }

    if (mInput.dodge)
    {
        startDodge();
        return;
    }

    if (mCharacterType == PlayerCharacterType::Wizard && mInput.guard)
    {
        startWizardMagicAttack2();
        return;
    }

    if (mInput.guard)
    {
        startGuard();
        return;
    }

    if (mCharacterType == PlayerCharacterType::Wizard && mInput.counter)
    {
        startWizardAreaPowerUp();
        return;
    }

    if (mInput.counter)
    {
        startCounter();
        return;
    }

    if (VSize(mInput.move) <= 0.0f)
    {
        mState = PlayerState::Idle;
        return;
    }

    handleMove(deltaTime);
}

void Player::handleMove(float deltaTime)
{
    /// ロックオン中はキャラクター向き基準、非ロックオン時はカメラ基準で移動方向を作る。
    VECTOR forward = mIsLockOn ? VGet(sinf(mRot.y), 0.0f, cosf(mRot.y)) : mCamera->getForwardXZ();
    VECTOR right = mIsLockOn ? VGet(cosf(mRot.y), 0.0f, -sinf(mRot.y)) : mCamera->getRightXZ();

    VECTOR moveDir = VAdd(
        VScale(right, mInput.move.x),
        VScale(forward, mInput.move.z)
    );

    if (VSize(moveDir) <= 0.0001f)
    {
        return;
    }

    moveDir = VNorm(moveDir);

    VECTOR moveVec = VScale(moveDir, mMoveSpeed * deltaTime);
    mPos = VAdd(mPos, moveVec);
    mRot.y = atan2f(moveDir.x, moveDir.z);

    if (mCharacterType == PlayerCharacterType::Knight)
    {
        /// 足音は移動距離ではなく一定間隔で鳴らし、歩行アニメーションと大きくずれないようにする。
        mFootstepSeTimer -= deltaTime;
        if (mFootstepSeTimer <= 0.0f)
        {
            AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightWalking);
            mFootstepSeTimer = 0.36f;
        }
    }
}

void Player::updateDodge(float deltaTime)
{
    mStateTimer += deltaTime;

    float normalizedTime = 0.0f;

    if (mAnimator.getAnimTotalTime() > 0.0f)
    {
        /// 回避距離はアニメーション時間に同期し、実際の移動と見た目を合わせる。
        normalizedTime = mAnimator.getAnimTime() / mAnimator.getAnimTotalTime();
    }
    else
    {
        normalizedTime = mDodgeDuration > 0.0f ? mStateTimer / mDodgeDuration : 1.0f;
    }

    if (normalizedTime > 1.0f)
    {
        normalizedTime = 1.0f;
    }

    float moveTime = normalizedTime / PlayerParam::DodgeMoveEndRate;
    if (moveTime > 1.0f)
    {
        moveTime = 1.0f;
    }

    float stopRate = 1.0f - moveTime;
    /// 終盤ほど移動量を小さくすることで、回避の着地を自然に止める。
    float dodgeProgress = 1.0f - (stopRate * stopRate * stopRate);
    float deltaProgress = dodgeProgress - mPrevDodgeProgress;
    mPrevDodgeProgress = dodgeProgress;

    if (deltaProgress > 0.0f)
    {
        VECTOR move = VScale(mDodgeDirection, mDodgeTotalDistance * deltaProgress);
        mPos = VAdd(mPos, move);
    }

    bool dodgeFinished = normalizedTime >= PlayerParam::DodgeAnimEndRate || mAnimator.isFinished();
    if (!dodgeFinished && mAnimator.getAnimTotalTime() > 0.0f)
    {
        dodgeFinished = normalizedTime >= 1.0f;
    }

    if (dodgeFinished)
    {
        mState = VSize(mInput.move) > 0.0f ? PlayerState::Move : PlayerState::Idle;
        mStateTimer = 0.0f;

        if (mState == PlayerState::Move && mMoveAnimSourceHandle != -1)
        {
            mAnimator.play(mMoveAnimIndex, mMoveAnimSourceHandle, true, 0.0f);
        }
        else if (mIdleAnimSourceHandle != -1)
        {
            mAnimator.play(mIdleAnimIndex, mIdleAnimSourceHandle, true, 0.0f);
        }
    }
}

void Player::startDodge()
{
    if (!consumeStamina(PlayerParam::DodgeStamina))
    {
        return;
    }

    mState = PlayerState::Dodge;
    mStateTimer = 0.0f;
    AudioManager::instance()->playSe(mCharacterType == PlayerCharacterType::Wizard ? SoundResource::Se::Chara::WizardRolling : SoundResource::Se::Chara::KnightRolling);

    if (VSize(mInput.move) > 0.0f)
    {
        /// 入力がある場合はその方向へ回避し、無入力なら現在向いている方向へ回避する。
        VECTOR forward = mIsLockOn ? VGet(sinf(mRot.y), 0.0f, cosf(mRot.y)) : mCamera->getForwardXZ();
        VECTOR right = mIsLockOn ? VGet(cosf(mRot.y), 0.0f, -sinf(mRot.y)) : mCamera->getRightXZ();

        VECTOR moveDir = VAdd(
            VScale(right, mInput.move.x),
            VScale(forward, mInput.move.z)
        );

        if (VSize(moveDir) > 0.0001f)
        {
            mDodgeDirection = VNorm(moveDir);
            mRot.y = atan2f(mDodgeDirection.x, mDodgeDirection.z);
        }
    }
    else
    {
        mDodgeDirection = VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
    }

    mDodgeStartPos = mPos;
    mDodgeEndPos = VAdd(mDodgeStartPos, VScale(mDodgeDirection, mDodgeTotalDistance));
    mPrevDodgeProgress = 0.0f;

    if (mDodgeAnimSourceHandle != -1)
    {
        mAnimator.play(mDodgeAnimIndex, mDodgeAnimSourceHandle, false);
    }
}

void Player::startJump()
{
    mState = PlayerState::Jump;
    mStateTimer = 0.0f;
    mJumpVelocityY = mJumpSpeed;
    mIsGrounded = false;

    if (mJumpAnimSourceHandle != -1)
    {
        mAnimator.play(mJumpAnimIndex, mJumpAnimSourceHandle, false);
    }
}

void Player::updateJump(float deltaTime)
{
    mStateTimer += deltaTime;

    if (VSize(mInput.move) > 0.0f)
    {
        handleMove(deltaTime);
    }

    const float gravity = PlayerParam::JumpGravity;
    /// ジャンプは単純な鉛直速度で管理し、着地時に通常の移動状態へ戻す。
    mJumpVelocityY -= gravity * deltaTime;
    mPos.y += mJumpVelocityY * deltaTime;

    if (mPos.y <= 0.0f)
    {
        mPos.y = 0.0f;
        mJumpVelocityY = 0.0f;
        mIsGrounded = true;

        if (VSize(mInput.move) > 0.0f)
        {
            mState = PlayerState::Move;
        }
        else
        {
            mState = PlayerState::Idle;
        }

        mStateTimer = 0.0f;
    }
}
