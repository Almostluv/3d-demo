/**
 * @file
 * @brief chara_select_scene.cpp 実装を定義する。
 */

#include "chara_select_scene.h"
#include "audio_manager.h"
#include "vampire_scene.h"
#include "game_manager.h"
#include "game_config.h"
#include "input_manager.h"
#include "model_resource.h"
#include "mutant_scene.h"
#include "player_character_resource.h"
#include "player_param.h"
#include "resource_manager.h"
#include "scene_manager.h"
#include "sound_resource.h"
#include "story_state.h"
#include "title_scene.h"
#include "DxLib.h"

#include <cmath>
#include <cstring>
#include <string>

namespace
{
constexpr const char* HitEffectGraph = "Resource/Effect/KnightHitEffect.png";
constexpr const char* MagicMissileGraph = "Resource/Effect/magicMissile.png";
constexpr const char* MagicBeamGraph = "Resource/Effect/magicBeam.png";
constexpr const char* MagicCircleGraph = "Resource/Effect/magicCircle.png";
constexpr const char* CharaSelectBannerGraph = "Resource/GameUI/CharaSelectBanner.png";
constexpr const char* AGraph = "Resource/GameUI/button_xbox_digital_a_3.png";
constexpr const char* BGraph = "Resource/GameUI/button_xbox_digital_b_3.png";
constexpr const char* DpadRightGraph = "Resource/GameUI/button_xbox_dpad_light_2.png";
constexpr const char* DpadLeftGraph = "Resource/GameUI/button_xbox_dpad_light_4.png";
constexpr const char* PointerModel = "Resource/Model/Pointer/Low Poly Blue Crystal.mv1";
constexpr float StartTransitionTime = 2.0f; ///< 選択確定後に次シーンへ移るまでの時間。単位は seconds。
constexpr float PointerHeight = 24.0f; ///< 選択ポインタをキャラクター上に置く高さ。
constexpr float PointerBobAmplitude = 1.0f; ///< 選択ポインタ上下アニメーションの振幅。
constexpr float PointerBobSpeed = 3.5f; ///< 選択ポインタ上下アニメーションの速度。
constexpr float PointerScale = 0.48f; ///< 選択ポインタモデルの描画スケール。
constexpr int CharaSelectBannerWidth = 512;
constexpr int CharaSelectBannerHeight = 128;
constexpr float BattleCameraDistance = 60.0f; ///< キャラクター選択表示用カメラの距離。
constexpr float BattleCameraAngleH = DX_PI_F; ///< キャラクター選択表示用カメラの水平角。単位は radians。
constexpr float BattleCameraAngleV = 0.3f; ///< キャラクター選択表示用カメラの垂直角。単位は radians。
constexpr int CharacterNone = -1;
constexpr int CharacterKnight = 0;
constexpr int CharacterWizard = 1;

void drawUiText(int x, int y, const char* text, int color, int fontHandle)
{
    if (fontHandle != -1)
    {
        /// UI 文字は影付きで描き、3D 背景上でも読みやすくする。
        DrawStringToHandle(x + 2, y + 2, text, GetColor(8, 24, 44), fontHandle);
        DrawStringToHandle(x, y, text, color, fontHandle);
    }
    else
    {
        DrawString(x, y, text, color);
    }
}
void drawKeyHintBox(int x, int y, const char* key, const char* text, int color, int fontHandle)
{
    /// キー名の幅に合わせて入力ヒントの枠を伸ばす。
    int keyWidth = (fontHandle != -1 ? GetDrawStringWidthToHandle(key, static_cast<int>(strlen(key)), fontHandle) : GetDrawStringWidth(key, static_cast<int>(strlen(key)))) + 22;
    if (keyWidth < 42) keyWidth = 42;
    DrawBox(x, y, x + keyWidth, y + 30, GetColor(18, 24, 32), TRUE);
    DrawBox(x, y, x + keyWidth, y + 30, color, FALSE);
    drawUiText(x + 11, y + 4, key, color, fontHandle);
    drawUiText(x + keyWidth + 10, y + 4, text, color, fontHandle);
}

void drawPadHint(int handle, int x, int y, const char* text, int color, int fontHandle)
{
    /// パッド入力時はキー名ではなくボタン画像を表示する。
    if (handle != -1)
    {
        DrawExtendGraph(x, y, x + 30, y + 30, handle, TRUE);
    }
    drawUiText(x + 38, y + 4, text, color, fontHandle);
}

void drawCharacterConfirmDialog(PlayerCharacterType type, bool usePad, int aGraphHandle, int bGraphHandle, int fontHandle)
{
    /// 選択後すぐ開始せず、誤選択を避けるため確認ダイアログを表示する。
    const char* characterName = type == PlayerCharacterType::Wizard ? "Wizard" : "Knight";
    int left = 390;
    int top = 250;
    int right = 890;
    int bottom = 430;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
    DrawBox(left, top, right, bottom, GetColor(12, 18, 30), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(left, top, right, bottom, GetColor(210, 185, 90), FALSE);
    DrawFormatStringToHandle(left + 74, top + 48, GetColor(8, 24, 44), fontHandle, "%s で開始しますか？", characterName);
    DrawFormatStringToHandle(left + 72, top + 46, GetColor(245, 245, 235), fontHandle, "%s で開始しますか？", characterName);
    int hintColor = GetColor(220, 230, 245);
    if (usePad)
    {
        drawPadHint(aGraphHandle, left + 88, top + 104, "決定", hintColor, fontHandle);
        drawPadHint(bGraphHandle, left + 280, top + 104, "戻る", hintColor, fontHandle);
    }
    else
    {
        drawKeyHintBox(left + 88, top + 104, "Enter", "決定", hintColor, fontHandle);
        drawKeyHintBox(left + 280, top + 104, "Esc", "戻る", hintColor, fontHandle);
    }
}
VECTOR knightPos()
{
    /// Knight の選択画面上の固定表示位置。
    return VGet(40.0f, 0.0f, -4.0f);
}

VECTOR wizardPos()
{
    /// Wizard の選択画面上の固定表示位置。
    return VGet(20.0f, 0.0f, -4.0f);
}

VECTOR selectionCenter()
{
    /// 2 人の中央をタイトル表示用カメラの注視基準にする。
    return VScale(VAdd(knightPos(), wizardPos()), 0.5f);
}

float knightRotY()
{
    return 0.15f;
}

float wizardRotY()
{
    return knightRotY();
}

VECTOR selectedPos(PlayerCharacterType type)
{
    /// 現在選択中のキャラクター位置をポインタとカメラ遷移で共有する。
    return type == PlayerCharacterType::Wizard ? wizardPos() : knightPos();
}

VECTOR battleTarget(PlayerCharacterType type)
{
    /// 戦闘開始風のカメラへ寄せる時、キャラクター上半身を注視する。
    return VAdd(selectedPos(type), VGet(0.0f, 15.0f, 0.0f));
}

VECTOR cameraEyeByAngle(VECTOR center, float angleH)
{
    /// 水平角と固定垂直角から、選択画面用のカメラ位置を計算する。
    float cosV = cosf(BattleCameraAngleV);
    float sinV = sinf(BattleCameraAngleV);
    float cosH = cosf(angleH);
    float sinH = sinf(angleH);

    return VAdd(center, VGet(
        -BattleCameraDistance * cosV * sinH,
        15.0f + BattleCameraDistance * sinV,
        -BattleCameraDistance * cosV * cosH));
}

VECTOR titleEye()
{
    return cameraEyeByAngle(selectionCenter(), 0.0f);
}

VECTOR titleTarget()
{
    return VAdd(selectionCenter(), VGet(0.0f, 12.0f, 0.0f));
}

VECTOR battleEye(PlayerCharacterType type)
{
    return cameraEyeByAngle(selectedPos(type), BattleCameraAngleH);
}

VECTOR transitionEye(PlayerCharacterType type, float rate)
{
    /// タイトル視点から選択キャラクター寄りの視点へ線形補間する。
    VECTOR eyeA = titleEye();
    VECTOR eyeB = battleEye(type);
    return VGet(
        eyeA.x + (eyeB.x - eyeA.x) * rate,
        eyeA.y + (eyeB.y - eyeA.y) * rate,
        eyeA.z + (eyeB.z - eyeA.z) * rate);
}

float smoothStep(float t)
{
    /// カメラ移動の始点と終点をなめらかにするための補間係数。
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

bool containsText(const TCHAR* text, const char* keyword)
{
    /// DxLib から取得した名前が null の場合も安全に検索できるようにする。
    if (text == nullptr || keyword == nullptr)
    {
        return false;
    }

    return std::string(text).find(keyword) != std::string::npos;
}

void setupStageMaterial(int stageModelHandle)
{
    if (stageModelHandle == -1)
    {
        return;
    }

    int materialNum = MV1GetMaterialNum(stageModelHandle);
    for (int i = 0; i < materialNum; ++i)
    {
        /// 床らしいマテリアルだけ明るめにし、キャラクター選択時の視認性を上げる。
        int diffuseTexture = MV1GetMaterialDifMapTexture(stageModelHandle, i);
        const TCHAR* materialName = MV1GetMaterialName(stageModelHandle, i);
        const TCHAR* texturePath = diffuseTexture != -1 ? MV1GetTextureColorFilePath(stageModelHandle, diffuseTexture) : "";
        bool isFloorLikeMaterial =
            containsText(materialName, "Floor") ||
            containsText(materialName, "floor") ||
            containsText(materialName, "WorldGrid") ||
            containsText(materialName, "lambert") ||
            containsText(materialName, "setka") ||
            containsText(texturePath, "Floor") ||
            containsText(texturePath, "floor") ||
            containsText(texturePath, "setka");

        if (diffuseTexture != -1)
        {
            MV1SetMaterialDifColor(stageModelHandle, i, GetColorF(1.0f, 1.0f, 0.96f, 1.0f));
            MV1SetMaterialAmbColor(
                stageModelHandle,
                i,
                isFloorLikeMaterial ? GetColorF(0.82f, 0.82f, 0.76f, 1.0f) : GetColorF(0.42f, 0.42f, 0.38f, 1.0f));
            MV1SetMaterialEmiColor(
                stageModelHandle,
                i,
                isFloorLikeMaterial ? GetColorF(0.14f, 0.14f, 0.12f, 1.0f) : GetColorF(0.025f, 0.025f, 0.022f, 1.0f));
        }
        else
        {
            MV1SetMaterialDifColor(stageModelHandle, i, GetColorF(0.45f, 0.45f, 0.42f, 1.0f));
            MV1SetMaterialAmbColor(stageModelHandle, i, GetColorF(0.55f, 0.55f, 0.50f, 1.0f));
            MV1SetMaterialEmiColor(stageModelHandle, i, GetColorF(0.08f, 0.08f, 0.07f, 1.0f));
        }
    }
}
}

CharaSelectScene::CharaSelectScene()
    : mPlayerModelHandle(-1)
    , mWizardModelHandle(-1)
    , mPointerModelHandle(-1)
    , mPlayerSittingAnimSourceHandle(-1)
    , mPlayerSitToStandAnimSourceHandle(-1)
    , mWizardSittingAnimSourceHandle(-1)
    , mWizardSitToStandAnimSourceHandle(-1)
    , mPlayerBarrelModelHandle(-1)
    , mWizardBarrelModelHandle(-1)
    , mStageModelHandle(-1)
    , mSkyModelHandle(-1)
    , mOverheadLightHandle(-1)
    , mShadowMapHandle(-1)
    , mCharaSelectBannerGraphHandle(-1)
    , mPlayerHpFrameGraphHandle(-1)
    , mEnemyHpFrameGraphHandle(-1)
    , mStaminaFrameGraphHandle(-1)
    , mAGraphHandle(-1)
    , mBGraphHandle(-1)
    , mDpadLeftGraphHandle(-1)
    , mDpadRightGraphHandle(-1)
    , mUiFontHandle(-1)
    , mSelectedCharacter(PlayerCharacterType::Wizard)
    , mHoveredCharacter(CharacterNone)
    , mHasSelectedCharacter(true)
    , mTransitionTimer(0.0f)
    , mGlowTimer(0.0f)
    , mIsStarting(false)
    , mConfirmCharacterDialog(false)
    , mPreloadTaskIndex(0)
{
    /// キャラクター選択中はマウスを自由に使うため、ロックを解除する。
    AudioManager::instance()->stopBgm();
    InputManager::instance()->setMouseLock(false);
    SetMouseDispFlag(TRUE);

    /// 選択画面で見せるモデルと、遷移後に必要な UI 素材を先に取得する。
    mPlayerModelHandle = ResourceManager::instance()->createModelInstance(ModelResource::Player::Model);
    mWizardModelHandle = ResourceManager::instance()->createModelInstance(ModelResource::Wizard::Model);
    mPointerModelHandle = ResourceManager::instance()->createModelInstance(PointerModel);
    mPlayerSittingAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Player::Sitting);
    mPlayerSitToStandAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Player::SitToStand);
    mWizardSittingAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Wizard::Sitting);
    mWizardSitToStandAnimSourceHandle = ResourceManager::instance()->getModel(ModelResource::Wizard::SitToStand);
    mPlayerBarrelModelHandle = ResourceManager::instance()->createModelInstance(ModelResource::Stage::Object08);
    mWizardBarrelModelHandle = ResourceManager::instance()->createModelInstance(ModelResource::Stage::Object08);
    mStageModelHandle = ResourceManager::instance()->getModel(ModelResource::Stage::Arena);
    mSkyModelHandle = ResourceManager::instance()->getModel(ModelResource::Stage::Sky);
    mCharaSelectBannerGraphHandle = ResourceManager::instance()->getGraph(CharaSelectBannerGraph);
    mPlayerHpFrameGraphHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::PlayerHpFrame);
    mEnemyHpFrameGraphHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::EnemyHpFrame);
    mStaminaFrameGraphHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::StaminaFrame);
    mAGraphHandle = ResourceManager::instance()->getGraph(AGraph);
    mBGraphHandle = ResourceManager::instance()->getGraph(BGraph);
    mDpadLeftGraphHandle = ResourceManager::instance()->getGraph(DpadLeftGraph);
    mDpadRightGraphHandle = ResourceManager::instance()->getGraph(DpadRightGraph);
    mUiFontHandle = CreateFontToHandle("Yu Gothic UI", 20, 2, DX_FONTTYPE_ANTIALIASING_EDGE);
    if (mUiFontHandle != -1) SetFontCharCodeFormatToHandle(DX_CHARCODEFORMAT_UTF8, mUiFontHandle);
    /// 選択中の待ち時間で MutantScene 用リソースを少しずつ読み込む。
    setupMutantScenePreloadQueue();

    if (mPlayerModelHandle != -1)
    {
        /// Knight は待機中の座りアニメーションから表示を始める。
        MV1SetScale(mPlayerModelHandle, VGet(PlayerParam::Scale, PlayerParam::Scale, PlayerParam::Scale));
        int materialNum = MV1GetMaterialNum(mPlayerModelHandle);
        for (int i = 0; i < materialNum; ++i)
        {
            MV1SetMaterialDifColor(mPlayerModelHandle, i, GetColorF(1.20f, 1.16f, 1.06f, 1.0f));
            MV1SetMaterialAmbColor(mPlayerModelHandle, i, GetColorF(0.90f, 0.86f, 0.76f, 1.0f));
        }
        mPlayerAnimator.init(mPlayerModelHandle);
        if (mPlayerSittingAnimSourceHandle != -1 && MV1GetAnimNum(mPlayerSittingAnimSourceHandle) > 0)
        {
            mPlayerAnimator.play(0, mPlayerSittingAnimSourceHandle, true, 0.0f);
        }
    }

    if (mWizardModelHandle != -1)
    {
        /// Wizard も Knight と同じ選択演出になるよう座りアニメーションを設定する。
        MV1SetScale(mWizardModelHandle, VGet(PlayerParam::WizardScale, PlayerParam::WizardScale, PlayerParam::WizardScale));
        int materialNum = MV1GetMaterialNum(mWizardModelHandle);
        for (int i = 0; i < materialNum; ++i)
        {
            MV1SetMaterialDifColor(mWizardModelHandle, i, GetColorF(1.20f, 1.16f, 1.06f, 1.0f));
            MV1SetMaterialAmbColor(mWizardModelHandle, i, GetColorF(0.90f, 0.86f, 0.76f, 1.0f));
        }
        mWizardAnimator.init(mWizardModelHandle);
        if (mWizardSittingAnimSourceHandle != -1 && MV1GetAnimNum(mWizardSittingAnimSourceHandle) > 0)
        {
            mWizardAnimator.play(0, mWizardSittingAnimSourceHandle, true, 0.0f);
        }
    }

    if (mStageModelHandle != -1)
    {
        /// 戦闘シーンと同じステージを使い、選択後の遷移を自然に見せる。
        MV1SetPosition(mStageModelHandle, VGet(0.0f, MutantSceneConfig::StagePositionY, 0.0f));
        MV1SetRotationXYZ(mStageModelHandle, VGet(0.0f, 0.0f, 0.0f));
        MV1SetScale(mStageModelHandle, VGet(MutantSceneConfig::StageScale, MutantSceneConfig::StageScale, MutantSceneConfig::StageScale));
        setupStageMaterial(mStageModelHandle);
    }

    if (mSkyModelHandle != -1)
    {
        MV1SetPosition(mSkyModelHandle, VGet(0.0f, MutantSceneConfig::StagePositionY, 0.0f));
        MV1SetRotationXYZ(mSkyModelHandle, VGet(0.0f, 0.0f, 0.0f));
        MV1SetScale(mSkyModelHandle, VGet(MutantSceneConfig::StageScale, MutantSceneConfig::StageScale, MutantSceneConfig::StageScale));
    }

    mOverheadLightHandle = CreatePointLightHandle(
        VGet(0.0f, MutantSceneConfig::OverheadLightY, 0.0f),
        MutantSceneConfig::OverheadLightRange,
        MutantSceneConfig::OverheadLightAtten0,
        MutantSceneConfig::OverheadLightAtten1,
        0.0f);

    if (mOverheadLightHandle != -1)
    {
        SetLightDifColorHandle(mOverheadLightHandle, GetColorF(1.0f, 0.92f, 0.72f, 1.0f));
        SetLightAmbColorHandle(mOverheadLightHandle, GetColorF(0.18f, 0.16f, 0.12f, 1.0f));
        SetLightSpcColorHandle(mOverheadLightHandle, GetColorF(0.08f, 0.07f, 0.05f, 1.0f));
        SetLightEnableHandle(mOverheadLightHandle, FALSE);
    }

    mShadowMapHandle = MakeShadowMap(MutantSceneConfig::ShadowMapSize, MutantSceneConfig::ShadowMapSize);
    if (mShadowMapHandle != -1)
    {
        /// 選択画面でもキャラクターの接地感を出すため、戦闘と同じ方向光で影を作る。
        VECTOR lightDir = VNorm(VGet(MutantSceneConfig::MainLightX, MutantSceneConfig::MainLightY, MutantSceneConfig::MainLightZ));
        SetShadowMapLightDirection(mShadowMapHandle, lightDir);
        SetShadowMapAdjustDepth(mShadowMapHandle, MutantSceneConfig::ShadowAdjustDepth);
    }
}

