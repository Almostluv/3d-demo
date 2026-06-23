/**
 * @file
 * @brief vampire_scene_marker.cpp 実装を定義する。
 */

#include "vampire_scene.h"
#include "audio_manager.h"
#include "mutant_scene.h"
#include "result_scene.h"
#include "scene_manager.h"
#include "story_state.h"
#include "game_config.h"
#include "game_settings.h"
#include "input_manager.h"
#include "resource_manager.h"
#include "player_param.h"
#include "sound_resource.h"
#include "DxLib.h"

#include <cmath>

namespace
{
constexpr float ExitMarkerRadius = 12.0f; ///< 退出マーカーの判定半径。
constexpr float ExitTeleportCircleSize = 13.0f; ///< 退出テレポート演出円の描画サイズ。
constexpr float TeleportEffectDuration = 1.0f; ///< テレポート演出の表示時間。単位は seconds。
constexpr float RoomMinX = -96.0f; ///< VampireScene 部屋内移動制限の最小 X。
constexpr float RoomMaxX = 96.0f; ///< VampireScene 部屋内移動制限の最大 X。
constexpr float RoomMinZ = -120.0f; ///< VampireScene 部屋内移動制限の最小 Z。
constexpr float RoomMaxZ = 72.0f; ///< VampireScene 部屋内移動制限の最大 Z。
constexpr const char* BuffArrowGraph = "Resource/GameUI/buff_arrow.png";
constexpr int BuffArrowSize = 30;

bool isBindingPush(InputManager* inputMgr, InputBinding binding)
{
    /// 設定済みのアクション入力を、キーボード・マウス・ゲームパッドの実 API へ振り分ける。
    if (binding.type == InputBindingMouse) return inputMgr->isMousePush(binding.code);
    if (binding.type == InputBindingPad)
    {
        if (binding.code == PadBindingLeftTrigger) return inputMgr->isPadLeftTriggerPush();
        if (binding.code == PadBindingRightTrigger) return inputMgr->isPadRightTriggerPush();
        return inputMgr->isPadPush(binding.code);
    }
    return inputMgr->isPush(binding.code);
}

float clampRate(float value, float maxValue)
{
    if (maxValue <= 0.0f)
    {
        return 0.0f;
    }

    float rate = value / maxValue;
    /// HP やスタミナのバー幅に使うため、負値や上限超過を描画前に丸める。
    if (rate < 0.0f) rate = 0.0f;
    if (rate > 1.0f) rate = 1.0f;
    return rate;
}

void drawBuffArrow(int graphHandle, int frameX, int frameY, int frameWidth, int frameHeight)
{
    if (graphHandle == -1)
    {
        return;
    }

    /// バフアイコンはフレーム外側へ置き、HP バーの読み取りを邪魔しない。
    int x = frameX + frameWidth + 8;
    int y = frameY + (frameHeight - BuffArrowSize) / 2;
    DrawExtendGraph(x, y, x + BuffArrowSize, y + BuffArrowSize, graphHandle, TRUE);
}

VERTEX3D makeGroundVertex(VECTOR pos, COLOR_U8 color, float u, float v)
{
    /// テレポート円を床面へ貼るための、上向き法線を持つ頂点を作る。
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

    VERTEX3D vertices[6];
    /// DxLib の 3D ポリゴン描画に合わせ、1 枚の四角形を 2 三角形で構成する。
    vertices[0] = makeGroundVertex(VAdd(center, VGet(-halfSize, 0.0f, -halfSize)), color, 0.0f, 0.0f);
    vertices[1] = makeGroundVertex(VAdd(center, VGet( halfSize, 0.0f, -halfSize)), color, 1.0f, 0.0f);
    vertices[2] = makeGroundVertex(VAdd(center, VGet( halfSize, 0.0f,  halfSize)), color, 1.0f, 1.0f);
    vertices[3] = vertices[0];
    vertices[4] = vertices[2];
    vertices[5] = makeGroundVertex(VAdd(center, VGet(-halfSize, 0.0f,  halfSize)), color, 0.0f, 1.0f);

    DrawPolygon3D(vertices, 2, graphHandle, TRUE);
}

void drawSpiralRiseEffect(VECTOR basePos, float elapsed, float duration)
{
    float rate = elapsed / duration;
    if (rate < 0.0f) rate = 0.0f;
    if (rate > 1.0f) rate = 1.0f;

    VECTOR spiralCenter = VAdd(basePos, VGet(0.0f, 13.0f, 0.0f));
    /// ライン描画を加算合成にし、テレポート時の上昇する光を表現する。
    SetUseLighting(FALSE);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(FALSE);
    for (int i = 0; i < 6; ++i)
    {
        float startAngle = DX_TWO_PI_F * static_cast<float>(i) / 6.0f;
        int alpha = static_cast<int>(240.0f * (1.0f - rate * 0.18f));
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);

        const int segmentCount = 10;
        for (int segment = 0; segment < segmentCount; ++segment)
        {
            float t0 = static_cast<float>(segment) / segmentCount;
            float t1 = static_cast<float>(segment + 1) / segmentCount;
            float angle0 = startAngle + elapsed * 4.0f + DX_TWO_PI_F * t0 * 1.45f;
            float angle1 = startAngle + elapsed * 4.0f + DX_TWO_PI_F * t1 * 1.45f;
            float y0 = -13.0f + t0 * rate * 34.0f;
            float y1 = -13.0f + t1 * rate * 34.0f;
            float radius = 10.5f;

            VECTOR p0 = VAdd(spiralCenter, VGet(cosf(angle0) * radius, y0, sinf(angle0) * radius));
            VECTOR p1 = VAdd(spiralCenter, VGet(cosf(angle1) * radius, y1, sinf(angle1) * radius));
            VECTOR side0 = VGet(cosf(angle0 + DX_PI_F * 0.5f) * 3.8f, 0.0f, sinf(angle0 + DX_PI_F * 0.5f) * 3.8f);
            VECTOR side1 = VGet(cosf(angle1 + DX_PI_F * 0.5f) * 3.8f, 0.0f, sinf(angle1 + DX_PI_F * 0.5f) * 3.8f);
            VECTOR thick = VGet(0.0f, 3.8f, 0.0f);
            int streakColor = GetColor(12, 120, 48);
            int streakCoreColor = GetColor(60, 235, 105);

            DrawLine3D(p0, p1, streakCoreColor);
            DrawLine3D(VAdd(p0, side0), VAdd(p1, side1), streakColor);
            DrawLine3D(VSub(p0, side0), VSub(p1, side1), streakColor);
            DrawLine3D(VAdd(p0, thick), VAdd(p1, thick), streakColor);
            DrawLine3D(VSub(p0, thick), VSub(p1, thick), streakColor);
        }
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetWriteZBuffer3D(TRUE);
    SetUseLighting(TRUE);
}
}

