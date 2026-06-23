/**
 * @file
 * @brief game_settings.cpp 実装を定義する。
 */

#include "game_settings.h"
#include "audio_manager.h"
#include "game_config.h"
#include "resource_manager.h"
#include "DxLib.h"

#include <cstdio>

GameSettings::GameSettings()
    : mMasterVolume(80)
    , mBgmVolume(70)
    , mSeVolume(80)
    , mMouseSensitivity(50)
    , mInvertMouseY(false)
    , mFullscreen(false)
    , mAutoLockTarget(false)
{
    /// 起動時は既定の音量、マウス、画面設定で開始し、操作設定は resetControlSettings() に任せる。
    resetControlSettings();
}

int GameSettings::clampPercent(int value) const
{
    /// 音量や感度の UI 値は 0..100 に制限する。
    if (value < 0)
    {
        return 0;
    }

    if (value > 100)
    {
        return 100;
    }

    return value;
}

void GameSettings::addMasterVolume(int value)
{
    /// マスター音量は BGM と SE の両方に影響するため、再生中 BGM へ即時反映する。
    mMasterVolume = clampPercent(mMasterVolume + value);
    AudioManager::instance()->updateCurrentBgmVolume();
}

void GameSettings::addBgmVolume(int value)
{
    /// BGM 音量だけを変更した場合も、現在再生中の BGM へ反映する。
    mBgmVolume = clampPercent(mBgmVolume + value);
    AudioManager::instance()->updateCurrentBgmVolume();
}

void GameSettings::addSeVolume(int value)
{
    mSeVolume = clampPercent(mSeVolume + value);
}

void GameSettings::addMouseSensitivity(int value)
{
    mMouseSensitivity = clampPercent(mMouseSensitivity + value);
}

InputBinding GameSettings::getInputBinding(int role, int action) const
{
    return mInputBindings[role][action];
}

InputBinding GameSettings::getPadBinding(int action) const
{
    return mPadBindings[action];
}

void GameSettings::setInputBinding(int role, int action, InputBinding binding)
{
    mInputBindings[role][action] = binding;
}

void GameSettings::setInputBindingAllRoles(int action, InputBinding binding)
{
    for (int role = 0; role < 2; ++role)
    {
        mInputBindings[role][action] = binding;
    }
}

void GameSettings::setPadBinding(int action, InputBinding binding)
{
    mPadBindings[action] = binding;
}

void GameSettings::getInputBindingText(InputBinding binding, char* text, int textSize) const
{
    if (binding.type == InputBindingPad)
    {
        /// パッド入力は UI に表示しやすい短い名前へ変換する。
        const char* name = nullptr;
        switch (binding.code)
        {
        case XINPUT_BUTTON_DPAD_UP: name = "D-PAD UP"; break;
        case XINPUT_BUTTON_DPAD_DOWN: name = "D-PAD DOWN"; break;
        case XINPUT_BUTTON_DPAD_LEFT: name = "D-PAD LEFT"; break;
        case XINPUT_BUTTON_DPAD_RIGHT: name = "D-PAD RIGHT"; break;
        case XINPUT_BUTTON_START: name = "START"; break;
        case XINPUT_BUTTON_BACK: name = "BACK"; break;
        case XINPUT_BUTTON_LEFT_SHOULDER: name = "LB"; break;
        case XINPUT_BUTTON_RIGHT_SHOULDER: name = "RB"; break;
        case XINPUT_BUTTON_A: name = "A"; break;
        case XINPUT_BUTTON_B: name = "B"; break;
        case XINPUT_BUTTON_X: name = "X"; break;
        case XINPUT_BUTTON_Y: name = "Y"; break;
        case PadBindingLeftTrigger: name = "LT"; break;
        case PadBindingRightTrigger: name = "RT"; break;
        default: break;
        }
        if (name != nullptr) sprintf_s(text, textSize, "%s", name);
        else sprintf_s(text, textSize, "PAD %d", binding.code);
        return;
    }

    if (binding.type == InputBindingMouse)
    {
        /// マウス入力もキー名と同じ形式の短縮表記へそろえる。
        const char* name = "MMB";
        if (binding.code == MOUSE_INPUT_LEFT) name = "LMB";
        else if (binding.code == MOUSE_INPUT_RIGHT) name = "RMB";
        sprintf_s(text, textSize, "%s", name);
        return;
    }

    if (binding.code == KEY_INPUT_LSHIFT)
    {
        sprintf_s(text, textSize, "LSHIFT");
    }
    else if (binding.code == KEY_INPUT_TAB)
    {
        sprintf_s(text, textSize, "TAB");
    }
    else if (binding.code == KEY_INPUT_SPACE)
    {
        sprintf_s(text, textSize, "SPACE");
    }
    else if (binding.code == KEY_INPUT_RETURN)
    {
        sprintf_s(text, textSize, "ENTER");
    }
    else
    {
        /// 英数字キーは明示名を返し、それ以外はキーコード表示へフォールバックする。
        const char* name = nullptr;
        switch (binding.code)
        {
        case KEY_INPUT_A: name = "A"; break;
        case KEY_INPUT_B: name = "B"; break;
        case KEY_INPUT_C: name = "C"; break;
        case KEY_INPUT_D: name = "D"; break;
        case KEY_INPUT_E: name = "E"; break;
        case KEY_INPUT_F: name = "F"; break;
        case KEY_INPUT_G: name = "G"; break;
        case KEY_INPUT_H: name = "H"; break;
        case KEY_INPUT_I: name = "I"; break;
        case KEY_INPUT_J: name = "J"; break;
        case KEY_INPUT_K: name = "K"; break;
        case KEY_INPUT_L: name = "L"; break;
        case KEY_INPUT_M: name = "M"; break;
        case KEY_INPUT_N: name = "N"; break;
        case KEY_INPUT_O: name = "O"; break;
        case KEY_INPUT_P: name = "P"; break;
        case KEY_INPUT_Q: name = "Q"; break;
        case KEY_INPUT_R: name = "R"; break;
        case KEY_INPUT_S: name = "S"; break;
        case KEY_INPUT_T: name = "T"; break;
        case KEY_INPUT_U: name = "U"; break;
        case KEY_INPUT_V: name = "V"; break;
        case KEY_INPUT_W: name = "W"; break;
        case KEY_INPUT_X: name = "X"; break;
        case KEY_INPUT_Y: name = "Y"; break;
        case KEY_INPUT_Z: name = "Z"; break;
        case KEY_INPUT_0: name = "0"; break;
        case KEY_INPUT_1: name = "1"; break;
        case KEY_INPUT_2: name = "2"; break;
        case KEY_INPUT_3: name = "3"; break;
        case KEY_INPUT_4: name = "4"; break;
        case KEY_INPUT_5: name = "5"; break;
        case KEY_INPUT_6: name = "6"; break;
        case KEY_INPUT_7: name = "7"; break;
        case KEY_INPUT_8: name = "8"; break;
        case KEY_INPUT_9: name = "9"; break;
        default: break;
        }

        if (name != nullptr) sprintf_s(text, textSize, "%s", name);
        else sprintf_s(text, textSize, "KEY %d", binding.code);
    }
}

