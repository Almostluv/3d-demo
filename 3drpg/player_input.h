#ifndef _PLAYER_INPUT_H_
#define _PLAYER_INPUT_H_


/**
 * @file
 * @brief 1 フレーム分のプレイヤー入力状態を表す構造体を定義する。
 */

#include "DxLib.h"

/**
 * @brief Player へ渡す入力スナップショット。
 *
 * シーンまたは AI が毎フレーム値を組み立て、`Player::setInput()` 経由で渡す。
 * 構造体自体は入力デバイスを所有せず、永続状態も保持しない。
 */
struct PlayerInput
{
    VECTOR move; ///< XZ 平面移動に使う方向ベクトル。通常は正規化またはゼロベクトル。

    bool attack;  ///< 通常攻撃の開始要求。
    bool dodge;   ///< 回避行動の開始要求。
    bool guard;   ///< ガード入力が押されているかどうか。
    bool jump;    ///< ジャンプの開始要求。
    bool counter; ///< カウンター行動の開始要求。

    /**
     * @brief 入力なしの状態で初期化する。
     */
    PlayerInput()
        : move(VGet(0, 0, 0))
        , attack(false)
        , dodge(false)
        , guard(false)
        , jump(false)
        , counter(false)
    {
    }
};

#endif
