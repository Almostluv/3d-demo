#ifndef _DEBUG_DRAW_H_
#define _DEBUG_DRAW_H_


/**
 * @file
 * @brief 当たり判定や攻撃範囲を可視化するデバッグ描画関数を宣言する。
 */

#include "DxLib.h"

/**
 * @brief XZ 平面上の円をデバッグ描画する。
 * @param[in] center 円中心のワールド座標。
 * @param[in] radius 半径。DxLib ワールド単位。
 * @param[in] color DxLib のカラー値。
 * @param[in] yOffset 地面から上へずらす量。DxLib ワールド単位。
 */
void drawDebugCircleXZ(VECTOR center, float radius, unsigned int color, float yOffset = 0.12f);

/**
 * @brief Y 軸方向のカプセル形状をデバッグ描画する。
 * @param[in] baseCenter 下端中心のワールド座標。
 * @param[in] radius カプセル半径。DxLib ワールド単位。
 * @param[in] height カプセル高さ。DxLib ワールド単位。
 * @param[in] color DxLib のカラー値。
 */
void drawDebugCapsuleY(VECTOR baseCenter, float radius, float height, unsigned int color);

/**
 * @brief 前方向へ伸びる矩形範囲をデバッグ描画する。
 * @param[in] start 矩形の開始位置。
 * @param[in] forward 前方向ベクトル。
 * @param[in] length 前方向の長さ。DxLib ワールド単位。
 * @param[in] halfWidth 横幅の半分。DxLib ワールド単位。
 * @param[in] color DxLib のカラー値。
 * @param[in] yOffset 地面から上へずらす量。DxLib ワールド単位。
 */
void drawDebugForwardRect(VECTOR start, VECTOR forward, float length, float halfWidth, unsigned int color, float yOffset = 0.16f);

/**
 * @brief 前方半円を XZ 平面上にデバッグ描画する。
 * @param[in] center 半円中心のワールド座標。
 * @param[in] forward 半円の向き。
 * @param[in] radius 半径。DxLib ワールド単位。
 * @param[in] color DxLib のカラー値。
 * @param[in] yOffset 地面から上へずらす量。DxLib ワールド単位。
 */
void drawDebugFrontHalfCircleXZ(VECTOR center, VECTOR forward, float radius, unsigned int color, float yOffset = 0.18f);

#endif // !_DEBUG_DRAW_H_
