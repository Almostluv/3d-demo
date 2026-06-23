#ifndef _PLAYER_CHARACTER_RESOURCE_H_
#define _PLAYER_CHARACTER_RESOURCE_H_


/**
 * @file
 * @brief プレイヤーキャラクター別のモデルとアニメーションリソースを定義する。
 */


#include "player_character_type.h"

/**
 * @brief 1 キャラクター分のモデル、アニメーション、マテリアル関連パス。
 *
 * 各文字列は静的リテラルを指す非所有ポインタ。呼び出し側は解放しない。
 */
struct PlayerCharacterResourceSet
{
    const char* modelPath;       ///< 本体モデルのパス。
    const char* idlePath;        ///< 待機アニメーションモデルのパス。
    const char* movePath;        ///< 前進または移動アニメーションモデルのパス。
    const char* walkLeftPath;    ///< 左移動アニメーションモデルのパス。
    const char* walkRightPath;   ///< 右移動アニメーションモデルのパス。
    const char* walkBackPath;    ///< 後退アニメーションモデルのパス。
    const char* attack1Path;     ///< 1 段目攻撃アニメーションモデルのパス。
    const char* attack2Path;     ///< 2 段目攻撃アニメーションモデルのパス。
    const char* attack3Path;     ///< 3 段目攻撃アニメーションモデルのパス。
    const char* dodgePath;       ///< 回避アニメーションモデルのパス。
    const char* guardPath;       ///< ガードアニメーションモデルのパス。
    const char* guardImpactPath; ///< ガード成功リアクションのアニメーションモデルのパス。
    const char* counterPath;     ///< カウンターアニメーションモデルのパス。
    const char* jumpPath;        ///< ジャンプアニメーションモデルのパス。
    const char* hitPath;         ///< 被弾アニメーションモデルのパス。
    const char* knockbackPath;   ///< ノックバックアニメーションモデルのパス。
    const char* deadPath;        ///< 死亡アニメーションモデルのパス。
    const char* normalMapPath;   ///< ノーマルマップ画像のパス。
    const char* specularMapPath; ///< スペキュラマップ画像のパス。
};

/**
 * @brief キャラクター種別に対応するリソースセットを取得する。
 * @param[in] characterType 取得対象のキャラクター種別。
 * @return 静的に保持されるリソースセットへの参照。
 */
const PlayerCharacterResourceSet& getPlayerCharacterResourceSet(
    PlayerCharacterType characterType
);

#endif
