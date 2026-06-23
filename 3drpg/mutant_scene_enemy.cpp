/**
 * @file
 * @brief mutant_scene_enemy.cpp 実装を定義する。
 */

#include "mutant_scene.h"
#include "DxLib.h"
#include "player_param.h"
#include "story_state.h"

#include <algorithm>
#include <memory>

namespace
{
    bool isCompanionInAttackRange(VECTOR companionPos, float attackRadius, const Enemy& enemy)
    {
        /// 相棒の攻撃は Enemy の複数ヒット円のいずれかに届けば命中候補とする。
        for (int i = 0; i < enemy.getHitCircleCount(); ++i)
        {
            VECTOR toHitCircle = VSub(enemy.getHitCircleCenter(i), companionPos);
            toHitCircle.y = 0.0f;
            if (VSize(toHitCircle) <= attackRadius + enemy.getHitCircleRadius(i))
            {
                return true;
            }
        }

        return false;
    }

    bool isInsideEnemyAttackRange(VECTOR targetPos, float targetRadius, const Enemy& enemy)
    {
        VECTOR attackCenter = enemy.getAttackCenter();
        if (enemy.isJumpAttacking())
        {
            /// ジャンプ攻撃は着地点中心の円形範囲として判定する。
            VECTOR toTarget = VSub(targetPos, attackCenter);
            toTarget.y = 0.0f;
            return VSize(toTarget) <= enemy.getAttackRadius() + targetRadius;
        }

        /// 通常攻撃と突進系は前方方向に沿った幅付き範囲として判定する。
        VECTOR forward = enemy.getForward();
        VECTOR toTarget = VSub(targetPos, attackCenter);
        toTarget.y = 0.0f;
        forward.y = 0.0f;
        if (VSize(forward) <= 0.0001f)
        {
            return false;
        }

        forward = VNorm(forward);
        float forwardDistance = toTarget.x * forward.x + toTarget.z * forward.z;
        if (forwardDistance < 0.0f || forwardDistance > enemy.getAttackRadius() + targetRadius)
        {
            return false;
        }

        VECTOR closest = VAdd(attackCenter, VScale(forward, forwardDistance));
        VECTOR side = VSub(targetPos, closest);
        side.y = 0.0f;
        return VSize(side) <= enemy.getAttackWidth() + targetRadius;
    }
}

void MutantScene::setupEnemies()
{
    /// FinalBoss 用の Mutant は上空から着地演出を開始する。
    VECTOR landingPos = VGet(0.0f, 0.0f, -180.0f);

    auto enemy =
        std::make_unique<Enemy>(
            VGet(landingPos.x, 120.0f, landingPos.z)
        );

    enemy->setBoss(true);
    enemy->setMovementEnabled(mEnemyMovementEnabled);

    enemy->startIntroLanding(landingPos);

    mEnemies.push_back(std::move(enemy));
}