void VampireScene::drawGround() const
{
    if (mStageModelHandle != -1)
    {
        /// ステージモデルが読み込めた場合は正式モデルを優先する。
        MV1DrawModel(mStageModelHandle);
        return;
    }

    /// モデル読み込みに失敗した場合でも位置確認できるよう、簡易グリッドを描画する。
    SetUseLighting(FALSE);
    for (int i = -8; i <= 8; ++i)
    {
        float p = static_cast<float>(i) * 12.0f;
        DrawLine3D(VGet(RoomMinX, 0.0f, p), VGet(RoomMaxX, 0.0f, p), GetColor(60, 72, 96));
        DrawLine3D(VGet(p, 0.0f, RoomMinZ), VGet(p, 0.0f, RoomMaxZ), GetColor(60, 72, 96));
    }
    SetUseLighting(TRUE);
}

bool VampireScene::canUseExitMarker() const
{
    if (mMagicEnemy == nullptr || !mMagicEnemy->isDeadAnimationFinished())
    {
        return false;
    }

    /// 退出操作はボス撃破後、プレイヤーがマーカー範囲内にいる時だけ許可する。
    VECTOR markerPos = mMagicEnemy->getPosition();
    markerPos.y = 0.12f;
    VECTOR diff = VSub(mPlayer->getPosition(), markerPos);
    diff.y = 0.0f;
    return VSize(diff) <= ExitMarkerRadius;
}

void VampireScene::updateExitMarker()
{
    if (!canUseExitMarker())
    {
        return;
    }

    InputManager* inputMgr = InputManager::instance();
    if (inputMgr->isPush(KEY_INPUT_RETURN) || inputMgr->isPush(KEY_INPUT_E) ||
        inputMgr->isPadPush(XINPUT_BUTTON_A) || isBindingPush(inputMgr, GameSettings::instance()->getPadBinding(GameActionSpecial)))
    {
        /// 決定入力を受けた時点で遷移せず、短いテレポート演出を先に再生する。
        AudioManager::instance()->playSe(SoundResource::Se::Teleport);
        mTeleportEffectTimer = TeleportEffectDuration;
    }
}

void VampireScene::updateTeleportEffect(float deltaTime)
{
    mTeleportEffectTimer -= deltaTime;
    if (mTeleportEffectTimer > 0.0f)
    {
        /// 演出中は空入力を渡し、キャラクターのアニメーション更新だけ継続する。
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
    /// VampireScene 終了時点の HP を保存し、次のボス戦またはリザルトへ引き継ぐ。
    StoryState::instance()->saveVampirePartyState(
        mPlayer->getHp(),
        mCompanion != nullptr ? mCompanion->getHp() : 0);
    StoryState::instance()->setMagicBossCleared(true);

    if (StoryState::instance()->areBothBossesCleared())
    {
        /// 両方のボスを倒している場合は、そのままハッピーエンドのリザルトへ進む。
        StoryState::instance()->setPhase(StoryPhase::HappyEnd);
        SceneManager::instance()->changeScene<ResultScene>(true, mPlayerType);
        return;
    }

    /// まだ Mutant 側が残っている場合はボス選択フェーズへ戻す。
    StoryState::instance()->setPhase(StoryPhase::BossSelect);
    SceneManager::instance()->changeScene<MutantScene>(mPlayerType);
}

void VampireScene::drawExitMarker() const
{
    if (mMagicEnemy == nullptr || !mMagicEnemy->isDeadAnimationFinished())
    {
        return;
    }

    /// マーカーは床面に加算合成で描き、撃破後の次アクションを示す。
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
    VECTOR markerPos = mMagicEnemy->getPosition();
    markerPos.y = 0.12f;
    drawTeleportCircleImage(
        markerPos,
        ExitTeleportCircleSize,
        mExitTeleportCircleGraphHandle,
        GetColorU8(255, 255, 255, 220));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);

    if (canUseExitMarker())
    {
        /// 操作可能な距離に入った時だけ入力ヒントを表示する。
        DrawString(440, 610, "E / ENTER：エリア選択へ戻る", GetColor(180, 220, 255));
    }
}