void CharaSelectScene::setupMutantScenePreloadQueue()
{
    /// 選択画面の更新中に少しずつ読むため、MutantScene で必要なリソース一覧を作る。
    mPreloadTasks.clear();
    mPreloadTaskIndex = 0;

    const PlayerCharacterResourceSet& knightResources = getPlayerCharacterResourceSet(PlayerCharacterType::Knight);
    const PlayerCharacterResourceSet& wizardResources = getPlayerCharacterResourceSet(PlayerCharacterType::Wizard);
    const PlayerCharacterResourceSet playerResources[] = { knightResources, wizardResources };

    for (const PlayerCharacterResourceSet& resources : playerResources)
    {
        addPreloadModel(resources.modelPath);
        addPreloadModel(resources.idlePath);
        addPreloadModel(resources.movePath);
        addPreloadModel(resources.walkLeftPath);
        addPreloadModel(resources.walkRightPath);
        addPreloadModel(resources.walkBackPath);
        addPreloadModel(resources.attack1Path);
        addPreloadModel(resources.attack2Path);
        addPreloadModel(resources.attack3Path);
        addPreloadModel(resources.dodgePath);
        addPreloadModel(resources.guardPath);
        addPreloadModel(resources.guardImpactPath);
        addPreloadModel(resources.counterPath);
        addPreloadModel(resources.jumpPath);
        addPreloadModel(resources.hitPath);
        addPreloadModel(resources.deadPath);
    }

    addPreloadModel(ModelResource::Enemy::Model);
    addPreloadModel(ModelResource::Enemy::Idle);
    addPreloadModel(ModelResource::Enemy::Walk);
    addPreloadModel(ModelResource::Enemy::Attack);
    addPreloadModel(ModelResource::Enemy::JumpAttack);
    addPreloadModel(ModelResource::Enemy::Hit1);
    addPreloadModel(ModelResource::Enemy::Hit2);
    addPreloadModel(ModelResource::Enemy::Dying);
    addPreloadModel(ModelResource::Stage::ArenaCollision);

    addPreloadGraph(MutantSceneConfig::PlayerHpFrame);
    addPreloadGraph(MutantSceneConfig::EnemyHpFrame);
    addPreloadGraph(MutantSceneConfig::StaminaFrame);
    addPreloadGraph(HitEffectGraph);
    addPreloadGraph(MagicMissileGraph);
    addPreloadGraph(MagicBeamGraph);
    addPreloadGraph(MagicCircleGraph);
}

