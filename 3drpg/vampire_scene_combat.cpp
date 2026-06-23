/**
 * @file
 * @brief vampire_scene_combat.cpp 実装を定義する。
 */

#include "vampire_scene.h"
#include "audio_manager.h"
#include "game_config.h"
#include "game_settings.h"
#include "input_manager.h"
#include "player_param.h"
#include "sound_resource.h"
#include "DxLib.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float WizardMagicBeamRange = 118.0f; ///< Wizard ビームの射程。ワールド空間。
constexpr float WizardMagicBeamWidth = 15.0f; ///< Wizard ビームの横幅判定。

bool isBindingPush(InputManager* inputMgr, InputBinding binding)
{
    /// キーボード、マウス、ゲームパッドの入力設定を同じ呼び出し口で扱う。
    if (binding.type == InputBindingMouse) return inputMgr->isMousePush(binding.code);
    if (binding.type == InputBindingPad)
    {
        if (binding.code == PadBindingLeftTrigger) return inputMgr->isPadLeftTriggerPush();
        if (binding.code == PadBindingRightTrigger) return inputMgr->isPadRightTriggerPush();
        return inputMgr->isPadPush(binding.code);
    }
    return inputMgr->isPush(binding.code);
}

bool isBindingPress(InputManager* inputMgr, InputBinding binding)
{
    if (binding.type == InputBindingMouse) return inputMgr->isMousePress(binding.code);
    if (binding.type == InputBindingPad)
    {
        if (binding.code == PadBindingLeftTrigger) return inputMgr->isPadLeftTriggerPress();
        if (binding.code == PadBindingRightTrigger) return inputMgr->isPadRightTriggerPress();
        return inputMgr->isPadPress(binding.code);
    }
    return inputMgr->isPress(binding.code);
}

void addPadMoveInput(InputManager* inputMgr, PlayerInput* input)
{
    input->move.x += inputMgr->getPadLeftStickX();
    input->move.z += inputMgr->getPadLeftStickY();
}

bool isMagicEnemyInsideMagicBeam(VECTOR startPos, VECTOR forward, const MagicProjectileEnemy& enemy)
{
    /// ボス中心をビーム線分へ射影し、前方距離と横幅の両方で命中を判定する。
    forward.y = 0.0f;
    if (VSize(forward) <= 0.0001f)
    {
        return false;
    }

    forward = VNorm(forward);
    VECTOR toEnemy = VSub(enemy.getPosition(), startPos);
    toEnemy.y = 0.0f;
    float forwardDistance = toEnemy.x * forward.x + toEnemy.z * forward.z;
    if (forwardDistance < 0.0f || forwardDistance > WizardMagicBeamRange)
    {
        return false;
    }

    VECTOR nearest = VAdd(startPos, VScale(forward, forwardDistance));
    VECTOR side = VSub(enemy.getPosition(), nearest);
    side.y = 0.0f;
    return VSize(side) <= WizardMagicBeamWidth + enemy.getHitRadius();
}

VECTOR getMagicEnemyBeamHitPosition(VECTOR startPos, VECTOR forward, const MagicProjectileEnemy& enemy)
{
    forward.y = 0.0f;
    if (VSize(forward) <= 0.0001f)
    {
        return enemy.getLockPointPosition();
    }

    forward = VNorm(forward);
    VECTOR toEnemy = VSub(enemy.getPosition(), startPos);
    toEnemy.y = 0.0f;
    float forwardDistance = toEnemy.x * forward.x + toEnemy.z * forward.z;
    if (forwardDistance < 0.0f) forwardDistance = 0.0f;
    if (forwardDistance > WizardMagicBeamRange) forwardDistance = WizardMagicBeamRange;

    VECTOR hitPos = VAdd(startPos, VScale(forward, forwardDistance));
    hitPos.y = enemy.getLockPointPosition().y;
    return hitPos;
}

bool isInsideAreaPowerUp(VECTOR center, VECTOR targetPos, float radius)
{
    VECTOR toTarget = VSub(targetPos, center);
    toTarget.y = 0.0f;
    return VSize(toTarget) <= radius;
}

