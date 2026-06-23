/**
 * @file
 * @brief mutant_scene_combat.cpp 実装を定義する。
 */

#include "mutant_scene.h"
#include "player_param.h"
#include "audio_manager.h"
#include "sound_resource.h"
#include "story_state.h"
#include "game_config.h"
#include "DxLib.h"

#include <cmath>

namespace
{
    constexpr float WizardMagicBeamRange = 118.0f; ///< Wizard ビームの射程。ワールド空間。
    constexpr float WizardMagicBeamWidth = 15.0f; ///< Wizard ビームの横幅判定。
    constexpr float RamCounterForwardBuffer = 12.0f; ///< 突進に対するカウンター判定を前方へ広げる距離。
    constexpr float MutantArenaMaxGroundStep = 8.0f; ///< 最終ボス arena で地面へ吸着できる最大段差。

    void clampToMutantArena(Player* player)
    {
        if (player == nullptr || !StoryState::instance()->isFinalBoss())
        {
            return;
        }

        VECTOR pos = player->getPosition();
        VECTOR flat = VGet(pos.x, 0.0f, pos.z);
        float distance = VSize(flat);
        if (distance <= MutantSceneConfig::MutantArenaLimitRadius || distance <= 0.0001f)
        {
            return;
        }

        /// 最終戦では円形 arena から出ないよう、XZ 平面だけを制限する。
        flat = VScale(VNorm(flat), MutantSceneConfig::MutantArenaLimitRadius);
        pos.x = flat.x;
        pos.z = flat.z;
        player->setPosition(pos);
    }

    void clampEnemyToMutantArena(Enemy* enemy)
    {
        if (enemy == nullptr || !StoryState::instance()->isFinalBoss())
        {
            return;
        }

        VECTOR pos = enemy->getPosition();
        VECTOR flat = VGet(pos.x, 0.0f, pos.z);
        float distance = VSize(flat);
        if (distance <= MutantSceneConfig::MutantArenaLimitRadius || distance <= 0.0001f)
        {
            return;
        }

        /// 敵も arena 外へ押し出されないよう、プレイヤーと同じ半径制限をかける。
        flat = VScale(VNorm(flat), MutantSceneConfig::MutantArenaLimitRadius);
        pos.x = flat.x;
        pos.z = flat.z;
        enemy->setPosition(pos);
    }

    bool canSnapToStageGround(float currentY, float hitY)
    {
        /// 最終戦では急な段差へ吸着しないようにし、arena 境界付近の不自然な高さ補正を避ける。
        return !StoryState::instance()->isFinalBoss() || hitY <= currentY + MutantArenaMaxGroundStep;
    }
    bool isInsideFrontAttack(VECTOR attackerPos, VECTOR forward, float attackRadius, VECTOR targetPos, float targetRadius)
    {
        VECTOR toTarget = VSub(targetPos, attackerPos);
        toTarget.y = 0.0f;
        float distance = VSize(toTarget);
        if (distance <= 0.0001f)
        {
            return true;
        }

        VECTOR attackDir = VNorm(toTarget);
        float attackDot = forward.x * attackDir.x + forward.z * attackDir.z;
        return attackDot >= PlayerParam::AttackFrontDotThreshold && distance <= attackRadius + targetRadius;
    }

    bool isEnemyInsideFrontAttack(VECTOR attackerPos, VECTOR forward, float attackRadius, const Enemy& enemy)
    {
        for (int i = 0; i < enemy.getHitCircleCount(); ++i)
        {
            if (isInsideFrontAttack(attackerPos, forward, attackRadius, enemy.getHitCircleCenter(i), enemy.getHitCircleRadius(i)))
            {
                return true;
            }
        }

        return false;
    }

