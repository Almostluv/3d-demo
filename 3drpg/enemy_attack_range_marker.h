#ifndef _ENEMY_ATTACK_RANGE_MARKER_H_
#define _ENEMY_ATTACK_RANGE_MARKER_H_


/**
 * @file
 * @brief 敵の攻撃範囲予告マーカーを描画する free function を宣言する。
 */

#include "DxLib.h"

/**
 * @brief XZ 平面上に敵攻撃の範囲予告マーカーを描画する。
 * @param[in] center マーカー中心のワールド座標。
 * @param[in] radius マーカー半径。DxLib ワールド単位。
 * @param[in] landingSoon 着地や攻撃成立が近い場合に true。
 * @param[in] blinkTimer 点滅表現に使うタイマー値（秒）。
 * @param[in] stageCollisionModelHandle 地面補正に使うステージ衝突モデル。-1 の場合は補正しない。
 */
void drawEnemyAttackRangeMarker(VECTOR center, float radius, bool landingSoon, float blinkTimer, int stageCollisionModelHandle = -1);


#endif // !_ENEMY_ATTACK_RANGE_MARKER_H_


