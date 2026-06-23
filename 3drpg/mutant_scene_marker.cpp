/**
 * @file
 * @brief mutant_scene_marker.cpp 実装を定義する。
 */

#include "mutant_scene.h"
#include "audio_manager.h"
#include "vampire_scene.h"
#include "scene_manager.h"
#include "game_config.h"
#include "game_settings.h"
#include "model_resource.h"
#include "player_param.h"
#include "story_state.h"
#include "resource_manager.h"
#include "sound_resource.h"
#include "DxLib.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
float updateDelayedHpValue(float delayedHp, float currentHp, float deltaTime)
{
    constexpr float DelayDrainSpeed = 8.0f; ///< UI 表示用 HP が実 HP へ追いつく速度。単位は HP/second。
    /// UI 用 HP は実 HP より遅れて減らし、被弾した量を視覚的に残す。
    if (delayedHp <= currentHp)
    {
        return currentHp;
    }

    delayedHp -= DelayDrainSpeed * deltaTime;
    return delayedHp < currentHp ? currentHp : delayedHp;
}

const VECTOR MagicBossMarkerPos = { 0.0f, 0.12f, -32.0f };
const VECTOR FinalBossMarkerPos = { 40.0f, 0.12f, -32.0f };
constexpr float BossMarkerRadius = 12.0f; ///< ボス戦終了後マーカーの判定半径。
constexpr float TeleportCircleSize = 13.0f; ///< テレポート演出円の描画サイズ。
constexpr float TeleportEffectDuration = 1.0f; ///< テレポート演出の表示時間。単位は seconds。

void drawPlayKeyHintBox(int x, int y, const char* key, int color)
{
    int keyWidth = GetDrawStringWidth(key, static_cast<int>(strlen(key))) + 22;
    if (keyWidth < 42) keyWidth = 42;
    DrawBox(x, y, x + keyWidth, y + 30, GetColor(18, 24, 32), TRUE);
    DrawBox(x, y, x + keyWidth, y + 30, color, FALSE);
    DrawString(x + 11, y + 6, key, color);
}

int drawPlayPadHint(int handle, int x, int y, int color)
{
    if (handle != -1)
    {
        DrawExtendGraph(x, y, x + 30, y + 30, handle, TRUE);
    }
    return x + 38;
}

VERTEX3D makeGroundVertex(VECTOR pos, COLOR_U8 color, float u, float v)
{
    /// 地面に貼り付ける 2D マーカー用の頂点を作成する。
    VERTEX3D vertex;
    vertex.pos = pos;
    vertex.norm = VGet(0.0f, 1.0f, 0.0f);
    vertex.dif = color;
    vertex.spc = GetColorU8(0, 0, 0, 0);
    vertex.u = u;
    vertex.v = v;
    vertex.su = 0.0f;
    vertex.sv = 0.0f;
    return vertex;
}

void drawTeleportCircleImage(VECTOR center, float halfSize, int graphHandle, COLOR_U8 color)
{
    if (graphHandle == -1)
    {
        return;
    }

    /// 1 枚の画像を地面上の四角形として 2 三角形で描く。
    VERTEX3D vertices[6];
    vertices[0] = makeGroundVertex(VAdd(center, VGet(-halfSize, 0.0f, -halfSize)), color, 0.0f, 0.0f);
    vertices[1] = makeGroundVertex(VAdd(center, VGet( halfSize, 0.0f, -halfSize)), color, 1.0f, 0.0f);
    vertices[2] = makeGroundVertex(VAdd(center, VGet( halfSize, 0.0f,  halfSize)), color, 1.0f, 1.0f);
    vertices[3] = vertices[0];
    vertices[4] = vertices[2];
    vertices[5] = makeGroundVertex(VAdd(center, VGet(-halfSize, 0.0f,  halfSize)), color, 0.0f, 1.0f);

    DrawPolygon3D(vertices, 2, graphHandle, TRUE);
}