float updateDelayedHpValue(float delayedHp, float currentHp, float deltaTime)
{
    constexpr float DelayDrainSpeed = 8.0f; ///< UI 表示用 HP が実 HP へ追いつく速度。単位は HP/second。
    /// UI 用 HP は実 HP より遅れて減らし、被弾量を見やすくする。
    if (delayedHp <= currentHp)
    {
        return currentHp;
    }

    delayedHp -= DelayDrainSpeed * deltaTime;
    return delayedHp < currentHp ? currentHp : delayedHp;
}
}

void VampireScene::updatePlayer(float deltaTime)
{
    InputManager* inputMgr = InputManager::instance();
    GameSettings* settings = GameSettings::instance();
    int role = mPlayerType == PlayerCharacterType::Wizard ? 1 : 0;
    PlayerInput input;
    bool onExitMarker = canUseExitMarker();
    /// プレイヤー種類に応じたキー設定と共通パッド設定を合成して入力を作る。
    if (isBindingPress(inputMgr, settings->getInputBinding(role, GameActionMoveForward))) input.move.z += 1.0f;
    if (isBindingPress(inputMgr, settings->getInputBinding(role, GameActionMoveBack))) input.move.z -= 1.0f;
    if (isBindingPress(inputMgr, settings->getInputBinding(role, GameActionMoveLeft))) input.move.x -= 1.0f;
    if (isBindingPress(inputMgr, settings->getInputBinding(role, GameActionMoveRight))) input.move.x += 1.0f;
    if (isBindingPress(inputMgr, settings->getPadBinding(GameActionMoveForward))) input.move.z += 1.0f;
    if (isBindingPress(inputMgr, settings->getPadBinding(GameActionMoveBack))) input.move.z -= 1.0f;
    if (isBindingPress(inputMgr, settings->getPadBinding(GameActionMoveLeft))) input.move.x -= 1.0f;
    if (isBindingPress(inputMgr, settings->getPadBinding(GameActionMoveRight))) input.move.x += 1.0f;
    addPadMoveInput(inputMgr, &input);
    input.attack = isBindingPush(inputMgr, settings->getInputBinding(role, GameActionPrimary)) || isBindingPush(inputMgr, settings->getPadBinding(GameActionPrimary));
    input.dodge = isBindingPush(inputMgr, settings->getInputBinding(role, GameActionDodge)) || isBindingPush(inputMgr, settings->getPadBinding(GameActionDodge));
    input.guard = isBindingPress(inputMgr, settings->getInputBinding(role, GameActionSecondary)) || isBindingPress(inputMgr, settings->getPadBinding(GameActionSecondary));
    input.counter = !onExitMarker && (isBindingPush(inputMgr, settings->getInputBinding(role, GameActionSpecial)) || isBindingPush(inputMgr, settings->getPadBinding(GameActionSpecial)));

    if (isBindingPush(inputMgr, settings->getInputBinding(role, GameActionLockOn)) || isBindingPush(inputMgr, settings->getPadBinding(GameActionLockOn)))
    {
        /// ロックオンは生存中のボスがいる場合だけ切り替えられる。
        mIsEnemyLocked = !mIsEnemyLocked && mMagicEnemy != nullptr && !mMagicEnemy->isDead();
    }

    if (settings->isAutoLockTarget() && mMagicEnemy != nullptr && !mMagicEnemy->isDead())
    {
        mIsEnemyLocked = true;
    }
    mPlayer->setLockOn(mIsEnemyLocked && mMagicEnemy != nullptr && !mMagicEnemy->isDead());
    mPlayer->setInput(input);
    mPlayer->update(deltaTime);
    if (mIsEnemyLocked && !mPlayer->isDodging())
    {
        /// 回避中は回避方向を優先し、それ以外のロックオン中だけボスへ向き直る。
        if (mMagicEnemy != nullptr && !mMagicEnemy->isDead())
        {
            faceEnemy(mPlayer.get());
        }
        else if (mMagicEnemy == nullptr || mMagicEnemy->isDeadAnimationFinished())
        {
            mIsEnemyLocked = false;
        }
    }
}

