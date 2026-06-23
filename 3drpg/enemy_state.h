#ifndef _ENEMY_STATE_H_
#define _ENEMY_STATE_H_


/**
 * @file
 * @brief Mutant 系ボス敵の行動状態を定義する。
 */

/**
 * @brief Enemy の状態機械で使用する行動状態。
 */
enum class EnemyState
{
    Idle,       ///< 待機中。
    Chase,      ///< ターゲットへ接近中。
    Attack,     ///< 通常攻撃中。
    Charging,   ///< 突進攻撃の予備動作中。
    Ram,        ///< 突進攻撃中。
    Attack360,  ///< 周囲を巻き込む回転攻撃中。
    JumpAttack, ///< ジャンプ攻撃中。
    Hit,        ///< 被ダメージリアクション中。
    Dying       ///< 死亡演出中。
};

#endif // !_ENEMY_STATE_H_
