/**
 * @file
 * @brief SettingScene のシーン入口と基本更新を定義する。
 */

#include "setting_scene.h"
#include "setting_scene_internal.h"
#include "audio_manager.h"
#include "game_config.h"
#include "game_settings.h"
#include "input_manager.h"
#include "resource_manager.h"
#include "scene_manager.h"
#include "title_scene.h"
#include "chara_select_scene.h"
#include "sound_resource.h"
#include "DxLib.h"

using namespace SettingSceneInternal;

SettingScene::SettingScene(SettingReturnMode returnMode)
    : mCurrentPage(GamePage)
    , mConfigMode(ConfigNone)
    , mSelectedItem(AutoLockTarget)
    , mIsWaitingBinding(false)
    , mHoveredBindingAction(-1)
    , mWaitingAction(-1)
    , mConflictPromptTimer(0.0f)
    , mReturnMode(returnMode)
    , mIsFinished(false)
    , mControllerGraphHandle(-1)
    , mLbGraphHandle(-1)
    , mRbGraphHandle(-1)
    , mLeftStickGraphHandle(-1)
    , mRightStickGraphHandle(-1)
    , mLtGraphHandle(-1)
    , mRtGraphHandle(-1)
    , mAGraphHandle(-1)
    , mBGraphHandle(-1)
    , mYGraphHandle(-1)
    , mXGraphHandle(-1)
    , mStartGraphHandle(-1)
    , mViewGraphHandle(-1)
    , mDpadUpGraphHandle(-1)
    , mDpadRightGraphHandle(-1)
    , mDpadDownGraphHandle(-1)
    , mDpadLeftGraphHandle(-1)
{
    /// 設定画面ではマウス操作を許可し、コントローラー説明用画像を先に取得する。
    InputManager::instance()->setMouseLock(false);
    SetMouseDispFlag(InputManager::instance()->isLastInputPad() ? FALSE : TRUE);
    mControllerGraphHandle = ResourceManager::instance()->getGraph(ControllerGraph);
    mLbGraphHandle = ResourceManager::instance()->getGraph(LbGraph);
    mRbGraphHandle = ResourceManager::instance()->getGraph(RbGraph);
    mLeftStickGraphHandle = ResourceManager::instance()->getGraph(LeftStickGraph);
    mRightStickGraphHandle = ResourceManager::instance()->getGraph(RightStickGraph);
    mLtGraphHandle = ResourceManager::instance()->getGraph(LtGraph);
    mRtGraphHandle = ResourceManager::instance()->getGraph(RtGraph);
    mAGraphHandle = ResourceManager::instance()->getGraph(AGraph);
    mBGraphHandle = ResourceManager::instance()->getGraph(BGraph);
    mYGraphHandle = ResourceManager::instance()->getGraph(YGraph);
    mXGraphHandle = ResourceManager::instance()->getGraph(XGraph);
    mStartGraphHandle = ResourceManager::instance()->getGraph(StartGraph);
    mViewGraphHandle = ResourceManager::instance()->getGraph(ViewGraph);
    mDpadUpGraphHandle = ResourceManager::instance()->getGraph(DpadUpGraph);
    mDpadRightGraphHandle = ResourceManager::instance()->getGraph(DpadRightGraph);
    mDpadDownGraphHandle = ResourceManager::instance()->getGraph(DpadDownGraph);
    mDpadLeftGraphHandle = ResourceManager::instance()->getGraph(DpadLeftGraph);
}
void SettingScene::update(float deltaTime)
{
    (void)deltaTime;

    InputManager* input = InputManager::instance();
    SetMouseDispFlag(input->isLastInputPad() ? FALSE : TRUE);
    mHoveredBindingAction = -1;
    if (mConflictPromptTimer > 0.0f)
    {
        /// 入力重複警告は一定時間だけ表示する。
        mConflictPromptTimer -= deltaTime;
    }

    if (mIsWaitingBinding)
    {
        /// 割り当て待ち中は通常メニュー操作を止め、次に押された入力だけを読む。
        if (input->isPush(KEY_INPUT_ESCAPE) || input->isPadPush(XINPUT_BUTTON_START))
        {
            resetWaitingBindingDefault();
            AudioManager::instance()->playSe(SoundResource::Se::ValueChange);
            mIsWaitingBinding = false;
            mWaitingAction = -1;
            return;
        }

        InputBinding binding;
        bool read = mConfigMode == ConfigController ? readPressedPadBinding(&binding) : readPressedBinding(&binding);
        if (read)
        {
            /// コントローラー設定とキーボード/マウス設定で保存先を分ける。
            if (mConfigMode == ConfigController)
            {
                GameSettings::instance()->setPadBinding(mWaitingAction, binding);
            }
            else
            {
                GameSettings::instance()->setInputBindingAllRoles(mWaitingAction, binding);
            }

            if (hasBindingConflict())
            {
                /// 同じ入力が複数アクションに割り当たった場合、画面上で警告する。
                mConflictPromptTimer = 1.0f;
            }

            AudioManager::instance()->playSe(SoundResource::Se::Confirm);
            mIsWaitingBinding = false;
            mWaitingAction = -1;
        }
        return;
    }

    if (mConfigMode != ConfigNone)
    {
        /// ボタン/キー配置モードでは、通常設定項目ではなくアクション一覧を操作する。
        if (!input->isLastInputPad())
        {
            mHoveredBindingAction = getMouseBindingAction(input->getMouseScreenX(), input->getMouseScreenY());
        }
        if (input->isPush(KEY_INPUT_ESCAPE) || input->isPadPush(XINPUT_BUTTON_B))
        {
            if (hasBindingConflict())
            {
                /// 重複状態のまま戻ろうとした場合は既定配置へ戻して安全な状態にする。
                GameSettings::instance()->resetControlSettings();
                mConflictPromptTimer = 0.0f;
                AudioManager::instance()->playSe(SoundResource::Se::ValueChange);
                return;
            }
            AudioManager::instance()->playSe(SoundResource::Se::Cancel);
            mConfigMode = ConfigNone;
            mSelectedItem = getPageItem(0);
            return;
        }
        if (input->isPush(KEY_INPUT_UP) || input->isPush(KEY_INPUT_W) || input->isPadPush(XINPUT_BUTTON_DPAD_UP)) moveSelection(-1);
        if (input->isPush(KEY_INPUT_DOWN) || input->isPush(KEY_INPUT_S) || input->isPadPush(XINPUT_BUTTON_DPAD_DOWN)) moveSelection(1);
        if (input->isPush(KEY_INPUT_Y) || input->isPadPush(XINPUT_BUTTON_Y))
        {
            GameSettings::instance()->resetControlSettings();
            mConflictPromptTimer = 0.0f;
            AudioManager::instance()->playSe(SoundResource::Se::ValueChange);
        }
        if (mHoveredBindingAction >= 0)
        {
            mSelectedItem = mHoveredBindingAction;
        }
        if (input->isPush(KEY_INPUT_RETURN) || input->isPush(KEY_INPUT_SPACE) || input->isPadPush(XINPUT_BUTTON_A)) decideSelected();
        if (!input->isLastInputPad() && input->isMousePush(MOUSE_INPUT_LEFT) && mHoveredBindingAction >= 0)
        {
            mSelectedItem = mHoveredBindingAction;
            decideSelected();
        }
        return;
    }

    int mouseItem = input->isLastInputPad() ? -1 : getMouseItem(input->getMouseScreenX(), input->getMouseScreenY());
    if (mouseItem >= 0)
    {
        /// マウス操作時はカーソル下の項目を選択状態にする。
        mSelectedItem = mouseItem;
    }

    if (input->isPush(KEY_INPUT_ESCAPE) || input->isPadPush(XINPUT_BUTTON_B))
    {
        AudioManager::instance()->playSe(SoundResource::Se::Cancel);
        closeSetting();
        return;
    }

    if (input->isPush(KEY_INPUT_UP) || input->isPush(KEY_INPUT_W) || input->isPadPush(XINPUT_BUTTON_DPAD_UP)) moveSelection(-1);
    if (input->isPush(KEY_INPUT_DOWN) || input->isPush(KEY_INPUT_S) || input->isPadPush(XINPUT_BUTTON_DPAD_DOWN)) moveSelection(1);
    if (input->isPush(KEY_INPUT_LEFT) || input->isPush(KEY_INPUT_A) || input->isPadPush(XINPUT_BUTTON_DPAD_LEFT)) adjustSelected(-1);
    if (input->isPush(KEY_INPUT_RIGHT) || input->isPush(KEY_INPUT_D) || input->isPadPush(XINPUT_BUTTON_DPAD_RIGHT)) adjustSelected(1);
    if ((input->isPush(KEY_INPUT_Y) || input->isPadPush(XINPUT_BUTTON_Y)) && isResettableItem(mSelectedItem)) resetSelectedDefault();
    if (input->isPush(KEY_INPUT_RETURN) || input->isPush(KEY_INPUT_SPACE) || input->isPadPush(XINPUT_BUTTON_A) || input->isPadPush(XINPUT_BUTTON_START)) decideSelected();

    if (input->isPadPush(XINPUT_BUTTON_LEFT_SHOULDER)) switchPage((mCurrentPage + SettingPageCount - 1) % SettingPageCount);
    if (input->isPadPush(XINPUT_BUTTON_RIGHT_SHOULDER)) switchPage((mCurrentPage + 1) % SettingPageCount);

    if (!input->isLastInputPad() && input->isMousePush(MOUSE_INPUT_LEFT))
    {
        /// マウスクリックはタブ、左右調整、項目決定の順に判定する。
        int mousePage = getMousePage(input->getMouseScreenX(), input->getMouseScreenY());
        if (mousePage >= 0)
        {
            switchPage(mousePage);
            return;
        }

        int adjustDirection = getMouseAdjustDirection(input->getMouseScreenX(), input->getMouseScreenY());
        if (adjustDirection != 0)
        {
            adjustSelected(adjustDirection);
            return;
        }

        if (mouseItem >= 0)
        {
            decideSelected();
        }
    }
}
void SettingScene::draw()
{
    /// 背景とタイトルを先に描き、現在ページの内容をその上へ配置する。
    DrawBox(0, 0, kWindowWidth, kWindowHeight, GetColor(12, 14, 18), TRUE);
    DrawString(96, 70, "SETTING", GetColor(235, 242, 255));
    drawPageTabs();

    if (mConfigMode != ConfigNone)
    {
        /// 入力割り当てモードでは通常項目ではなくアクション別の binding 一覧を描く。
        DrawString(96, 165, mConfigMode == ConfigController ? "ボタン配置" : "キー配置", GetColor(235, 242, 255));
        drawBindingList();
    }
    else
    {
        char value[32];
        for (int i = 0; i < getPageItemCount(); ++i)
        {
            /// 現在ページに属する項目だけを順番に描画する。
            int item = getPageItem(i);
            const char* label = "";
            switch (item)
            {
            case MasterVolume: label = "マスター音量"; break;
            case BgmVolume: label = "BGM音量"; break;
            case SeVolume: label = "SE音量"; break;
            case MouseSensitivity: label = "マウス感度"; break;
            case InvertMouseY: label = "上下反転"; break;
            case ButtonConfig: label = "ボタン設定"; break;
            case KeyConfig: label = "キー設定"; break;
            case Fullscreen: label = "フルスクリーン"; break;
            case AutoLockTarget: label = "自動ロック"; break;
            case Back: label = "戻る"; break;
            default: break;
            }

            makeValueText(item, value, sizeof(value));
            drawItem(item, getItemY(item), label, value);
        }

        if (mCurrentPage == ControllerPage)
        {
            drawControllerGuide();
        }
    }

    if (mConflictPromptTimer > 0.0f) drawConflictPrompt();

}
void SettingScene::adjustSelected(int direction, bool playSound)
{
    GameSettings* settings = GameSettings::instance();
    int step = direction * 5;
    bool changed = true;

    /// 数値設定は 5 単位で増減し、トグル設定は左右入力で切り替える。
    switch (mSelectedItem)
    {
    case MasterVolume: settings->addMasterVolume(step); break;
    case BgmVolume: settings->addBgmVolume(step); break;
    case SeVolume: settings->addSeVolume(step); break;
    case MouseSensitivity: settings->addMouseSensitivity(step); break;
    case InvertMouseY: settings->toggleInvertMouseY(); break;
    case AutoLockTarget: settings->toggleAutoLockTarget(); break;
    case Fullscreen: settings->toggleFullscreen(); break;
    default: changed = false; break;
    }

    if (changed && playSound) AudioManager::instance()->playSe(SoundResource::Se::ValueChange);
}
void SettingScene::resetSelectedDefault()
{
    /// 現在選択中の設定だけを既定値へ戻す。
    GameSettings* settings = GameSettings::instance();
    switch (mSelectedItem)
    {
    case MasterVolume: settings->addMasterVolume(80 - settings->getMasterVolume()); break;
    case BgmVolume: settings->addBgmVolume(70 - settings->getBgmVolume()); break;
    case SeVolume: settings->addSeVolume(80 - settings->getSeVolume()); break;
    case MouseSensitivity: settings->addMouseSensitivity(50 - settings->getMouseSensitivity()); break;
    case InvertMouseY: if (settings->isInvertMouseY()) settings->toggleInvertMouseY(); break;
    case Fullscreen: if (settings->isFullscreen()) settings->toggleFullscreen(); break;
    case AutoLockTarget: if (settings->isAutoLockTarget()) settings->toggleAutoLockTarget(); break;
    default: return;
    }
    AudioManager::instance()->playSe(SoundResource::Se::ValueChange);
}
void SettingScene::decideSelected()
{
    if (mConfigMode != ConfigNone)
    {
        if (mConfigMode == ConfigController && mSelectedItem >= GameActionMoveForward && mSelectedItem <= GameActionMoveRight)
        {
            /// 移動は左スティック固定表示なので、個別ボタン割り当ては受け付けない。
            return;
        }
        /// アクション項目を決定したら、次の入力をそのアクションの binding として読む。
        mIsWaitingBinding = true;
        mWaitingAction = mSelectedItem;
        AudioManager::instance()->playSe(SoundResource::Se::Confirm);
        return;
    }

    if (mSelectedItem == Back)
    {
        /// 入力重複がある状態では戻れないようにし、無効な設定保存を避ける。
        if (hasBindingConflict()) return;
        AudioManager::instance()->playSe(SoundResource::Se::Cancel);
        closeSetting();
        return;
    }

    if (mSelectedItem == ButtonConfig)
    {
        /// コントローラー設定ページのアクション一覧へ入る。
        mConfigMode = ConfigController;
        mSelectedItem = 0;
        AudioManager::instance()->playSe(SoundResource::Se::Confirm);
        return;
    }

    if (mSelectedItem == KeyConfig)
    {
        /// キーボード/マウス設定ページのアクション一覧へ入る。
        mConfigMode = ConfigKeyboardMouse;
        mSelectedItem = 0;
        AudioManager::instance()->playSe(SoundResource::Se::Confirm);
        return;
    }

    if (mSelectedItem == InvertMouseY || mSelectedItem == Fullscreen || mSelectedItem == AutoLockTarget)
    {
        AudioManager::instance()->playSe(SoundResource::Se::Confirm);
        adjustSelected(1, false);
    }
}
void SettingScene::closeSetting()
{
    if (mReturnMode == SettingReturnOverlay)
    {
        /// ポーズ画面から開いた場合はシーン遷移せず、完了フラグだけ返す。
        mIsFinished = true;
        return;
    }
    if (mReturnMode == SettingReturnCharaSelect)
    {
        /// タイトル導線から来た場合はキャラクター選択へ戻す。
        SceneManager::instance()->changeScene<CharaSelectScene>();
        return;
    }
    SceneManager::instance()->changeScene<TitleScene>();
}
