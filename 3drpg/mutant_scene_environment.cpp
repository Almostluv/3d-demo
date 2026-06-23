/**
 * @file
 * @brief mutant_scene_environment.cpp 実装を定義する。
 */

#include "mutant_scene.h"
#include "resource_manager.h"
#include "model_resource.h"
#include "story_state.h"
#include "game_config.h"
#include "DxLib.h"

#include <string>

namespace
{
bool containsText(const TCHAR* text, const char* keyword)
{
    /// マテリアル名やテクスチャパスに特定語が含まれるかを簡易判定する。
    if (text == nullptr || keyword == nullptr)
    {
        return false;
    }

    return std::string(text).find(keyword) != std::string::npos;
}

bool getStageGroundY(int stageCollisionModelHandle, VECTOR pos, float* groundY)
{
    if (stageCollisionModelHandle == -1 || groundY == nullptr)
    {
        return false;
    }

    /// 指定位置の上下へレイを飛ばし、ステージ衝突モデル上の地面高さを取得する。
    VECTOR start = VAdd(pos, VGet(0.0f, MutantSceneConfig::ShadowGroundRayTop, 0.0f));
    VECTOR end = VAdd(pos, VGet(0.0f, -MutantSceneConfig::ShadowGroundRayBottom, 0.0f));
    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(stageCollisionModelHandle, -1, start, end);

    if (!hit.HitFlag)
    {
        return false;
    }

    *groundY = hit.HitPosition.y;
    return true;
}
}

void MutantScene::setupStage()
{
    /// 通常ボス選択エリアと FinalBoss arena で、表示モデルと衝突モデルを切り替える。
    const char* stageModel = StoryState::instance()->isFinalBoss() ? ModelResource::Stage::MutantArena : ModelResource::Stage::Arena;
    const char* stageCollisionModel = StoryState::instance()->isFinalBoss() ? ModelResource::Stage::MutantArenaCollision : ModelResource::Stage::ArenaCollision;
    float stageScale = StoryState::instance()->isFinalBoss() ? MutantSceneConfig::MutantStageScale : MutantSceneConfig::StageScale;
    mStageModelHandle = ResourceManager::instance()->getModel(stageModel);
    mStageCollisionModelHandle = ResourceManager::instance()->getModel(stageCollisionModel);
    mSkyModelHandle = ResourceManager::instance()->getModel(ModelResource::Stage::Sky);

    if (mStageModelHandle != -1)
    {
        MV1SetPosition(mStageModelHandle, VGet(0.0f, MutantSceneConfig::StagePositionY, 0.0f));
        MV1SetRotationXYZ(mStageModelHandle, VGet(0.0f, 0.0f, 0.0f));
        MV1SetScale(mStageModelHandle, VGet(stageScale, stageScale, stageScale));

        mStageMaterialNum = MV1GetMaterialNum(mStageModelHandle);
        mStageNoDiffuseTextureNum = 0;

        for (int i = 0; i < mStageMaterialNum; ++i)
        {
            /// 床系マテリアルは少し明るめ、それ以外は背景として沈む色味に調整する。
            int diffuseTexture = MV1GetMaterialDifMapTexture(mStageModelHandle, i);
            const TCHAR* materialName = MV1GetMaterialName(mStageModelHandle, i);

            const TCHAR* currentTexturePath = diffuseTexture != -1 ? MV1GetTextureColorFilePath(mStageModelHandle, diffuseTexture) : "";
            bool isFloorLikeMaterial =
                containsText(materialName, "Floor") ||
                containsText(materialName, "floor") ||
                containsText(materialName, "WorldGrid") ||
                containsText(materialName, "lambert") ||
                containsText(materialName, "setka") ||
                containsText(currentTexturePath, "Floor") ||
                containsText(currentTexturePath, "floor") ||
                containsText(currentTexturePath, "setka");

            if (diffuseTexture != -1)
            {
                MV1SetMaterialDifColor(mStageModelHandle, i, GetColorF(1.0f, 1.0f, 0.96f, 1.0f));
                MV1SetMaterialAmbColor(
                    mStageModelHandle,
                    i,
                    isFloorLikeMaterial ? GetColorF(0.82f, 0.82f, 0.76f, 1.0f) : GetColorF(0.42f, 0.42f, 0.38f, 1.0f));
                MV1SetMaterialEmiColor(
                    mStageModelHandle,
                    i,
                    isFloorLikeMaterial ? GetColorF(0.14f, 0.14f, 0.12f, 1.0f) : GetColorF(0.025f, 0.025f, 0.022f, 1.0f));
            }

            if (MV1GetMaterialDifMapTexture(mStageModelHandle, i) == -1)
            {
                /// Diffuse テクスチャなしのマテリアルは暗くなりすぎないよう色を補う。
                ++mStageNoDiffuseTextureNum;

                MV1SetMaterialDifColor(mStageModelHandle, i, GetColorF(0.45f, 0.45f, 0.42f, 1.0f));
                MV1SetMaterialAmbColor(mStageModelHandle, i, GetColorF(0.55f, 0.55f, 0.50f, 1.0f));
                MV1SetMaterialEmiColor(mStageModelHandle, i, GetColorF(0.08f, 0.08f, 0.07f, 1.0f));
            }
        }
    }

    if (mStageCollisionModelHandle != -1)
    {
        /// 衝突モデルは描画せず、線分判定用のコリジョン情報だけを構築する。
        MV1SetPosition(mStageCollisionModelHandle, VGet(0.0f, MutantSceneConfig::StagePositionY, 0.0f));
        MV1SetRotationXYZ(mStageCollisionModelHandle, VGet(0.0f, 0.0f, 0.0f));
        MV1SetScale(mStageCollisionModelHandle, VGet(stageScale, stageScale, stageScale));
        MV1SetupCollInfo(mStageCollisionModelHandle, -1);
    }

    if (mSkyModelHandle != -1)
    {
        MV1SetPosition(mSkyModelHandle, VGet(0.0f, MutantSceneConfig::StagePositionY, 0.0f));
        MV1SetRotationXYZ(mSkyModelHandle, VGet(0.0f, 0.0f, 0.0f));
        MV1SetScale(mSkyModelHandle, VGet(MutantSceneConfig::StageScale, MutantSceneConfig::StageScale, MutantSceneConfig::StageScale));
    }
}