void CharaSelectScene::addPreloadModel(const char* path)
{
    if (path != nullptr)
    {
        /// null パスは登録せず、ResourceManager へ無効な要求を出さない。
        mPreloadTasks.push_back({ TitlePreloadType::Model, path });
    }
}

void CharaSelectScene::addPreloadGraph(const char* path)
{
    if (path != nullptr)
    {
        mPreloadTasks.push_back({ TitlePreloadType::Graph, path });
    }
}

void CharaSelectScene::updatePreloadQueue(int maxCount)
{
    /// 1 フレームあたりのロード数を制限し、選択画面の停止時間を抑える。
    for (int i = 0; i < maxCount && mPreloadTaskIndex < mPreloadTasks.size(); ++i)
    {
        const TitlePreloadTask& task = mPreloadTasks[mPreloadTaskIndex];
        if (task.type == TitlePreloadType::Model)
        {
            ResourceManager::instance()->getModel(task.path);
        }
        else
        {
            ResourceManager::instance()->getGraph(task.path);
        }

        ++mPreloadTaskIndex;
    }
}

bool CharaSelectScene::isPreloadFinished() const
{
    return mPreloadTaskIndex >= mPreloadTasks.size();
}
CharaSelectScene::~CharaSelectScene()
{
    if (mPlayerModelHandle != -1) MV1DeleteModel(mPlayerModelHandle);
    if (mWizardModelHandle != -1) MV1DeleteModel(mWizardModelHandle);
    if (mPointerModelHandle != -1) MV1DeleteModel(mPointerModelHandle);
    if (mPlayerBarrelModelHandle != -1) MV1DeleteModel(mPlayerBarrelModelHandle);
    if (mWizardBarrelModelHandle != -1) MV1DeleteModel(mWizardBarrelModelHandle);
    if (mOverheadLightHandle != -1) DeleteLightHandle(mOverheadLightHandle);
    if (mShadowMapHandle != -1) DeleteShadowMap(mShadowMapHandle);
    if (mUiFontHandle != -1) DeleteFontToHandle(mUiFontHandle);
}

