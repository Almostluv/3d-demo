/**
 * @file
 * @brief mutant_scene_pause.cpp 実装を定義する。
 */

#include "mutant_scene.h"
#include "game_config.h"
#include "scene_manager.h"
#include "setting_scene.h"
#include "title_scene.h"
#include "story_state.h"
#include "DxLib.h"

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
        /// 専用フォントでは影を付け、暗転背景上でも読みやすくする。
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
    /// PauseItem の内部値をメニュー表示用の文字列へ変換する。
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

void MutantScene::updatePauseMenu()
{
    InputManager* input = InputManager::instance();
    int mouseItem = getPauseMouseItem(input->getMouseScreenX(), input->getMouseScreenY());
    if (!input->isLastInputPad() && mouseItem >= 0)
    {
        /// マウス操作中はカーソル下の項目を選択状態にする。
        mPauseSelectedItem = mouseItem;
    }

    if (input->isPush(KEY_INPUT_UP) || input->isPush(KEY_INPUT_W) || input->isPadPush(XINPUT_BUTTON_DPAD_UP))
    {
        /// キーボードとパッドの上下入力を同じ選択移動へまとめる。
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
        /// 決定入力または項目クリックで現在の選択を実行する。
        decidePauseSelection();
    }
}

void MutantScene::drawPauseMenu() const
{
    /// パッド操作中はマウスカーソルを隠し、操作デバイスに合わせた表示にする。
    SetMouseDispFlag(InputManager::instance()->isLastInputPad() ? FALSE : TRUE);

    /// 戦闘画面を暗転させ、ポーズメニューの視認性を上げる。
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
    DrawBox(0, 0, kWindowWidth, kWindowHeight, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    drawPauseText(535, 165, "PAUSE", GetColor(235, 242, 255), mPauseFontHandle);

    for (int item = 0; item < PauseItemCount; ++item)
    {
        /// 選択中の項目だけ背景とカーソル記号を描く。
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

void MutantScene::movePauseSelection(int direction)
{
    mPauseSelectedItem += direction;
    if (mPauseSelectedItem < 0)
    {
        /// 先頭から上へ移動した時は末尾へ回り込む。
        mPauseSelectedItem = PauseItemCount - 1;
    }
    else if (mPauseSelectedItem >= PauseItemCount)
    {
        /// 末尾から下へ移動した時は先頭へ回り込む。
        mPauseSelectedItem = 0;
    }
}

void MutantScene::decidePauseSelection()
{
    /// 選択項目に応じて、復帰・再戦・タイトル・設定を実行する。
    switch (mPauseSelectedItem)
    {
    case Resume:
        setPaused(false);
        break;
    case Restart:
        /// Mutant 戦をやり直すため、FinalBoss クリア状態を戻してシーンを作り直す。
        StoryState::instance()->setFinalBossCleared(false);
        SceneManager::instance()->changeScene<MutantScene>(mPlayerType);
        break;
    case Title:
        SceneManager::instance()->changeScene<TitleScene>();
        break;
    case Setting:
        /// 設定はシーン遷移ではなく、ポーズ中のオーバーレイとして開く。
        mPauseSettingScene = std::make_unique<SettingScene>(SettingReturnOverlay);
        break;
    default:
        break;
    }
}

int MutantScene::getPauseMouseItem(int x, int y) const
{
    for (int item = 0; item < PauseItemCount; ++item)
    {
        /// 描画と同じ矩形でホバー・クリック対象を判定する。
        int itemY = getPauseItemY(item);
        if (x >= PauseItemX - 28 && x <= PauseItemX + PauseItemWidth && y >= itemY - 10 && y <= itemY + PauseItemHeight)
        {
            return item;
        }
    }

    return -1;
}

int MutantScene::getPauseItemY(int item) const
{
    /// メニュー項目は固定間隔で縦に並べる。
    return PauseItemStartY + PauseItemSpacing * item;
}

void MutantScene::setPaused(bool paused)
{
    /// ポーズ中はマウス固定を解除し、メニュー操作を優先する。
    mIsPaused = paused;
    InputManager::instance()->setMouseLock(!paused);
    SetMouseDispFlag(paused && !InputManager::instance()->isLastInputPad() ? TRUE : FALSE);
    if (!paused)
    {
        /// 戦闘へ戻る時はカーソルを中央に戻し、復帰直後のカメラ跳ねを抑える。
        SetMousePoint(kWindowWidth / 2, kWindowHeight / 2);
    }
}
