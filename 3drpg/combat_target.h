#ifndef _COMBAT_TARGET_H_
#define _COMBAT_TARGET_H_


/**
 * @file
 * @brief 敵 AI が攻撃対象を評価するためのターゲット情報を定義する。
 */

#include "DxLib.h"
#include "threat_component.h"

/**
 * @brief Enemy のターゲット選択に渡す候補情報。
 *
 * プレイヤーや相棒など、敵が注目できる対象を 1 件分表す。
 * この構造体は対象オブジェクトを所有せず、評価時点のスナップショットだけを保持する。
 */
struct CombatTarget
{
    ThreatSource source; ///< 脅威値管理で使う対象の種類。
    VECTOR position;     ///< 対象のワールド座標。
    bool active;         ///< 評価対象として有効かどうか。
    bool attacking;      ///< 対象が攻撃中かどうか。
    bool support;        ///< 支援行動中の対象として扱うかどうか。
};

#endif