void CharaSelectScene::update(float deltaTime)
{
    InputManager* input = InputManager::instance();
    mGlowTimer += deltaTime;
    /// 毎フレーム少しだけプリロードし、開始時のロード待ちを減らす。
    updatePreloadQueue(1);


    if (!mIsStarting && !mConfirmCharacterDialog)
    {
        updateSelection();
    }

    bool startInput = mHasSelectedCharacter && (
        input->isPush(KEY_INPUT_RETURN) ||
        input->isPush(KEY_INPUT_SPACE) ||
        input->isPadPush(XINPUT_BUTTON_A) ||
        input->isPadPush(XINPUT_BUTTON_START));

    if (!mIsStarting && mConfirmCharacterDialog)
    {
        /// 確認ダイアログ中は選択変更せず、決定かキャンセルだけを受け付ける。
        if (input->isPush(KEY_INPUT_RETURN) || input->isPadPush(XINPUT_BUTTON_A) || input->isPadPush(XINPUT_BUTTON_START))
        {
            AudioManager::instance()->playSe(SoundResource::Se::Confirm);
            StoryState::instance()->resetForNewGame();
            mConfirmCharacterDialog = false;
            mIsStarting = true;
            mTransitionTimer = 0.0f;
            if (mPlayerSitToStandAnimSourceHandle != -1 && MV1GetAnimNum(mPlayerSitToStandAnimSourceHandle) > 0)
            {
                /// 開始演出として座り状態から立ち上がるアニメーションへ切り替える。
                mPlayerAnimator.play(0, mPlayerSitToStandAnimSourceHandle, false, 0.12f);
            }
            if (mWizardSitToStandAnimSourceHandle != -1 && MV1GetAnimNum(mWizardSitToStandAnimSourceHandle) > 0)
            {
                mWizardAnimator.play(0, mWizardSitToStandAnimSourceHandle, false, 0.12f);
            }
        }
        else if (input->isPush(KEY_INPUT_ESCAPE) || input->isPadPush(XINPUT_BUTTON_B))
        {
            AudioManager::instance()->playSe(SoundResource::Se::Cancel);
            mConfirmCharacterDialog = false;
        }
        updateModel(deltaTime);
        return;
    }

    if (!mIsStarting && (input->isPush(KEY_INPUT_ESCAPE) || input->isPadPush(XINPUT_BUTTON_B)))
    {
        AudioManager::instance()->playSe(SoundResource::Se::Cancel);
        SceneManager::instance()->changeScene<TitleScene>();
        return;
    }

    if (!mIsStarting && startInput)
    {
        AudioManager::instance()->playSe(SoundResource::Se::Confirm);
        mConfirmCharacterDialog = true;
        updateModel(deltaTime);
        return;
    }

    if (mIsStarting)
    {
        /// カメラ遷移が終わり、プリロードも完了してから戦闘シーンへ進む。
        mTransitionTimer += deltaTime;
        if (mTransitionTimer >= StartTransitionTime && isPreloadFinished())
        {
            SceneManager::instance()->changeScene<MutantScene>(mSelectedCharacter);
        }
    }

    updateModel(deltaTime);
}