void drawSpiralRiseEffect(VECTOR basePos, float elapsed, float duration, int streakColor, int streakCoreColor)
{
    /// 転送中のキャラクター周囲に、下から上へ伸びる螺旋ラインを描く。
    float rate = elapsed / duration;
    if (rate < 0.0f) rate = 0.0f;
    if (rate > 1.0f) rate = 1.0f;

    VECTOR spiralCenter = VAdd(basePos, VGet(0.0f, 13.0f, 0.0f));
    SetUseLighting(FALSE);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(FALSE);
    for (int i = 0; i < 6; ++i)
    {
        float startAngle = DX_TWO_PI_F * static_cast<float>(i) / 6.0f;
        float headRate = rate;
        float tailRate = 0.0f;

        float radius = 10.5f;
        int alpha = static_cast<int>(240.0f * (1.0f - rate * 0.18f));
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);

        const int segmentCount = 10;
        for (int segment = 0; segment < segmentCount; ++segment)
        {
            float t0 = static_cast<float>(segment) / segmentCount;
            float t1 = static_cast<float>(segment + 1) / segmentCount;
            float rate0 = tailRate + (headRate - tailRate) * t0;
            float rate1 = tailRate + (headRate - tailRate) * t1;
            float angle0 = startAngle + elapsed * 4.0f + DX_TWO_PI_F * rate0 * 1.45f;
            float angle1 = startAngle + elapsed * 4.0f + DX_TWO_PI_F * rate1 * 1.45f;
            float y0 = -13.0f + rate0 * 34.0f;
            float y1 = -13.0f + rate1 * 34.0f;

            VECTOR p0 = VAdd(spiralCenter, VGet(cosf(angle0) * radius, y0, sinf(angle0) * radius));
            VECTOR p1 = VAdd(spiralCenter, VGet(cosf(angle1) * radius, y1, sinf(angle1) * radius));
            VECTOR side0 = VGet(cosf(angle0 + DX_PI_F * 0.5f) * 3.8f, 0.0f, sinf(angle0 + DX_PI_F * 0.5f) * 3.8f);
            VECTOR side1 = VGet(cosf(angle1 + DX_PI_F * 0.5f) * 3.8f, 0.0f, sinf(angle1 + DX_PI_F * 0.5f) * 3.8f);
            VECTOR thick = VGet(0.0f, 3.8f, 0.0f);

            DrawLine3D(p0, p1, streakCoreColor);
            DrawLine3D(VAdd(p0, side0), VAdd(p1, side1), streakColor);
            DrawLine3D(VSub(p0, side0), VSub(p1, side1), streakColor);
            DrawLine3D(VAdd(p0, thick), VAdd(p1, thick), streakColor);
            DrawLine3D(VSub(p0, thick), VSub(p1, thick), streakColor);
            DrawLine3D(VAdd(VAdd(p0, side0), thick), VAdd(VAdd(p1, side1), thick), streakColor);
            DrawLine3D(VSub(VSub(p0, side0), thick), VSub(VSub(p1, side1), thick), streakColor);
        }
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetWriteZBuffer3D(TRUE);
    SetUseLighting(TRUE);
}

bool isBindingPush(InputManager* inputMgr, InputBinding binding)
{
    /// 設定済みの入力種別に応じて、キーボード/マウス/パッドの Push 判定へ振り分ける。
    if (binding.type == InputBindingMouse) return inputMgr->isMousePush(binding.code);
    if (binding.type == InputBindingPad)
    {
        if (binding.code == PadBindingLeftTrigger) return inputMgr->isPadLeftTriggerPush();
        if (binding.code == PadBindingRightTrigger) return inputMgr->isPadRightTriggerPush();
        return inputMgr->isPadPush(binding.code);
    }
    return inputMgr->isPush(binding.code);
}
}

