/**
 * @file
 * @brief SettingScene の UI 描画処理を定義する。
 */

#include "setting_scene.h"
#include "setting_scene_internal.h"
#include "game_settings.h"
#include "DxLib.h"

#include <cstdio>
#include <cstring>

using namespace SettingSceneInternal;

void SettingScene::drawItem(int item, int y, const char* label, const char* value) const
{
    /// 入力重複中は戻る項目を無効表示にして、修正を促す。
    bool disabled = item == Back && hasBindingConflict();
    bool selected = mSelectedItem == item && !disabled;
    int color = disabled ? GetColor(95, 100, 110) : (selected ? GetColor(120, 210, 255) : GetColor(210, 218, 230));
    if (selected)
    {
        DrawBox(ItemX - 26, y - 8, ValueX + 210, y + 32, GetColor(28, 44, 58), TRUE);
        DrawString(ItemX - 54, y, ">", GetColor(120, 210, 255));
    }
    if (item != Back)
    {
        DrawString(ItemX, y, label, color);
    }
    if (item == Back)
    {
        constexpr int HintX = ItemX;
        /// 戻る行には現在選択中の項目に応じた操作ヒントを表示する。
        if (isConfirmItem(mSelectedItem))
        {
            drawButtonHint(mAGraphHandle, HintX, y - 6, "決定", color);
            drawButtonHint(mBGraphHandle, HintX + 120, y - 6, "戻る", color);
        }
        else if (isResettableItem(mSelectedItem))
        {
            drawButtonHint(mBGraphHandle, HintX, y - 6, "戻る", color);
            drawButtonHint(mYGraphHandle, HintX + 120, y - 6, "初期化", color);
        }
        else
        {
            drawButtonHint(mBGraphHandle, HintX, y - 6, "戻る", color);
        }
        return;
    }
    DrawString(ValueX, y, value, color);
}
void SettingScene::drawButtonHint(int handle, int x, int y, const char* text, int color) const
{
    /// パッド操作の説明はボタン画像と文字を横並びで描く。
    if (handle != -1)
    {
        DrawExtendGraph(x, y, x + 30, y + 30, handle, TRUE);
    }
    DrawString(x + 38, y + 6, text, color);
}
void SettingScene::drawWaitingPrompt() const
{
    /// binding 変更中は通常 UI より前面に確認枠を出す。
    DrawBox(300, 250, 980, 420, GetColor(8, 12, 18), TRUE);
    DrawBox(300, 250, 980, 420, GetColor(120, 210, 255), FALSE);
    DrawString(430, 310, "割り当てるボタンを押してください", GetColor(235, 242, 255));
    DrawString(450, 355, mConfigMode == ConfigController ? "Xbox コントローラー" : "キーボード または マウス", GetColor(180, 190, 205));
}
void SettingScene::drawConflictPrompt() const
{
    /// 入力重複を短時間だけ強調表示する。
    DrawBox(330, 515, 950, 570, GetColor(52, 22, 26), TRUE);
    DrawBox(330, 515, 950, 570, GetColor(255, 90, 90), FALSE);
    DrawString(395, 535, "入力が重複しています", GetColor(255, 150, 150));
}

void SettingScene::drawPageTabs() const
{
    /// LB/RB とタブを同時に表示し、パッドでもページ移動できることを示す。
    if (mLbGraphHandle != -1) DrawExtendGraph(135, 112, 185, 150, mLbGraphHandle, TRUE);
    if (mRbGraphHandle != -1) DrawExtendGraph(1065, 112, 1115, 150, mRbGraphHandle, TRUE);
    for (int page = 0; page < SettingPageCount; ++page)
    {
        drawPageTab(page, TabStartX + TabSpacing * page, pageLabel(page));
    }
}