void MutantScene::snapInitialFinalBossGround()
{
    if (!StoryState::instance()->isFinalBoss() || mStageCollisionModelHandle == -1)
    {
        return;
    }

    /// FinalBoss arena では初期配置直後にプレイヤーと相棒を地面高さへ合わせる。
    Player* players[] = { mPlayer.get(), mCompanion.get() };
    for (Player* player : players)
    {
        if (player == nullptr)
        {
            continue;
        }

        VECTOR pos = player->getPosition();
        float groundY = 0.0f;
        if (getStageGroundY(mStageCollisionModelHandle, pos, &groundY))
        {
            pos.y = groundY;
            player->setPosition(pos);
        }
    }
}
void MutantScene::setupLighting()
{
    /// ステージ全体を照らす補助ライトを作成するが、通常描画では必要な場面まで無効にしておく。
    mOverheadLightHandle = CreatePointLightHandle(
        VGet(0.0f, MutantSceneConfig::OverheadLightY, 0.0f),
        MutantSceneConfig::OverheadLightRange,
        MutantSceneConfig::OverheadLightAtten0,
        MutantSceneConfig::OverheadLightAtten1,
        0.0f
    );

    if (mOverheadLightHandle != -1)
    {
        SetLightDifColorHandle(mOverheadLightHandle, GetColorF(1.0f, 0.92f, 0.72f, 1.0f));
        SetLightAmbColorHandle(mOverheadLightHandle, GetColorF(0.18f, 0.16f, 0.12f, 1.0f));
        SetLightSpcColorHandle(mOverheadLightHandle, GetColorF(0.08f, 0.07f, 0.05f, 1.0f));
        SetLightEnableHandle(mOverheadLightHandle, FALSE);
    }

    mCenterSpotLightHandle = -1;
}

void MutantScene::setupShadowMap()
{
    /// キャラクターと敵の影用に、シーン共通のシャドウマップを作成する。
    mShadowMapHandle = MakeShadowMap(MutantSceneConfig::ShadowMapSize, MutantSceneConfig::ShadowMapSize);

    if (mShadowMapHandle == -1)
    {
        return;
    }

    VECTOR lightDir = VNorm(VGet(MutantSceneConfig::MainLightX, MutantSceneConfig::MainLightY, MutantSceneConfig::MainLightZ));
    SetShadowMapLightDirection(mShadowMapHandle, lightDir);
    SetShadowMapAdjustDepth(mShadowMapHandle, MutantSceneConfig::ShadowAdjustDepth);
}

void MutantScene::renderShadowMap()
{
    if (mShadowMapHandle == -1)
    {
        return;
    }

    /// 影の描画範囲はプレイヤー周辺を基準にしつつ、相棒と敵の高さも含める。
    VECTOR playerPos = mPlayer->getPosition();
    float minGroundY = playerPos.y;
    float maxCasterY = playerPos.y + MutantSceneConfig::ShadowAreaTopMargin;
    float groundY = 0.0f;

    if (getStageGroundY(mStageCollisionModelHandle, playerPos, &groundY))
    {
        minGroundY = groundY;
    }

    if (mCompanion != nullptr)
    {
        VECTOR companionPos = mCompanion->getPosition();

        if (getStageGroundY(mStageCollisionModelHandle, companionPos, &groundY) && groundY < minGroundY)
        {
            minGroundY = groundY;
        }

        if (companionPos.y + MutantSceneConfig::ShadowAreaTopMargin > maxCasterY)
        {
            maxCasterY = companionPos.y + MutantSceneConfig::ShadowAreaTopMargin;
        }
    }

    for (auto& enemy : mEnemies)
    {
        VECTOR enemyPos = enemy->getPosition();

        if (getStageGroundY(mStageCollisionModelHandle, enemyPos, &groundY) && groundY < minGroundY)
        {
            minGroundY = groundY;
        }

        if (enemyPos.y + MutantSceneConfig::ShadowAreaTopMargin > maxCasterY)
        {
            maxCasterY = enemyPos.y + MutantSceneConfig::ShadowAreaTopMargin;
        }
    }

    VECTOR minPos = VGet(
        playerPos.x - MutantSceneConfig::ShadowAreaHalfSize,
        minGroundY - MutantSceneConfig::ShadowAreaBottomMargin,
        playerPos.z - MutantSceneConfig::ShadowAreaHalfSize);
    VECTOR maxPos = VGet(
        playerPos.x + MutantSceneConfig::ShadowAreaHalfSize,
        maxCasterY,
        playerPos.z + MutantSceneConfig::ShadowAreaHalfSize);

    SetShadowMapDrawArea(mShadowMapHandle, minPos, maxPos);
    ShadowMap_DrawSetup(mShadowMapHandle);

    /// シャドウマップにはステージではなく、影を落とすキャラクターだけを描く。
    mPlayer->draw();
    if (mCompanion != nullptr)
    {
        mCompanion->draw();
    }

    for (auto& enemy : mEnemies)
    {
        enemy->draw();
    }

    ShadowMap_DrawEnd();
}
