/**
 * @file
 * @brief player.cpp 実装を定義する。
 */

#include "player.h"
#include "camera.h"
#include "player_param.h"
#include "player_character_resource.h"
#include "resource_manager.h"
#include "DxLib.h"

#include <cmath>

namespace
{
VECTOR transformFrameLocalPosition(int modelHandle, int frameIndex, VECTOR localPos)
{
    /// モデルフレームのローカル座標をワールド座標へ変換する。
    return VTransform(localPos, MV1GetFrameLocalWorldMatrix(modelHandle, frameIndex));
}

int getModelIfExists(const char* path)
    {
        /// optional なアニメーションパスは nullptr の場合に無効ハンドルとして扱う。
        return path != nullptr ? ResourceManager::instance()->getModel(path) : -1;
    }
}

Player::Player(Camera* camera, PlayerCharacterType characterType)
    : CharBase(CharBaseParam{
        PlayerParam::Hp,
        PlayerParam::HitRadius,
        PlayerParam::CollisionRadius,
        characterType == PlayerCharacterType::Wizard ? PlayerParam::WizardCollisionHeight : PlayerParam::CollisionHeight,
        PlayerParam::AttackRadius,
        PlayerParam::MoveSpeed,
        PlayerParam::AttackPower
        })
    , mCamera(camera)
    , mIsLockOn(false)
    , mIsAiControlled(false)
    , mCharacterType(characterType)
    , mState(PlayerState::Idle)
    , mStateTimer(0.0f)
    , mAttackDuration(PlayerParam::AttackDuration)
    , mDodgeDuration(PlayerParam::DodgeDuration)
    , mDodgeTotalDistance(PlayerParam::DodgeTotalDistance)
    , mAttackHitRequest(false)
    , mDodgeDirection(VGet(0.0f, 0.0f, 1.0f))
    , mDodgeStartPos(VGet(0.0f, 0.0f, 0.0f))
    , mDodgeEndPos(VGet(0.0f, 0.0f, 0.0f))
    , mPrevDodgeProgress(0.0f)
    , mHitKnockbackDirection(VGet(0.0f, 0.0f, 0.0f))
    , mHitKnockbackDistance(0.0f)
    , mPrevHitKnockbackProgress(0.0f)
    , mHitKnockbackActive(false)
    , mHitKnockbackStartBodyOffset(VGet(0.0f, 0.0f, 0.0f))
    , mIdleAnimIndex(0)
    , mMoveAnimIndex(0)
    , mAttack1AnimIndex(0)
    , mAttack2AnimIndex(0)
    , mAttack3AnimIndex(0)
    , mDodgeAnimIndex(0)
    , mGuardAnimIndex(0)
    , mGuardImpactAnimIndex(0)
    , mCounterAnimIndex(0)
    , mJumpAnimIndex(0)
    , mHitAnimIndex(0)
    , mKnockbackAnimIndex(0)
    , mDeadAnimIndex(0)
    , mIdleAnimSourceHandle(-1)
    , mMoveAnimSourceHandle(-1)
    , mMoveLeftAnimSourceHandle(-1)
    , mMoveRightAnimSourceHandle(-1)
    , mMoveBackAnimSourceHandle(-1)
    , mAttack1AnimSourceHandle(-1)
    , mAttack2AnimSourceHandle(-1)
    , mAttack3AnimSourceHandle(-1)
    , mDodgeAnimSourceHandle(-1)
    , mGuardAnimSourceHandle(-1)
    , mGuardImpactAnimSourceHandle(-1)
    , mCounterAnimSourceHandle(-1)
    , mJumpAnimSourceHandle(-1)
    , mHitAnimSourceHandle(-1)
    , mKnockbackAnimSourceHandle(-1)
    , mDeadAnimSourceHandle(-1)
    , mAttackStep(0)
    , mNextAttackQueued(false)
    , mWizardMagicHitTimer(0.0f)
    , mWizardBeamFireSePlayed(false)
    , mWizardBeamLoopSeHandle(-1)
    , mMagicMissileSeTimer(0.0f)
    , mMagicCircleSeTimer(0.0f)
    , mKnightParrySeTimer(0.0f)
    , mWizardMagicHandFrameIndex(-1)
    , mSwordFrameIndex(-1)
    , mShieldFrameIndex(-1)
    , mCounterEffectFrameIndex(-1)
    , mBodyFrameIndex(-1)
    , mPowerUpTimer(0.0f)
    , mStamina(PlayerParam::Stamina)
    , mStaminaRegenDelayTimer(0.0f)
    , mAttackHitDone(false)
    , mGuardImpactEffectRequest(false)
    , mHitStopTimer(0.0f)
    , mGuardEnterDuration(PlayerParam::GuardEnterDuration)
    , mCounterDuration(PlayerParam::CounterDuration)
    , mJumpDuration(PlayerParam::JumpDuration)
    , mHitDuration(PlayerParam::HitDuration)
    , mDeadDuration(PlayerParam::DeadDuration)
    , mJumpSpeed(PlayerParam::JumpSpeed)
    , mJumpVelocityY(0.0f)
    , mIsGrounded(true)
    , mFootstepSeTimer(0.0f)
{
    /// キャラクター種別ごとのモデル、当たり判定高さ、アニメーションセットをまとめて初期化する。
    const PlayerCharacterResourceSet& resources =
        getPlayerCharacterResourceSet(characterType);

    mModelHandle = ResourceManager::instance()->createModelInstance(resources.modelPath);
    if (mModelHandle != -1)
    {
        /// 手、剣、盾、体などのフレームは攻撃エフェクトや補正位置の基準に使う。
        mWizardMagicHandFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:RightHandMiddle1");
        mSwordFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:Sword_joint");
        mShieldFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:Shield_Joint");
        mCounterEffectFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:LeftHandIndex1");
        mBodyFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:Hips");
    }

    mIdleAnimSourceHandle = getModelIfExists(resources.idlePath);
    mMoveAnimSourceHandle = getModelIfExists(resources.movePath);
    mMoveLeftAnimSourceHandle = getModelIfExists(resources.walkLeftPath);
    mMoveRightAnimSourceHandle = getModelIfExists(resources.walkRightPath);
    mMoveBackAnimSourceHandle = getModelIfExists(resources.walkBackPath);
    mAttack1AnimSourceHandle = getModelIfExists(resources.attack1Path);
    mAttack2AnimSourceHandle = getModelIfExists(resources.attack2Path);
    mAttack3AnimSourceHandle = getModelIfExists(resources.attack3Path);
    mDodgeAnimSourceHandle = getModelIfExists(resources.dodgePath);
    mGuardAnimSourceHandle = getModelIfExists(resources.guardPath);
    mGuardImpactAnimSourceHandle = getModelIfExists(resources.guardImpactPath);
    mCounterAnimSourceHandle = getModelIfExists(resources.counterPath);
    mJumpAnimSourceHandle = getModelIfExists(resources.jumpPath);
    mHitAnimSourceHandle = getModelIfExists(resources.hitPath);
    mKnockbackAnimSourceHandle = getModelIfExists(resources.knockbackPath);
    mDeadAnimSourceHandle = getModelIfExists(resources.deadPath);

    if (mIdleAnimSourceHandle == -1 || MV1GetAnimNum(mIdleAnimSourceHandle) <= 0)
    {
        /// アセットが存在しないアニメーションは無効化し、再生処理側で安全にスキップする。
        mIdleAnimSourceHandle = -1;
    }

    if (mMoveAnimSourceHandle == -1 || MV1GetAnimNum(mMoveAnimSourceHandle) <= 0)
    {
        mMoveAnimSourceHandle = -1;
    }

    if (mMoveLeftAnimSourceHandle == -1 || MV1GetAnimNum(mMoveLeftAnimSourceHandle) <= 0)
    {
        mMoveLeftAnimSourceHandle = -1;
    }

    if (mMoveRightAnimSourceHandle == -1 || MV1GetAnimNum(mMoveRightAnimSourceHandle) <= 0)
    {
        mMoveRightAnimSourceHandle = -1;
    }

    if (mMoveBackAnimSourceHandle == -1 || MV1GetAnimNum(mMoveBackAnimSourceHandle) <= 0)
    {
        mMoveBackAnimSourceHandle = -1;
    }

    if (mAttack1AnimSourceHandle == -1 || MV1GetAnimNum(mAttack1AnimSourceHandle) <= 0)
    {
        mAttack1AnimSourceHandle = -1;
    }

    if (mAttack2AnimSourceHandle == -1 || MV1GetAnimNum(mAttack2AnimSourceHandle) <= 0)
    {
        mAttack2AnimSourceHandle = -1;
    }

    if (mAttack3AnimSourceHandle == -1 || MV1GetAnimNum(mAttack3AnimSourceHandle) <= 0)
    {
        mAttack3AnimSourceHandle = -1;
    }

    if (mDodgeAnimSourceHandle == -1 || MV1GetAnimNum(mDodgeAnimSourceHandle) <= 0)
    {
        mDodgeAnimSourceHandle = -1;
    }

    if (mGuardAnimSourceHandle == -1 || MV1GetAnimNum(mGuardAnimSourceHandle) <= 0)
    {
        mGuardAnimSourceHandle = -1;
    }

    if (mGuardImpactAnimSourceHandle == -1 || MV1GetAnimNum(mGuardImpactAnimSourceHandle) <= 0)
    {
        mGuardImpactAnimSourceHandle = -1;
    }

    if (mCounterAnimSourceHandle == -1 || MV1GetAnimNum(mCounterAnimSourceHandle) <= 0)
    {
        mCounterAnimSourceHandle = -1;
    }

    if (mJumpAnimSourceHandle == -1 || MV1GetAnimNum(mJumpAnimSourceHandle) <= 0)
    {
        mJumpAnimSourceHandle = -1;
    }

    if (mHitAnimSourceHandle == -1 || MV1GetAnimNum(mHitAnimSourceHandle) <= 0)
    {
        mHitAnimSourceHandle = -1;
    }

    if (mKnockbackAnimSourceHandle == -1 || MV1GetAnimNum(mKnockbackAnimSourceHandle) <= 0)
    {
        mKnockbackAnimSourceHandle = -1;
    }

    if (mDeadAnimSourceHandle == -1 || MV1GetAnimNum(mDeadAnimSourceHandle) <= 0)
    {
        mDeadAnimSourceHandle = -1;
    }

    mPos = VGet(0.0f, 0.0f, 0.0f);
    mRot = VGet(0.0f, DX_PI_F, 0.0f);
    mScale = characterType == PlayerCharacterType::Wizard ? PlayerParam::WizardScale : PlayerParam::Scale;

    if (mModelHandle != -1)
    {
        /// キャラクター種別に合わせたスケールとマテリアル補正をモデルへ反映する。
        MV1SetScale(mModelHandle, VGet(mScale, mScale, mScale));

        int normalMapTexture = resources.normalMapPath != nullptr ? MV1AddTexture(mModelHandle, "PlayerNormalMap", resources.normalMapPath, nullptr, nullptr, nullptr, DX_TEXADDRESS_WRAP, DX_TEXADDRESS_WRAP, DX_DRAWMODE_ANISOTROPIC, TRUE, 0.08f) : -1;
        int specularMapTexture = resources.specularMapPath != nullptr ? MV1AddTexture(mModelHandle, "PlayerSpecularMap", resources.specularMapPath) : -1;
        int materialNum = MV1GetMaterialNum(mModelHandle);

        for (int i = 0; i < materialNum; ++i)
        {
            /// ライト環境に対して見やすくなるよう、Diffuse/Ambient/Specular を軽く補正する。
            MV1SetMaterialDifColor(mModelHandle, i, GetColorF(1.15f, 1.12f, 1.06f, 1.0f));
            MV1SetMaterialAmbColor(mModelHandle, i, mCharacterType == PlayerCharacterType::Knight ? GetColorF(0.90f, 0.86f, 0.76f, 1.0f) : GetColorF(0.62f, 0.60f, 0.56f, 1.0f));
            MV1SetMaterialSpcColor(mModelHandle, i, GetColorF(0.95f, 0.88f, 0.72f, 1.0f));
            MV1SetMaterialSpcPower(mModelHandle, i, 34.0f);
            MV1SetMaterialEmiColor(mModelHandle, i, mCharacterType == PlayerCharacterType::Knight ? GetColorF(0.12f, 0.105f, 0.085f, 1.0f) : GetColorF(0.025f, 0.024f, 0.022f, 1.0f));
            if (normalMapTexture != -1 && MV1GetMaterialNormalMapTexture(mModelHandle, i) == -1)
            {
                /// モデル側に法線マップがない場合だけ、キャラクター共通の法線マップを補う。
                MV1SetMaterialNormalMapTexture(mModelHandle, i, normalMapTexture);
            }
            if (specularMapTexture != -1 && MV1GetMaterialSpcMapTexture(mModelHandle, i) == -1)
            {
                MV1SetMaterialSpcMapTexture(mModelHandle, i, specularMapTexture);
            }
        }
    }

    mAnimator.init(mModelHandle);

    if (mIdleAnimSourceHandle != -1)
    {
        /// 生成直後は入力が入るまで待機アニメーションをループ再生する。
        mAnimator.play(mIdleAnimIndex, mIdleAnimSourceHandle, true);
    }
}