void SettingScene::drawPageTab(int page, int x, const char* label) const
{
    bool selected = mCurrentPage == page;
    int color = selected ? GetColor(120, 210, 255) : GetColor(180, 190, 205);
    DrawBox(x - 12, TabY - 8, x + TabWidth, TabY + TabHeight, selected ? GetColor(28, 44, 58) : GetColor(18, 24, 32), TRUE);
    if (page == KeyboardMousePage)
    {
        DrawString(x, TabY - 4, "キーボード・", color);
        DrawString(x, TabY + 14, "マウス設定", color);
        return;
    }
    DrawString(x, TabY, label, color);
}
void SettingScene::drawControllerGuide() const
{
    /// コントローラーページでは、実際の配置を画像と線で説明する。
    int lineColor = GetColor(95, 185, 245);
    int textColor = GetColor(210, 225, 240);
    auto drawIcon = [](int handle, int x, int y, int boxW, int boxH)
    {
        /// アイコンは指定枠内にアスペクト比を保って収める。
        if (handle == -1)
        {
            return;
        }

        int graphW = 0;
        int graphH = 0;
        GetGraphSize(handle, &graphW, &graphH);
        if (graphW <= 0 || graphH <= 0)
        {
            return;
        }

        int drawW = boxW;
        int drawH = graphH * boxW / graphW;
        if (drawH > boxH)
        {
            drawH = boxH;
            drawW = graphW * boxH / graphH;
        }

        int drawX = x + (boxW - drawW) / 2;
        int drawY = y + (boxH - drawH) / 2;
        DrawExtendGraph(drawX, drawY, drawX + drawW, drawY + drawH, handle, TRUE);
    };

    constexpr int controllerX = 350;
    constexpr int controllerY = 245;
    constexpr int controllerW = 590;
    constexpr int controllerH = 395;

    if (mControllerGraphHandle != -1)
    {
        DrawRectExtendGraph(
            controllerX,
            controllerY,
            controllerX + controllerW,
            controllerY + controllerH,
            1030,
            1120,
            2260,
            1480,
            mControllerGraphHandle,
            TRUE);
    }

    drawIcon(mLtGraphHandle, 125, 318, 42, 42);
    DrawString(190, 325, "ガード/魔法ビーム", textColor);
    DrawLine(330, 337, 512, 337, lineColor);
    DrawLine(512, 337, 512, 278, lineColor);
    drawIcon(mLeftStickGraphHandle, 125, 378, 42, 42);
    DrawString(190, 390, "移動", textColor);
    DrawLine(330, 402, 515, 402, lineColor);
    DrawLine(515, 402, 515, 392, lineColor);

    drawIcon(mRtGraphHandle, 950, 273, 42, 42);
    DrawString(1015, 280, "攻撃/魔法攻撃", textColor);
    DrawLine(940, 292, 740, 292, lineColor);
    DrawLine(740, 292, 740, 278, lineColor);
    drawIcon(mRbGraphHandle, 950, 318, 42, 42);
    DrawString(1015, 325, "ロックオン", textColor);
    DrawLine(940, 337, 740, 337, lineColor);
    DrawLine(740, 337, 740, 278, lineColor);
    drawIcon(mYGraphHandle, 950, 363, 42, 42);
    DrawString(1015, 370, "カウンター/魔法杖", textColor);
    DrawLine(940, 382, 785, 382, lineColor);
    DrawLine(785, 382, 785, 330, lineColor);
    drawIcon(mBGraphHandle, 950, 408, 42, 42);
    DrawString(1015, 415, "回避", textColor);
    DrawLine(940, 427, 825, 427, lineColor);
    DrawLine(825, 427, 825, 370, lineColor);
    drawIcon(mAGraphHandle, 950, 453, 42, 42);
    DrawString(1015, 460, "調べる", textColor);
    DrawLine(940, 472, 785, 472, lineColor);
    DrawLine(785, 472, 785, 408, lineColor);
    drawIcon(mRightStickGraphHandle, 950, 493, 42, 42);
    DrawString(1015, 505, "カメラ", textColor);
    DrawLine(940, 517, 700, 517, lineColor);
    DrawLine(700, 517, 700, 462, lineColor);
    drawIcon(mStartGraphHandle, 950, 543, 42, 42);
    DrawString(1015, 550, "ポーズ", textColor);
    DrawLine(940, 562, 690, 562, lineColor);
    DrawLine(690, 562, 690, 356, lineColor);
}
void SettingScene::drawBindingList() const
{
    /// 現在の入力種別に合わせて、全アクションの割り当て一覧を描く。
    for (int action = 0; action < GameActionCount; ++action)
    {
        InputBinding binding = mConfigMode == ConfigController
            ? GameSettings::instance()->getPadBinding(action)
            : GameSettings::instance()->getInputBinding(0, action);
        int y = KeyListY + KeyListSpacing * action;
        bool selected = mSelectedItem == action;
        bool conflict = mConfigMode == ConfigController ? isPadBindingConflictAction(action) : isKeyboardBindingConflictAction(action);
        /// 重複している binding は赤く表示し、保存前に気付けるようにする。
        int labelColor = selected ? GetColor(120, 210, 255) : GetColor(170, 180, 195);
        int keyColor = conflict ? GetColor(255, 90, 90) : (selected ? GetColor(120, 210, 255) : GetColor(210, 218, 230));
        if (selected)
        {
            DrawBox(ItemX - 26, y - 6, ValueX + 210, y + 30, GetColor(28, 44, 58), TRUE);
            DrawString(ItemX - 54, y, ">", GetColor(120, 210, 255));
        }
        DrawString(ItemX, y, actionLabel(action), labelColor);
        if (!(mIsWaitingBinding && action == mWaitingAction))
        {
            drawBindingValue(action, binding, ValueX, y - 5, keyColor, conflict);
        }
    }

    constexpr int HintX = ItemX;
    int hintColor = GetColor(180, 190, 205);
    if (mIsWaitingBinding)
    {
        drawButtonHint(mStartGraphHandle, HintX, 610, "戻る", hintColor);
        return;
    }
    drawButtonHint(mAGraphHandle, HintX, 610, "決定", hintColor);
    drawButtonHint(mBGraphHandle, HintX + 120, 610, "戻る", hintColor);
    drawButtonHint(mYGraphHandle, HintX + 240, 610, "初期化", hintColor);
}
void SettingScene::drawBindingValue(int action, InputBinding binding, int x, int y, int color, bool conflict) const
{
    if (mConfigMode == ConfigController)
    {
        /// 移動アクションは左スティック固定のため、割り当て値ではなく薄いスティック画像を描く。
        bool fixedStick = action >= GameActionMoveForward && action <= GameActionMoveRight;
        int handle = fixedStick ? mLeftStickGraphHandle : getPadBindingGraphHandle(binding);
        if (handle != -1)
        {
            if (fixedStick)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
            }
            DrawExtendGraph(x, y - 2, x + 34, y + 32, handle, TRUE);
            if (fixedStick)
            {
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            if (conflict)
            {
                DrawBox(x - 2, y - 4, x + 36, y + 34, GetColor(255, 90, 90), FALSE);
            }
            return;
        }
    }

    /// キーボード/マウス binding は文字幅に合わせた枠で表示する。
    char keyText[32];
    GameSettings::instance()->getInputBindingText(binding, keyText, sizeof(keyText));
    int textWidth = GetDrawStringWidth(keyText, static_cast<int>(strlen(keyText)));
    int boxWidth = textWidth + 24;
    if (boxWidth < 44) boxWidth = 44;
    DrawBox(x, y - 2, x + boxWidth, y + 30, conflict ? GetColor(58, 18, 22) : GetColor(18, 24, 32), TRUE);
    DrawBox(x, y - 2, x + boxWidth, y + 30, color, FALSE);
    DrawString(x + 12, y + 5, keyText, color);
}
void SettingScene::makeValueText(int item, char* value, int valueSize) const
{
    /// 通常設定項目の現在値を、画面表示用の短い文字列へ変換する。
    GameSettings* settings = GameSettings::instance();
    switch (item)
    {
    case MasterVolume: sprintf_s(value, valueSize, "< %d >", settings->getMasterVolume()); break;
    case BgmVolume: sprintf_s(value, valueSize, "< %d >", settings->getBgmVolume()); break;
    case SeVolume: sprintf_s(value, valueSize, "< %d >", settings->getSeVolume()); break;
    case MouseSensitivity: sprintf_s(value, valueSize, "< %d >", settings->getMouseSensitivity()); break;
    case InvertMouseY: sprintf_s(value, valueSize, "< %s >", boolText(settings->isInvertMouseY())); break;
    case Fullscreen: sprintf_s(value, valueSize, "< %s >", boolText(settings->isFullscreen())); break;
    case AutoLockTarget: sprintf_s(value, valueSize, "< %s >", boolText(settings->isAutoLockTarget())); break;
    case ButtonConfig: sprintf_s(value, valueSize, "ボタン配置"); break;
    case KeyConfig: sprintf_s(value, valueSize, "キー配置"); break;
    default: value[0] = '\0'; break;
    }
}