void CharaSelectScene::draw()
{
    /// パッド操作時はマウスカーソルを隠し、入力デバイスに合わせた表示にする。
    SetMouseDispFlag(InputManager::instance()->isLastInputPad() ? FALSE : TRUE);
    renderShadowMap();

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

    float rate = mIsStarting ? smoothStep(mTransitionTimer / StartTransitionTime) : 0.0f;
    /// 開始時はタイトル風の俯瞰視点から、選択キャラクター寄りの戦闘視点へ寄せる。
    VECTOR eye = mIsStarting ? transitionEye(mSelectedCharacter, rate) : titleEye();
    VECTOR target = mIsStarting ? battleTarget(mSelectedCharacter) : titleTarget();
    SetCameraPositionAndTarget_UpVecY(eye, target);
    SetCameraNearFar(1.0f, 100000.0f);

    if (mSkyModelHandle != -1)
    {
        /// 空モデルは背景なので、Z 書き込みを切って先に描画する。
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
        SetUseLighting(TRUE);
        SetMaterialUseVertDifColor(FALSE);
        SetMaterialUseVertSpcColor(FALSE);
        SetUseBackCulling(FALSE);
        MV1DrawModel(mStageModelHandle);
        SetUseBackCulling(TRUE);
        SetMaterialUseVertDifColor(TRUE);
        SetMaterialUseVertSpcColor(TRUE);
    }

    if (mPlayerBarrelModelHandle != -1) MV1DrawModel(mPlayerBarrelModelHandle);
    if (mWizardBarrelModelHandle != -1) MV1DrawModel(mWizardBarrelModelHandle);

    drawCharacterModel(mPlayerModelHandle);
    drawCharacterModel(mWizardModelHandle);
    if (mPointerModelHandle != -1 && !mIsStarting) MV1DrawModel(mPointerModelHandle);

    if (mIsStarting)
    {
        /// 戦闘開始演出中はゲーム UI を少しずつ表示する。
        drawTransitionGameUi(rate);
    }
    else
    {
        drawTitleUi();
        if (mConfirmCharacterDialog)
        {
            drawCharacterConfirmDialog(mSelectedCharacter, InputManager::instance()->isLastInputPad(), mAGraphHandle, mBGraphHandle, mUiFontHandle);
        }
    }

    SetUseShadowMap(0, -1);
}