    bool getNearestEnemyFrontAttackHitPoint(VECTOR attackerPos, VECTOR forward, float attackRadius, const Enemy& enemy, VECTOR* hitPos)
    {
        /// 複数の当たり円から最も近い命中点を選び、ヒットエフェクトを体の近い位置へ出す。
        bool hit = false;
        float nearestDistance = 0.0f;
        VECTOR nearestPos = enemy.getPosition();

        for (int i = 0; i < enemy.getHitCircleCount(); ++i)
        {
            VECTOR circleCenter = enemy.getHitCircleCenter(i);
            if (!isInsideFrontAttack(attackerPos, forward, attackRadius, circleCenter, enemy.getHitCircleRadius(i)))
            {
                continue;
            }

            VECTOR toCircle = VSub(circleCenter, attackerPos);
            toCircle.y = 0.0f;
            float distance = VSize(toCircle);
            if (!hit || distance < nearestDistance)
            {
                hit = true;
                nearestDistance = distance;
                nearestPos = circleCenter;
            }
        }

        if (hit && hitPos != nullptr)
        {
            *hitPos = nearestPos;
        }

        return hit;
    }

    bool isEnemyInsideMagicBeam(VECTOR startPos, VECTOR forward, const Enemy& enemy)
    {
        return enemy.isMagicBeamHitting(startPos, forward, WizardMagicBeamRange, WizardMagicBeamWidth);
    }

    bool isInsideAreaPowerUp(VECTOR center, VECTOR targetPos, float radius)
    {
        VECTOR toTarget = VSub(targetPos, center);
        toTarget.y = 0.0f;
        return VSize(toTarget) <= radius;
    }

    void playHitImpactForResult(AttackEffect& effect, EnemyDamageResult result, VECTOR hitPos, int attackStep, bool magic)
    {
        /// シールドと HP ダメージでエフェクトを分け、同じ命中でも結果を視覚的に区別する。
        if (result == EnemyDamageResult::None)
        {
            return;
        }

        if (magic)
        {
            effect.startMagicHitImpact(hitPos, attackStep);
        }
        else if (result == EnemyDamageResult::Shield)
        {
            effect.startShieldHitImpact(hitPos, attackStep);
        }
        else if (result == EnemyDamageResult::Hp)
        {
            effect.startHitImpact(hitPos, attackStep);
        }
    }
    bool isInsideCircle(VECTOR center, VECTOR targetPos, float radius, float targetRadius)
    {
        VECTOR toTarget = VSub(targetPos, center);
        toTarget.y = 0.0f;
        return VSize(toTarget) <= radius + targetRadius;
    }


    bool isOverlappingCapsule(VECTOR center, float radius, float height, VECTOR targetPos, float targetRadius, float targetHeight)
    {
        float bottom = center.y;
        float top = center.y + height;
        float targetBottom = targetPos.y;
        float targetTop = targetPos.y + targetHeight;
        if (top < targetBottom || targetTop < bottom)
        {
            return false;
        }

        VECTOR toTarget = VSub(targetPos, center);
        toTarget.y = 0.0f;
        return VSize(toTarget) <= radius + targetRadius;
    }

    bool isInCounterFront(const Player& player, VECTOR sourcePos)
    {
        VECTOR toSource = VSub(sourcePos, player.getPosition());
        toSource.y = 0.0f;
        if (VSize(toSource) <= 0.0001f)
        {
            return true;
        }

        VECTOR front = VGet(sinf(player.getRotationY()), 0.0f, cosf(player.getRotationY()));
        toSource = VNorm(toSource);
        return front.x * toSource.x + front.z * toSource.z >= PlayerParam::GuardFrontDotThreshold;
    }