void VampireScene::drawTeleportEffect() const
{
    if (mTeleportEffectTimer <= 0.0f)
    {
        return;
    }

    /// プレイヤーと相棒の両方に同じ経過時間の螺旋演出を出す。
    float elapsed = TeleportEffectDuration - mTeleportEffectTimer;
    drawSpiralRiseEffect(mPlayer->getPosition(), elapsed, TeleportEffectDuration);
    if (mCompanion != nullptr)
    {
        drawSpiralRiseEffect(mCompanion->getPosition(), elapsed, TeleportEffectDuration);
    }
}
void VampireScene::drawGameUi() const
{
    /// UI 素材は ResourceManager 経由で取得し、ロード失敗時はバー描画を中止する。
    int playerFrameHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::PlayerHpFrame);
    int enemyFrameHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::EnemyHpFrame);
    int staminaFrameHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::StaminaFrame);
    int buffArrowHandle = ResourceManager::instance()->getGraph(BuffArrowGraph);
    if (playerFrameHandle == -1 || enemyFrameHandle == -1)
    {
        return;
    }

    /// 実 HP と遅延 HP を別色で描き、ダメージ量を短時間視認できるようにする。
    float playerHpRate = clampRate(static_cast<float>(mPlayer->getHp()), static_cast<float>(PlayerParam::Hp));
    float playerDelayedHpRate = clampRate(mPlayerDelayedHp, static_cast<float>(PlayerParam::Hp));
    float staminaRate = clampRate(mPlayer->getStamina(), PlayerParam::Stamina);
    float enemyHpRate = clampRate(static_cast<float>(mMagicEnemy->getHp()), static_cast<float>(mMagicEnemy->getMaxHp()));

    DrawBox(40, 36, 40 + static_cast<int>(358.0f * playerDelayedHpRate), 66, GetColor(220, 35, 45), TRUE);
    DrawBox(40, 36, 40 + static_cast<int>(358.0f * playerHpRate), 66, GetColor(45, 220, 70), TRUE);
    DrawRectExtendGraph(24, 18, 414, 74, 153, 401, 1264, 181, playerFrameHandle, TRUE);
    if (mPlayer->isPowerUpActive())
    {
        drawBuffArrow(buffArrowHandle, 24, 18, 390, 56);
    }


    if (mCompanion != nullptr)
    {
        /// 相棒がいる場合は小さい HP フレームを左側に追加する。
        float companionHpRate = clampRate(static_cast<float>(mCompanion->getHp()), static_cast<float>(PlayerParam::Hp));
        float companionDelayedHpRate = clampRate(mCompanionDelayedHp, static_cast<float>(PlayerParam::Hp));
        DrawBox(36, 264, 36 + static_cast<int>(276.0f * companionDelayedHpRate), 286, GetColor(220, 35, 45), TRUE);
        DrawBox(36, 264, 36 + static_cast<int>(276.0f * companionHpRate), 286, GetColor(45, 220, 70), TRUE);
        DrawRectExtendGraph(24, 250, 324, 294, 153, 401, 1264, 181, playerFrameHandle, TRUE);
        if (mCompanion->isPowerUpActive())
        {
            drawBuffArrow(buffArrowHandle, 24, 250, 300, 44);
        }
        DrawString(38, 228, mPlayerType == PlayerCharacterType::Knight ? "WIZARD" : "KNIGHT", GetColor(215, 225, 235));
    }
    if (staminaFrameHandle != -1)
    {
        /// スタミナはプレイヤー操作に直結するため、HP の直下に常時表示する。
        DrawBox(38, 86, 38 + static_cast<int>(312.0f * staminaRate), 110, GetColor(230, 205, 55), TRUE);
        DrawRectExtendGraph(24, 70, 364, 118, 153, 401, 1264, 181, staminaFrameHandle, TRUE);
    }

    /// ボス HP は画面下部に横長で表示し、戦闘全体の進行度を見せる。
    DrawBox(300, 644, 300 + static_cast<int>(700.0f * enemyHpRate), 676, GetColor(220, 35, 45), TRUE);
    DrawRectExtendGraph(230, 612, 1050, 694, 51, 333, 1433, 332, enemyFrameHandle, TRUE);
}
