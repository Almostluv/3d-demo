/**
 * @file
 * @brief vampire_scene_pause.cpp 実装を定義する。
 */

#include "vampire_scene.h"
#include "game_config.h"
#include "input_manager.h"
#include "scene_manager.h"
#include "setting_scene.h"
#include "story_state.h"
#include "title_scene.h"
#include "DxLib.h"

#include <memory>

namespace
{
enum PauseItem
{
    Resume,
    Restart,
    Title,
    Setting,
    PauseItemCount
};

constexpr int PauseItemX = 500;
constexpr int PauseItemStartY = 250;
constexpr int PauseItemSpacing = 58;
constexpr int PauseItemWidth = 280;
constexpr int PauseItemHeight = 42;

void drawPauseText(int x, int y, const char* text, int color, int fontHandle)
{
    if (fontHandle != -1)
    {
        /// 専用フォントがある場合は薄い影を付け、背景暗転上でも読みやすくする。
        DrawStringToHandle(x + 2, y + 2, text, GetColor(8, 24, 44), fontHandle);
        DrawStringToHandle(x, y, text, color, fontHandle);
    }
    else
    {
        DrawString(x, y, text, color);
    }
}

const char* pauseItemText(int item)
{
    /// PauseItem の内部値を、ポーズメニュー用の表示文字列へ変換する。
    switch (item)
    {
    case Resume: return "ゲームに戻る";
    case Restart: return "リトライ";
    case Title: return "タイトルへ戻る";
    case Setting: return "設定";
    default: return "";
    }
}
}

void VampireScene::updatePauseMenu()
{
    InputManager* input = InputManager::instance();
    int mouseItem = getPauseMouseItem(input->getMouseScreenX(), input->getMouseScreenY());
    if (!input->isLastInputPad() && mouseItem >= 0)
    {
        /// 直近入力がマウスの場合だけ、カーソル位置で選択項目を更新する。
        mPauseSelectedItem = mouseItem;
    }

    if (input->isPush(KEY_INPUT_UP) || input->isPush(KEY_INPUT_W) || input->isPadPush(XINPUT_BUTTON_DPAD_UP))
    {
        /// キーボードとゲームパッドの上下入力を同じ選択移動へ集約する。
        movePauseSelection(-1);
    }

    if (input->isPush(KEY_INPUT_DOWN) || input->isPush(KEY_INPUT_S) || input->isPadPush(XINPUT_BUTTON_DPAD_DOWN))
    {
        movePauseSelection(1);
    }

    if (input->isPush(KEY_INPUT_RETURN) || input->isPush(KEY_INPUT_SPACE) ||
        input->isPadPush(XINPUT_BUTTON_A) || input->isPadPush(XINPUT_BUTTON_START) ||
        (input->isMousePush(MOUSE_INPUT_LEFT) && mouseItem >= 0))
    {
        /// 決定入力は現在選択中の項目、またはクリックされた項目を実行する。
        decidePauseSelection();
    }
}

void VampireScene::drawPauseMenu() const
{
    /// パッド操作中はカーソルを隠し、マウス操作へ戻った時だけ表示する。
    SetMouseDispFlag(InputManager::instance()->isLastInputPad() ? FALSE : TRUE);

    /// 戦闘画面を暗転させて、ポーズ中であることと選択肢を見やすくする。
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
    DrawBox(0, 0, kWindowWidth, kWindowHeight, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    drawPauseText(535, 165, "PAUSE", GetColor(235, 242, 255), mPauseFontHandle);

    for (int item = 0; item < PauseItemCount; ++item)
    {
        /// 選択中の項目だけ背景とカーソル記号を付ける。
        int y = getPauseItemY(item);
        bool selected = mPauseSelectedItem == item;
        int color = selected ? GetColor(120, 210, 255) : GetColor(215, 225, 235);

        if (selected)
        {
            DrawBox(PauseItemX - 28, y - 10, PauseItemX + PauseItemWidth, y + PauseItemHeight, GetColor(28, 44, 58), TRUE);
            drawPauseText(PauseItemX - 54, y, ">", GetColor(120, 210, 255), mPauseFontHandle);
        }

        drawPauseText(PauseItemX, y, pauseItemText(item), color, mPauseFontHandle);
    }
}

void VampireScene::movePauseSelection(int direction)
{
    mPauseSelectedItem += direction;
    if (mPauseSelectedItem < 0)
    {
        /// 先頭から上へ移動した場合は末尾へ回り込む。
        mPauseSelectedItem = PauseItemCount - 1;
    }
    else if (mPauseSelectedItem >= PauseItemCount)
    {
        /// 末尾から下へ移動した場合は先頭へ回り込む。
        mPauseSelectedItem = 0;
    }
}

void VampireScene::decidePauseSelection()
{
    /// 選択項目ごとに、ポーズ解除・再戦・タイトル遷移・設定オーバーレイを実行する。
    switch (mPauseSelectedItem)
    {
    case Resume:
        setPaused(false);
        break;
    case Restart:
        /// Vampire 戦をやり直すため、魔法ボス撃破フラグを戻して同じシーンを作り直す。
        StoryState::instance()->setMagicBossCleared(false);
        SceneManager::instance()->changeScene<VampireScene>(mPlayerType);
        break;
    case Title:
        SceneManager::instance()->changeScene<TitleScene>();
        break;
    case Setting:
        /// 設定画面は別シーンへ完全遷移せず、ポーズ画面上のオーバーレイとして開く。
        mSettingScene = std::make_unique<SettingScene>(SettingReturnOverlay);
        break;
    default:
        break;
    }
}

int VampireScene::getPauseMouseItem(int x, int y) const
{
    for (int item = 0; item < PauseItemCount; ++item)
    {
        /// 描画時と同じ矩形を使い、マウスホバーとクリック対象を判定する。
        int itemY = getPauseItemY(item);
        if (x >= PauseItemX - 28 && x <= PauseItemX + PauseItemWidth && y >= itemY - 10 && y <= itemY + PauseItemHeight)
        {
            return item;
        }
    }

    return -1;
}

int VampireScene::getPauseItemY(int item) const
{
    /// ポーズ項目は等間隔に縦並びで配置する。
    return PauseItemStartY + PauseItemSpacing * item;
}

void VampireScene::setPaused(bool paused)
{
    /// ポーズ中はマウス固定を解除し、メニュー操作を優先する。
    mIsPaused = paused;
    InputManager::instance()->setMouseLock(!paused);
    SetMouseDispFlag(paused && !InputManager::instance()->isLastInputPad() ? TRUE : FALSE);
    if (!paused)
    {
        /// 戦闘へ戻る時はカーソルを中央へ戻し、復帰直後のカメラ跳ねを抑える。
        SetMousePoint(kWindowWidth / 2, kWindowHeight / 2);
    }
}