void CharaSelectScene::renderShadowMap()
{
    if (mShadowMapHandle == -1)
    {
        return;
    }

    /// 2 人を含む範囲だけをシャドウマップ対象にして、選択画面の影を作る。
    VECTOR center = selectionCenter();
    VECTOR minPos = VGet(
        center.x - MutantSceneConfig::ShadowAreaHalfSize,
        center.y - MutantSceneConfig::ShadowAreaBottomMargin,
        center.z - MutantSceneConfig::ShadowAreaHalfSize);
    VECTOR maxPos = VGet(
        center.x + MutantSceneConfig::ShadowAreaHalfSize,
        center.y + MutantSceneConfig::ShadowAreaTopMargin,
        center.z + MutantSceneConfig::ShadowAreaHalfSize);

    SetShadowMapDrawArea(mShadowMapHandle, minPos, maxPos);
    ShadowMap_DrawSetup(mShadowMapHandle);

    if (mPlayerBarrelModelHandle != -1) MV1DrawModel(mPlayerBarrelModelHandle);
    if (mWizardBarrelModelHandle != -1) MV1DrawModel(mWizardBarrelModelHandle);
    drawCharacterModel(mPlayerModelHandle);
    drawCharacterModel(mWizardModelHandle);

    ShadowMap_DrawEnd();
}

