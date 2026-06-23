/**
 * @file
 * @brief vampire_scene.cpp 実装を定義する。
 */

#include "vampire_scene.h"
#include "audio_manager.h"
#include "mutant_scene.h"
#include "result_scene.h"
#include "scene_manager.h"
#include "setting_scene.h"
#include "story_state.h"
#include "sound_resource.h"
#include "title_scene.h"
#include "input_manager.h"
#include "game_config.h"
#include "game_settings.h"
#include "resource_manager.h"
#include "player_param.h"
#include "DxLib.h"

#include <algorithm>
#include <cmath>

namespace
{
VECTOR guardImpactEffectPosition(const Player& player)
{
    /// ガード成功エフェクトはキャラクター側が持つカウンター基準点に合わせる。
    return player.getCounterEffectPosition();
}

VECTOR enemyPos()
{
    /// VampireScene のボス初期位置。プレイヤー初期位置より奥側に固定する。
    return VGet(0.0f, 0.0f, -90.0f);
}
constexpr const char* VampireStageModel = "Resource/Model/Stage/hell-arena/source/hell_arena.mv1";
constexpr const char* VampireStageCollisionModel = "Resource/Model/Stage/hell-arena/source/hell_arena_c.mv1";
constexpr const char* ExitTeleportCircleGraph = "Resource/Effect/TeleportCircle2.png";
constexpr float RoomMinX = -96.0f; ///< VampireScene 部屋内移動制限の最小 X。
constexpr float RoomMaxX = 96.0f; ///< VampireScene 部屋内移動制限の最大 X。
constexpr float RoomMinZ = -120.0f; ///< VampireScene 部屋内移動制限の最小 Z。
constexpr float RoomMaxZ = 72.0f; ///< VampireScene 部屋内移動制限の最大 Z。
constexpr float DoorRoomMaxGroundStep = 8.0f; ///< 地面へ吸着できる最大段差。

void clampDoorRoomPosition(Player* player)
{
    if (player == nullptr)
    {
        return;
    }

    VECTOR pos = player->getPosition();
    /// ステージ外へ出ないよう、ワールド座標の XZ を部屋範囲に制限する。
    if (pos.x < RoomMinX) pos.x = RoomMinX;
    if (pos.x > RoomMaxX) pos.x = RoomMaxX;
    if (pos.z < RoomMinZ) pos.z = RoomMinZ;
    if (pos.z > RoomMaxZ) pos.z = RoomMaxZ;
    player->setPosition(pos);
}

void snapDoorRoomGround(Player* player, int collisionModelHandle)
{
    if (player == nullptr || collisionModelHandle == -1 || !player->canSnapToGround())
    {
        return;
    }

    VECTOR pos = player->getPosition();
    /// 上から下へレイを飛ばし、許容段差内の床だけへ吸着させる。
    VECTOR start = VAdd(pos, VGet(0.0f, 80.0f, 0.0f));
    VECTOR end = VAdd(pos, VGet(0.0f, -200.0f, 0.0f));
    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(collisionModelHandle, -1, start, end);
    if (hit.HitFlag && hit.HitPosition.y <= pos.y + DoorRoomMaxGroundStep)
    {
        pos.y = hit.HitPosition.y;
        player->setPosition(pos);
    }
}

}