void MutantScene::updateDelayedPlayerHp(float deltaTime)
{
    /// プレイヤーと相棒の HP 表示用遅延値を毎フレーム更新する。
    mPlayerDelayedHp = updateDelayedHpValue(mPlayerDelayedHp, static_cast<float>(mPlayer->getHp()), deltaTime);
    if (mCompanion != nullptr)
    {
        mCompanionDelayedHp = updateDelayedHpValue(mCompanionDelayedHp, static_cast<float>(mCompanion->getHp()), deltaTime);
    }
}
void MutantScene::updateKnightSwordTrails()
{
    if (mPlayer != nullptr && !mPlayer->isWizard() && mPlayer->isAttacking())
    {
        /// Knight 攻撃中だけ剣の根元と先端をサンプリングし、軌跡エフェクトへ渡す。
        mAttackEffect.addSwordTrailSample(
            mPlayer->getSwordJointPosition(),
            mPlayer->getSwordTipPosition(),
            mPlayer->getAttackStep()
        );
    }

    if (mCompanion != nullptr && !mCompanion->isWizard() && mCompanion->isAttacking())
    {
        mAttackEffect.addSwordTrailSample(
            mCompanion->getSwordJointPosition(),
            mCompanion->getSwordTipPosition(),
            mCompanion->getAttackStep()
        );
    }
}
Enemy* MutantScene::getNearestEnemy() const
{
    /// ロックオンや相棒 AI の基準として、プレイヤーに最も近い Enemy を探す。
    Enemy* nearestEnemy = nullptr;
    float nearestDistance = 1000000000.0f;
    VECTOR playerPos = mPlayer->getPosition();

    for (const auto& enemy : mEnemies)
    {
        VECTOR diff = VSub(enemy->getPosition(), playerPos);
        diff.y = 0.0f;
        float distance = VSize(diff);

        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestEnemy = enemy.get();
        }
    }

    return nearestEnemy;
}

void MutantScene::updatePendingMagicMissileHits(float deltaTime)
{
    /// 魔法弾エフェクトの到達タイミングに合わせ、ダメージ適用を遅延させる。
    for (auto& hit : mPendingMagicMissileHits)
    {
        hit.timer -= deltaTime;
    }

    for (auto& hit : mPendingMagicMissileHits)
    {
        if (hit.timer <= 0.0f && hit.target != nullptr)
        {
            VECTOR hitPos = hit.hitPos;
            EnemyDamageResult result = hit.target->damage(mPlayer->getAttackPower(), 1, mPlayer->getPosition(), ThreatSource::Player, hit.shieldBreaker);
            if (result == EnemyDamageResult::Shield || result == EnemyDamageResult::Hp)
            {
                mAttackEffect.startMagicHitImpact(hitPos, 1);
            }
            hit.target = nullptr;
        }
    }

    mPendingMagicMissileHits.erase(
        std::remove_if(
            mPendingMagicMissileHits.begin(),
            mPendingMagicMissileHits.end(),
            [](const PendingMagicMissileHit& hit)
            {
                /// 適用済みまたは無効化された予約をリストから取り除く。
                return hit.target == nullptr;
            }),
        mPendingMagicMissileHits.end());
}


