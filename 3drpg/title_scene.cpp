/**
 * @file
 * @brief title_scene.cpp 実装を定義する。
 */

#include "title_scene.h"
#include "audio_manager.h"
#include "chara_select_scene.h"
#include "game_manager.h"
#include "game_config.h"
#include "input_manager.h"
#include "loading_scene.h"
#include "resource_manager.h"
#include "scene_manager.h"
#include "setting_scene.h"
#include "sound_resource.h"
#include "DxLib.h"
#include <cmath>

namespace
{
constexpr int PlaceholderColorTop = 18;
constexpr int PlaceholderColorBottom = 38;
constexpr const char* TitleGraph = "Resource/GameUI/menu_title.png";
constexpr const char* BackgroundGraph = "Resource/GameUI/title.png";
constexpr const char* StartGraph = "Resource/GameUI/menu_start.png";
constexpr const char* SettingGraph = "Resource/GameUI/menu_setting.png";
constexpr const char* ExitGraph = "Resource/GameUI/menu_exit.png";
constexpr const char* PointerGraph = "Resource/GameUI/title_pointer.png";
constexpr int TitleUiWidth = 512;
constexpr int TitleUiHeight = 342;
constexpr int MenuUiWidth = 342;
constexpr int MenuUiHeight = 114;
constexpr int MenuUiX = (kWindowWidth - MenuUiWidth) / 2;
constexpr int StartUiY = 320;
constexpr int SettingUiY = 440;
constexpr int ExitUiY = 560;
constexpr int MenuHitLeftPadding = 40;
constexpr int MenuHitRightPadding = 40;
constexpr int MenuHitTopPadding = 28;
constexpr int MenuHitBottomPadding = 28;
constexpr int MenuStart = 0;
constexpr int MenuSetting = 1;
constexpr int MenuExit = 2;
constexpr int MenuCount = 3;
constexpr float MenuSelectedScale = 1.06f; ///< 選択中メニュー項目の拡大率。
constexpr float MenuPressedScale = 0.96f; ///< 押下中メニュー項目の縮小率。
constexpr float MenuClickEffectDuration = 0.18f; ///< メニュークリック演出の表示時間。単位は seconds。
constexpr int MenuPointerGap = 18;
constexpr int MenuPointerWidth = 64;
constexpr int MenuPointerHeight = 48;
constexpr float MenuPointerMoveAmplitude = 6.0f; ///< メニューポインタ横揺れの振幅。
constexpr float MenuPointerMoveSpeed = 4.0f; ///< メニューポインタ横揺れの速度。
}

TitleScene::TitleScene()
    : mTitleGraphHandle(-1)
    , mBackgroundGraphHandle(-1)
    , mStartGraphHandle(-1)
    , mSettingGraphHandle(-1)
    , mExitGraphHandle(-1)
    , mPointerGraphHandle(-1)
    , mSelectedMenu(MenuStart)
    , mClickedMenu(-1)
    , mPendingMenu(-1)
    , mLastMouseX(-1)
    , mLastMouseY(-1)
    , mClickEffectTimer(0.0f)
    , mClickActionTimer(0.0f)
    , mPointerAnimTimer(0.0f)
{
    /// タイトル画面ではマウス操作を許可し、メニュー画像と BGM を読み込む。
    InputManager::instance()->setMouseLock(false);
    SetMouseDispFlag(TRUE);
    mTitleGraphHandle = ResourceManager::instance()->getGraph(TitleGraph);
    mBackgroundGraphHandle = ResourceManager::instance()->getGraph(BackgroundGraph);
    mStartGraphHandle = ResourceManager::instance()->getGraph(StartGraph);
    mSettingGraphHandle = ResourceManager::instance()->getGraph(SettingGraph);
    mExitGraphHandle = ResourceManager::instance()->getGraph(ExitGraph);
    mPointerGraphHandle = ResourceManager::instance()->getGraph(PointerGraph);
    AudioManager::instance()->playBgm(SoundResource::Bgm::TitleScene);
}