void VampireScene::updateCompanion(float deltaTime)
{
    if (mCompanion == nullptr)
    {
        return;
    }

    if (mCompanion->isDead())
    {
        PlayerInput input;
        mCompanion->setInput(input);
        mCompanion->update(deltaTime);
        return;
    }

    if (mCompanionAttackTimer > 0.0f) mCompanionAttackTimer -= deltaTime;
    if (mCompanionDodgeTimer > 0.0f) mCompanionDodgeTimer -= deltaTime;
    if (mCompanionSpecialTimer > 0.0f) mCompanionSpecialTimer -= deltaTime;
    if (mCompanionStrafeTimer > 0.0f) mCompanionStrafeTimer -= deltaTime;
    if (mCompanionProjectileDecisionTimer > 0.0f) mCompanionProjectileDecisionTimer -= deltaTime;

    PlayerInput input;
    VECTOR companionPos = mCompanion->getPosition();
    VECTOR toEnemy = VSub(mMagicEnemy->getPosition(), companionPos);
    toEnemy.y = 0.0f;
    float distance = VSize(toEnemy);
    if (distance <= 0.0001f)
    {
        mCompanion->setInput(input);
        mCompanion->update(deltaTime);
        return;
    }

    VECTOR toEnemyDir = VNorm(toEnemy);
    VECTOR awayDir = VScale(toEnemyDir, -1.0f);
    VECTOR sideDir = VGet(toEnemyDir.z * static_cast<float>(mCompanionStrafeDir), 0.0f, -toEnemyDir.x * static_cast<float>(mCompanionStrafeDir));
    VECTOR nearestProjectilePos = VGet(0.0f, 0.0f, 0.0f);
    VECTOR nearestProjectileVelocity = VGet(0.0f, 0.0f, 0.0f);
    float nearestProjectileDistance = mMagicEnemy->getNearestMovingProjectileDistance(companionPos);
    bool hasProjectileThreat = mMagicEnemy->getNearestMovingProjectileInfo(companionPos, &nearestProjectilePos, &nearestProjectileVelocity);
    bool projectileApproaching = false;
    VECTOR projectileAvoidDir = sideDir;
    if (hasProjectileThreat && VSize(nearestProjectileVelocity) > 0.0001f)
    {
        /// 近い弾でも離れていく弾は無視し、接近中の弾だけ回避対象にする。
        VECTOR projectileDir = VNorm(nearestProjectileVelocity);
        projectileDir.y = 0.0f;
        VECTOR awayFromProjectile = VSub(companionPos, nearestProjectilePos);
        awayFromProjectile.y = 0.0f;
        if (VSize(awayFromProjectile) > 0.0001f)
        {
            VECTOR toCompanion = VNorm(awayFromProjectile);
            projectileApproaching = projectileDir.x * toCompanion.x + projectileDir.z * toCompanion.z > 0.15f;
        }
        projectileAvoidDir = VGet(projectileDir.z, 0.0f, -projectileDir.x);
        if (projectileAvoidDir.x * awayFromProjectile.x + projectileAvoidDir.z * awayFromProjectile.z < 0.0f)
        {
            projectileAvoidDir = VScale(projectileAvoidDir, -1.0f);
        }
    }
    bool projectileThreat = projectileApproaching && nearestProjectileDistance < 58.0f;
    if (!projectileThreat)
    {
        mCompanionProjectileDecisionTimer = 0.0f;
        mCompanionIgnoreProjectile = false;
    }
    else if (mCompanionProjectileDecisionTimer <= 0.0f)
    {
        /// 弾回避判断に短い保持時間を作り、毎フレーム判断が揺れないようにする。
        mCompanionIgnoreProjectile = false;
        mCompanionProjectileDecisionTimer = 0.65f;
    }
    bool projectilePressure = projectileThreat && !mCompanionIgnoreProjectile;
    bool projectileDanger = projectilePressure && nearestProjectileDistance < 26.0f;
    bool shouldRollProjectile = projectileDanger && (nearestProjectileDistance < 18.0f || GetRand(99) < 75);

    if (mCompanionStrafeTimer <= 0.0f)
    {
        mCompanionStrafeDir *= -1;
        mCompanionStrafeTimer = 1.4f + static_cast<float>(GetRand(8)) * 0.1f;
    }

    auto setMove = [&](VECTOR dir)
    {
        /// ロックオン移動用に、ワールド方向を相棒の入力軸へ変換する。
        dir.y = 0.0f;
        if (VSize(dir) <= 0.0001f)
        {
            return;
        }
        dir = VNorm(dir);
        VECTOR lockRight = VGet(toEnemyDir.z, 0.0f, -toEnemyDir.x);
        input.move.x = dir.x * lockRight.x + dir.z * lockRight.z;
        input.move.z = dir.x * toEnemyDir.x + dir.z * toEnemyDir.z;
    };

    if (mCompanion->isWizard())
    {
        /// Wizard 相棒は距離を取りつつ、危険弾の回避と範囲回復を優先する。
        if (projectilePressure)
        {
            setMove(projectileAvoidDir);
            if (shouldRollProjectile && mCompanionDodgeTimer <= 0.0f && mCompanion->getStamina() >= PlayerParam::DodgeStamina)
            {
                input.dodge = true;
                mCompanionDodgeTimer = 2.1f;
            }
        }
        else if (distance < 58.0f)
        {
            setMove(awayDir);
        }
        else if (distance > 92.0f)
        {
            setMove(toEnemyDir);
        }
        else
        {
            setMove(sideDir);
        }

        if (mCompanionSpecialTimer <= 0.0f && mCompanion->getStamina() >= PlayerParam::WizardAreaPowerUpStamina &&
            (mPlayer->getHp() <= PlayerParam::Hp * 6 / 10 || mCompanion->getHp() <= PlayerParam::Hp * 6 / 10))
        {
            input.counter = true;
            mCompanionSpecialTimer = 8.0f;
        }
        else if (!projectilePressure && mCompanionAttackTimer <= 0.0f && distance <= 95.0f)
        {
            input.attack = true;
            mCompanionAttackTimer = 1.35f;
        }
    }
    else
    {
        /// Knight 相棒は近距離維持を基本にし、弾圧がある時だけ回避へ切り替える。
        if (projectilePressure)
        {
            setMove(projectileAvoidDir);
            if (shouldRollProjectile && mCompanionDodgeTimer <= 0.0f && mCompanion->getStamina() >= PlayerParam::DodgeStamina)
            {
                input.dodge = true;
                mCompanionDodgeTimer = 2.0f;
            }
        }
        else if (distance > 50.0f)
        {
            setMove(toEnemyDir);
        }
        else if (distance < 32.0f)
        {
            setMove(awayDir);
        }
        else
        {
            setMove(sideDir);
        }

        if (!projectilePressure && mCompanionAttackTimer <= 0.0f && distance <= 46.0f)
        {
            input.attack = true;
            mCompanionAttackTimer = 1.45f;
        }
    }

    mCompanion->setLockOn(true);
    if (!mCompanion->isDodging())
    {
        faceEnemy(mCompanion.get());
    }
    mCompanion->setInput(input);
    mCompanion->update(deltaTime);
}
void VampireScene::updateDelayedPlayerHp(float deltaTime)
{
    mPlayerDelayedHp = updateDelayedHpValue(mPlayerDelayedHp, static_cast<float>(mPlayer->getHp()), deltaTime);
    if (mCompanion != nullptr)
    {
        mCompanionDelayedHp = updateDelayedHpValue(mCompanionDelayedHp, static_cast<float>(mCompanion->getHp()), deltaTime);
    }
}
void VampireScene::updateKnightSwordTrails()
{
    if (mPlayer != nullptr && !mPlayer->isWizard() && mPlayer->isAttacking())
    {
        mAttackEffect.addSwordTrailSample(
            mPlayer->getSwordJointPosition(),
            mPlayer->getSwordTipPosition(),
            mPlayer->getAttackStep());
    }

    if (mCompanion != nullptr && !mCompanion->isWizard() && mCompanion->isAttacking())
    {
        mAttackEffect.addSwordTrailSample(
            mCompanion->getSwordJointPosition(),
            mCompanion->getSwordTipPosition(),
            mCompanion->getAttackStep());
    }
}
void VampireScene::processPlayerAttack()
{
    if (!mPlayer->consumeAttackHitRequest())
    {
        return;
    }

    VECTOR playerPos = mPlayer->getPosition();
    VECTOR magicStartPos = mPlayer->getMagicHandPosition();

    if (mPlayer->isAreaPowerUp())
    {
        /// 範囲強化は自分を必ず回復し、相棒は範囲内にいる場合だけ対象にする。
        mPlayer->applyAreaPowerUp();
        mAttackEffect.startHealEffect(mPlayer->getPosition());
        if (mCompanion != nullptr && !mCompanion->isDead() &&
            isInsideAreaPowerUp(playerPos, mCompanion->getPosition(), mPlayer->getAreaPowerUpRadius()))
        {
            mCompanion->applyAreaPowerUp();
            mAttackEffect.startHealEffect(mCompanion->getPosition());
        }
        mAttackEffect.startMagicEffect(playerPos, mPlayer->getRotationY(), mPlayer->getAttackStep(), mStageCollisionModelHandle);
        return;
    }

    if (mPlayer->isWizard() && mPlayer->getAttackStep() == 1 && mIsEnemyLocked)
    {
        /// ロックオン魔法弾はエフェクト到達に合わせるため、ダメージを遅延キューへ入れる。
        mAttackEffect.startMagicEffect(
            magicStartPos,
            mPlayer->getRotationY(),
            mPlayer->getAttackStep(),
            mMagicEnemy->getLockPointPosition());

        PendingMagicMissileHit pendingHit;
        pendingHit.damage = mPlayer->getAttackPower();
        pendingHit.attackStep = mPlayer->getAttackStep();
        pendingHit.timer = 0.62f;
        mPendingMagicMissileHits.push_back(pendingHit);
        return;
    }

    if (mPlayer->isWizard())
    {
        mAttackEffect.startMagicEffect(
            magicStartPos,
            mPlayer->getRotationY(),
            mPlayer->getAttackStep());
    }

    if (mPlayer->isWizard() && mPlayer->getAttackStep() == 2)
    {
        /// ビームはボスの当たり半径を含めた線分判定で命中を確認する。
        VECTOR playerForward = VGet(sinf(mPlayer->getRotationY()), 0.0f, cosf(mPlayer->getRotationY()));
        if (isMagicEnemyInsideMagicBeam(magicStartPos, playerForward, *mMagicEnemy))
        {
            mMagicEnemy->damage(mPlayer->getAttackPower());
            mAttackEffect.startMagicHitImpact(getMagicEnemyBeamHitPosition(magicStartPos, playerForward, *mMagicEnemy), mPlayer->getAttackStep());
        }
        return;
    }

    if (!isEnemyInAttackRange(mPlayer.get()))
    {
        return;
    }

    mMagicEnemy->damage(mPlayer->getAttackPower());
    if (mPlayer->isWizard())
    {
        mAttackEffect.startMagicHitImpact(mMagicEnemy->getLockPointPosition(), mPlayer->getAttackStep());
    }
    else
    {
        mAttackEffect.startHitImpact(mMagicEnemy->getHeadPosition(), mPlayer->getAttackStep());
        AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightSlashHitFlesh);
        mPlayer->startHitStop(PlayerParam::HitStopDuration);
    }
}

