/**
 * @file
 * @brief Enemy の生成、基本更新、描画入口を定義する。
 */

#include "enemy.h"
#include "enemy_param.h"
#include "model_resource.h"
#include "resource_manager.h"
#include "DxLib.h"

#include <cmath>

Enemy::Enemy(VECTOR startPos)
    : CharBase(CharBaseParam{
        EnemyParam::Hp,
        EnemyParam::HitRadius,
        EnemyParam::CollisionRadius,
        EnemyParam::CollisionHeight,
        EnemyParam::AttackRadius,
        EnemyParam::MoveSpeed,
        EnemyParam::AttackPower
    })
    , mDetectRadius(EnemyParam::DetectRadius)
    , mStateTimer(0.0f)
    , mHitDuration(EnemyParam::HitDuration)
    , mDyingDuration(EnemyParam::DyingDuration)
    , mKnockbackSpeed(EnemyParam::KnockbackSpeed)
    , mKnockbackDuration(EnemyParam::KnockbackDuration)
    , mKnockbackTotalDistance(EnemyParam::KnockbackTotalDistance)
    , mKnockbackAppliedDistance(0.0f)
    , mKnockbackEndCompensationDistance(EnemyParam::KnockbackEndCompensationDistance)
    , mAttackCooldown(EnemyParam::AttackCooldown)
    , mAttackCooldownTimer(0.0f)
    , mRamCooldownTimer(0.0f)
    , mSpinAttackCooldownTimer(0.0f)
    , mCloseContactTimer(0.0f)
    , mCurrentAttackRadius(EnemyParam::AttackRadius)
    , mJumpAttackRadius(EnemyParam::JumpAttackRadius)
    , mJumpAttackEffectiveDistance(EnemyParam::JumpAttackEffectiveDistance)
    , mJumpAttackCooldown(EnemyParam::JumpAttackCooldown)
    , mJumpAttackCooldownTimer(0.0f)
    , mJumpAttackStartupDuration(EnemyParam::JumpAttackStartupDuration)
    , mJumpAttackRecoveryDuration(EnemyParam::JumpAttackRecoveryDuration)
    , mJumpAttackLandingOffsetDistance(EnemyParam::JumpAttackLandingOffsetDistance)
    , mJumpAttackHeight(EnemyParam::JumpAttackHeight)
    , mJumpAttackMinHeight(EnemyParam::JumpAttackMinHeight)
    , mJumpAttackMaxHeight(EnemyParam::JumpAttackMaxHeight)
    , mJumpAttackLandTiming(EnemyParam::JumpAttackLandTiming)
    , mJumpAttackDuration(EnemyParam::JumpAttackDuration)
    , mJumpAttackMinDuration(EnemyParam::JumpAttackMinDuration)
    , mJumpAttackMaxDuration(EnemyParam::JumpAttackMaxDuration)
    , mJumpAttackGroundY(startPos.y)
    , mIdleAnimSourceHandle(-1)
    , mWalkAnimSourceHandle(-1)
    , mAttackAnimSourceHandle(-1)
    , mAttack2AnimSourceHandle(-1)
    , mAttack3AnimSourceHandle(-1)
    , mAttack360AnimSourceHandle(-1)
    , mChargingAnimSourceHandle(-1)
    , mRamAnimSourceHandle(-1)
    , mJumpAttackAnimSourceHandle(-1)
    , mHit1AnimSourceHandle(-1)
    , mHit2AnimSourceHandle(-1)
    , mDyingAnimSourceHandle(-1)
    , mIdleAnimIndex(0)
    , mWalkAnimIndex(0)
    , mAttackAnimIndex(0)
    , mAttack2AnimIndex(0)
    , mAttack3AnimIndex(0)
    , mAttack360AnimIndex(0)
    , mChargingAnimIndex(0)
    , mRamAnimIndex(0)
    , mJumpAttackAnimIndex(0)
    , mHit1AnimIndex(0)
    , mHit2AnimIndex(0)
    , mDyingAnimIndex(0)
    , mJumpAttackToeFrameIndex(-1)
    , mJumpAttackBodyFrameIndex(-1)
    , mHeadFrameIndex(-1)
    , mSpineFrameIndex(-1)
    , mLeftUpLegFrameIndex(-1)
    , mRightUpLegFrameIndex(-1)
    , mLeftLegFrameIndex(-1)
    , mRightLegFrameIndex(-1)
    , mLeftForeArmFrameIndex(-1)
    , mLeftHandFrameIndex(-1)
    , mRightForeArmFrameIndex(-1)
    , mRightHandFrameIndex(-1)
    , mRightHandIndex3FrameIndex(-1)
    , mRightHandIndex4FrameIndex(-1)
    , mState(EnemyState::Idle)
    , mCurrentTargetSource(ThreatSource::Player)
    , mTargetSwitchCooldownTimer(0.0f)
    , mIsBoss(false)
    , mMovementEnabled(true)
    , mBossShieldEnabled(false)
    , mBossShieldHp(0)
    , mBossShieldBreakTimer(0.0f)
    , mAttackHitRequest(false)
    , mAttackHitDone(false)
    , mAttackStep(1)
    , mNextAttackStep(1)
    , mAttackTrailPointCount(0)
    , mAttackTrailFadeTimer(0.0f)
    , mJumpAttackHitDone(false)
    , mJumpAttackStarted(false)
    , mJumpAttackLanded(false)
    , mHit2ReactionActive(false)
    , mHitAnimPlaying(false)
    , mHitStopTimer(0.0f)
    , mDyingAnimPlaying(false)
    , mKnockbackActive(false)
    , mIsIntroLanding(false)
    , mKnockbackDirection(VGet(0.0f, 0.0f, 0.0f))
    , mRamDirection(VGet(0.0f, 0.0f, 1.0f))
    , mRamStartPos(startPos)
    , mRamLength(EnemyParam::RamLength)
    , mRamFootstepSeTimer(0.0f)
    , mJumpAttackStartPos(VGet(0.0f, 0.0f, 0.0f))
    , mJumpAttackTargetPos(VGet(0.0f, 0.0f, 0.0f))
    , mJumpAttackHitCenter(VGet(0.0f, 0.0f, 0.0f))
    , mJumpAttackMarkerCenter(VGet(0.0f, 0.0f, 0.0f))
    , mJumpAttackLandingBodyPos(VGet(0.0f, 0.0f, 0.0f))
    , mJumpAttackStageCollisionModelHandle(-1)
{
    mModelHandle = ResourceManager::instance()->createModelInstance(ModelResource::Enemy::Model);
    mIdleAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Idle);
    mWalkAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Walk);
    mAttackAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Attack);
    mAttack2AnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Attack2);
    mAttack3AnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Attack3);
    mAttack360AnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Attack360);
    mChargingAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Charging);
    mRamAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Ram);
    mJumpAttackAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::JumpAttack);
    mHit1AnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Hit1);
    mHit2AnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Hit2);
    mDyingAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Enemy::Dying);

    mPos = startPos;
    mRot = VGet(0.0f, 0.0f, 0.0f);
    mScale = EnemyParam::Scale;

    if (mModelHandle != -1)
    {
        /// モデルの各フレームを検索して、攻撃判定、ロックオン、デバッグ表示の基準点にする。
        MV1SetScale(mModelHandle, VGet(mScale, mScale, mScale));
        MV1SetPosition(mModelHandle, mPos);
        mJumpAttackToeFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:RightToeBase");
        mJumpAttackBodyFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:Hips");
        mHeadFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:Head");
        mSpineFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:Spine");
        mLeftUpLegFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:LeftUpLeg");
        mRightUpLegFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:RightUpLeg");
        mLeftLegFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:LeftLeg");
        mRightLegFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:RightLeg");
        mLeftForeArmFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:LeftForeArm");
        mLeftHandFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:LeftHand");
        mRightForeArmFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:RightForeArm");
        mRightHandFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:RightHand");
        mRightHandIndex3FrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:RightHandIndex3");
        mRightHandIndex4FrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:RightHandIndex4");

        int materialNum = MV1GetMaterialNum(mModelHandle);
        for (int i = 0; i < materialNum; ++i)
        {
            MV1SetMaterialAmbColor(mModelHandle, i, GetColorF(0.68f, 0.64f, 0.58f, 1.0f));
            MV1SetMaterialEmiColor(mModelHandle, i, GetColorF(0.05f, 0.045f, 0.038f, 1.0f));
        }
    }

    if (mIdleAnimSourceHandle == -1 || MV1GetAnimNum(mIdleAnimSourceHandle) <= 0)
    {
        /// アニメーションが存在しない場合は無効化し、アセット不足でも実行を継続できるようにする。
        mIdleAnimSourceHandle = -1;
    }

    if (mWalkAnimSourceHandle == -1 || MV1GetAnimNum(mWalkAnimSourceHandle) <= 0)
    {
        mWalkAnimSourceHandle = -1;
    }

    if (mAttackAnimSourceHandle == -1 || MV1GetAnimNum(mAttackAnimSourceHandle) <= 0)
    {
        mAttackAnimSourceHandle = -1;
    }

    if (mAttack2AnimSourceHandle == -1 || MV1GetAnimNum(mAttack2AnimSourceHandle) <= 0)
    {
        mAttack2AnimSourceHandle = -1;
    }

    if (mAttack3AnimSourceHandle == -1 || MV1GetAnimNum(mAttack3AnimSourceHandle) <= 0)
    {
        mAttack3AnimSourceHandle = -1;
    }

    if (mAttack360AnimSourceHandle == -1 || MV1GetAnimNum(mAttack360AnimSourceHandle) <= 0)
    {
        mAttack360AnimSourceHandle = -1;
    }

    if (mChargingAnimSourceHandle == -1 || MV1GetAnimNum(mChargingAnimSourceHandle) <= 0)
    {
        mChargingAnimSourceHandle = -1;
    }

    if (mRamAnimSourceHandle == -1 || MV1GetAnimNum(mRamAnimSourceHandle) <= 0)
    {
        mRamAnimSourceHandle = -1;
    }

    if (mJumpAttackAnimSourceHandle == -1 || MV1GetAnimNum(mJumpAttackAnimSourceHandle) <= 0)
    {
        mJumpAttackAnimSourceHandle = -1;
    }

    if (mHit1AnimSourceHandle == -1 || MV1GetAnimNum(mHit1AnimSourceHandle) <= 0)
    {
        mHit1AnimSourceHandle = -1;
    }

    if (mHit2AnimSourceHandle == -1 || MV1GetAnimNum(mHit2AnimSourceHandle) <= 0)
    {
        mHit2AnimSourceHandle = -1;
    }

    if (mDyingAnimSourceHandle == -1 || MV1GetAnimNum(mDyingAnimSourceHandle) <= 0)
    {
        mDyingAnimSourceHandle = -1;
    }

    mAnimator.init(mModelHandle);
}
VECTOR Enemy::getForward() const
{
    return VGet(sinf(mRot.y), 0.0f, cosf(mRot.y));
}
VECTOR Enemy::getHeadPosition() const
{
    if (mModelHandle != -1 && mHeadFrameIndex != -1)
    {
        /// ロックオンや表示用の頭位置はモデルフレームを優先する。
        return MV1GetFramePosition(mModelHandle, mHeadFrameIndex);
    }

    return VAdd(mPos, VGet(0.0f, 9.0f, 0.0f));
}
VECTOR Enemy::getLockPointPosition() const
{
    if (mModelHandle != -1 && mSpineFrameIndex != -1)
    {
        /// ロックオン表示は頭より安定しやすい Spine フレームを優先する。
        return MV1GetFramePosition(mModelHandle, mSpineFrameIndex);
    }

    return getHeadPosition();
}
Enemy::~Enemy()
{
    deleteModel();
}
void Enemy::setBoss(bool isBoss)
{
    /// ボス化するとシールドを有効にし、通常敵と違う耐久ルールにする。
    mIsBoss = isBoss;
    mBossShieldEnabled = isBoss;
    mBossShieldHp = isBoss ? EnemyParam::BossShieldHp : 0;
    mBossShieldBreakTimer = 0.0f;
}
void Enemy::updateBossShield(float deltaTime)
{
    if (!mBossShieldEnabled || mBossShieldHp > 0 || mState == EnemyState::Dying)
    {
        return;
    }

    if (mBossShieldBreakTimer > 0.0f)
    {
        /// シールド破壊後は一定時間だけ無防備にし、時間経過で再展開する。
        mBossShieldBreakTimer -= deltaTime;
        if (mBossShieldBreakTimer <= 0.0f)
        {
            mBossShieldBreakTimer = 0.0f;
            mBossShieldHp = EnemyParam::BossShieldHp;
        }
    }
}
void Enemy::update(float deltaTime)
{
    update(deltaTime, mPos);
}
void Enemy::update(float deltaTime, VECTOR playerPos)
{
    /// 旧 API 用に、プレイヤーだけをターゲットとする CombatTarget へ変換する。
    CombatTarget target;
    target.source = ThreatSource::Player;
    target.position = playerPos;
    target.active = true;
    target.attacking = false;
    target.support = false;
    update(deltaTime, &target, 1);
}
void Enemy::update(float deltaTime, const CombatTarget* targets, int targetCount)
{
    if (mHitStopTimer > 0.0f)
    {
        /// ヒットストップ中は状態更新を止めるが、モデル位置だけは現在値へ同期する。
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

    mThreat.update(deltaTime);
    updateBossShield(deltaTime);
    updateTargetThreat(deltaTime, targets, targetCount);
    VECTOR targetPos = chooseTargetPosition(targets, targetCount, mPos);
    if (mHp <= 0 && mState != EnemyState::Dying)
    {
        /// HP 0 になった瞬間だけ死亡状態へ遷移し、以降は死亡更新に一本化する。
        startDying();
    }

    if (mState == EnemyState::Dying)
    {
        mStateTimer += deltaTime;
        updateDying(deltaTime);
        mAnimator.update(deltaTime);

        if (mModelHandle != -1)
        {
            syncModelTransform();
        }
        return;
    }

    if (mAttackCooldownTimer > 0.0f)
    {
        /// 各攻撃タイプのクールダウンを状態処理前に進める。
        mAttackCooldownTimer -= deltaTime;
    }

    if (mJumpAttackCooldownTimer > 0.0f)
    {
        mJumpAttackCooldownTimer -= deltaTime;
    }

    if (mRamCooldownTimer > 0.0f)
    {
        mRamCooldownTimer -= deltaTime;
    }

    if (mSpinAttackCooldownTimer > 0.0f)
    {
        mSpinAttackCooldownTimer -= deltaTime;
    }

    mStateTimer += deltaTime;

    VECTOR toPlayer = VSub(targetPos, mPos);
    toPlayer.y = 0.0f;
    float distanceToPlayer = VSize(toPlayer);

    /// 状態ごとの処理はここに集約し、各 updateXxx では自分の状態遷移だけを扱う。
    switch (mState)
    {
    case EnemyState::Idle:
        updateIdle(deltaTime, distanceToPlayer);
        break;

    case EnemyState::Chase:
        updateChase(deltaTime, toPlayer, distanceToPlayer);
        break;

    case EnemyState::Attack:
        updateAttack(deltaTime);
        break;

    case EnemyState::Charging:
        updateCharging(deltaTime, toPlayer, distanceToPlayer);
        break;

    case EnemyState::Ram:
        updateRam(deltaTime);
        break;

    case EnemyState::Attack360:
        updateAttack360(deltaTime);
        break;

    case EnemyState::JumpAttack:
        updateJumpAttack(deltaTime);
        break;

    case EnemyState::Hit:
        updateHit(deltaTime, targetPos);
        break;

    case EnemyState::Dying:
        updateDying(deltaTime);
        break;
    }

    mAnimator.update(deltaTime);

    if (mModelHandle != -1)
    {
        syncModelTransform();
    }

    updateAttackTrail(deltaTime);
}
void Enemy::draw()
{
    if (mHp <= 0 && mState != EnemyState::Dying)
    {
        return;
    }

    /// 攻撃予告、モデル本体、腕軌跡の順で描画する。
    drawJumpAttackMarker();

    drawModel();
    drawAttackTrail();
}
