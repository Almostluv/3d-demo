/**
 * @file
 * @brief result_scene.cpp 実装を定義する。
 */

#include "result_scene.h"
#include "audio_manager.h"
#include "game_config.h"
#include "input_manager.h"
#include "vampire_scene.h"
#include "mutant_scene.h"
#include "scene_manager.h"
#include "resource_manager.h"
#include "story_state.h"
#include "title_scene.h"
#include "sound_resource.h"
#include "DxLib.h"

#include <cstring>

namespace
{
constexpr int ItemX = 520;
constexpr int RetryY = 385;
constexpr int TitleY = 455;
constexpr int ItemWidth = 240;
constexpr int ItemHeight = 44;
constexpr const char* AGraph = "Resource/GameUI/button_xbox_digital_a_3.png";
constexpr const char* DpadUpGraph = "Resource/GameUI/button_xbox_dpad_light_1.png";
constexpr const char* DpadDownGraph = "Resource/GameUI/button_xbox_dpad_light_3.png";
}

ResultScene::ResultScene(bool isVictory, PlayerCharacterType playerType, bool retryVampireScene)
    : mIsVictory(isVictory)
    , mPlayerType(playerType)
    , mRetryVampireScene(retryVampireScene)
    , mSelectedItem(isVictory ? Title : Retry)
    , mFontHandle(-1)
    , mAGraphHandle(-1)
    , mDpadUpGraphHandle(-1)
    , mDpadDownGraphHandle(-1)
{
    /// 勝利時はタイトル項目のみ、敗北時はリトライ項目から選択開始する。
    InputManager::instance()->setMouseLock(false);
    SetMouseDispFlag(InputManager::instance()->isLastInputPad() ? FALSE : TRUE);
    mFontHandle = CreateFontToHandle("Yu Gothic UI", 28, 2, DX_FONTTYPE_ANTIALIASING_EDGE);
    if (mFontHandle != -1) SetFontCharCodeFormatToHandle(DX_CHARCODEFORMAT_UTF8, mFontHandle);
    mAGraphHandle = ResourceManager::instance()->getGraph(AGraph);
    mDpadUpGraphHandle = ResourceManager::instance()->getGraph(DpadUpGraph);
    mDpadDownGraphHandle = ResourceManager::instance()->getGraph(DpadDownGraph);
    if (mIsVictory)
    {
        /// 勝利時はステージ BGM を止め、静かなリザルト表示にする。
        AudioManager::instance()->stopBgm();
    }
    else
    {
        AudioManager::instance()->playBgm(SoundResource::Bgm::Defeat);
    }
}

ResultScene::~ResultScene()
{
    if (mFontHandle != -1)
    {
        DeleteFontToHandle(mFontHandle);
    }
}

void ResultScene::update(float deltaTime)
{
    (void)deltaTime;

    InputManager* input = InputManager::instance();
    SetMouseDispFlag(input->isLastInputPad() ? FALSE : TRUE);
    /// マウス操作時だけカーソル位置から選択項目を更新する。
    int mouseItem = input->isLastInputPad() ? -1 : getMouseItem(input->getMouseScreenX(), input->getMouseScreenY());
    if (mouseItem >= 0)
    {
        mSelectedItem = mouseItem;
    }

    if (!mIsVictory && (input->isPush(KEY_INPUT_UP) || input->isPush(KEY_INPUT_W) || input->isPadPush(XINPUT_BUTTON_DPAD_UP)))
    {
        /// 勝利時は選択肢がタイトルのみなので、上下移動は敗北時だけ有効にする。
        moveSelection(-1);
    }

    if (!mIsVictory && (input->isPush(KEY_INPUT_DOWN) || input->isPush(KEY_INPUT_S) || input->isPadPush(XINPUT_BUTTON_DPAD_DOWN)))
    {
        moveSelection(1);
    }

    if (input->isPush(KEY_INPUT_RETURN) || input->isPush(KEY_INPUT_SPACE) ||
        input->isPadPush(XINPUT_BUTTON_A) || input->isPadPush(XINPUT_BUTTON_START) ||
        (!input->isLastInputPad() && input->isMousePush(MOUSE_INPUT_LEFT) && mouseItem >= 0))
    {
        decideSelected();
    }
}