void VampireScene::processCompanionAttack()
{
    if (mCompanion == nullptr || !mCompanion->consumeAttackHitRequest())
    {
        return;
    }

    VECTOR companionPos = mCompanion->getPosition();
    VECTOR magicStartPos = mCompanion->getMagicHandPosition();

    if (mCompanion->isAreaPowerUp())
    {
        mCompanion->applyAreaPowerUp();
        mAttackEffect.startHealEffect(mCompanion->getPosition());
        if (!mPlayer->isDead() &&
            isInsideAreaPowerUp(companionPos, mPlayer->getPosition(), mCompanion->getAreaPowerUpRadius()))
        {
            mPlayer->applyAreaPowerUp();
            mAttackEffect.startHealEffect(mPlayer->getPosition());
        }
        mAttackEffect.startMagicEffect(companionPos, mCompanion->getRotationY(), mCompanion->getAttackStep(), mStageCollisionModelHandle);
        return;
    }

    if (mCompanion->isWizard() && mCompanion->getAttackStep() == 1)
    {
        /// 相棒の魔法弾も同じ遅延キューを使い、見た目とダメージタイミングを揃える。
        mAttackEffect.startMagicEffect(
            magicStartPos,
            mCompanion->getRotationY(),
            mCompanion->getAttackStep(),
            mMagicEnemy->getLockPointPosition());

        PendingMagicMissileHit pendingHit;
        pendingHit.damage = mCompanion->getAttackPower();
        pendingHit.attackStep = mCompanion->getAttackStep();
        pendingHit.timer = 0.62f;
        mPendingMagicMissileHits.push_back(pendingHit);
        return;
    }

    if (mCompanion->isWizard())
    {
        mAttackEffect.startMagicEffect(
            magicStartPos,
            mCompanion->getRotationY(),
            mCompanion->getAttackStep());
    }

    if (mCompanion->isWizard() && mCompanion->getAttackStep() == 2)
    {
        VECTOR companionForward = VGet(sinf(mCompanion->getRotationY()), 0.0f, cosf(mCompanion->getRotationY()));
        if (isMagicEnemyInsideMagicBeam(magicStartPos, companionForward, *mMagicEnemy))
        {
            mMagicEnemy->damage(mCompanion->getAttackPower());
            mAttackEffect.startMagicHitImpact(getMagicEnemyBeamHitPosition(magicStartPos, companionForward, *mMagicEnemy), mCompanion->getAttackStep());
        }
        return;
    }

    if (!isEnemyInAttackRange(mCompanion.get()))
    {
        return;
    }

    mMagicEnemy->damage(mCompanion->getAttackPower());
    if (mCompanion->isWizard())
    {
        mAttackEffect.startMagicHitImpact(mMagicEnemy->getLockPointPosition(), mCompanion->getAttackStep());
    }
    else
    {
        mAttackEffect.startHitImpact(mMagicEnemy->getHeadPosition(), mCompanion->getAttackStep());
        AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightSlashHitFlesh);
        mCompanion->startHitStop(PlayerParam::HitStopDuration);
    }
}

