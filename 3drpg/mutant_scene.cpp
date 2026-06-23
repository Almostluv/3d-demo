/**
 * @file
 * @brief mutant_scene.cpp 実装を定義する。
 */

#include "mutant_scene.h"
#include "audio_manager.h"
#include "result_scene.h"
#include "vampire_scene.h"
#include "scene_manager.h"
#include "setting_scene.h"
#include "title_scene.h"
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
#include <memory>

namespace
{
VECTOR guardImpactEffectPosition(const Player& player)
{
    /// ガード成功エフェクトはプレイヤー側が計算したカウンター基準点へ出す。
    return player.getCounterEffectPosition();
}

VECTOR startPosByType(PlayerCharacterType type)
{
    /// 選択キャラクターごとに初期位置を少しずらし、相棒と重ならないようにする。
    return type == PlayerCharacterType::Wizard
        ? VGet(20.0f, 0.0f, -4.0f)
        : VGet(40.0f, 0.0f, -4.0f);
}

bool isBindingPush(InputManager* inputMgr, InputBinding binding)
{
    /// 設定済み入力の種別に合わせて、押下瞬間の判定へ振り分ける。
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
    /// 設定済み入力の種別に合わせて、押しっぱなし判定へ振り分ける。
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
    /// 左スティック入力を PlayerInput に加算し、キー入力と同じ移動処理へ流す。
    input->move.x += inputMgr->getPadLeftStickX();
    input->move.z += inputMgr->getPadLeftStickY();
}

float updateDelayedHpValue(float delayedHp, float currentHp, float deltaTime)
{
    constexpr float DelayDrainSpeed = 8.0f; ///< UI 表示用 HP が実 HP へ追いつく速度。単位は HP/second。
    if (delayedHp <= currentHp)
    {
        return currentHp;
    }

    /// ダメージ表示用の遅延 HP は実 HP へゆっくり追従させる。
    delayedHp -= DelayDrainSpeed * deltaTime;
    return delayedHp < currentHp ? currentHp : delayedHp;
}
const VECTOR MagicBossMarkerPos = { 0.0f, 0.12f, -32.0f };
const VECTOR FinalBossMarkerPos = { 40.0f, 0.12f, -32.0f };
constexpr const char* MagicBossTeleportCircleGraph = "Resource/Effect/TeleportCircle2.png";
constexpr const char* FinalBossTeleportCircleGraph = "Resource/Effect/TeleportCircle1.png";
constexpr const char* AButtonGraph = "Resource/GameUI/button_xbox_digital_a_3.png";
constexpr float BossMarkerRadius = 12.0f; ///< ボス戦終了後マーカーの判定半径。
constexpr float TeleportCircleSize = 13.0f; ///< テレポート演出円の描画サイズ。
constexpr float TeleportEffectDuration = 1.0f; ///< テレポート演出の表示時間。単位は seconds。
void drawPlayKeyHintBox(int x, int y, const char* key, int color)
{
    /// キー名の文字幅に合わせて枠を広げ、短いキーでも最低幅を保つ。
    int keyWidth = GetDrawStringWidth(key, static_cast<int>(strlen(key))) + 22;
    if (keyWidth < 42) keyWidth = 42;
    DrawBox(x, y, x + keyWidth, y + 30, GetColor(18, 24, 32), TRUE);
    DrawBox(x, y, x + keyWidth, y + 30, color, FALSE);
    DrawString(x + 11, y + 6, key, color);
}

int drawPlayPadHint(int handle, int x, int y, int color)
{
    /// パッドアイコンを描いた後、次のヒント文字を置く X 座標を返す。
    if (handle != -1)
    {
        DrawExtendGraph(x, y, x + 30, y + 30, handle, TRUE);
    }
    return x + 38;
}
VERTEX3D makeGroundVertex(VECTOR pos, COLOR_U8 color, float u, float v)
{
    /// テレポート円を床面に貼るため、上向き法線の頂点を作る。
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
    /// 四角形を 2 三角形で構成し、XZ 平面にテクスチャを描画する。
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
    float rate = elapsed / duration;
    if (rate < 0.0f) rate = 0.0f;
    if (rate > 1.0f) rate = 1.0f;

    VECTOR spiralCenter = VAdd(basePos, VGet(0.0f, 13.0f, 0.0f));
    /// 複数の加算ラインを上方向へ伸ばし、テレポートの立ち上がる光を作る。
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
}

MutantScene::MutantScene(PlayerCharacterType playerType)
    : mStageModelHandle(-1)
    , mStageCollisionModelHandle(-1)
    , mSkyModelHandle(-1)
    , mOverheadLightHandle(-1)
    , mCenterSpotLightHandle(-1)
    , mShadowMapHandle(-1)
    , mMagicBossTeleportCircleGraphHandle(-1)
    , mFinalBossTeleportCircleGraphHandle(-1)
    , mAGraphHandle(-1)
    , mStageMaterialNum(0)
    , mStageNoDiffuseTextureNum(0)
    , mIsEnemyLocked(false)
    , mPlayerType(playerType)
    , mIntroTimer(0.0f)
    , mEnemyHpBarRevealTimer(0.0f)
    , mPlayerDelayedHp(static_cast<float>(PlayerParam::Hp))
    , mCompanionDelayedHp(static_cast<float>(PlayerParam::Hp))
    , mEnemyIntroStarted(false)
    , mCompanionAttackTimer(0.0f)
    , mCompanionSpellTimer(0.0f)
    , mCompanionDodgeTimer(0.0f)
    , mCompanionGuardHoldTimer(0.0f)
    , mCompanionSpecialTimer(0.0f)
    , mCompanionDefenseReactionTimer(-1.0f)
    , mCompanionDeadTimer(0.0f)
    , mCompanionDebugState("Follow")
    , mIsPaused(false)
    , mShowDebugHitCollision(false)
    , mEnemyMovementEnabled(true)
    , mPauseSelectedItem(0)
    , mPauseFontHandle(-1)
    , mTeleportEffectTimer(0.0f)
    , mTeleportToFinalBoss(false)
    , mTeleportReturnToBossSelect(false)
    , mTeleportResourcesLoaded(false)
    , mReturnMarkerPos(FinalBossMarkerPos)
{
    /// 戦闘シーンではマウスを中央固定し、カメラ入力として扱う。
    InputManager::instance()->setMouseLock(true);
    SetMousePoint(kWindowWidth / 2, kWindowHeight / 2);
    SetMouseDispFlag(FALSE);


    mCamera = std::make_unique<Camera>();
    mPlayer = std::make_unique<Player>(mCamera.get(), playerType);
    mPlayer->setPosition(startPosByType(playerType));
    bool createCompanion = true;
    if (createCompanion)
    {
        /// プレイヤーが選ばなかった職業を AI 相棒として生成する。
        PlayerCharacterType companionType = playerType == PlayerCharacterType::Knight ? PlayerCharacterType::Wizard : PlayerCharacterType::Knight;
        mCompanion = std::make_unique<Player>(mCamera.get(), companionType);
        mCompanion->setAiControlled(true);
        mCompanion->setPosition(startPosByType(companionType));
    }
    if (StoryState::instance()->isMagicBossCleared())
    {
        if (StoryState::instance()->hasVampirePartyState())
        {
            /// VampireScene 後に戻ってきた場合は保存済み HP を引き継ぐ。
            mPlayer->restoreHpForScene(StoryState::instance()->getVampirePlayerHp());
            if (mCompanion != nullptr)
            {
                mCompanion->restoreHpForScene(StoryState::instance()->getVampireCompanionHp());
            }
        }
        /// ボス選択から戻った時はイントロ待機を短縮する。
        mIntroTimer = 2.0f;
    }
    mPlayerDelayedHp = static_cast<float>(mPlayer->getHp());
    mCompanionDelayedHp = mCompanion != nullptr ? static_cast<float>(mCompanion->getHp()) : 0.0f;
    mCamera->update(mPlayer->getPosition(), 0.0f);
    AudioManager::instance()->updateListener(mCamera->getEyePosition(), mCamera->getTargetPosition());

    mPauseFontHandle = CreateFontToHandle("Yu Gothic UI", 28, 2, DX_FONTTYPE_ANTIALIASING_EDGE);
    if (mPauseFontHandle != -1) SetFontCharCodeFormatToHandle(DX_CHARCODEFORMAT_UTF8, mPauseFontHandle);
    setupStage();
    snapInitialFinalBossGround();
    /// ステージ補正後の位置でカメラとリスナーを同期し直す。
    mCamera->update(mPlayer->getPosition(), 0.0f);
    AudioManager::instance()->updateListener(mCamera->getEyePosition(), mCamera->getTargetPosition());
    setupLighting();
    setupShadowMap();
    mMagicBossTeleportCircleGraphHandle = ResourceManager::instance()->getGraph(MagicBossTeleportCircleGraph);
    mFinalBossTeleportCircleGraphHandle = ResourceManager::instance()->getGraph(FinalBossTeleportCircleGraph);
    mAGraphHandle = ResourceManager::instance()->getGraph(AButtonGraph);
    if (StoryState::instance()->isFinalBoss())
    {
        /// Mutant 戦中だけ専用 BGM を再生する。
        AudioManager::instance()->playBgm(SoundResource::Bgm::MutantScene);
    }
    else
    {
        /// ボス選択フェーズでは BGM を止め、戦闘開始との区別を付ける。
        AudioManager::instance()->stopBgm();
    }
}

MutantScene::~MutantScene()
{
    releaseGameUi();
    if (mPauseFontHandle != -1) DeleteFontToHandle(mPauseFontHandle);

    if (mOverheadLightHandle != -1)
    {
        DeleteLightHandle(mOverheadLightHandle);
        mOverheadLightHandle = -1;
    }

    if (mCenterSpotLightHandle != -1)
    {
        DeleteLightHandle(mCenterSpotLightHandle);
        mCenterSpotLightHandle = -1;
    }

    if (mShadowMapHandle != -1)
    {
        DeleteShadowMap(mShadowMapHandle);
        mShadowMapHandle = -1;
    }

}

void MutantScene::update(float deltaTime) {
    auto inputMgr = InputManager::instance();

    if (mIsPaused && mPauseSettingScene != nullptr)
    {
        /// 設定画面をポーズ中のオーバーレイとして更新し、戦闘側は止める。
        mPauseSettingScene->update(deltaTime);
        if (mPauseSettingScene->isFinished())
        {
            mPauseSettingScene.reset();
        }
        return;
    }


    if (inputMgr->isPush(KEY_INPUT_F5))
    {
        /// F5 は当たり判定確認用のデバッグ表示を切り替える。
        mShowDebugHitCollision = !mShowDebugHitCollision;
    }

    if (inputMgr->isPush(KEY_INPUT_F6))
    {
        /// F6 は敵移動の有無を切り替え、戦闘検証時に攻撃範囲を確認しやすくする。
        mEnemyMovementEnabled = !mEnemyMovementEnabled;
        for (auto& enemy : mEnemies)
        {
            enemy->setMovementEnabled(mEnemyMovementEnabled);
        }
    }

    if (inputMgr->isPush(KEY_INPUT_F7))
    {
        /// F7 は最寄り敵のジャンプ攻撃を即時確認するデバッグ操作。
        Enemy* enemy = getNearestEnemy();
        if (enemy != nullptr)
        {
            enemy->debugStartJumpAttack(mPlayer->getPosition());
        }
    }

    if (inputMgr->isPush(KEY_INPUT_ESCAPE) || inputMgr->isPadPush(XINPUT_BUTTON_START))
    {
        /// ポーズ切替後は同フレームで通常入力を処理しない。
        setPaused(!mIsPaused);
        return;
    }

    if (mIsPaused)
    {
        updatePauseMenu();
        return;
    }

    if (mTeleportEffectTimer > 0.0f)
    {
        /// テレポート演出中は遷移完了まで戦闘更新を止める。
        updateTeleportEffect(deltaTime);
        return;
    }

    mIntroTimer += deltaTime;
    if (StoryState::instance()->isFinalBoss() && !mEnemyIntroStarted && mIntroTimer >= 3.0f)
    {
        /// FinalBoss フェーズでは短い待機後に敵を生成し、開始演出の間を作る。
        setupEnemies();
        mEnemyIntroStarted = true;
    }

    bool introPlayersLocked = mIntroTimer < 2.0f;
    PlayerInput input;
    int role = mPlayerType == PlayerCharacterType::Wizard ? 1 : 0;
    GameSettings* settings = GameSettings::instance();
    bool onVampireMarker = canUseVampireMarker() || canUseFinalBossMarker() || canUseReturnMarker();

    /// イントロロック中とマーカー上では、戦闘入力の一部を抑制する。
    if (!introPlayersLocked && isBindingPress(inputMgr, settings->getInputBinding(role, GameActionMoveForward))) input.move.z += 1.0f;
    if (!introPlayersLocked && isBindingPress(inputMgr, settings->getInputBinding(role, GameActionMoveBack))) input.move.z -= 1.0f;
    if (!introPlayersLocked && isBindingPress(inputMgr, settings->getInputBinding(role, GameActionMoveLeft))) input.move.x -= 1.0f;
    if (!introPlayersLocked && isBindingPress(inputMgr, settings->getInputBinding(role, GameActionMoveRight))) input.move.x += 1.0f;
    if (!introPlayersLocked && isBindingPress(inputMgr, settings->getPadBinding(GameActionMoveForward))) input.move.z += 1.0f;
    if (!introPlayersLocked && isBindingPress(inputMgr, settings->getPadBinding(GameActionMoveBack))) input.move.z -= 1.0f;
    if (!introPlayersLocked && isBindingPress(inputMgr, settings->getPadBinding(GameActionMoveLeft))) input.move.x -= 1.0f;
    if (!introPlayersLocked && isBindingPress(inputMgr, settings->getPadBinding(GameActionMoveRight))) input.move.x += 1.0f;
    if (!introPlayersLocked) addPadMoveInput(inputMgr, &input);

    input.attack = !introPlayersLocked && (isBindingPush(inputMgr, settings->getInputBinding(role, GameActionPrimary)) || isBindingPush(inputMgr, settings->getPadBinding(GameActionPrimary)));
    input.dodge = !introPlayersLocked && (isBindingPush(inputMgr, settings->getInputBinding(role, GameActionDodge)) || isBindingPush(inputMgr, settings->getPadBinding(GameActionDodge)));
    input.guard = !introPlayersLocked && (isBindingPress(inputMgr, settings->getInputBinding(role, GameActionSecondary)) || isBindingPress(inputMgr, settings->getPadBinding(GameActionSecondary)));
    input.counter = !introPlayersLocked && !onVampireMarker && (isBindingPush(inputMgr, settings->getInputBinding(role, GameActionSpecial)) || isBindingPush(inputMgr, settings->getPadBinding(GameActionSpecial)));

    if (!introPlayersLocked && (isBindingPush(inputMgr, settings->getInputBinding(role, GameActionLockOn)) || isBindingPush(inputMgr, settings->getPadBinding(GameActionLockOn))))
    {
        mIsEnemyLocked = !mIsEnemyLocked && getNearestEnemy() != nullptr;
    }

    if (!introPlayersLocked && settings->isAutoLockTarget() && getNearestEnemy() != nullptr)
    {
        mIsEnemyLocked = true;
    }
    mPlayer->setLockOn(!introPlayersLocked && mIsEnemyLocked && getNearestEnemy() != nullptr);
    mPlayer->setInput(input);
    mPlayer->update(deltaTime);
    if (mIsEnemyLocked && !mPlayer->isDodging())
    {
        /// ロック中は回避していない時だけ、プレイヤーを最寄り敵へ向ける。
        Enemy* lockTarget = getNearestEnemy();
        if (lockTarget != nullptr)
        {
            VECTOR toEnemy = VSub(lockTarget->getPosition(), mPlayer->getPosition());
            toEnemy.y = 0.0f;
            if (VSize(toEnemy) > 0.0001f)
            {
                mPlayer->setRotation(VGet(0.0f, atan2f(toEnemy.x, toEnemy.z), 0.0f));
            }
        }
        else
        {
            mIsEnemyLocked = false;
        }
    }
    if (introPlayersLocked)
    {
        if (mCompanion != nullptr)
        {
            /// イントロ中の相棒は空入力でアニメーション更新のみ行う。
            PlayerInput companionInput;
            mCompanion->setInput(companionInput);
            mCompanion->update(deltaTime);
        }
    }
    else
    {
        /// 操作可能になってから相棒 AI を通常更新する。
        updateCompanion(deltaTime);
    }
    resolvePlayerStageGroundCollision();
    resolveCompanionStageGroundCollision();
    /// 位置補正後に攻撃判定を行い、当たり判定と見た目のずれを抑える。
    processPlayerAttack();
    processCompanionAttack();
    updateVampireMarker();
    updateDelayedPlayerHp(deltaTime);
    mAttackEffect.update(deltaTime);
    updatePendingMagicMissileHits(deltaTime);
    if (mEnemyIntroStarted && !mEnemies.empty() && mEnemyHpBarRevealTimer < 2.0f)
    {
        /// 敵登場直後は HP バーを短時間で表示し、戦闘開始を示す。
        mEnemyHpBarRevealTimer += deltaTime;
        if (mEnemyHpBarRevealTimer > 2.0f)
        {
            mEnemyHpBarRevealTimer = 2.0f;
        }
    }
    updateEnemies(deltaTime);
    resolveEnemiesStageGroundCollision();
    resolvePlayerEnemyCollision();
    mPlayer->syncModelTransform();
    if (mCompanion != nullptr)
    {
        mCompanion->syncModelTransform();
    }
    if (inputMgr->isPush(KEY_INPUT_F10))
    {
        /// F10 はカウンターヒット演出の位置確認用デバッグ操作。
        Player* effectPlayer = mPlayer->isWizard() && mCompanion != nullptr ? mCompanion.get() : mPlayer.get();
        effectPlayer->syncModelTransform();
        mAttackEffect.startCounterHitImpact(effectPlayer->getCounterEffectPosition());
    }
    updateKnightSwordTrails();
    processEnemyAttack();
    if (mPlayer->consumeGuardImpactEffectRequest())
    {
        /// ガード成功リクエストは一度だけ消費してエフェクトへ変換する。
        mAttackEffect.startMagicHitImpact(guardImpactEffectPosition(*mPlayer), 2);
    }
    if (mCompanion != nullptr && mCompanion->consumeGuardImpactEffectRequest())
    {
        mAttackEffect.startMagicHitImpact(guardImpactEffectPosition(*mCompanion), 2);
    }

    if (mPlayer->isDeadAnimationFinished())
    {
        /// 死亡演出終了後にリザルトへ遷移する。
        SceneManager::instance()->changeScene<ResultScene>(false, mPlayerType);
        return;
    }

    if (mEnemyIntroStarted && mEnemies.empty() && StoryState::instance()->isFinalBoss() && !StoryState::instance()->isFinalBossCleared())
    {
        /// FinalBoss の敵が全滅した時点でクリア状態を保存する。
        StoryState::instance()->setFinalBossCleared(true);
        if (StoryState::instance()->areBothBossesCleared())
        {
            StoryState::instance()->setPhase(StoryPhase::HappyEnd);
            SceneManager::instance()->changeScene<ResultScene>(true, mPlayerType);
            return;
        }
    }

    if (introPlayersLocked)
    {
        /// イントロ中は通常追従カメラを動かさず、現在ターゲットへ同期する。
        mCamera->syncToTarget(mPlayer->getPosition());
        AudioManager::instance()->updateListener(mCamera->getEyePosition(), mCamera->getTargetPosition());
    }
    else
    {
        Enemy* lockEnemy = mIsEnemyLocked ? getNearestEnemy() : nullptr;
        if (lockEnemy != nullptr)
        {
            /// ロック対象がいる間はプレイヤーと敵を同時に収めるカメラへ切り替える。
            mCamera->updateLockOn(mPlayer->getPosition(), lockEnemy->getPosition(), deltaTime);
        }
        else
        {
            mIsEnemyLocked = false;
            mCamera->update(mPlayer->getPosition(), deltaTime);
        }
        AudioManager::instance()->updateListener(mCamera->getEyePosition(), mCamera->getTargetPosition());
    }
}

void MutantScene::draw() {
    /// 先にシャドウマップを描き、通常描画で影を参照できるようにする。
    renderShadowMap();

    mCamera->draw();

    /// MutantScene は薄い自然光寄りの設定で、屋外アリーナの見た目に合わせる。
    SetUseLighting(TRUE);
    SetUseHalfLambertLighting(TRUE);
    SetGlobalAmbientLight(GetColorF(0.48f, 0.48f, 0.45f, 1.0f));

    VECTOR lightDir = VNorm(VGet(MutantSceneConfig::MainLightX, MutantSceneConfig::MainLightY, MutantSceneConfig::MainLightZ));
    ChangeLightTypeDir(lightDir);
    SetLightDirection(lightDir);
    SetLightDifColor(GetColorF(0.66f, 0.64f, 0.60f, 1.0f));
    SetLightSpcColor(GetColorF(0.10f, 0.10f, 0.09f, 1.0f));
    SetLightAmbColor(GetColorF(0.36f, 0.36f, 0.33f, 1.0f));
    SetUseSpecular(TRUE);
    SetLightUseShadowMap(0, TRUE);

    if (mShadowMapHandle != -1)
    {
        /// シャドウマップが作成できている時だけ 0 番スロットへ設定する。
        SetUseShadowMap(0, mShadowMapHandle);
    }

    if (mOverheadLightHandle != -1)
    {
        SetLightPositionHandle(mOverheadLightHandle, VGet(0.0f, MutantSceneConfig::OverheadLightY, 0.0f));
        SetLightRangeAttenHandle(
            mOverheadLightHandle,
            MutantSceneConfig::OverheadLightRange,
            MutantSceneConfig::OverheadLightAtten0,
            MutantSceneConfig::OverheadLightAtten1,
            0.0f);
        SetLightEnableHandle(mOverheadLightHandle, FALSE);
    }

    if (mSkyModelHandle != -1)
    {
        /// 空モデルは背景として描くため、ライティングと Z 書き込みを一時的に切る。
        SetUseLighting(FALSE);
        SetWriteZBuffer3D(FALSE);
        SetUseBackCulling(FALSE);
        MV1DrawModel(mSkyModelHandle);
        SetUseBackCulling(TRUE);
        SetWriteZBuffer3D(TRUE);
        SetUseLighting(TRUE);
    }

    if (mStageModelHandle != -1)
    {
        /// ステージは両面表示にして、モデルの裏面抜けを避ける。
        SetUseLighting(TRUE);
        SetMaterialUseVertDifColor(FALSE);
        SetMaterialUseVertSpcColor(FALSE);
        SetUseBackCulling(FALSE);
        MV1DrawModel(mStageModelHandle);
        SetUseBackCulling(TRUE);
        SetMaterialUseVertDifColor(TRUE);
        SetMaterialUseVertSpcColor(TRUE);
    }

    drawVampireMarker();

    for (auto& enemy : mEnemies)
    {
        enemy->drawRamMarker();
    }

    mPlayer->draw();
    if (mCompanion != nullptr)
    {
        mCompanion->draw();
    }

    for (auto& enemy : mEnemies)
    {
        enemy->draw();
    }

    Enemy* lockEnemy = mIsEnemyLocked ? getNearestEnemy() : nullptr;
    if (lockEnemy != nullptr)
    {
        /// ロック対象を Z 無効の発光球で示し、敵に隠れても見失いにくくする。
        SetUseLighting(FALSE);
        VECTOR lockPoint = lockEnemy->getLockPointPosition();
        SetUseZBuffer3D(FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 55);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 95);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
        DrawSphere3D(lockPoint, 1.3f, 16, GetColor(255, 255, 255), GetColor(255, 255, 255), TRUE);
        SetUseZBuffer3D(TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetUseLighting(TRUE);
    }

    drawTeleportEffect();
    mAttackEffect.draw();
    drawDebugHitCollision();

    SetUseShadowMap(0, -1);

    drawGameUi();
    if (mShowDebugHitCollision)
    {
        /// デバッグ HUD は通常 UI の後に重ねて、数値確認を優先する。
        drawDebugHud();
    }

    if (mIsPaused)
    {
        if (mPauseSettingScene != nullptr)
        {
            /// 設定画面はポーズメニューの代わりにオーバーレイ表示する。
            mPauseSettingScene->draw();
        }
        else
        {
            drawPauseMenu();
        }
    }
}