VampireScene::VampireScene(PlayerCharacterType playerType)
    : mPlayerType(playerType)
    , mPlayerDelayedHp(static_cast<float>(PlayerParam::Hp))
    , mCompanionDelayedHp(static_cast<float>(PlayerParam::Hp))
    , mCompanionAttackTimer(0.0f)
    , mIsEnemyLocked(false)
    , mShowDebugHitCollision(false)
    , mIsPaused(false)
    , mPauseSelectedItem(0)
    , mPauseFontHandle(-1)
    , mStageModelHandle(-1)
    , mStageCollisionModelHandle(-1)
    , mExitTeleportCircleGraphHandle(-1)
    , mTeleportEffectTimer(0.0f)
    , mIntroState(IntroMoveToBoss)
    , mIntroTimer(0.0f)
    , mRoaringStarted(false)
    , mIntroBallsReleased(false)
{
    /// 戦闘中はマウスを画面中央へ固定し、カメラ操作用の入力として扱う。
    InputManager::instance()->setMouseLock(true);
    SetMouseDispFlag(FALSE);
    AudioManager::instance()->playBgm(SoundResource::Bgm::VampireScene);
    mPauseFontHandle = CreateFontToHandle("Yu Gothic UI", 28, 2, DX_FONTTYPE_ANTIALIASING_EDGE);
    if (mPauseFontHandle != -1) SetFontCharCodeFormatToHandle(DX_CHARCODEFORMAT_UTF8, mPauseFontHandle);

    mCamera = std::make_unique<Camera>();
    mPlayer = std::make_unique<Player>(mCamera.get(), playerType);
    /// プレイヤーが選ばなかった職業を AI 相棒として同行させる。
    PlayerCharacterType companionType = playerType == PlayerCharacterType::Knight ? PlayerCharacterType::Wizard : PlayerCharacterType::Knight;
    mCompanion = std::make_unique<Player>(mCamera.get(), companionType);
    mCompanion->setAiControlled(true);
    mMagicEnemy = std::make_unique<MagicProjectileEnemy>(enemyPos());
    mStageModelHandle = ResourceManager::instance()->getModel(VampireStageModel);
    if (mStageModelHandle != -1)
    {
        /// 見た目用モデルと判定用モデルで同じ座標変換を使い、描画と衝突を一致させる。
        MV1SetPosition(mStageModelHandle, VGet(0.0f, -10.0f, 0.0f));
        MV1SetRotationXYZ(mStageModelHandle, VGet(0.0f, 0.0f, 0.0f));
        MV1SetScale(mStageModelHandle, VGet(1.0f, 1.0f, 1.0f));
    }
    mExitTeleportCircleGraphHandle = ResourceManager::instance()->getGraph(ExitTeleportCircleGraph);
    mStageCollisionModelHandle = ResourceManager::instance()->getModel(VampireStageCollisionModel);
    if (mStageCollisionModelHandle != -1)
    {
        /// DxLib のモデル衝突情報を事前構築し、毎フレームの床吸着判定で利用する。
        MV1SetPosition(mStageCollisionModelHandle, VGet(0.0f, -10.0f, 0.0f));
        MV1SetRotationXYZ(mStageCollisionModelHandle, VGet(0.0f, 0.0f, 0.0f));
        MV1SetScale(mStageCollisionModelHandle, VGet(1.0f, 1.0f, 1.0f));
        MV1SetupCollInfo(mStageCollisionModelHandle, -1);
    }

    /// 初期配置後にモデルとカメラを同期し、初回描画で古い姿勢が出ないようにする。
    mPlayer->setPosition(VGet(20.0f, 0.0f, 0.0f));
    mCompanion->setPosition(VGet(40.0f, 0.0f, 0.0f));
    mPlayerDelayedHp = static_cast<float>(mPlayer->getHp());
    mCompanionDelayedHp = static_cast<float>(mCompanion->getHp());
    mPlayer->syncModelTransform();
    mCompanion->syncModelTransform();
    mCamera->update(mPlayer->getPosition(), 0.0f);
    AudioManager::instance()->updateListener(mCamera->getEyePosition(), mCamera->getTargetPosition());
}

VampireScene::~VampireScene()
{
    if (mPauseFontHandle != -1) DeleteFontToHandle(mPauseFontHandle);
}