void CharaSelectScene::updateModel(float deltaTime)
{
    /// 表示モデルのアニメーションだけを更新し、ゲーム用 Player はまだ生成しない。
    mPlayerAnimator.update(deltaTime);
    mWizardAnimator.update(deltaTime);

    if (mPlayerBarrelModelHandle != -1)
    {
        MV1SetPosition(mPlayerBarrelModelHandle, VAdd(knightPos(), VGet(0.0f, -4.5f, 0.0f)));
        MV1SetRotationXYZ(mPlayerBarrelModelHandle, VGet(0.0f, 0.35f, 0.0f));
        MV1SetScale(mPlayerBarrelModelHandle, VGet(0.09f, 0.09f, 0.09f));
    }

    if (mWizardBarrelModelHandle != -1)
    {
        MV1SetPosition(mWizardBarrelModelHandle, VAdd(wizardPos(), VGet(0.0f, -4.5f, 0.0f)));
        MV1SetRotationXYZ(mWizardBarrelModelHandle, VGet(0.0f, 0.35f, 0.0f));
        MV1SetScale(mWizardBarrelModelHandle, VGet(0.09f, 0.09f, 0.09f));
    }

    if (mPlayerModelHandle != -1)
    {
        MV1SetPosition(mPlayerModelHandle, knightPos());
        MV1SetRotationXYZ(mPlayerModelHandle, VGet(0.0f, knightRotY(), 0.0f));
    }

    if (mWizardModelHandle != -1)
    {
        MV1SetPosition(mWizardModelHandle, wizardPos());
        MV1SetRotationXYZ(mWizardModelHandle, VGet(0.0f, wizardRotY(), 0.0f));
    }

    if (mPointerModelHandle != -1)
    {
        /// 選択ポインタは上下に揺らして、現在選択中のキャラクターを示す。
        float bob = sinf(mGlowTimer * PointerBobSpeed) * PointerBobAmplitude;
        MV1SetPosition(mPointerModelHandle, VAdd(selectedPos(mSelectedCharacter), VGet(0.0f, PointerHeight + bob, 0.0f)));
        MV1SetRotationXYZ(mPointerModelHandle, VGet(0.0f, 0.0f, 0.0f));
        MV1SetScale(mPointerModelHandle, VGet(PointerScale, PointerScale, PointerScale));
    }

    applyCharacterGlow(mPlayerModelHandle, mHoveredCharacter == CharacterKnight, mHasSelectedCharacter && mSelectedCharacter == PlayerCharacterType::Knight, mGlowTimer);
    applyCharacterGlow(mWizardModelHandle, mHoveredCharacter == CharacterWizard, mHasSelectedCharacter && mSelectedCharacter == PlayerCharacterType::Wizard, mGlowTimer);
}

void CharaSelectScene::updateSelection()
{
    InputManager* input = InputManager::instance();
    /// マウス判定で使用するため、選択画面カメラを先に設定する。
    SetCameraPositionAndTarget_UpVecY(titleEye(), titleTarget());
    SetCameraNearFar(1.0f, 100000.0f);

    PlayerCharacterType previousCharacter = mSelectedCharacter;

    if (input->isPush(KEY_INPUT_A) || input->isPadPush(XINPUT_BUTTON_DPAD_LEFT))
    {
        mSelectedCharacter = PlayerCharacterType::Wizard;
        mHasSelectedCharacter = true;
    }
    else if (input->isPush(KEY_INPUT_D) || input->isPadPush(XINPUT_BUTTON_DPAD_RIGHT))
    {
        mSelectedCharacter = PlayerCharacterType::Knight;
        mHasSelectedCharacter = true;
    }

    mHoveredCharacter = CharacterNone;
    if (isMouseOverCharacter(knightPos()))
    {
        mHoveredCharacter = CharacterKnight;
    }
    else if (isMouseOverCharacter(wizardPos()))
    {
        mHoveredCharacter = CharacterWizard;
    }

    if (input->isMousePush(MOUSE_INPUT_LEFT) && mHoveredCharacter != CharacterNone)
    {
        mSelectedCharacter = mHoveredCharacter == CharacterWizard ? PlayerCharacterType::Wizard : PlayerCharacterType::Knight;
        mHasSelectedCharacter = true;
    }

    if (mSelectedCharacter != previousCharacter)
    {
        /// 選択が変わった時だけ SE を鳴らし、連続再生を避ける。
        AudioManager::instance()->playSe(SoundResource::Se::CharacterSelect);
    }
}
void CharaSelectScene::drawTitleUi()
{
    if (mCharaSelectBannerGraphHandle != -1)
    {
        /// 画面左上にキャラクター選択用バナーを固定表示する。
        DrawExtendGraph(0, 0, CharaSelectBannerWidth, CharaSelectBannerHeight, mCharaSelectBannerGraphHandle, TRUE);
    }

    bool usePad = InputManager::instance()->isLastInputPad();
    int textColor = GetColor(230, 235, 245);
    int hintColor = GetColor(185, 200, 220);
    DrawFormatStringToHandle(38, 407, GetColor(8, 24, 44), mUiFontHandle, "選択中：%s", mSelectedCharacter == PlayerCharacterType::Wizard ? "Wizard" : "Knight");
    DrawFormatStringToHandle(36, 405, textColor, mUiFontHandle, "選択中：%s", mSelectedCharacter == PlayerCharacterType::Wizard ? "Wizard" : "Knight");
    if (usePad)
    {
        if (mDpadLeftGraphHandle != -1) DrawExtendGraph(36, 430, 66, 460, mDpadLeftGraphHandle, TRUE);
        drawUiText(74, 434, "/", hintColor, mUiFontHandle);
        if (mDpadRightGraphHandle != -1) DrawExtendGraph(92, 430, 122, 460, mDpadRightGraphHandle, TRUE);
        drawUiText(130, 434, "切替", hintColor, mUiFontHandle);
        drawPadHint(mAGraphHandle, 36, 470, "決定", hintColor, mUiFontHandle);
        drawPadHint(mBGraphHandle, 156, 470, "戻る", hintColor, mUiFontHandle);
    }
    else
    {
        drawKeyHintBox(36, 430, "A", "", hintColor, mUiFontHandle);
        drawUiText(88, 434, "/", hintColor, mUiFontHandle);
        drawKeyHintBox(106, 430, "D", "", hintColor, mUiFontHandle);
        drawUiText(158, 434, "切替", hintColor, mUiFontHandle);
        drawKeyHintBox(36, 470, "Enter", "決定", hintColor, mUiFontHandle);
        drawKeyHintBox(206, 470, "Esc", "戻る", hintColor, mUiFontHandle);
    }
}

