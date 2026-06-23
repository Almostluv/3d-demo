#ifndef _PLAYER_STATE_H_
#define _PLAYER_STATE_H_


/**
 * @file
 * @brief プレイヤーキャラクターの行動状態を定義する。
 */

/**
 * @brief Player の状態機械で使用する行動状態。
 */
enum class PlayerState
{
    Idle,        ///< 待機中。
    Move,        ///< 移動中。
    Attack,      ///< 通常攻撃または魔法攻撃中。
    Dodge,       ///< 回避中。
    Guard,       ///< ガード入力を維持している状態。
    GuardImpact, ///< ガード成功時のリアクション中。
    Counter,     ///< カウンター攻撃中。
    Jump,        ///< ジャンプ中。
    Hit,         ///< 被ダメージリアクション中。
    Dead         ///< 死亡状態。
};

#endif