void MutantScene::updateTeleportEffect(float deltaTime)
{
    mTeleportEffectTimer -= deltaTime;
    if (mTeleportEffectTimer > 0.0f)
    {
        /// 転送演出中は入力を空にし、キャラクターとカメラだけを更新する。
        PlayerInput input;
        mPlayer->setInput(input);
        mPlayer->update(deltaTime);
        mPlayer->syncModelTransform();
        if (mCompanion != nullptr)
        {
            mCompanion->setInput(input);
            mCompanion->update(deltaTime);
            mCompanion->syncModelTransform();
        }
        mCamera->update(mPlayer->getPosition(), deltaTime);
        AudioManager::instance()->updateListener(mCamera->getEyePosition(), mCamera->getTargetPosition());
        return;
    }

    mTeleportEffectTimer = 0.0f;
    if (mTeleportReturnToBossSelect)
    {
        /// FinalBoss arena からボス選択地点へ戻る転送。
        mTeleportReturnToBossSelect = false;
        StoryState::instance()->setPhase(StoryPhase::BossSelect);
        SceneManager::instance()->changeScene<MutantScene>(mPlayerType);
    }
    else if (mTeleportToFinalBoss)
    {
        /// FinalBoss へ向かう前に arena リソースを読み込み、次シーンの初期停止を減らす。
        if (!mTeleportResourcesLoaded)
        {
            ResourceManager::instance()->getModel(ModelResource::Stage::MutantArena);
            ResourceManager::instance()->getModel(ModelResource::Stage::MutantArenaCollision);
            mTeleportResourcesLoaded = true;
        }
        StoryState::instance()->setPhase(StoryPhase::FinalBoss);
        SceneManager::instance()->changeScene<MutantScene>(mPlayerType);
    }
    else
    {
        SceneManager::instance()->changeScene<VampireScene>(mPlayerType);
    }
}

void MutantScene::drawTeleportEffect() const
{
    if (mTeleportEffectTimer <= 0.0f)
    {
        return;
    }

    int streakColor = mTeleportToFinalBoss ? GetColor(150, 12, 18) : GetColor(12, 120, 48);
    int streakCoreColor = mTeleportToFinalBoss ? GetColor(255, 42, 48) : GetColor(60, 235, 105);
    float elapsed = TeleportEffectDuration - mTeleportEffectTimer;

    drawSpiralRiseEffect(mPlayer->getPosition(), elapsed, TeleportEffectDuration, streakColor, streakCoreColor);
    if (mCompanion != nullptr)
    {
        drawSpiralRiseEffect(mCompanion->getPosition(), elapsed, TeleportEffectDuration, streakColor, streakCoreColor);
    }
}

bool MutantScene::canUseVampireMarker() const
{
    if (!StoryState::instance()->isBossSelect() || StoryState::instance()->isMagicBossCleared())
    {
        return false;
    }

    VECTOR diff = VSub(mPlayer->getPosition(), MagicBossMarkerPos);
    diff.y = 0.0f;
    return VSize(diff) <= BossMarkerRadius;
}

bool MutantScene::canUseFinalBossMarker() const
{
    if (!StoryState::instance()->isBossSelect() || StoryState::instance()->isFinalBossCleared())
    {
        return false;
    }

    VECTOR diff = VSub(mPlayer->getPosition(), FinalBossMarkerPos);
    diff.y = 0.0f;
    return VSize(diff) <= BossMarkerRadius;
}

bool MutantScene::canUseReturnMarker() const
{
    if (!StoryState::instance()->isFinalBoss() || !StoryState::instance()->isFinalBossCleared() ||
        StoryState::instance()->isMagicBossCleared())
    {
        return false;
    }

    VECTOR diff = VSub(mPlayer->getPosition(), mReturnMarkerPos);
    diff.y = 0.0f;
    return VSize(diff) <= BossMarkerRadius;
}

void MutantScene::updateVampireMarker()
{
    auto inputMgr = InputManager::instance();
    if (!inputMgr->isPush(KEY_INPUT_RETURN) && !inputMgr->isPush(KEY_INPUT_E) &&
        !inputMgr->isPadPush(XINPUT_BUTTON_A) && !isBindingPush(inputMgr, GameSettings::instance()->getPadBinding(GameActionSpecial)))
    {
        return;
    }

    if (canUseReturnMarker())
    {
        AudioManager::instance()->playSe(SoundResource::Se::Teleport);
        mTeleportToFinalBoss = true;
        mTeleportReturnToBossSelect = true;
        mTeleportEffectTimer = TeleportEffectDuration;
        return;
    }

    if (canUseVampireMarker())
    {
        AudioManager::instance()->playSe(SoundResource::Se::Teleport);
        mTeleportToFinalBoss = false;
        mTeleportReturnToBossSelect = false;
        mTeleportEffectTimer = TeleportEffectDuration;
        return;
    }

    if (canUseFinalBossMarker())
    {
        AudioManager::instance()->playSe(SoundResource::Se::Teleport);
        mTeleportToFinalBoss = true;
        mTeleportReturnToBossSelect = false;
        mTeleportEffectTimer = TeleportEffectDuration;
    }
}