    bool canCounterEnemyAttack(const Player& player, const Enemy& enemy, VECTOR attackCenter, float attackRadius)
    {
        /// カウンターは有効時間中かつ攻撃種類ごとの実判定に触れている場合だけ成立させる。
        if (player.isDead() || !player.isCounterActive())
        {
            return false;
        }

        VECTOR playerPos = player.getPosition();
        if (enemy.isSpinAttacking())
        {
            return isInsideCircle(attackCenter, playerPos, enemy.getSpinAttackRadius(), player.getHitRadius());
        }

        if (!isInCounterFront(player, enemy.getPosition()))
        {
            return false;
        }
        if (enemy.isNormalAttacking())
        {
            return enemy.isArmAttackHitting(playerPos, player.getCollisionRadius(), player.getCollisionHeight());
        }

        if (enemy.isRamAttacking())
        {
            /// 突進は移動が速いため、敵位置より少し前方のカプセルで受け止め判定を行う。
            VECTOR ramForward = enemy.getRamAttackForward();
            ramForward.y = 0.0f;
            if (VSize(ramForward) <= 0.0001f)
            {
                return false;
            }

            VECTOR bufferedPos = VAdd(enemy.getPosition(), VScale(VNorm(ramForward), RamCounterForwardBuffer));
            return isOverlappingCapsule(bufferedPos, enemy.getCollisionRadius(), enemy.getCollisionHeight(), playerPos, player.getCollisionRadius(), player.getCollisionHeight());
        }

        return isInsideCircle(attackCenter, playerPos, attackRadius, player.getHitRadius());
    }
}
void MutantScene::processEnemyAttack()
{
    VECTOR playerPos = mPlayer->getPosition();
    VECTOR companionPos = mCompanion != nullptr ? mCompanion->getPosition() : playerPos;

    for (auto& enemy : mEnemies)
    {
        VECTOR attackCenter = enemy->getAttackCenter();
        float attackRadius = enemy->getAttackRadius();
        VECTOR attackHitPos = enemy->getAttackHitPosition();
        if (!enemy->consumeAttackHitRequest())
        {
            continue;
        }
        /// 1 回の攻撃要求で複数対象へ当たっても、敵側のヒット完了は最後にまとめて確定する。
        bool ramHitDone = false;
        bool armHitDone = false;


        bool countered = false;
        if (enemy->isCounterTiming() && canCounterEnemyAttack(*mPlayer, *enemy, attackCenter, attackRadius))
        {
            /// プレイヤーのカウンターを先に評価し、成功時は通常ダメージ処理へ進ませない。
            countered = enemy->counterReact(playerPos);
            if (countered)
            {
                mAttackEffect.startCounterHitImpact(mPlayer->getCounterEffectPosition());
                AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightParrySuccess);
                mPlayer->startHitStop(PlayerParam::CounterHitStopDuration);
            }
        }
        else if (enemy->isCounterTiming() && mCompanion != nullptr && canCounterEnemyAttack(*mCompanion, *enemy, attackCenter, attackRadius))
        {
            countered = enemy->counterReact(companionPos);
            if (countered)
            {
                mAttackEffect.startCounterHitImpact(mCompanion->getCounterEffectPosition());
                AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightParrySuccess);
                mCompanion->startHitStop(PlayerParam::CounterHitStopDuration);
            }
        }

        if (countered)
        {
            enemy->markAttackHitDone();
            continue;
        }
        if (!mPlayer->isDead())
        {
            bool hitPlayer = false;
            if (enemy->isJumpAttacking())
            {
                VECTOR toPlayer = VSub(playerPos, attackCenter);
                toPlayer.y = 0.0f;
                hitPlayer = VSize(toPlayer) <= attackRadius + mPlayer->getHitRadius();
            }
            else if (enemy->isSpinAttacking())
            {
                hitPlayer = enemy->isArmAttackHitting(playerPos, mPlayer->getCollisionRadius(), mPlayer->getCollisionHeight());
            }
            else if (enemy->isRamAttacking())
            {
                hitPlayer = isOverlappingCapsule(enemy->getPosition(), enemy->getCollisionRadius(), enemy->getCollisionHeight(), playerPos, mPlayer->getCollisionRadius(), mPlayer->getCollisionHeight());
            }
            else
            {
                hitPlayer = enemy->isArmAttackHitting(playerPos, mPlayer->getCollisionRadius(), mPlayer->getCollisionHeight());
            }

            if (hitPlayer)
            {
                if (enemy->isRamAttacking())
                {
                    mPlayer->damage(enemy->getAttackPower(), attackCenter, enemy->getRamAttackForward(), PlayerParam::RamKnockbackDistance);
                }
                else
                {
                    mPlayer->damage(enemy->getAttackPower(), attackHitPos);
                }
                if (enemy->isRamAttacking())
                {
                    ramHitDone = true;
                }
                else if (!enemy->isJumpAttacking())
                {
                    armHitDone = true;
                }
            }
        }

        if (mCompanion != nullptr && !mCompanion->isDead())
        {
            bool hitCompanion = false;
            if (enemy->isJumpAttacking())
            {
                VECTOR toCompanion = VSub(companionPos, attackCenter);
                toCompanion.y = 0.0f;
                hitCompanion = VSize(toCompanion) <= attackRadius + mCompanion->getHitRadius();
            }
            else if (enemy->isSpinAttacking())
            {
                hitCompanion = enemy->isArmAttackHitting(companionPos, mCompanion->getCollisionRadius(), mCompanion->getCollisionHeight());
            }
            else if (enemy->isRamAttacking())
            {
                hitCompanion = isOverlappingCapsule(enemy->getPosition(), enemy->getCollisionRadius(), enemy->getCollisionHeight(), companionPos, mCompanion->getCollisionRadius(), mCompanion->getCollisionHeight());
            }
            else
            {
                hitCompanion = enemy->isArmAttackHitting(companionPos, mCompanion->getCollisionRadius(), mCompanion->getCollisionHeight());
            }

            if (hitCompanion)
            {
                if (enemy->isRamAttacking())
                {
                    mCompanion->damage(enemy->getAttackPower(), attackCenter, enemy->getRamAttackForward(), PlayerParam::RamKnockbackDistance);
                }
                else
                {
                    mCompanion->damage(enemy->getAttackPower(), attackHitPos);
                }
                if (enemy->isRamAttacking())
                {
                    ramHitDone = true;
                }
                else if (!enemy->isJumpAttacking())
                {
                    armHitDone = true;
                }
            }
        }

        if (ramHitDone || armHitDone)
        {
            enemy->markAttackHitDone();
        }
    }
}
void MutantScene::resolvePlayerEnemyCollision()
{
    auto resolveCollision = [](Player* character, Enemy* enemy)
    {
        if (character == nullptr || character->isDead() || enemy == nullptr)
        {
            return;
        }

        VECTOR characterPos = character->getPosition();
        VECTOR enemyPos = enemy->getPosition();
        VECTOR diff = VSub(enemyPos, characterPos);
        diff.y = 0.0f;

        float distance = VSize(diff);
        float minDistance = character->getCollisionRadius() + enemy->getCollisionRadius();
        if (distance <= 0.0001f || distance >= minDistance)
        {
            return;
        }

        VECTOR pushDir = VNorm(diff);
        bool enemyFixed = enemy->isCharging();
        float overlap = minDistance - distance;
        float characterPushAmount = enemyFixed ? overlap : overlap * 0.5f;
        float enemyPushAmount = enemyFixed ? 0.0f : overlap * 0.5f;

        character->setPosition(
            VAdd(characterPos, VScale(pushDir, -characterPushAmount))
        );

        if (enemyPushAmount > 0.0f)
        {
            enemy->setPosition(
                VAdd(enemyPos, VScale(pushDir, enemyPushAmount))
            );
        }
    };

    for (auto& enemy : mEnemies)
    {
        if (enemy->isJumpAttacking())
        {
            continue;
        }
        /// ジャンプ攻撃中は着地位置制御を優先し、押し合い解決で軌道を崩さない。
        clampEnemyToMutantArena(enemy.get());
        resolveCollision(mPlayer.get(), enemy.get());
        resolveCollision(mCompanion.get(), enemy.get());
    }
}
void MutantScene::resolvePlayerStageGroundCollision()
{
    if (mStageCollisionModelHandle == -1 || !mPlayer->canSnapToGround())
    {
        return;
    }

    clampToMutantArena(mPlayer.get());

    VECTOR playerPos = mPlayer->getPosition();

    VECTOR start = VAdd(playerPos, VGet(0.0f, 80.0f, 0.0f));
    VECTOR end = VAdd(playerPos, VGet(0.0f, -200.0f, 0.0f));

    MV1_COLL_RESULT_POLY hit =
        MV1CollCheck_Line(
            mStageCollisionModelHandle,
            -1,
            start,
            end
        );

    if (hit.HitFlag && canSnapToStageGround(playerPos.y, hit.HitPosition.y))
    {
        playerPos.y = hit.HitPosition.y;
        mPlayer->setPosition(playerPos);
    }
}


