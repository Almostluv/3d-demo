#ifndef _SCENE_BASE_H_
#define _SCENE_BASE_H_


/**
 * @file
 * @brief ゲームシーン共通の抽象インターフェイスを定義する。
 */

/**
 * @brief すべてのシーンが継承する基底クラス。
 *
 * SceneManager から毎フレーム呼び出される更新処理と描画処理だけを定義する。
 * SceneBase 自身はリソースを所有せず、派生シーンが必要なハンドルやオブジェクトを管理する。
 */
class SceneBase {
public:
    /**
     * @brief シーン破棄時の仮想デストラクタ。
     */
    virtual ~SceneBase() = default;

    /**
     * @brief シーンの状態を 1 フレーム分更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) = 0;

    /**
     * @brief シーンを描画する。
     */
    virtual void draw() = 0;
};

#endif