void Player::setInput(const PlayerInput& input)
{
    mInput = input;
}

void Player::setLockOn(bool isLockOn)
{
    mIsLockOn = isLockOn;
}

void Player::setAiControlled(bool isAiControlled)
{
    mIsAiControlled = isAiControlled;
}

Player::~Player()
{
    deleteModel();

}

void Player::update(float deltaTime)
{
    /// 常時更新が必要な強化、スタミナ、遅延 SE を状態更新より先に進める。
    updatePowerUp(deltaTime);
    updateStamina(deltaTime);
    updateDelayedSe(deltaTime);

    if (mHitStopTimer > 0.0f)
    {
        /// ヒットストップ中は状態とアニメーションを止めるが、モデル位置だけ同期する。
        mHitStopTimer -= deltaTime;
        if (mHitStopTimer < 0.0f)
        {
            mHitStopTimer = 0.0f;
        }
        if (mModelHandle != -1)
        {
            syncModelTransform();
        }
        return;
    }

    updateState(deltaTime);

    /// 状態更新で選ばれたアニメーションを最後に進める。
    mAnimator.update(deltaTime);

    if (mModelHandle != -1)
    {
        syncModelTransform();
    }
}

void Player::syncModelTransform()
{
    CharBase::syncModelTransform();

    if (mHitKnockbackActive && mKnockbackAnimSourceHandle != -1 && mBodyFrameIndex != -1)
    {
        /// ノックバックアニメーションの腰位置ずれを補正し、論理位置と描画位置を合わせる。
        VECTOR bodyOffset = VSub(MV1GetFramePosition(mModelHandle, mBodyFrameIndex), mPos);
        VECTOR correction = VSub(mHitKnockbackStartBodyOffset, bodyOffset);
        correction.y = 0.0f;
        MV1SetPosition(mModelHandle, VAdd(mPos, correction));
    }
}