void MutantScene::resolveCompanionStageGroundCollision()
{
    if (mCompanion == nullptr || mStageCollisionModelHandle == -1 || !mCompanion->canSnapToGround())
    {
        return;
    }

    clampToMutantArena(mCompanion.get());

    VECTOR companionPos = mCompanion->getPosition();
    VECTOR start = VAdd(companionPos, VGet(0.0f, 80.0f, 0.0f));
    VECTOR end = VAdd(companionPos, VGet(0.0f, -200.0f, 0.0f));

    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(mStageCollisionModelHandle, -1, start, end);
    if (hit.HitFlag && canSnapToStageGround(companionPos.y, hit.HitPosition.y))
    {
        companionPos.y = hit.HitPosition.y;
        mCompanion->setPosition(companionPos);
    }
}
void MutantScene::resolveEnemiesStageGroundCollision()
{
    if (mStageCollisionModelHandle == -1)
    {
        return;
    }

    for (auto& enemy : mEnemies)
    {
        enemy->resolveJumpAttackStageCollision(
            mStageCollisionModelHandle
        );

        if (!enemy->canSnapToGround())
        {
            continue;
        }
        clampEnemyToMutantArena(enemy.get());


        VECTOR enemyPos = enemy->getPosition();

        VECTOR start = VAdd(enemyPos, VGet(0.0f, 80.0f, 0.0f));
        VECTOR end = VAdd(enemyPos, VGet(0.0f, -200.0f, 0.0f));

        MV1_COLL_RESULT_POLY hit =
            MV1CollCheck_Line(
                mStageCollisionModelHandle,
                -1,
                start,
                end
            );

        if (hit.HitFlag && canSnapToStageGround(enemyPos.y, hit.HitPosition.y))
        {
            enemyPos.y = hit.HitPosition.y;
            enemy->setPosition(enemyPos);
        }
    }
}

