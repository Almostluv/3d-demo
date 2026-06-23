#ifndef _PLAYER_CHARACTER_TYPE_H_
#define _PLAYER_CHARACTER_TYPE_H_


/**
 * @file
 * @brief プレイヤーが選択できるキャラクター種別を定義する。
 */

/**
 * @brief プレイヤーキャラクターの種類。
 */
enum class PlayerCharacterType
{
    Knight, ///< 近接攻撃とガードを主体にする騎士キャラクター。
    Wizard  ///< 魔法攻撃と支援効果を主体にする魔法使いキャラクター。
};

#endif
