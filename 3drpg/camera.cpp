/**
 * @file
 * @brief camera.cpp 実装を定義する。
 */

#include "camera.h"
#include "game_settings.h"
#include "input_manager.h"

#include <cmath>

namespace
{
    constexpr float PadCameraSpeed = 2.2f; ///< ゲームパッド右スティックによるカメラ回転速度。
}

Camera::Camera()
    : mDistance(60.0f)
    , mAngleH(DX_PI_F)
    , mAngleV(0.3f)
    , mSensitivity(0.0035f)
    , mHasLockForward(false)
    , mLockForward(VGet(0.0f, 0.0f, 1.0f))
{
    /// 初期カメラはプレイヤー後方から見下ろす角度で開始する。
    mEyePos = VGet(0, 0, 0);

    mTargetPos = VGet(0, 0, 0);
}

void Camera::update(VECTOR targetPos, float deltaTime)
{
    (void)deltaTime;
    mHasLockForward = false;

    /// 通常カメラではマウスと右スティックの入力を水平・垂直角へ直接加算する。
    int mouseX = InputManager::instance()->getMouseX();
    int mouseY = InputManager::instance()->getMouseY();
    float sensitivity = mSensitivity * GameSettings::instance()->getCameraSensitivityScale();
    float invertY = GameSettings::instance()->isInvertMouseY() ? -1.0f : 1.0f;
    float padX = InputManager::instance()->getPadRightStickX();
    float padY = InputManager::instance()->getPadRightStickY();

    mAngleH += static_cast<float>(mouseX) * sensitivity;
    mAngleV += static_cast<float>(mouseY) * sensitivity * invertY;
    mAngleH += padX * PadCameraSpeed * deltaTime;
    mAngleV += -padY * PadCameraSpeed * deltaTime * invertY;

    if (mAngleV < MIN_ANGLE_V)
    {
        /// 垂直角を制限し、地面下や真上へ回り込むカメラを防ぐ。
        mAngleV = MIN_ANGLE_V;
    }

    if (mAngleV > MAX_ANGLE_V)
    {
        mAngleV = MAX_ANGLE_V;
    }

    mTargetPos = VAdd(targetPos, VGet(0.0f, 15.0f, 0.0f));

    /// 球面座標の角度から、注視点を中心にしたカメラ位置を計算する。
    float cosV = cosf(mAngleV);
    float sinV = sinf(mAngleV);
    float cosH = cosf(mAngleH);
    float sinH = sinf(mAngleH);

    mEyePos.x = mTargetPos.x - mDistance * cosV * sinH;
    mEyePos.y = mTargetPos.y + mDistance * sinV;
    mEyePos.z = mTargetPos.z - mDistance * cosV * cosH;
}

void Camera::syncToTarget(VECTOR targetPos)
{
    /// 演出中など入力更新なしで、現在角度のまま対象位置へカメラを同期する。
    mHasLockForward = false;
    mTargetPos = VAdd(targetPos, VGet(0.0f, 15.0f, 0.0f));

    float cosV = cosf(mAngleV);
    float sinV = sinf(mAngleV);
    float cosH = cosf(mAngleH);
    float sinH = sinf(mAngleH);

    mEyePos.x = mTargetPos.x - mDistance * cosV * sinH;
    mEyePos.y = mTargetPos.y + mDistance * sinV;
    mEyePos.z = mTargetPos.z - mDistance * cosV * cosH;
}
void Camera::updateLockOn(
    VECTOR playerPos,
    VECTOR enemyPos,
    float deltaTime)
{
    (void)deltaTime;

    int mouseY = InputManager::instance()->getMouseY();
    /// ロックオン中は水平角を敵方向で決めるため、手動操作は垂直角だけ反映する。
    float sensitivity = mSensitivity * GameSettings::instance()->getCameraSensitivityScale();
    float invertY = GameSettings::instance()->isInvertMouseY() ? -1.0f : 1.0f;
    float padY = InputManager::instance()->getPadRightStickY();
    mAngleV += static_cast<float>(mouseY) * sensitivity * invertY;
    mAngleV += -padY * PadCameraSpeed * deltaTime * invertY;

    if (mAngleV < MIN_ANGLE_V)
    {
        mAngleV = MIN_ANGLE_V;
    }

    if (mAngleV > MAX_ANGLE_V)
    {
        mAngleV = MAX_ANGLE_V;
    }

    VECTOR toEnemy = VSub(enemyPos, playerPos);

    toEnemy.y = 0.0f;

    if (VSize(toEnemy) <= 0.0001f)
    {
        /// 敵とプレイヤーが同位置の場合は方向が決められないため通常カメラへ戻す。
        update(playerPos, deltaTime);
        return;
    }

    VECTOR desiredForward = VNorm(toEnemy);
    if (!mHasLockForward)
    {
        /// ロックオン開始直後は現在の敵方向を基準方向として保存する。
        mLockForward = desiredForward;
        mHasLockForward = true;
    }
    else
    {
        float smoothRate = 7.0f * deltaTime;
        if (smoothRate > 1.0f)
        {
            smoothRate = 1.0f;
        }

        /// ロック対象方向を補間し、敵の移動でカメラが急に跳ねないようにする。
        mLockForward = VAdd(mLockForward, VScale(VSub(desiredForward, mLockForward), smoothRate));
        if (VSize(mLockForward) <= 0.0001f)
        {
            mLockForward = desiredForward;
        }
        else
        {
            mLockForward = VNorm(mLockForward);
        }
    }
    VECTOR forward = mLockForward;

    mTargetPos = VAdd(playerPos, VGet(0.0f, 15.0f, 0.0f));

    mTargetPos = VAdd(mTargetPos, VScale(forward, 12.0f));

    float cosV = cosf(mAngleV);
    float sinV = sinf(mAngleV);

    mEyePos.x = mTargetPos.x - mDistance * cosV * forward.x;
    mEyePos.y = mTargetPos.y + mDistance * sinV;
    mEyePos.z = mTargetPos.z - mDistance * cosV * forward.z;
}

void Camera::syncAnglesToCurrentView()
{
    /// 外部演出でカメラ位置を直接変えた後、現在の視線から内部角度を復元する。
    VECTOR forward = VSub(mTargetPos, mEyePos);
    float horizontal = sqrtf(forward.x * forward.x + forward.z * forward.z);
    if (horizontal <= 0.0001f)
    {
        return;
    }

    mAngleH = atan2f(forward.x, forward.z);
    mAngleV = atan2f(forward.y, horizontal);
    if (mAngleV < MIN_ANGLE_V)
    {
        mAngleV = MIN_ANGLE_V;
    }
    if (mAngleV > MAX_ANGLE_V)
    {
        mAngleV = MAX_ANGLE_V;
    }
    mHasLockForward = false;
}

void Camera::draw()
{
    /// DxLib のビュー行列へ、計算済みのカメラ位置と注視点を反映する。
    SetCameraPositionAndTarget_UpVecY(mEyePos, mTargetPos);

    SetCameraNearFar(1.0f, 100000.0f);
}

VECTOR Camera::getForwardXZ() const
{
    VECTOR forward = VSub(mTargetPos, mEyePos);

    forward.y = 0.0f;

    if (VSize(forward) <= 0.0001f)
    {
        /// カメラと注視点が重なった場合は、前方を固定値へフォールバックする。
        return VGet(0.0f, 0.0f, 1.0f);
    }

    return VNorm(forward);
}

VECTOR Camera::getRightXZ() const
{
    VECTOR forward = getForwardXZ();

    return VGet(forward.z, 0.0f, -forward.x);
}