void MutantScene::processPlayerAttack()
{
    if (!mPlayer->consumeAttackHitRequest())
    {
        return;
    }

    VECTOR playerPos = mPlayer->getPosition();
    VECTOR magicStartPos = mPlayer->getMagicHandPosition();

    if (mPlayer->isWizard() && mPlayer->getAttackStep() == 1 && mIsEnemyLocked)
    {
        /// ロックオン中の魔法弾は演出到達に合わせるため、命中処理を少し遅らせる。
        Enemy* targetEnemy = getNearestEnemy();
        if (targetEnemy != nullptr)
        {
            VECTOR toTarget = VSub(targetEnemy->getLockPointPosition(), magicStartPos);
            float missileRange = VSize(toTarget) + 18.0f;
            VECTOR hitPos = targetEnemy->getMagicBeamHitPosition(magicStartPos, toTarget, missileRange);
            mAttackEffect.startMagicEffect(
                magicStartPos,
                mPlayer->getRotationY(),
                mPlayer->getAttackStep(),
                hitPos
            );

            PendingMagicMissileHit pendingHit;
            pendingHit.target = targetEnemy;
            pendingHit.hitPos = hitPos;
            pendingHit.timer = 0.62f;
            pendingHit.shieldBreaker = mPlayer->isPowerUpActive();
            mPendingMagicMissileHits.push_back(pendingHit);
            return;
        }
    }

    if (mPlayer->isAreaPowerUp())
    {
        /// 範囲強化は自分を必ず対象にし、相棒は半径内にいる場合だけ巻き込む。
        mPlayer->applyAreaPowerUp();
        mAttackEffect.startHealEffect(mPlayer->getPosition());
        if (mCompanion != nullptr && !mCompanion->isDead() &&
            isInsideAreaPowerUp(playerPos, mCompanion->getPosition(), mPlayer->getAreaPowerUpRadius()))
        {
            mCompanion->applyAreaPowerUp();
            mAttackEffect.startHealEffect(mCompanion->getPosition());
        }
        mAttackEffect.startMagicEffect(
            playerPos,
            mPlayer->getRotationY(),
            mPlayer->getAttackStep(),
            mStageCollisionModelHandle
        );
        return;
    }
    VECTOR playerForward =
        VGet(
            sinf(mPlayer->getRotationY()),
            0.0f,
            cosf(mPlayer->getRotationY())
        );

    if (mPlayer->isWizard() && mPlayer->getAttackStep() == 2)
    {
        /// ビームは瞬間判定ではなく、攻撃更新ごとに範囲内の敵へ継続的に当てる。
        mAttackEffect.startMagicEffect(
            magicStartPos,
            mPlayer->getRotationY(),
            mPlayer->getAttackStep()
        );

        for (auto& enemy : mEnemies)
        {
            if (isEnemyInsideMagicBeam(magicStartPos, playerForward, *enemy))
            {
                EnemyDamageResult result = enemy->damage(
                    mPlayer->getAttackPower(),
                    mPlayer->getAttackStep(),
                    playerPos,
                    ThreatSource::Player,
                    mPlayer->isPowerUpActive()
                );
                VECTOR hitPos = enemy->getMagicBeamHitPosition(magicStartPos, playerForward, WizardMagicBeamRange);
                playHitImpactForResult(mAttackEffect, result, hitPos, mPlayer->getAttackStep(), true);
            }
        }
        return;
    }
    float attackRadius = mPlayer->getAttackRadius();

    if (mPlayer->isWizard())
    {
        mAttackEffect.startMagicEffect(
            magicStartPos,
            mPlayer->getRotationY(),
            mPlayer->getAttackStep()
        );
    }


    for (auto& enemy : mEnemies)
    {
        VECTOR hitPos;
        if (getNearestEnemyFrontAttackHitPoint(playerPos, playerForward, attackRadius, *enemy, &hitPos))
        {
            EnemyDamageResult result = enemy->damage(
                mPlayer->getAttackPower(),
                mPlayer->getAttackStep(),
                playerPos,
                ThreatSource::Player,
                mPlayer->isPowerUpActive()
            );
            playHitImpactForResult(mAttackEffect, result, hitPos, mPlayer->getAttackStep(), mPlayer->isWizard());
            if (!mPlayer->isWizard() && result == EnemyDamageResult::Shield)
            {
                AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightSlashHitShield);
            }
            if (!mPlayer->isWizard() && result == EnemyDamageResult::Hp)
            {
                AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightSlashHitFlesh);
                mPlayer->startHitStop(PlayerParam::HitStopDuration);
            }
        }
    }
}
void MutantScene::processCompanionAttack()
{
    if (mCompanion == nullptr || !mCompanion->consumeAttackHitRequest())
    {
        return;
    }

    Enemy* targetEnemy = getNearestEnemy();
    if (targetEnemy == nullptr)
    {
        return;
    }

    VECTOR companionPos = mCompanion->getPosition();
    VECTOR magicStartPos = mCompanion->getMagicHandPosition();
    VECTOR toEnemy = VSub(targetEnemy->getPosition(), companionPos);
    toEnemy.y = 0.0f;
    float distance = VSize(toEnemy);
    if (distance <= 0.0001f)
    {
        return;
    }

    VECTOR companionForward = VGet(sinf(mCompanion->getRotationY()), 0.0f, cosf(mCompanion->getRotationY()));

    if (mCompanion->isWizard())
    {
        /// 相棒 Wizard はプレイヤーと同じ攻撃種別を使うが、基本的に最寄りの敵へ集中する。
        if (mCompanion->getAttackStep() == 3)
        {
            mCompanion->applyAreaPowerUp();
            mAttackEffect.startHealEffect(mCompanion->getPosition());
            if (!mPlayer->isDead() &&
                isInsideAreaPowerUp(companionPos, mPlayer->getPosition(), mCompanion->getAreaPowerUpRadius()))
            {
                mPlayer->applyAreaPowerUp();
                mAttackEffect.startHealEffect(mPlayer->getPosition());
            }
            mAttackEffect.startMagicEffect(
                companionPos,
                mCompanion->getRotationY(),
                mCompanion->getAttackStep(),
                mStageCollisionModelHandle
            );
            return;
        }
        if (mCompanion->getAttackStep() == 2)
        {
            mAttackEffect.startMagicEffect(
                magicStartPos,
                mCompanion->getRotationY(),
                mCompanion->getAttackStep()
            );

            if (isEnemyInsideMagicBeam(magicStartPos, companionForward, *targetEnemy))
            {
                EnemyDamageResult result = targetEnemy->damage(
                    mCompanion->getAttackPower(),
                    mCompanion->getAttackStep(),
                    companionPos,
                    ThreatSource::Companion,
                    mCompanion->isPowerUpActive()
                );
                VECTOR hitPos = targetEnemy->getMagicBeamHitPosition(magicStartPos, companionForward, WizardMagicBeamRange);
                playHitImpactForResult(mAttackEffect, result, hitPos, mCompanion->getAttackStep(), true);
            }
            return;
        }

        if (distance <= 90.0f)
        {
            VECTOR toTarget = VSub(targetEnemy->getLockPointPosition(), magicStartPos);
            float missileRange = VSize(toTarget) + 18.0f;
            VECTOR hitPos = targetEnemy->getMagicBeamHitPosition(magicStartPos, toTarget, missileRange);
            mAttackEffect.startMagicEffect(
                magicStartPos,
                mCompanion->getRotationY(),
                mCompanion->getAttackStep(),
                hitPos
            );
            EnemyDamageResult result = targetEnemy->damage(
                mCompanion->getAttackPower(),
                mCompanion->getAttackStep(),
                companionPos,
                ThreatSource::Companion,
                mCompanion->isPowerUpActive()
            );
            playHitImpactForResult(mAttackEffect, result, hitPos, mCompanion->getAttackStep(), true);
        }
        return;
    }
    if (isEnemyInsideFrontAttack(companionPos, companionForward, mCompanion->getAttackRadius(), *targetEnemy))
    {
        EnemyDamageResult result = targetEnemy->damage(
            mCompanion->getAttackPower(),
            mCompanion->getAttackStep(),
            companionPos,
            ThreatSource::Companion,
            mCompanion->isPowerUpActive()
        );
        playHitImpactForResult(mAttackEffect, result, targetEnemy->getHeadPosition(), mCompanion->getAttackStep(), false);
        if (result == EnemyDamageResult::Shield)
        {
            AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightSlashHitShield);
        }
        if (result == EnemyDamageResult::Hp)
        {
            AudioManager::instance()->playSe(SoundResource::Se::Chara::KnightSlashHitFlesh);
            mCompanion->startHitStop(PlayerParam::HitStopDuration);
        }
    }
}