void MutantScene::drawVampireMarker() const
{
    bool showBossSelectMarkers = StoryState::instance()->isBossSelect();
    bool showReturnMarker = StoryState::instance()->isFinalBoss() && StoryState::instance()->isFinalBossCleared() &&
        !StoryState::instance()->isMagicBossCleared();
    if (!showBossSelectMarkers && !showReturnMarker)
    {
        return;
    }

    bool restoreShadowMap = mShadowMapHandle != -1;
    if (restoreShadowMap)
    {
        SetUseShadowMap(0, -1);
    }

    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 220);

    if (showBossSelectMarkers)
    {
        drawTeleportCircleImage(
            MagicBossMarkerPos,
            TeleportCircleSize,
            mMagicBossTeleportCircleGraphHandle,
            StoryState::instance()->isMagicBossCleared() ? GetColorU8(70, 90, 70, 100) : GetColorU8(255, 255, 255, 220));
        drawTeleportCircleImage(
            FinalBossMarkerPos,
            TeleportCircleSize,
            mFinalBossTeleportCircleGraphHandle,
            StoryState::instance()->isFinalBossCleared() ? GetColorU8(90, 70, 70, 100) : GetColorU8(255, 255, 255, 220));
    }
    else
    {
        drawTeleportCircleImage(
            mReturnMarkerPos,
            TeleportCircleSize,
            mFinalBossTeleportCircleGraphHandle,
            GetColorU8(255, 255, 255, 220));
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);

    const char* promptText = nullptr;
    int promptColor = GetColor(255, 255, 255);
    if (canUseVampireMarker())
    {
        promptText = "vampireに挑む";
        promptColor = GetColor(180, 220, 255);
    }
    else if (canUseFinalBossMarker())
    {
        promptText = "mutantに挑む";
        promptColor = GetColor(255, 190, 170);
    }
    else if (canUseReturnMarker())
    {
        promptText = "エリア選択へ戻る";
        promptColor = GetColor(255, 190, 170);
    }
    else if (showBossSelectMarkers && StoryState::instance()->isMagicBossCleared())
    {
        VECTOR diff = VSub(mPlayer->getPosition(), MagicBossMarkerPos);
        diff.y = 0.0f;
        if (VSize(diff) <= BossMarkerRadius)
        {
            promptText = "vampireは倒された";
            promptColor = GetColor(150, 170, 150);
        }
    }
    if (promptText == nullptr && showBossSelectMarkers && StoryState::instance()->isFinalBossCleared())
    {
        VECTOR diff = VSub(mPlayer->getPosition(), FinalBossMarkerPos);
        diff.y = 0.0f;
        if (VSize(diff) <= BossMarkerRadius)
        {
            promptText = "mutantは倒された";
            promptColor = GetColor(170, 150, 150);
        }
    }

    if (promptText != nullptr)
    {
        if (InputManager::instance()->isLastInputPad())
        {
            int textX = drawPlayPadHint(mAGraphHandle, 420, 604, promptColor);
            DrawString(textX, 610, promptText, promptColor);
        }
        else
        {
            drawPlayKeyHintBox(420, 604, "E", promptColor);
            drawPlayKeyHintBox(474, 604, "Enter", promptColor);
            DrawString(566, 610, promptText, promptColor);
        }
    }

    if (restoreShadowMap)
    {
        SetUseShadowMap(0, mShadowMapHandle);
    }
}