void ResultScene::draw()
{
    /// リザルトは背景を暗く固定し、勝敗テキストとメニューだけを前面に出す。
    DrawBox(0, 0, kWindowWidth, kWindowHeight, GetColor(10, 12, 18), TRUE);

    drawText(520, 230, mIsVictory ? "VICTORY" : "DEFEAT", mIsVictory ? GetColor(245, 220, 90) : GetColor(230, 80, 80));

    int firstItem = mIsVictory ? Title : Retry;
    for (int item = firstItem; item < ItemCount; ++item)
    {
        int y = getItemY(item);
        bool selected = mSelectedItem == item;
        int color = selected ? GetColor(120, 210, 255) : GetColor(215, 225, 235);
        if (selected)
        {
            /// 選択中の項目は背景帯とカーソルで強調する。
            DrawBox(ItemX - 28, y - 10, ItemX + ItemWidth, y + ItemHeight, GetColor(28, 44, 58), TRUE);
            drawText(ItemX - 54, y, ">", GetColor(120, 210, 255));
        }

        drawText(ItemX, y, item == Retry ? "RETRY" : "確認", color);
    }

    int hintColor = GetColor(215, 225, 235);
    bool usePad = InputManager::instance()->isLastInputPad();
    if (usePad)
    {
        /// 最後の入力デバイスに合わせて、パッド用またはキーボード用の操作ヒントを出す。
        if (!mIsVictory)
        {
            drawPadHint(mDpadUpGraphHandle, 430, 560, "選択", hintColor);
            drawPadHint(mDpadDownGraphHandle, 540, 560, "選択", hintColor);
        }
        drawPadHint(mAGraphHandle, 660, 560, "決定", hintColor);
    }
    else
    {
        if (!mIsVictory)
        {
            drawKeyHintBox(390, 560, "W/S", "選択", hintColor);
        }
        drawKeyHintBox(580, 560, "Enter", "決定", hintColor);
    }
}

void ResultScene::moveSelection(int direction)
{
    /// Retry と Title の 2 項目を循環選択する。
    mSelectedItem += direction;
    if (mSelectedItem < Retry)
    {
        mSelectedItem = Title;
    }
    else if (mSelectedItem > Title)
    {
        mSelectedItem = Retry;
    }
}

void ResultScene::decideSelected()
{
    if (!mIsVictory && mSelectedItem == Retry)
    {
        /// VampireScene から敗北した場合は VampireScene へ、通常は MutantScene へリトライする。
        if (mRetryVampireScene)
        {
            SceneManager::instance()->changeScene<VampireScene>(mPlayerType);
            return;
        }
        SceneManager::instance()->changeScene<MutantScene>(mPlayerType);
        return;
    }

    SceneManager::instance()->changeScene<TitleScene>();
}

void ResultScene::drawText(int x, int y, const char* text, int color) const
{
    if (mFontHandle != -1)
    {
        /// 文字は影を先に描き、暗い背景上で読みやすくする。
        DrawStringToHandle(x + 2, y + 2, text, GetColor(8, 24, 44), mFontHandle);
        DrawStringToHandle(x, y, text, color, mFontHandle);
    }
    else
    {
        DrawString(x, y, text, color);
    }
}

void ResultScene::drawPadHint(int handle, int x, int y, const char* text, int color) const
{
    if (handle != -1)
    {
        /// パッドヒントはアイコン画像と説明テキストを横並びで表示する。
        DrawExtendGraph(x, y, x + 30, y + 30, handle, TRUE);
    }
    drawText(x + 38, y - 1, text, color);
}

void ResultScene::drawKeyHintBox(int x, int y, const char* key, const char* text, int color) const
{
    int keyWidth = mFontHandle != -1
        ? GetDrawStringWidthToHandle(key, static_cast<int>(strlen(key)), mFontHandle) + 22
        : GetDrawStringWidth(key, static_cast<int>(strlen(key))) + 22;
    if (keyWidth < 50) keyWidth = 50;
    /// キー名の幅に合わせて枠を伸ばし、長いキー名でも文字がはみ出さないようにする。
    DrawBox(x, y, x + keyWidth, y + 34, GetColor(18, 24, 32), TRUE);
    DrawBox(x, y, x + keyWidth, y + 34, color, FALSE);
    drawText(x + 11, y, key, color);
    drawText(x + keyWidth + 10, y, text, color);
}
int ResultScene::getMouseItem(int x, int y) const
{
    int firstItem = mIsVictory ? Title : Retry;
    for (int item = firstItem; item < ItemCount; ++item)
    {
        /// 表示されている項目だけをマウス判定対象にする。
        int itemY = getItemY(item);
        if (x >= ItemX - 28 && x <= ItemX + ItemWidth && y >= itemY - 10 && y <= itemY + ItemHeight)
        {
            return item;
        }
    }

    return -1;
}

int ResultScene::getItemY(int item) const
{
    return item == Retry ? RetryY : TitleY;
}
