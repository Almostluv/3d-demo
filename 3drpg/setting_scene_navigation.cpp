/**
 * @file
 * @brief SettingScene のページ移動と項目選択を定義する。
 */

#include "setting_scene.h"
#include "setting_scene_internal.h"
#include "audio_manager.h"
#include "game_settings.h"
#include "sound_resource.h"
#include "DxLib.h"

#include <cstring>

using namespace SettingSceneInternal;

void SettingScene::moveSelection(int direction)
{
    /// 現在ページ内の項目だけを循環選択する。
    int count = getPageItemCount();
    int index = getItemRow(mSelectedItem);
    if (index < 0) index = 0;
    index += direction;
    if (index < 0) index = count - 1;
    else if (index >= count) index = 0;
    mSelectedItem = getPageItem(index);
    AudioManager::instance()->playSe(SoundResource::Se::ValueChange);
}
bool SettingScene::isConfirmItem(int item) const
{
    return item == ButtonConfig || item == KeyConfig;
}
bool SettingScene::isResettableItem(int item) const
{
    return item == MasterVolume || item == BgmVolume || item == SeVolume || item == MouseSensitivity || item == InvertMouseY || item == Fullscreen || item == AutoLockTarget;
}
void SettingScene::switchPage(int page)
{
    if (mCurrentPage == page) return;
    /// ページ切替時は設定モードを抜け、先頭項目を選び直す。
    AudioManager::instance()->playSe(SoundResource::Se::ValueChange);
    mCurrentPage = page;
    mConfigMode = ConfigNone;
    mSelectedItem = getPageItem(0);
}
bool SettingScene::isInsideBackButton(int x, int y) const
{
    if (hasBindingConflict()) return false;
    return x >= ItemLeft && x <= ItemRight && y >= BackY + ItemTopOffset && y <= BackY + ItemBottomOffset;
}

int SettingScene::getPageItemCount() const
{
    /// 通常ページと binding 編集ページで、選択可能な行数を切り替える。
    if (mConfigMode != ConfigNone) return GameActionCount;
    switch (mCurrentPage)
    {
    case GamePage: return 2;
    case ScreenPage: return 2;
    case SoundPage: return 4;
    case ControllerPage: return 2;
    case KeyboardMousePage: return 4;
    default: return 1;
    }
}

int SettingScene::getPageItem(int index) const
{
    /// ページごとの表示項目を固定配列で管理し、描画と入力で共有する。
    static const int gameItems[] = { AutoLockTarget, Back };
    static const int screenItems[] = { Fullscreen, Back };
    static const int soundItems[] = { MasterVolume, BgmVolume, SeVolume, Back };
    static const int controllerItems[] = { ButtonConfig, Back };
    static const int keyboardItems[] = { KeyConfig, MouseSensitivity, InvertMouseY, Back };
    if (mConfigMode != ConfigNone) return index;
    switch (mCurrentPage)
    {
    case GamePage: return gameItems[index];
    case ScreenPage: return screenItems[index];
    case SoundPage: return soundItems[index];
    case ControllerPage: return controllerItems[index];
    case KeyboardMousePage: return keyboardItems[index];
    default: return Back;
    }
}
int SettingScene::getItemRow(int item) const
{
    if (mConfigMode != ConfigNone) return item >= 0 && item < GameActionCount ? item : -1;
    for (int i = 0; i < getPageItemCount(); ++i)
    {
        if (getPageItem(i) == item) return i;
    }
    return -1;
}
int SettingScene::getItemY(int item) const
{
    if (item == Back) return BackY;
    int row = getItemRow(item);
    return ItemStartY + ItemSpacing * row;
}
int SettingScene::getMouseItem(int x, int y) const
{
    /// 通常設定項目のクリック対象を、描画と同じ行矩形で判定する。
    for (int i = 0; i < getPageItemCount(); ++i)
    {
        int item = getPageItem(i);
        int itemY = getItemY(item);
        if (x >= ItemLeft && x <= ItemRight && y >= itemY + ItemTopOffset && y <= itemY + ItemBottomOffset) return item;
    }
    return -1;
}
int SettingScene::getMousePage(int x, int y) const
{
    /// タブ領域内のクリックから、切り替え先ページ番号を求める。
    if (y < TabY - 8 || y > TabY + TabHeight) return -1;
    for (int page = 0; page < SettingPageCount; ++page)
    {
        int tabX = TabStartX + TabSpacing * page;
        if (x >= tabX - 12 && x <= tabX + TabWidth) return page;
    }
    return -1;
}
int SettingScene::getMouseBindingAction(int x, int y) const
{
    /// binding 一覧モードでは、クリック位置から対象アクションを求める。
    if (mConfigMode == ConfigNone || x < ItemLeft || x > ItemRight) return -1;
    for (int action = 0; action < GameActionCount; ++action)
    {
        int actionY = KeyListY + KeyListSpacing * action;
        if (y >= actionY + ItemTopOffset && y <= actionY + ItemBottomOffset) return action;
    }
    return -1;
}
int SettingScene::getMouseAdjustDirection(int x, int y) const
{
    /// 値表示の左右矢印だけを調整クリックとして扱う。
    int item = getMouseItem(x, y);
    if (item != MasterVolume && item != BgmVolume && item != SeVolume && item != MouseSensitivity && item != Fullscreen && item != InvertMouseY && item != AutoLockTarget) return 0;
    int leftArrowWidth = GetDrawStringWidth("<", 1);
    if (x >= ValueX && x <= ValueX + leftArrowWidth + ArrowHitPadding) return -1;
    char value[32];
    makeValueText(item, value, sizeof(value));
    int valueWidth = GetDrawStringWidth(value, static_cast<int>(strlen(value)));
    int rightArrowWidth = GetDrawStringWidth(">", 1);
    int rightArrowLeft = ValueX + valueWidth - rightArrowWidth - ArrowHitPadding;
    int rightArrowRight = ValueX + valueWidth + ArrowHitPadding;
    if (x >= rightArrowLeft && x <= rightArrowRight) return 1;
    return 0;
}