void VampireScene::update(float deltaTime)
{
    InputManager* inputMgr = InputManager::instance();
    if (mIsPaused && mSettingScene != nullptr)
    {
        /// 設定画面をポーズ上のオーバーレイとして扱い、戦闘側の更新は止める。
        mSettingScene->update(deltaTime);
        if (mSettingScene->isFinished())
        {
            mSettingScene.reset();
        }
        return;
    }

    if (inputMgr->isPush(KEY_INPUT_F5))
    {
        /// F5 は当たり判定確認用のデバッグ表示を切り替える。
        mShowDebugHitCollision = !mShowDebugHitCollision;
    }

    if (inputMgr->isPush(KEY_INPUT_F9) && mMagicEnemy != nullptr)
    {
        /// F9 は戦闘終了確認用にボス HP を 1 へ落とすデバッグ操作。
        mMagicEnemy->debugSetHpToOne();
    }

    if (inputMgr->isPush(KEY_INPUT_ESCAPE) || inputMgr->isPadPush(XINPUT_BUTTON_START))
    {
        /// ポーズ切替後は同じフレームで戦闘入力を処理しない。
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
        /// テレポート演出中は移動先決定まで戦闘更新を停止する。
        updateTeleportEffect(deltaTime);
        return;
    }

    if (mIntroState != IntroDone)
    {
        /// イントロ中は専用カメラと演出のみを進め、プレイヤー操作を受け付けない。
        updateIntro(deltaTime);
        return;
    }

    /// プレイヤーと相棒を先に更新し、その後でステージ境界と床へ補正する。
    updatePlayer(deltaTime);
    updateCompanion(deltaTime);
    clampDoorRoomPosition(mPlayer.get());
    snapDoorRoomGround(mPlayer.get(), mStageCollisionModelHandle);
    clampDoorRoomPosition(mCompanion.get());
    snapDoorRoomGround(mCompanion.get(), mStageCollisionModelHandle);
    mMagicEnemy->update(deltaTime, mPlayer.get(), mCompanion.get());
    if (mPlayer->consumeGuardImpactEffectRequest())
    {
        /// ガード成功リクエストは一度だけ消費してヒット停止風のエフェクトへ変換する。
        mAttackEffect.startMagicHitImpact(guardImpactEffectPosition(*mPlayer), 2);
    }
    if (mCompanion != nullptr && mCompanion->consumeGuardImpactEffectRequest())
    {
        mAttackEffect.startMagicHitImpact(guardImpactEffectPosition(*mCompanion), 2);
    }

    /// 入力、AI、敵更新の後に攻撃判定を処理し、同フレームの位置を反映させる。
    processPlayerAttack();
    processCompanionAttack();
    mAttackEffect.update(deltaTime);
    updatePendingMagicMissileHits(deltaTime);
    updateDelayedPlayerHp(deltaTime);

    if (mPlayer->isDeadAnimationFinished())
    {
        /// 死亡演出が終わってからリザルトへ遷移し、倒れモーションを最後まで見せる。
        SceneManager::instance()->changeScene<ResultScene>(false, mPlayerType, true);
        return;
    }

    mPlayer->syncModelTransform();
    if (mCompanion != nullptr)
    {
        mCompanion->syncModelTransform();
    }
    updateKnightSwordTrails();

    if (mMagicEnemy->isDeadAnimationFinished())
    {
        if (StoryState::instance()->isFinalBossCleared())
        {
            /// もう一方のボス撃破済みなら、この場でエンディングへ進める。
            StoryState::instance()->saveVampirePartyState(
                mPlayer->getHp(),
                mCompanion != nullptr ? mCompanion->getHp() : 0);
            StoryState::instance()->setMagicBossCleared(true);
            StoryState::instance()->setPhase(StoryPhase::HappyEnd);
            SceneManager::instance()->changeScene<ResultScene>(true, mPlayerType);
            return;
        }

        /// 片方だけ撃破済みの場合は退出マーカーでボス選択へ戻す。
        updateExitMarker();
    }

    bool exitMarkerVisible = mMagicEnemy != nullptr && mMagicEnemy->isDeadAnimationFinished();
    bool keepLockCamera = mIsEnemyLocked && mMagicEnemy != nullptr && !exitMarkerVisible;
    if (keepLockCamera)
    {
        /// ボス生存中だけロックオンカメラを維持する。
        mCamera->updateLockOn(mPlayer->getPosition(), mMagicEnemy->getPosition(), deltaTime);
    }
    else if (mIsEnemyLocked)
    {
        /// ロック解除時に現在ビューへ角度を同期し、通常カメラへ自然に戻す。
        mCamera->syncAnglesToCurrentView();
        mIsEnemyLocked = false;
    }
    else
    {
        mCamera->update(mPlayer->getPosition(), deltaTime);
    }
    AudioManager::instance()->updateListener(mCamera->getEyePosition(), mCamera->getTargetPosition());
}

void VampireScene::draw()
{
    if (mIntroState != IntroDone)
    {
        /// イントロ中は通常カメラではなく演出用カメラを直接設定する。
        drawIntroCamera();
    }
    else
    {
        mCamera->draw();
    }

    /// VampireScene は赤みのある環境光で、地獄アリーナの色味を維持する。
    SetUseLighting(TRUE);
    SetUseHalfLambertLighting(TRUE);
    SetGlobalAmbientLight(GetColorF(0.62f, 0.50f, 0.46f, 1.0f));
    VECTOR lightDir = VNorm(VGet(-0.35f, -0.82f, 0.42f));
    ChangeLightTypeDir(lightDir);
    SetLightDirection(lightDir);
    SetLightDifColor(GetColorF(0.95f, 0.72f, 0.58f, 1.0f));
    SetLightAmbColor(GetColorF(0.44f, 0.34f, 0.32f, 1.0f));
    SetLightSpcColor(GetColorF(0.20f, 0.14f, 0.10f, 1.0f));
    SetUseSpecular(TRUE);

    /// 3D オブジェクトを先に描き、その後エフェクトと 2D UI を重ねる。
    drawGround();
    mPlayer->draw();
    if (mCompanion != nullptr)
    {
        mCompanion->draw();
    }
    mMagicEnemy->draw();
    if (mIsEnemyLocked && mMagicEnemy != nullptr && !mMagicEnemy->isDead())
    {
        /// ロック対象の視認性を上げるため、Z を無効にした発光球をロック点へ描く。
        SetUseLighting(FALSE);
        VECTOR lockPoint = mMagicEnemy->getLockPointPosition();
        SetUseZBuffer3D(FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 55);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 95);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
        DrawSphere3D(lockPoint, 1.3f, 16, GetColor(255, 255, 255), GetColor(255, 255, 255), TRUE);
        SetUseZBuffer3D(TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetUseLighting(TRUE);
    }
    if (mShowDebugHitCollision)
    {
        /// デバッグ表示はゲームプレイ用 UI より前に描画し、ボスの判定位置を確認しやすくする。
        mMagicEnemy->drawDebugHitCollision();
    }
    drawExitMarker();
    drawTeleportEffect();
    mAttackEffect.draw();
    drawGameUi();

    if (mIsPaused)
    {
        if (mSettingScene != nullptr)
        {
            /// 設定画面を開いている間はポーズメニューの代わりに設定 UI を描く。
            mSettingScene->draw();
        }
        else
        {
            drawPauseMenu();
        }
    }
}


