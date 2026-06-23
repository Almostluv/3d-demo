/**
 * @file
 * @brief loading_scene.cpp 実装を定義する。
 */

#include "loading_scene.h"
#include "chara_select_scene.h"
#include "game_config.h"
#include "model_resource.h"
#include "resource_manager.h"
#include "scene_manager.h"
#include "DxLib.h"
#include <cmath>

namespace
{
constexpr const char* VampireArena = "Resource/Model/Stage/hell-arena/source/hell_arena.mv1";
constexpr const char* VampireArenaCollision = "Resource/Model/Stage/hell-arena/source/hell_arena_c.mv1";
constexpr const char* LoadingBackground = "Resource/GameUI/loading.png";
constexpr int BarX = 320;
constexpr int BarY = 515;
constexpr int BarWidth = 640;
constexpr int BarHeight = 24;
constexpr int CircleX = kWindowWidth / 2;
constexpr int TextY = 410;

enum LoadType
{
    /// 現在のローディング対象は DxLib モデルリソースのみ。
    LoadModel
};

struct LoadTask
{
    /// ローディング画面中に 1 件ずつ処理するリソース情報。
    LoadType type;
    const char* path;
};

const LoadTask LoadTasks[] =
{
    { LoadModel, ModelResource::Stage::Arena },
    { LoadModel, ModelResource::Stage::ArenaCollision },
    { LoadModel, ModelResource::Stage::Sky },
    { LoadModel, ModelResource::Stage::Object08 },
    { LoadModel, ModelResource::Stage::MutantArena },
    { LoadModel, ModelResource::Stage::MutantArenaCollision },
    { LoadModel, VampireArena },
    { LoadModel, VampireArenaCollision },
};

constexpr int LoadTaskCount = sizeof(LoadTasks) / sizeof(LoadTasks[0]);
}

LoadingScene::LoadingScene()
    : mBackgroundGraphHandle(ResourceManager::instance()->getGraph(LoadingBackground))
    , mFontHandle(CreateFontToHandle("Georgia", 38, 3, DX_FONTTYPE_ANTIALIASING_EDGE))
    , mTaskIndex(0)
    , mAnimTimer(0.0f)
{
    /// 事前に重いモデルを読み込み、キャラクター選択以降の初回停止を減らす。
    if (mFontHandle != -1) SetFontCharCodeFormatToHandle(DX_CHARCODEFORMAT_UTF8, mFontHandle);
}

LoadingScene::~LoadingScene()
{
    if (mFontHandle != -1)
    {
        DeleteFontToHandle(mFontHandle);
    }
}

void LoadingScene::update(float deltaTime)
{
    mAnimTimer += deltaTime;

    if (mTaskIndex < LoadTaskCount)
    {
        /// 1 フレームに 1 件ずつロードし、ローディング演出が更新される余地を残す。
        const LoadTask& task = LoadTasks[mTaskIndex];
        if (task.type == LoadModel)
        {
            ResourceManager::instance()->getModel(task.path);
        }
        ++mTaskIndex;
        return;
    }

    /// 全タスクが完了したらキャラクター選択へ進む。
    SceneManager::instance()->changeScene<CharaSelectScene>();
}

void LoadingScene::draw()
{
    if (mBackgroundGraphHandle >= 0)
    {
        /// 専用背景が読めた場合は画面全体へ引き伸ばして表示する。
        DrawExtendGraph(0, 0, kWindowWidth, kWindowHeight, mBackgroundGraphHandle, TRUE);
    }
    else
    {
        /// 背景画像がない場合でも黒背景でローディング表示を継続する。
        DrawBox(0, 0, kWindowWidth, kWindowHeight, GetColor(10, 12, 18), TRUE);
    }

    float rate = LoadTaskCount > 0 ? static_cast<float>(mTaskIndex) / static_cast<float>(LoadTaskCount) : 1.0f;
    if (rate > 1.0f)
    {
        rate = 1.0f;
    }

    SetDrawBlendMode(DX_BLENDMODE_ADD, 135);
    for (int i = 0; i < 10; ++i)
    {
        /// バー周辺の小さな光点を時間差で上下させ、読み込み中の動きを出す。
        float t = mAnimTimer * 0.45f + static_cast<float>(i) * 0.17f;
        float wave = (sinf(t * 6.28318f) + 1.0f) * 0.5f;
        int x = CircleX - 190 + i * 42 + static_cast<int>(sinf(t * 5.0f) * 12.0f);
        int y = BarY + 18 - static_cast<int>(wave * 92.0f);
        int alpha = 45 + static_cast<int>(wave * 70.0f);
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
        DrawCircle(x, y, 3, GetColor(90, 210, 255), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const char* text = "Now Loading...";
    int textWidth = mFontHandle != -1 ? GetDrawStringWidthToHandle(text, -1, mFontHandle) : GetDrawStringWidth(text, -1);
    int textX = (kWindowWidth - textWidth) / 2;
    /// 文字色をゆっくり明滅させ、静止画に見えないようにする。
    int pulse = static_cast<int>((sinf(mAnimTimer * 3.0f) + 1.0f) * 25.0f);
    if (mFontHandle != -1)
    {
        DrawStringToHandle(textX + 3, TextY + 3, text, GetColor(8, 24, 44), mFontHandle);
        DrawStringToHandle(textX, TextY, text, GetColor(205 + pulse, 235, 255), mFontHandle);
    }
    else
    {
        DrawString(textX, TextY, text, GetColor(205 + pulse, 235, 255));
    }

    /// 現在のロード済み件数に応じて進捗バーを伸ばす。
    int fillRight = BarX + 4 + static_cast<int>((BarWidth - 8) * rate);
    DrawBox(BarX - 2, BarY - 2, BarX + BarWidth + 2, BarY + BarHeight + 2, GetColor(18, 42, 62), TRUE);
    DrawBox(BarX, BarY, BarX + BarWidth, BarY + BarHeight, GetColor(45, 62, 78), FALSE);
    int glow = static_cast<int>((sinf(mAnimTimer * 3.0f) + 1.0f) * 32.0f);
    DrawBox(BarX + 4, BarY + 4, fillRight, BarY + BarHeight - 4, GetColor(55 + glow, 150 + glow, 220 + glow / 2), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 55 + glow);
    DrawBox(BarX + 4, BarY + 3, fillRight, BarY + BarHeight - 3, GetColor(60, 190, 255), FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