void TitleScene::update(float deltaTime)
{
    InputManager* input = InputManager::instance();
    mPointerAnimTimer += deltaTime;

    /// マウス位置からホバー中のメニューを求め、最後のカーソル位置も保存する。
    int mouseX = input->getMouseScreenX();
    int mouseY = input->getMouseScreenY();
    int hoveredMenu = getHoveredMenu(mouseX, mouseY);
    bool mouseMoved = mouseX != mLastMouseX || mouseY != mLastMouseY;
    mLastMouseX = mouseX;
    mLastMouseY = mouseY;

    if (mClickEffectTimer > 0.0f)
    {
        /// 決定演出が終わったら、押下中表示を解除する。
        mClickEffectTimer -= deltaTime;
        if (mClickEffectTimer <= 0.0f)
        {
            mClickEffectTimer = 0.0f;
            mClickedMenu = -1;
        }
    }

    if (mPendingMenu != -1)
    {
        /// クリック演出を見せてから実際のシーン遷移や終了処理を行う。
        mClickActionTimer -= deltaTime;
        if (mClickActionTimer <= 0.0f)
        {
            int menu = mPendingMenu;
            mPendingMenu = -1;
            executeMenuAction(menu);
        }
        return;
    }

    if (mouseMoved && hoveredMenu != -1 && hoveredMenu != mSelectedMenu)
    {
        /// マウス移動で項目が変わった時だけ選択 SE を鳴らす。
        mSelectedMenu = hoveredMenu;
        AudioManager::instance()->playSe(SoundResource::Se::CharacterSelect);
    }

    if (input->isPush(KEY_INPUT_W) || input->isPadPush(XINPUT_BUTTON_DPAD_UP))
    {
        /// キーボード/パッド操作ではメニューを循環選択する。
        mSelectedMenu = (mSelectedMenu + MenuCount - 1) % MenuCount;
        AudioManager::instance()->playSe(SoundResource::Se::CharacterSelect);
    }

    if (input->isPush(KEY_INPUT_S) || input->isPadPush(XINPUT_BUTTON_DPAD_DOWN))
    {
        mSelectedMenu = (mSelectedMenu + 1) % MenuCount;
        AudioManager::instance()->playSe(SoundResource::Se::CharacterSelect);
    }

    if (input->isPush(KEY_INPUT_RETURN) || input->isPush(KEY_INPUT_SPACE) ||
        input->isPadPush(XINPUT_BUTTON_A) || input->isPadPush(XINPUT_BUTTON_START))
    {
        startMenuAction(mSelectedMenu);
        return;
    }

    if (input->isMousePush(MOUSE_INPUT_LEFT) && hoveredMenu != -1)
    {
        startMenuAction(hoveredMenu);
        return;
    }

    if (input->isPush(KEY_INPUT_ESCAPE) || input->isPadPush(XINPUT_BUTTON_B))
    {
        startMenuAction(MenuExit);
    }
}

void TitleScene::draw()
{
    /// 最後に使った入力デバイスに合わせてマウスカーソル表示を切り替える。
    SetMouseDispFlag(InputManager::instance()->isLastInputPad() ? FALSE : TRUE);
    if (mBackgroundGraphHandle != -1)
    {
        DrawExtendGraph(0, 0, kWindowWidth, kWindowHeight, mBackgroundGraphHandle, TRUE);
    }
    else
    {
        /// 背景画像がない場合でもタイトル画面として最低限見えるプレースホルダーを描く。
        DrawBox(0, 0, kWindowWidth, kWindowHeight, GetColor(PlaceholderColorTop, 24, PlaceholderColorBottom), TRUE);
        DrawBox(0, 0, kWindowWidth, kWindowHeight / 2, GetColor(25, 34, 52), TRUE);
        DrawBox(0, kWindowHeight / 2, kWindowWidth, kWindowHeight, GetColor(12, 16, 26), TRUE);
    }

    if (mTitleGraphHandle != -1)
    {
        int titleX = (kWindowWidth - TitleUiWidth) / 2;
        DrawExtendGraph(titleX, 0, titleX + TitleUiWidth, TitleUiHeight, mTitleGraphHandle, TRUE);
    }

    drawMenuButton(mStartGraphHandle, MenuStart, StartUiY);
    drawMenuButton(mSettingGraphHandle, MenuSetting, SettingUiY);
    drawMenuButton(mExitGraphHandle, MenuExit, ExitUiY);
}

