#ifndef _CAMERA_H_
#define _CAMERA_H_


/**
 * @file
 * @brief 三人称カメラとロックオン視点を管理する Camera を定義する。
 */


#include "DxLib.h"


/**
 * @brief プレイヤー追従とロックオン視点を扱う三人称カメラ。
 *
 * 通常時はマウス入力で視点角度を更新し、ロックオン時はプレイヤーと敵を
 * 見やすい位置に収める。移動方向計算用の XZ 平面ベクトルも提供する。
 */
class Camera {
public:
    /**
     * @brief 初期距離、角度、注視点を設定してカメラを生成する。
     */
    Camera();

    /**
     * @brief カメラのデストラクタ。
     */
    ~Camera() = default;

    /**
     * @brief 通常カメラを更新する。
     * @param[in] targetPos 追従対象のワールド座標。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void update(VECTOR targetPos, float deltaTime);

    /**
     * @brief 現在の角度を保ったまま追従対象へ即座に同期する。
     * @param[in] targetPos 追従対象のワールド座標。
     */
    void syncToTarget(VECTOR targetPos);

    /**
     * @brief ロックオン中のカメラ位置と注視点を更新する。
     * @param[in] playerPos プレイヤーのワールド座標。
     * @param[in] enemyPos ロックオン対象のワールド座標。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateLockOn(VECTOR playerPos, VECTOR enemyPos, float deltaTime);

    /**
     * @brief 現在のカメラ位置と注視点から水平・垂直角度を再計算する。
     */
    void syncAnglesToCurrentView();

    /**
     * @brief DxLib に現在のカメラ設定を反映する。
     */
    void draw();

    /**
     * @brief カメラの前方向を XZ 平面上の正規化ベクトルで取得する。
     * @return 前方向ベクトル。
     */
    VECTOR getForwardXZ() const;

    /**
     * @brief カメラの右方向を XZ 平面上の正規化ベクトルで取得する。
     * @return 右方向ベクトル。
     */
    VECTOR getRightXZ() const;

    /**
     * @brief カメラ位置を取得する。
     * @return カメラのワールド座標。
     */
    VECTOR getEyePosition() const { return mEyePos; }

    /**
     * @brief 注視点を取得する。
     * @return 注視点のワールド座標。
     */
    VECTOR getTargetPosition() const { return mTargetPos; }

private:
    float mDistance;    ///< 注視点からカメラ位置までの距離。DxLib ワールド単位。
    float mAngleH;      ///< 水平方向のカメラ角度（radian）。
    float mAngleV;      ///< 垂直方向のカメラ角度（radian）。
    float mSensitivity; ///< マウス入力から角度へ変換する感度倍率。
    bool mHasLockForward; ///< ロックオン用の前方向ベクトルが有効かどうか。
    VECTOR mLockForward;  ///< ロックオン中に維持する水平前方向ベクトル。

    VECTOR mEyePos;    ///< カメラ位置のワールド座標。
    VECTOR mTargetPos; ///< 注視点のワールド座標。

    const float MIN_ANGLE_V = -0.8f; ///< 垂直角度の下限（radian）。
    const float MAX_ANGLE_V = 0.8f;  ///< 垂直角度の上限（radian）。
};

#endif
