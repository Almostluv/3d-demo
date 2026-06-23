/**
 * @file
 * @brief SettingScene の入力割り当てと競合判定を定義する。
 */

#include "setting_scene.h"
#include "setting_scene_internal.h"
#include "game_settings.h"
#include "input_manager.h"
#include "DxLib.h"

using namespace SettingSceneInternal;

void SettingScene::resetWaitingBindingDefault()
{
    /// 割り当て待ちをキャンセルした時、対象アクションだけを既定値へ戻す。
    InputBinding binding = { InputBindingKey, KEY_INPUT_W };
    if (mConfigMode == ConfigController)
    {
        binding.type = InputBindingPad;
        switch (mWaitingAction)
        {
        case GameActionMoveForward: binding.code = XINPUT_BUTTON_DPAD_UP; break;
        case GameActionMoveBack: binding.code = XINPUT_BUTTON_DPAD_DOWN; break;
        case GameActionMoveLeft: binding.code = XINPUT_BUTTON_DPAD_LEFT; break;
        case GameActionMoveRight: binding.code = XINPUT_BUTTON_DPAD_RIGHT; break;
        case GameActionPrimary: binding.code = PadBindingRightTrigger; break;
        case GameActionDodge: binding.code = XINPUT_BUTTON_B; break;
        case GameActionSecondary: binding.code = PadBindingLeftTrigger; break;
        case GameActionSpecial: binding.code = XINPUT_BUTTON_Y; break;
        case GameActionLockOn: binding.code = XINPUT_BUTTON_RIGHT_SHOULDER; break;
        default: return;
        }
        GameSettings::instance()->setPadBinding(mWaitingAction, binding);
        return;
    }

    switch (mWaitingAction)
    {
    case GameActionMoveForward: binding = { InputBindingKey, KEY_INPUT_W }; break;
    case GameActionMoveBack: binding = { InputBindingKey, KEY_INPUT_S }; break;
    case GameActionMoveLeft: binding = { InputBindingKey, KEY_INPUT_A }; break;
    case GameActionMoveRight: binding = { InputBindingKey, KEY_INPUT_D }; break;
    case GameActionPrimary: binding = { InputBindingMouse, MOUSE_INPUT_LEFT }; break;
    case GameActionDodge: binding = { InputBindingKey, KEY_INPUT_LSHIFT }; break;
    case GameActionSecondary: binding = { InputBindingMouse, MOUSE_INPUT_RIGHT }; break;
    case GameActionSpecial: binding = { InputBindingKey, KEY_INPUT_E }; break;
    case GameActionLockOn: binding = { InputBindingKey, KEY_INPUT_TAB }; break;
    default: return;
    }
    GameSettings::instance()->setInputBindingAllRoles(mWaitingAction, binding);
}
int SettingScene::getPadBindingGraphHandle(InputBinding binding) const
{
    /// パッド binding のコードを、表示用ボタン画像ハンドルへ変換する。
    if (binding.type != InputBindingPad) return -1;
    switch (binding.code)
    {
    case XINPUT_BUTTON_DPAD_UP: return mDpadUpGraphHandle;
    case XINPUT_BUTTON_DPAD_RIGHT: return mDpadRightGraphHandle;
    case XINPUT_BUTTON_DPAD_DOWN: return mDpadDownGraphHandle;
    case XINPUT_BUTTON_DPAD_LEFT: return mDpadLeftGraphHandle;
    case XINPUT_BUTTON_START: return mStartGraphHandle;
    case XINPUT_BUTTON_BACK: return mViewGraphHandle;
    case XINPUT_BUTTON_LEFT_SHOULDER: return mLbGraphHandle;
    case XINPUT_BUTTON_RIGHT_SHOULDER: return mRbGraphHandle;
    case XINPUT_BUTTON_A: return mAGraphHandle;
    case XINPUT_BUTTON_B: return mBGraphHandle;
    case XINPUT_BUTTON_X: return mXGraphHandle;
    case XINPUT_BUTTON_Y: return mYGraphHandle;
    case PadBindingLeftTrigger: return mLtGraphHandle;
    case PadBindingRightTrigger: return mRtGraphHandle;
    default: return -1;
    }
}
bool SettingScene::readPressedBinding(InputBinding* binding) const
{
    /// キーボード全キーを走査し、次に押されたキーまたはマウスボタンを binding として読む。
    InputManager* input = InputManager::instance();
    for (int key = 0; key < 256; ++key)
    {
        if (input->isPush(key))
        {
            binding->type = InputBindingKey;
            binding->code = key;
            return true;
        }
    }
    if (!input->isLastInputPad() && input->isMousePush(MOUSE_INPUT_LEFT)) { binding->type = InputBindingMouse; binding->code = MOUSE_INPUT_LEFT; return true; }
    if (input->isMousePush(MOUSE_INPUT_RIGHT)) { binding->type = InputBindingMouse; binding->code = MOUSE_INPUT_RIGHT; return true; }
    if (input->isMousePush(MOUSE_INPUT_MIDDLE)) { binding->type = InputBindingMouse; binding->code = MOUSE_INPUT_MIDDLE; return true; }
    return false;
}
bool SettingScene::readPressedPadBinding(InputBinding* binding) const
{
    /// トリガーは通常ボタンと別 API なので、先に専用判定を行う。
    InputManager* input = InputManager::instance();
    if (input->isPadLeftTriggerPush()) { binding->type = InputBindingPad; binding->code = PadBindingLeftTrigger; return true; }
    if (input->isPadRightTriggerPush()) { binding->type = InputBindingPad; binding->code = PadBindingRightTrigger; return true; }
    for (int button = 0; button < 16; ++button)
    {
        if (input->isPadPush(button))
        {
            binding->type = InputBindingPad;
            binding->code = button;
            return true;
        }
    }
    return false;
}
bool SettingScene::hasBindingConflict() const
{
    /// キーボード/マウスとパッドのどちらかで重複があれば設定は未解決扱いにする。
    return hasKeyboardBindingConflict() || hasPadBindingConflict();
}
bool SettingScene::hasKeyboardBindingConflict() const
{
    for (int action = 0; action < GameActionCount; ++action)
    {
        if (isKeyboardBindingConflictAction(action)) return true;
    }
    return false;
}
bool SettingScene::hasPadBindingConflict() const
{
    for (int action = 0; action < GameActionCount; ++action)
    {
        if (isPadBindingConflictAction(action)) return true;
    }
    return false;
}
bool SettingScene::isKeyboardBindingConflictAction(int action) const
{
    /// 同じ keyboard/mouse binding が別アクションにも使われているか確認する。
    InputBinding binding = GameSettings::instance()->getInputBinding(0, action);
    for (int other = 0; other < GameActionCount; ++other)
    {
        if (other != action && sameBinding(binding, GameSettings::instance()->getInputBinding(0, other))) return true;
    }
    return false;
}
bool SettingScene::isPadBindingConflictAction(int action) const
{
    /// 同じ pad binding が別アクションにも使われているか確認する。
    InputBinding binding = GameSettings::instance()->getPadBinding(action);
    for (int other = 0; other < GameActionCount; ++other)
    {
        if (other != action && sameBinding(binding, GameSettings::instance()->getPadBinding(other))) return true;
    }
    return false;
}