VECTOR Player::getMagicHandPosition() const
{
    if (mModelHandle != -1 && mWizardMagicHandFrameIndex != -1)
    {
        /// Wizard の魔法発生位置は右手フレームを優先する。
        return MV1GetFramePosition(mModelHandle, mWizardMagicHandFrameIndex);
    }

    /// フレームが取れない場合は、キャラクター前方の手元らしい位置へフォールバックする。
    VECTOR forward = VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
    return VAdd(VAdd(mPos, VGet(0.0f, 11.0f, 0.0f)), VScale(forward, 8.0f));
}

VECTOR Player::getSwordJointPosition() const
{
    if (mModelHandle != -1 && mSwordFrameIndex != -1)
    {
        /// 剣軌跡の根元は Sword_joint フレームを使う。
        return MV1GetFramePosition(mModelHandle, mSwordFrameIndex);
    }

    VECTOR forward = VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
    return VAdd(VAdd(mPos, VGet(0.0f, 13.0f, 0.0f)), VScale(forward, 3.0f));
}
VECTOR Player::getShieldJointPosition() const
{
    if (mModelHandle != -1 && mShieldFrameIndex != -1)
    {
        return MV1GetFramePosition(mModelHandle, mShieldFrameIndex);
    }

    VECTOR forward = VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
    return VAdd(VAdd(mPos, VGet(0.0f, 9.0f, 0.0f)), VScale(forward, 8.0f));
}

