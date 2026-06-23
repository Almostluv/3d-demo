#ifndef _CHAR_BASE_H_
#define _CHAR_BASE_H_


/**
 * @file
 * @brief プレイヤーと敵に共通するキャラクター基底データを定義する。
 */

#include "game_object.h"
#include "animator.h"

/**
 * @brief CharBase 派生クラスの初期パラメータ。
 *
 * 値はコンストラクタでコピーされ、以後の実行時ステータスの初期値として使われる。
 */
struct CharBaseParam
{
    int hp;                ///< 初期 HP。
    float hitRadius;       ///< 被弾判定半径。DxLib ワールド単位。
    float collisionRadius; ///< 物理押し戻し用の衝突半径。DxLib ワールド単位。
    float collisionHeight; ///< 物理押し戻し用の衝突高さ。DxLib ワールド単位。
    float attackRadius;    ///< 基本攻撃判定半径。DxLib ワールド単位。
    float moveSpeed;       ///< 移動速度。1 秒あたりの DxLib ワールド単位。
    int attackPower;       ///< 基本攻撃力。
};

/**
 * @brief Player と Enemy に共通する HP、当たり判定、アニメーション管理を持つ基底クラス。
 *
 * モデルハンドルの具体的な読み込みと解放は派生クラスが行う。
 * `Animator` は値メンバとして所有され、派生クラスのモデルハンドルに対して初期化される。
 */
class CharBase : public GameObject
{
public:
    /**
     * @brief 初期パラメータをコピーしてキャラクター基底状態を作成する。
     * @param[in] param HP、半径、速度、攻撃力の初期値。
     */
    explicit CharBase(const CharBaseParam& param);

    /**
     * @brief 派生クラス経由で安全に破棄するための仮想デストラクタ。
     */
    virtual ~CharBase();

    /**
     * @brief 現在 HP を取得する。
     * @return 現在 HP。
     */
    int getHp() const { return mHp; }

    /**
     * @brief 被弾判定半径を取得する。
     * @return 半径。DxLib ワールド単位。
     */
    float getHitRadius() const { return mHitRadius; }

    /**
     * @brief 物理衝突半径を取得する。
     * @return 半径。DxLib ワールド単位。
     */
    float getCollisionRadius() const { return mCollisionRadius; }

    /**
     * @brief 物理衝突高さを取得する。
     * @return 高さ。DxLib ワールド単位。
     */
    float getCollisionHeight() const { return mCollisionHeight; }

    /**
     * @brief 基本攻撃判定半径を取得する。
     * @return 半径。DxLib ワールド単位。
     */
    float getAttackRadius() const { return mAttackRadius; }

    /**
     * @brief 基本攻撃力を取得する。
     * @return 攻撃力。
     */
    int getAttackPower() const { return mAttackPower; }

protected:
    /**
     * @brief 内部 transform を DxLib モデルへ反映する。
     * @param[in] addRotY モデル向き補正に加算する Y 軸回転値（radian）。
     */
    void syncModelTransform(float addRotY = DX_PI_F);

    /**
     * @brief 保持中のモデルハンドルを描画する。
     */
    void drawModel() const;

    /**
     * @brief 保持中のモデルハンドルを解放する。
     */
    void deleteModel();

    Animator mAnimator; ///< モデルに紐づくアニメーション再生状態。

    int mHp; ///< 現在 HP。

    float mHitRadius; ///< 被弾判定半径。DxLib ワールド単位。

    float mCollisionRadius; ///< 物理衝突半径。DxLib ワールド単位。

    float mCollisionHeight; ///< 物理衝突高さ。DxLib ワールド単位。

    float mAttackRadius; ///< 基本攻撃判定半径。DxLib ワールド単位。

    float mMoveSpeed; ///< 移動速度。1 秒あたりの DxLib ワールド単位。

    int mAttackPower; ///< 基本攻撃力。
};

#endif