void GameSettings::resetControlSettings()
{
    /// Knight/Wizard 共通の移動・基本操作を先に設定し、差分だけ後続で上書きする。
    mMouseSensitivity = 50;
    mInvertMouseY = false;
    for (int role = 0; role < 2; ++role)
    {
        mInputBindings[role][GameActionMoveForward] = { InputBindingKey, KEY_INPUT_W };
        mInputBindings[role][GameActionMoveBack] = { InputBindingKey, KEY_INPUT_S };
        mInputBindings[role][GameActionMoveLeft] = { InputBindingKey, KEY_INPUT_A };
        mInputBindings[role][GameActionMoveRight] = { InputBindingKey, KEY_INPUT_D };
        mInputBindings[role][GameActionPrimary] = { InputBindingMouse, MOUSE_INPUT_LEFT };
        mInputBindings[role][GameActionLockOn] = { InputBindingKey, KEY_INPUT_TAB };
    }

    mInputBindings[0][GameActionDodge] = { InputBindingKey, KEY_INPUT_LSHIFT };
    mInputBindings[0][GameActionSecondary] = { InputBindingMouse, MOUSE_INPUT_RIGHT };
    mInputBindings[0][GameActionSpecial] = { InputBindingKey, KEY_INPUT_E };

    mInputBindings[1][GameActionDodge] = { InputBindingKey, KEY_INPUT_LSHIFT };
    mInputBindings[1][GameActionSecondary] = { InputBindingMouse, MOUSE_INPUT_RIGHT };
    mInputBindings[1][GameActionSpecial] = { InputBindingKey, KEY_INPUT_E };

    /// パッド操作はキャラクター種別に依存しない共通割り当てとして扱う。
    mPadBindings[GameActionMoveForward] = { InputBindingPad, XINPUT_BUTTON_DPAD_UP };
    mPadBindings[GameActionMoveBack] = { InputBindingPad, XINPUT_BUTTON_DPAD_DOWN };
    mPadBindings[GameActionMoveLeft] = { InputBindingPad, XINPUT_BUTTON_DPAD_LEFT };
    mPadBindings[GameActionMoveRight] = { InputBindingPad, XINPUT_BUTTON_DPAD_RIGHT };
    mPadBindings[GameActionPrimary] = { InputBindingPad, PadBindingRightTrigger };
    mPadBindings[GameActionDodge] = { InputBindingPad, XINPUT_BUTTON_B };
    mPadBindings[GameActionSecondary] = { InputBindingPad, PadBindingLeftTrigger };
    mPadBindings[GameActionSpecial] = { InputBindingPad, XINPUT_BUTTON_Y };
    mPadBindings[GameActionLockOn] = { InputBindingPad, XINPUT_BUTTON_RIGHT_SHOULDER };
}

void GameSettings::toggleInvertMouseY()
{
    mInvertMouseY = !mInvertMouseY;
}

void GameSettings::toggleFullscreen()
{
    /// DxLib のウィンドウモード切り替えが失敗した場合は、内部状態を元へ戻す。
    mFullscreen = !mFullscreen;
    if (ChangeWindowMode(mFullscreen ? FALSE : TRUE) != 0)
    {
        mFullscreen = !mFullscreen;
        return;
    }

    /// モード切り替え後は描画先と Z バッファ設定を再設定する。
    SetGraphMode(kWindowWidth, kWindowHeight, 32);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetDrawScreen(DX_SCREEN_BACK);
    SetMouseDispFlag(TRUE);
}

void GameSettings::toggleAutoLockTarget()
{
    mAutoLockTarget = !mAutoLockTarget;
}
float GameSettings::getCameraSensitivityScale() const
{
    /// 50 を標準感度として、低すぎず高すぎない倍率範囲へ変換する。
    return 0.5f + static_cast<float>(mMouseSensitivity) / 50.0f;
}
