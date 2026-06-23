#ifndef _GAME_OBJECT_H_
#define _GAME_OBJECT_H_


/**
 * @file
 * @brief 3D モデルを持つゲーム内オブジェクトの共通基底クラスを定義する。
 */

#include "DxLib.h"

/**
 * @brief 毎フレーム更新と描画を持つゲームオブジェクトの抽象基底クラス。
 *
 * DxLib のモデルハンドル、ワールド座標、回転、スケールを共通データとして持つ。
 * 実際のモデル所有権と解放方法は派生クラス側で管理する。
 *
 * @note 座標は DxLib の `VECTOR` を使うワールド空間値。
 * @todo Verify: プロジェクト内で使用している空間単位がメートル相当か DxLib 単位か確認する。
 */
class GameObject {
public:
    /**
     * @brief 共通 transform とモデルハンドルを初期化する。
     */
    GameObject();
    /**
     * @brief 派生クラス経由で安全に破棄するための仮想デストラクタ。
     */
    virtual ~GameObject();

    /**
     * @brief オブジェクトの状態を 1 フレーム分更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) = 0;
    /**
     * @brief 現在の状態を画面へ描画する。
     */
    virtual void draw() = 0;

    /**
     * @brief ワールド座標を設定する。
     * @param[in] pos 新しいワールド座標。
     */
    void setPosition(VECTOR pos) { mPos = pos; }
    /**
     * @brief 現在のワールド座標を取得する。
     * @return オブジェクトのワールド座標。
     */
    VECTOR getPosition() const { return mPos; }

    /**
     * @brief ワールド回転を設定する。
     * @param[in] rot 各軸の回転値。角度単位は呼び出し側の DxLib 利用規約に従う。
     */
    void setRotation(VECTOR rot) { mRot = rot; }
    /**
     * @brief 一様スケールを設定する。
     * @param[in] scale モデルへ適用する倍率。
     */
    void setScale(float scale) { mScale = scale; }

protected:
    int    mModelHandle; ///< DxLib モデルハンドル。無効値や解放責任は派生クラスで管理する。
    VECTOR mPos;         ///< ワールド空間上の現在位置。
    VECTOR mRot;         ///< ワールド空間上の回転値。
    float  mScale;       ///< モデルへ適用する一様スケール倍率。
};

#endif