void MutantScene::updateEnemies(float deltaTime)
{
    /// 敵 AI がターゲット選択できるよう、プレイヤーと相棒の脅威情報を毎フレーム渡す。
    CombatTarget targets[2];
    targets[0].source = ThreatSource::Player;
    targets[0].position = mPlayer->getPosition();
    targets[0].active = !mPlayer->isDead();
    targets[0].attacking = mPlayer->isAttacking();
    targets[0].support = mPlayer->isAreaPowerUp();
    targets[1].source = ThreatSource::Companion;
    targets[1].position = mCompanion != nullptr ? mCompanion->getPosition() : mPlayer->getPosition();
    targets[1].active = mCompanion != nullptr && !mCompanion->isDead();
    targets[1].attacking = mCompanion != nullptr && mCompanion->isAttacking();
    targets[1].support = mCompanion != nullptr && mCompanion->isAreaPowerUp();

    for (auto& enemy : mEnemies)
    {
        enemy->update(deltaTime, targets, 2);
        if (StoryState::instance()->isFinalBoss() && enemy->isDead())
        {
            /// FinalBoss 撃破後の戻りマーカーは、倒れた敵の位置を基準に出す。
            mReturnMarkerPos = enemy->getPosition();
            mReturnMarkerPos.y += 0.12f;
        }
    }

    mEnemies.erase(
        std::remove_if(
            mEnemies.begin(),
            mEnemies.end(),
            [](const std::unique_ptr<Enemy>& enemy)
            {
                /// 死亡演出後の Enemy はリストから取り除き、戦闘判定対象外にする。
                return enemy->isDead();
            }
        ),
        mEnemies.end()
    );
}
void MutantScene::updateCompanion(float deltaTime)
{
    if (mCompanion == nullptr)
    {
        mCompanionDebugState = "None";
        return;
    }

    if (mCompanion->isDead())
    {
        /// 相棒死亡中も死亡アニメーションだけは更新し、完了後しばらくしてから削除する。
        mCompanionDebugState = "Dead";
        mCompanion->update(deltaTime);
        if (mCompanion->isDeadAnimationFinished())
        {
            mCompanionDeadTimer += deltaTime;
            if (mCompanionDeadTimer >= 5.0f)
            {
                mCompanion.reset();
            }
        }
        else
        {
            mCompanionDeadTimer = 0.0f;
        }
        return;
    }

    mCompanionDeadTimer = 0.0f;

    if (mCompanionAttackTimer > 0.0f) mCompanionAttackTimer -= deltaTime;
    if (mCompanionSpellTimer > 0.0f) mCompanionSpellTimer -= deltaTime;
    if (mCompanionDodgeTimer > 0.0f) mCompanionDodgeTimer -= deltaTime;
    if (mCompanionGuardHoldTimer > 0.0f) mCompanionGuardHoldTimer -= deltaTime;
    if (mCompanionSpecialTimer > 0.0f) mCompanionSpecialTimer -= deltaTime;
    if (mCompanionDefenseReactionTimer > 0.0f) mCompanionDefenseReactionTimer -= deltaTime;

    VECTOR companionPos = mCompanion->getPosition();
    Enemy* targetEnemy = getNearestEnemy();

    PlayerInput input;
    mCompanionDebugState = "Assist";
    mCompanion->setLockOn(targetEnemy != nullptr);

    if (targetEnemy != nullptr)
    {
        /// 相棒 AI は最寄りの敵を基準に、移動方向と攻撃/防御入力を組み立てる。
        VECTOR toTarget = VSub(targetEnemy->getPosition(), companionPos);
        toTarget.y = 0.0f;
        float distanceToTarget = VSize(toTarget);

        auto setMove = [&](VECTOR dir, VECTOR lockForward)
        {
            /// ワールド方向をロックオン移動用の入力軸へ変換する。
            dir.y = 0.0f;
            lockForward.y = 0.0f;
            if (VSize(dir) <= 0.0001f || VSize(lockForward) <= 0.0001f)
            {
                return;
            }
            dir = VNorm(dir);
            lockForward = VNorm(lockForward);
            VECTOR lockRight = VGet(lockForward.z, 0.0f, -lockForward.x);
            input.move.x = dir.x * lockRight.x + dir.z * lockRight.z;
            input.move.z = dir.x * lockForward.x + dir.z * lockForward.z;
        };
        if (distanceToTarget > 0.0001f)
        {
            VECTOR toTargetDir = VNorm(toTarget);
            VECTOR awayDir = VScale(toTargetDir, -1.0f);

            if (mCompanion->isWizard())
            {
                /// Wizard 相棒は距離を取り、HP が減った時は範囲強化を優先する。
                mCompanionDebugState = "WizardAI";
                if (distanceToTarget < 42.0f)
                {
                    setMove(awayDir, toTargetDir);
                    if (mCompanionDodgeTimer <= 0.0f && mCompanion->getStamina() >= PlayerParam::DodgeStamina)
                    {
                        input.dodge = true;
                        mCompanionDodgeTimer = 3.0f;
                    }
                }
                else if (distanceToTarget > 70.0f)
                {
                    setMove(toTargetDir, toTargetDir);
                }

                if (mCompanionSpecialTimer <= 0.0f && mCompanion->getStamina() >= PlayerParam::WizardAreaPowerUpStamina &&
                    (mPlayer->getHp() <= PlayerParam::Hp * 6 / 10 || mCompanion->getHp() <= PlayerParam::Hp * 6 / 10))
                {
                    input.counter = true;
                    mCompanionSpecialTimer = 8.0f;
                }
                else if (mCompanionGuardHoldTimer > 0.0f)
                {
                    input.guard = true;
                }
                else if (mCompanionSpellTimer <= 0.0f && mCompanion->getStamina() >= PlayerParam::WizardMagicAttack2StartStamina && distanceToTarget <= 80.0f)
                {
                    input.guard = true;
                    mCompanionGuardHoldTimer = 1.15f;
                    mCompanionSpellTimer = 5.0f;
                }
                else if (mCompanionAttackTimer <= 0.0f && distanceToTarget <= 90.0f)
                {
                    input.attack = true;
                    mCompanionAttackTimer = 1.2f;
                }
            }
            else
            {
                /// Knight 相棒は近距離維持を基本にし、敵攻撃の脅威に応じて防御行動を選ぶ。
                mCompanionDebugState = "KnightAI";
                if (distanceToTarget > 34.0f)
                {
                    setMove(toTargetDir, toTargetDir);
                }
                else if (distanceToTarget < 18.0f)
                {
                    setMove(awayDir, toTargetDir);
                }

                bool danger = targetEnemy->isAttackThreatening() &&
                    isInsideEnemyAttackRange(companionPos, mCompanion->getHitRadius(), *targetEnemy);
                if (!danger)
                {
                    mCompanionDefenseReactionTimer = -1.0f;
                }

                if (mCompanionGuardHoldTimer > 0.0f)
                {
                    input.guard = true;
                }
                else if (danger && mCompanionDefenseReactionTimer < 0.0f)
                {
                    /// 防御反応に短いランダム遅延を入れ、人間らしい反応速度にする。
                    mCompanionDefenseReactionTimer = 0.12f + static_cast<float>(GetRand(12)) * 0.01f;
                }
                else if (danger && mCompanionDefenseReactionTimer <= 0.0f)
                {
                    if (targetEnemy->isCounterTiming() &&
                        mCompanionSpecialTimer <= 0.0f &&
                        mCompanion->getStamina() >= PlayerParam::CounterStamina &&
                        GetRand(99) < 65)
                    {
                        input.counter = true;
                        mCompanionSpecialTimer = 2.4f;
                        mCompanionDefenseReactionTimer = -1.0f;
                    }
                    else if (targetEnemy->isDodgeTiming() &&
                        mCompanionDodgeTimer <= 0.0f &&
                        mCompanion->getStamina() >= PlayerParam::DodgeStamina &&
                        GetRand(99) < 55)
                    {
                        input.dodge = true;
                        mCompanionDodgeTimer = 2.6f;
                        mCompanionDefenseReactionTimer = -1.0f;
                    }
                    else if (mCompanionGuardHoldTimer <= 0.0f)
                    {
                        input.guard = true;
                        mCompanionGuardHoldTimer = 0.55f;
                        mCompanionDefenseReactionTimer = -1.0f;
                    }
                }
                else if (!danger && isCompanionInAttackRange(companionPos, mCompanion->getAttackRadius(), *targetEnemy) && mCompanionAttackTimer <= 0.0f)
                {
                    input.attack = true;
                    mCompanionAttackTimer = 1.45f;
                }
            }

            if (!mCompanion->isDodging())
            {
                /// 回避中は回避方向を尊重し、それ以外では敵の方向へ向ける。
                mCompanion->setRotation(VGet(0.0f, atan2f(toTarget.x, toTarget.z), 0.0f));
            }
        }
    }
    else
    {
        mCompanion->setLockOn(false);
    }

    mCompanion->setInput(input);
    mCompanion->update(deltaTime);

    if (targetEnemy != nullptr)
    {
        VECTOR toTarget = VSub(targetEnemy->getPosition(), mCompanion->getPosition());
        toTarget.y = 0.0f;
        if (VSize(toTarget) > 0.0001f)
        {
            if (!mCompanion->isDodging())
            {
                mCompanion->setRotation(VGet(0.0f, atan2f(toTarget.x, toTarget.z), 0.0f));
            }
        }
    }
}