bool TitleScene::isInsideStartButton(int x, int y) const
{
    return x >= MenuUiX + MenuHitLeftPadding && x <= MenuUiX + MenuUiWidth - MenuHitRightPadding &&
        y >= StartUiY + MenuHitTopPadding && y <= StartUiY + MenuUiHeight - MenuHitBottomPadding;
}

bool TitleScene::isInsideSettingButton(int x, int y) const
{
    return x >= MenuUiX + MenuHitLeftPadding && x <= MenuUiX + MenuUiWidth - MenuHitRightPadding &&
        y >= SettingUiY + MenuHitTopPadding && y <= SettingUiY + MenuUiHeight - MenuHitBottomPadding;
}

bool TitleScene::isInsideExitButton(int x, int y) const
{
    return x >= MenuUiX + MenuHitLeftPadding && x <= MenuUiX + MenuUiWidth - MenuHitRightPadding &&
        y >= ExitUiY + MenuHitTopPadding && y <= ExitUiY + MenuUiHeight - MenuHitBottomPadding;
}

int TitleScene::getHoveredMenu(int x, int y) const
{
    if (isInsideStartButton(x, y)) return MenuStart;
    if (isInsideSettingButton(x, y)) return MenuSetting;
    if (isInsideExitButton(x, y)) return MenuExit;
    return -1;
}

void TitleScene::startMenuAction(int menuIndex)
{
    /// 選択決定時は即遷移せず、押下演出用の pending 状態へ入れる。
    AudioManager::instance()->playSe(SoundResource::Se::Confirm);
    mSelectedMenu = menuIndex;
    mClickedMenu = menuIndex;
    mPendingMenu = menuIndex;
    mClickEffectTimer = MenuClickEffectDuration;
    mClickActionTimer = MenuClickEffectDuration;
}

void TitleScene::executeMenuAction(int menuIndex)
{
    if (menuIndex == MenuStart)
    {
        /// Start は直接戦闘ではなく、先読み用 LoadingScene を挟む。
        SceneManager::instance()->changeScene<LoadingScene>();
        return;
    }

    if (menuIndex == MenuSetting)
    {
        /// 設定画面から戻る先を Title として渡す。
        SceneManager::instance()->changeScene<SettingScene>(SettingReturnTitle);
        return;
    }

    GameManager::instance()->requestExit();
}

void TitleScene::drawMenuButton(int graphHandle, int menuIndex, int y) const
{
    if (graphHandle == -1)
    {
        return;
    }

    bool selected = mSelectedMenu == menuIndex;
    bool clicked = mClickedMenu == menuIndex && mClickEffectTimer > 0.0f;
    /// 選択中は少し拡大、押下中は少し縮小して入力フィードバックを出す。
    float scale = clicked ? MenuPressedScale : selected ? MenuSelectedScale : 1.0f;
    int width = static_cast<int>(MenuUiWidth * scale);
    int height = static_cast<int>(MenuUiHeight * scale);
    int x = MenuUiX + (MenuUiWidth - width) / 2;
    int drawY = y + (MenuUiHeight - height) / 2;

    if (clicked)
    {
        /// クリック直後だけ外枠を広げながら薄くし、決定感を出す。
        float rate = 1.0f - (mClickEffectTimer / MenuClickEffectDuration);
        int expand = static_cast<int>(26.0f * rate);
        int alpha = static_cast<int>(190.0f * (1.0f - rate));
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
        DrawBox(x - expand, drawY - expand, x + width + expand, drawY + height + expand, GetColor(255, 245, 180), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    if (selected && mPointerGraphHandle != -1)
    {
        /// 選択項目の左右にポインタを表示し、時間で少し横揺れさせる。
        int pointerCenterY = drawY + height / 2;
        int pointerY = pointerCenterY - MenuPointerHeight / 2;
        int move = static_cast<int>((sinf(mPointerAnimTimer * MenuPointerMoveSpeed) + 1.0f) * 0.5f * MenuPointerMoveAmplitude);
        DrawGraph(x - MenuPointerGap - MenuPointerWidth + move, pointerY, mPointerGraphHandle, TRUE);
        DrawTurnGraph(x + width + MenuPointerGap - move, pointerY, mPointerGraphHandle, TRUE);
    }

    DrawExtendGraph(x, drawY, x + width, drawY + height, graphHandle, TRUE);
}
