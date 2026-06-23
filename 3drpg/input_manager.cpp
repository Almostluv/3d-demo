/**
 * @file
 * @brief input_manager.cpp 実装を定義する。
 */

#include "input_manager.h"
#include "game_config.h"

#include <cmath>

namespace
{
    constexpr float PadStickMax = 32767.0f; ///< XInput スティック入力を正規化する最大値。
    constexpr float PadStickDeadZone = 0.25f; ///< 微小なスティック入力を無視するデッドゾーン。
    constexpr unsigned char PadTriggerThreshold = 30;

    float normalizePadAxis(short value)
    {
        /// XInput の short 値を -1..1 に変換し、デッドゾーン内は完全に 0 として扱う。
        float axis = static_cast<float>(value) / PadStickMax;
        if (axis > 1.0f) axis = 1.0f;
        if (axis < -1.0f) axis = -1.0f;
        return std::fabs(axis) < PadStickDeadZone ? 0.0f : axis;
    }
}

InputManager::InputManager() {
    /// 現在フレームと前フレームの入力状態を保持し、Push/Release 判定に使う。
    for (int i = 0; i < 256; i++) {
        mKeyStateCurr[i] = 0;
        mKeyStatePrev[i] = 0;
    }
    mMouseInputCurr = 0;
    mMouseInputPrev = 0;
    mMouseX = 0;
    mMouseY = 0;
    mMouseScreenX = kWindowWidth / 2;
    mMouseScreenY = kWindowHeight / 2;
    mMouseLock = true;
    mPadStateCurr = {};
    mPadStatePrev = {};
    mPadLeftStickX = 0.0f;
    mPadLeftStickY = 0.0f;
    mPadRightStickX = 0.0f;
    mPadRightStickY = 0.0f;
    mLastInputPad = false;
}

void InputManager::update() {
    /// 更新前に現在状態を前回状態へコピーし、差分で押下/離上を判定する。
    for (int i = 0; i < 256; i++) {
        mKeyStatePrev[i] = mKeyStateCurr[i];
    }

    GetHitKeyStateAll(mKeyStateCurr);

    mMouseInputPrev = mMouseInputCurr;
    mMouseInputCurr = GetMouseInput();

    mPadStatePrev = mPadStateCurr;
    if (GetJoypadXInputState(DX_INPUT_PAD1, &mPadStateCurr) != 0)
    {
        /// パッドが未接続の場合はゼロ状態に戻し、古い入力が残らないようにする。
        mPadStateCurr = {};
    }
    mPadLeftStickX = normalizePadAxis(mPadStateCurr.ThumbLX);
    mPadLeftStickY = normalizePadAxis(mPadStateCurr.ThumbLY);
    mPadRightStickX = normalizePadAxis(mPadStateCurr.ThumbRX);
    mPadRightStickY = normalizePadAxis(mPadStateCurr.ThumbRY);

    int prevScreenX = mMouseScreenX;
    int prevScreenY = mMouseScreenY;
    int curX, curY;
    GetMousePoint(&curX, &curY);
    mMouseScreenX = curX;
    mMouseScreenY = curY;

    if (mMouseLock)
    {
        /// マウスロック中は画面中央からの差分を相対移動として扱い、毎フレーム中央へ戻す。
        mMouseX = curX - (kWindowWidth / 2);
        mMouseY = curY - (kWindowHeight / 2);

        SetMousePoint(kWindowWidth / 2, kWindowHeight / 2);
    }
    else
    {
        mMouseX = 0;
        mMouseY = 0;
    }
    bool keyboardOrMouseUsed = mMouseInputCurr != 0 || curX != prevScreenX || curY != prevScreenY;
    for (int i = 0; i < 256 && !keyboardOrMouseUsed; ++i)
    {
        /// どの入力デバイスを最後に使ったかを UI 表示切り替えに利用する。
        keyboardOrMouseUsed = mKeyStateCurr[i] != 0;
    }
    bool padUsed = mPadStateCurr.LeftTrigger >= PadTriggerThreshold || mPadStateCurr.RightTrigger >= PadTriggerThreshold ||
        std::fabs(mPadLeftStickX) > 0.0f || std::fabs(mPadLeftStickY) > 0.0f ||
        std::fabs(mPadRightStickX) > 0.0f || std::fabs(mPadRightStickY) > 0.0f;
    for (int i = 0; i < 16 && !padUsed; ++i)
    {
        padUsed = mPadStateCurr.Buttons[i] != 0;
    }
    if (keyboardOrMouseUsed) mLastInputPad = false;
    if (padUsed) mLastInputPad = true;
}

bool InputManager::isPush(int keyCode) const {
    /// Push は現在押されていて前フレーム押されていない 1 フレームだけ true。
    return (mKeyStateCurr[keyCode] == 1) && (mKeyStatePrev[keyCode] == 0);
}

bool InputManager::isPress(int keyCode) const {
    /// Press は押されている間 true。
    return (mKeyStateCurr[keyCode] == 1);
}

bool InputManager::isRelease(int keyCode) const {
    /// Release は現在離されていて前フレーム押されていた 1 フレームだけ true。
    return (mKeyStateCurr[keyCode] == 0) && (mKeyStatePrev[keyCode] == 1);
}

bool InputManager::isMousePush(int mouseButton) const {
    return (mMouseInputCurr & mouseButton) != 0 && (mMouseInputPrev & mouseButton) == 0;
}

bool InputManager::isMousePress(int mouseButton) const {
    return (mMouseInputCurr & mouseButton) != 0;
}

bool InputManager::isMouseRelease(int mouseButton) const {
    return (mMouseInputCurr & mouseButton) == 0 && (mMouseInputPrev & mouseButton) != 0;
}

bool InputManager::isPadPush(int button) const
{
    return button >= 0 && button < 16 &&
        mPadStateCurr.Buttons[button] != 0 && mPadStatePrev.Buttons[button] == 0;
}

bool InputManager::isPadPress(int button) const
{
    return button >= 0 && button < 16 && mPadStateCurr.Buttons[button] != 0;
}

bool InputManager::isPadRelease(int button) const
{
    return button >= 0 && button < 16 &&
        mPadStateCurr.Buttons[button] == 0 && mPadStatePrev.Buttons[button] != 0;
}

bool InputManager::isPadLeftTriggerPush() const
{
    return mPadStateCurr.LeftTrigger >= PadTriggerThreshold &&
        mPadStatePrev.LeftTrigger < PadTriggerThreshold;
}

bool InputManager::isPadLeftTriggerPress() const
{
    return mPadStateCurr.LeftTrigger >= PadTriggerThreshold;
}

bool InputManager::isPadRightTriggerPush() const
{
    return mPadStateCurr.RightTrigger >= PadTriggerThreshold &&
        mPadStatePrev.RightTrigger < PadTriggerThreshold;
}

bool InputManager::isPadRightTriggerPress() const
{
    return mPadStateCurr.RightTrigger >= PadTriggerThreshold;
}
void InputManager::setMouseLock(bool lock)
{
    mMouseLock = lock;
}
