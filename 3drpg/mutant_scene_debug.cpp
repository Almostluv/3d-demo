/**
 * @file
 * @brief mutant_scene_debug.cpp 実装を定義する。
 */

#include "mutant_scene.h"
#include "debug_draw.h"
#include "DxLib.h"

#include <cmath>

void MutantScene::drawDebugHud()
{
    /// 戦闘中の主要状態を画面左上へ表示し、AI と配置確認に使う。
    DrawFormatString(20, 100, GetColor(255, 255, 255), "Enemies: %d", static_cast<int>(mEnemies.size()));
    DrawFormatString(20, 120, GetColor(255, 255, 255), "Player Pos: %.2f %.2f %.2f",
        mPlayer->getPosition().x,
        mPlayer->getPosition().y,
        mPlayer->getPosition().z);

    if (mCompanion != nullptr)
    {
        DrawFormatString(20, 140, GetColor(255, 255, 255), "Companion Pos: %.2f %.2f %.2f",
            mCompanion->getPosition().x,
            mCompanion->getPosition().y,
            mCompanion->getPosition().z);
        DrawFormatString(20, 160, GetColor(255, 255, 255), "Companion AI: %s", mCompanionDebugState);
    }

    DrawFormatString(20, 180, GetColor(255, 255, 255), "StageHandle: %d",
        mStageModelHandle);
    DrawFormatString(20, 200, GetColor(255, 255, 255), "OverheadLight: %d",
        mOverheadLightHandle);
    DrawFormatString(20, 220, GetColor(255, 255, 255), "StageMaterials: %d  NoDiffuseTex: %d",
        mStageMaterialNum, mStageNoDiffuseTextureNum);

    for (int i = 0; i < static_cast<int>(mEnemies.size()); ++i)
    {
        /// 敵ごとの位置、脅威値、現在ターゲットを並べて AI 遷移を確認する。
        VECTOR enemyPos = mEnemies[i]->getPosition();
        DrawFormatString(20, 300 + (i * 40), GetColor(255, 255, 255), "Enemy[%d] Pos: %.2f %.2f %.2f",
            i,
            enemyPos.x,
            enemyPos.y,
            enemyPos.z);
        DrawFormatString(20, 320 + (i * 40), GetColor(255, 255, 255), "Threat P: %.1f AI: %.1f Target: %d",
            mEnemies[i]->getThreat(ThreatSource::Player),
            mEnemies[i]->getThreat(ThreatSource::Companion),
            static_cast<int>(mEnemies[i]->getCurrentTargetSource()));
    }

    mPlayer->drawDebugHud();
}

void Player::drawDebugHud() const
{
    /// プレイヤーのアニメーションと攻撃コンボ状態を確認するためのデバッグ表示。
    DrawFormatString(20, 380, GetColor(255, 255, 255),
        "attach=%d anim=%d source=%d time=%.2f total=%.2f",
        mAnimator.getAttachAnimIndex(),
        mAnimator.getCurrentAnimIndex(),
        mAnimator.getCurrentAnimSourceModelHandle(),
        mAnimator.getAnimTime(),
        mAnimator.getAnimTotalTime());

    DrawFormatString(20, 400, GetColor(255, 255, 255),
        "attackStep=%d queued=%d hitDone=%d sword=%d",
        mAttackStep,
        mNextAttackQueued ? 1 : 0,
        mAttackHitDone ? 1 : 0,
        mSwordFrameIndex);

    VECTOR swordJoint = getSwordJointPosition();
    VECTOR swordTip = getSwordTipPosition();
    /// 剣の関節位置と先端位置を表示し、攻撃判定のずれを調べる。
    DrawFormatString(20, 420, GetColor(255, 255, 255),
        "swordJoint=%.2f %.2f %.2f",
        swordJoint.x,
        swordJoint.y,
        swordJoint.z);
    DrawFormatString(20, 440, GetColor(255, 255, 255),
        "swordTip=%.2f %.2f %.2f",
        swordTip.x,
        swordTip.y,
        swordTip.z);
}

void MutantScene::drawDebugHitCollision()
{
    if (!mShowDebugHitCollision)
    {
        return;
    }

    /// 色を固定して、プレイヤー・敵・攻撃範囲の種類を画面上で区別する。
    unsigned int playerCollisionColor = GetColor(80, 160, 255);
    unsigned int enemyCollisionColor = GetColor(40, 120, 255);
    unsigned int playerAttackColor = GetColor(80, 255, 255);
    unsigned int enemyAttackColor = GetColor(255, 80, 80);
    unsigned int jumpAttackColor = GetColor(255, 230, 80);


    if (mPlayer != nullptr)
    {
        /// プレイヤーの移動カプセル、被弾円、近接攻撃範囲を描画する。
        VECTOR playerPos = mPlayer->getPosition();
        drawDebugCapsuleY(playerPos, mPlayer->getCollisionRadius(), mPlayer->getCollisionHeight(), playerCollisionColor);
        drawDebugCircleXZ(playerPos, mPlayer->getHitRadius(), enemyAttackColor, 0.35f);

        if (mPlayer->isAttacking() && !mPlayer->isWizard())
        {
            VECTOR forward = VGet(sinf(mPlayer->getRotationY()), 0.0f, cosf(mPlayer->getRotationY()));
            drawDebugFrontHalfCircleXZ(playerPos, forward, mPlayer->getAttackRadius(), playerAttackColor, 0.28f);
        }
    }

    if (mCompanion != nullptr)
    {
        /// 相棒もプレイヤーと同じ形式で当たり判定を可視化する。
        VECTOR companionPos = mCompanion->getPosition();
        drawDebugCapsuleY(companionPos, mCompanion->getCollisionRadius(), mCompanion->getCollisionHeight(), playerCollisionColor);
        drawDebugCircleXZ(companionPos, mCompanion->getHitRadius(), enemyAttackColor, 0.35f);

        if (mCompanion->isAttacking() && !mCompanion->isWizard())
        {
            VECTOR forward = VGet(sinf(mCompanion->getRotationY()), 0.0f, cosf(mCompanion->getRotationY()));
            drawDebugFrontHalfCircleXZ(companionPos, forward, mCompanion->getAttackRadius(), playerAttackColor, 0.28f);
        }
    }

    for (auto& enemy : mEnemies)
    {
        /// 敵の状態に応じて、ジャンプ・突進・通常攻撃の判定を描き分ける。
        VECTOR enemyPos = enemy->getPosition();
        drawDebugCapsuleY(enemyPos, enemy->getCollisionRadius(), enemy->getCollisionHeight(), enemyCollisionColor);


        if (enemy->isJumpAttacking())
        {
            drawDebugCircleXZ(enemy->getAttackCenter(), enemy->getAttackRadius(), jumpAttackColor, 0.18f);
        }
        else if (enemy->isRamAttacking())
        {
            drawDebugForwardRect(enemyPos, enemy->getForward(), enemy->getAttackRadius(), enemy->getAttackWidth(), enemyAttackColor, 0.26f);
        }
        else
        {
            enemy->drawAttackArmHitBoxes();
        }
        enemy->drawLeftHandTrailDebugPoints();
    }

    DrawFormatString(20, 480, GetColor(80, 160, 255), "Debug: Blue collision capsules");
    DrawFormatString(20, 500, GetColor(255, 80, 80), "Debug: Red enemy normal attack / player hurt circle");
    DrawFormatString(20, 520, GetColor(80, 255, 255), "Debug: Cyan player melee attack");
}