void CharaSelectScene::drawTransitionGameUi(float rate)
{
    if (rate <= 0.0f)
    {
        return;
    }

    if (rate > 1.0f)
    {
        rate = 1.0f;
    }

    /// UI の表示幅を遷移率で伸ばし、戦闘画面へ入る流れを作る。
    int playerFillWidth = static_cast<int>(358.0f * rate);
    int staminaFillWidth = static_cast<int>(312.0f * rate);
    int playerFrameWidth = static_cast<int>(390.0f * rate);
    int staminaFrameWidth = static_cast<int>(340.0f * rate);

    if (playerFillWidth > 0)
    {
        DrawBox(40, 36, 40 + playerFillWidth, 66, GetColor(50, 210, 70), TRUE);
    }

    if (mPlayerHpFrameGraphHandle != -1 && playerFrameWidth > 0)
    {
        DrawRectExtendGraph(24, 18, 24 + playerFrameWidth, 74, 153, 401, static_cast<int>(1264.0f * rate), 181, mPlayerHpFrameGraphHandle, TRUE);
    }

    if (staminaFillWidth > 0)
    {
        DrawBox(38, 86, 38 + staminaFillWidth, 110, GetColor(215, 185, 35), TRUE);
    }

    if (mStaminaFrameGraphHandle != -1 && staminaFrameWidth > 0)
    {
        DrawRectExtendGraph(24, 70, 24 + staminaFrameWidth, 118, 153, 401, static_cast<int>(1264.0f * rate), 181, mStaminaFrameGraphHandle, TRUE);
    }

}

bool CharaSelectScene::isMouseOverCharacter(VECTOR pos) const
{
    /// 画面座標からワールド空間のレイを作り、キャラクター周辺の球でホバー判定する。
    InputManager* input = InputManager::instance();
    VECTOR nearPos = ConvScreenPosToWorldPos(VGet(static_cast<float>(input->getMouseScreenX()), static_cast<float>(input->getMouseScreenY()), 0.0f));
    VECTOR farPos = ConvScreenPosToWorldPos(VGet(static_cast<float>(input->getMouseScreenX()), static_cast<float>(input->getMouseScreenY()), 1.0f));
    return HitCheck_Line_Sphere(nearPos, farPos, VAdd(pos, VGet(0.0f, 8.0f, 0.0f)), 8.0f) == TRUE;
}

void CharaSelectScene::applyCharacterGlow(int modelHandle, bool hovered, bool selected, float timer) const
{
    if (modelHandle == -1)
    {
        return;
    }

    /// 選択中は常時発光、ホバー中は時間で揺れる発光にする。
    float glow = 0.0f;
    if (selected)
    {
        glow = 0.55f;
    }
    else if (hovered)
    {
        glow = 0.25f + (sinf(timer * 8.0f) + 1.0f) * 0.25f;
    }

    int materialNum = MV1GetMaterialNum(modelHandle);
    for (int i = 0; i < materialNum; ++i)
    {
        float baseGlow = 0.0f;
        MV1SetMaterialEmiColor(modelHandle, i, GetColorF(baseGlow + 0.08f * glow, baseGlow + 0.35f * glow, baseGlow + glow, 1.0f));
    }
}

void CharaSelectScene::drawCharacterModel(int modelHandle) const
{
    if (modelHandle != -1)
    {
        /// 読み込み失敗時は描画をスキップする。
        MV1DrawModel(modelHandle);
    }
}

VECTOR CharaSelectScene::lerpVector(VECTOR a, VECTOR b, float t) const
{
    /// カメラや UI 位置補間用の線形補間。
    return VGet(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t);
}