void VampireScene::updatePendingMagicMissileHits(float deltaTime)
{
    for (auto& hit : mPendingMagicMissileHits)
    {
        /// 演出用の待ち時間を消化してから、実際のボス HP を減らす。
        hit.timer -= deltaTime;
    }

    for (auto& hit : mPendingMagicMissileHits)
    {
        if (hit.timer <= 0.0f)
        {
            if (mMagicEnemy != nullptr && !mMagicEnemy->isDead())
            {
                mMagicEnemy->damage(hit.damage);
                mAttackEffect.startMagicHitImpact(mMagicEnemy->getLockPointPosition(), hit.attackStep);
            }
            hit.damage = 0;
        }
    }

    mPendingMagicMissileHits.erase(
        std::remove_if(
            mPendingMagicMissileHits.begin(),
            mPendingMagicMissileHits.end(),
            [](const PendingMagicMissileHit& hit)
            {
                return hit.damage <= 0;
            }),
        mPendingMagicMissileHits.end());
}
void VampireScene::faceEnemy(Player* player)
{
    if (player == nullptr || mMagicEnemy == nullptr || mMagicEnemy->isDead())
    {
        return;
    }

    VECTOR toEnemy = VSub(mMagicEnemy->getPosition(), player->getPosition());
    toEnemy.y = 0.0f;
    if (VSize(toEnemy) > 0.0001f)
    {
        /// XZ 平面だけで向きを決め、上下位置差で回転が傾かないようにする。
        player->setRotation(VGet(0.0f, atan2f(toEnemy.x, toEnemy.z), 0.0f));
    }
}

bool VampireScene::isEnemyInAttackRange(Player* player) const
{
    if (player == nullptr || mMagicEnemy == nullptr || mMagicEnemy->isDead())
    {
        return false;
    }

    VECTOR forward = VGet(sinf(player->getRotationY()), 0.0f, cosf(player->getRotationY()));
    return mMagicEnemy->isInsideAttackCapsule(player->getPosition(), forward, player->getAttackRadius());
}