VECTOR Player::getCounterEffectPosition() const
{
    if (mModelHandle != -1 && mCounterEffectFrameIndex != -1)
    {
        return MV1GetFramePosition(mModelHandle, mCounterEffectFrameIndex);
    }

    return getShieldJointPosition();
}
VECTOR Player::getSwordTipPosition() const
{
    VECTOR jointPos = getSwordJointPosition();
    if (mModelHandle != -1 && mSwordFrameIndex != -1)
    {
        /// 剣先は Sword_joint のローカルオフセットをワールドへ変換して求める。
        VECTOR frameBase = transformFrameLocalPosition(mModelHandle, mSwordFrameIndex, VGet(0.0f, 0.0f, 0.0f));
        VECTOR swordTipOffset = VGet(-18.848f, -4.200f, -74.516f);
        VECTOR frameTip = transformFrameLocalPosition(mModelHandle, mSwordFrameIndex, swordTipOffset);
        return VAdd(jointPos, VSub(frameTip, frameBase));
    }

    VECTOR forward = VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
    return VAdd(jointPos, VScale(forward, 4.5f));
}
void Player::updateState(float deltaTime)
{
    /// PlayerState ごとの処理を分離し、各状態関数で入力と遷移を扱う。
    switch (mState)
    {
    case PlayerState::Idle:
        updateIdle(deltaTime);
        break;

    case PlayerState::Move:
        updateMove(deltaTime);
        break;

    case PlayerState::Attack:
        updateAttack(deltaTime);
        break;

    case PlayerState::Dodge:
        updateDodge(deltaTime);
        break;

    case PlayerState::Guard:
        updateGuard(deltaTime);
        break;

    case PlayerState::GuardImpact:
        updateGuardImpact(deltaTime);
        break;

    case PlayerState::Counter:
        updateCounter(deltaTime);
        break;

    case PlayerState::Jump:
        updateJump(deltaTime);
        break;

    case PlayerState::Hit:
        updateHit(deltaTime);
        break;

    case PlayerState::Dead:
        updateDead(deltaTime);
        break;
    }
}

void Player::updateIdle(float deltaTime)
{
    (void)deltaTime;
    /// 停止中は歩行 SE タイマーをリセットし、次回移動開始時に足音が遅れないようにする。
    mFootstepSeTimer = 0.0f;

    if (mIdleAnimSourceHandle != -1)
    {
        mAnimator.play(
            mIdleAnimIndex,
            mIdleAnimSourceHandle,
            true
        );
    }

    if (mInput.attack)
    {
        /// Idle では攻撃、回避、防御、特殊行動、移動の順で入力優先度を決める。
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

    if (VSize(mInput.move) > 0.0f)
    {
        mState = PlayerState::Move;
    }
}

void Player::draw()
{
    drawModel();

}
