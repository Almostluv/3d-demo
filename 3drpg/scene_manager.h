#ifndef _SCENE_MANAGER_H_
#define _SCENE_MANAGER_H_


/**
 * @file
 * @brief 現在シーンと予約済みシーン遷移を管理する SceneManager を定義する。
 */

#include "manager.h"
#include "scene_base.h"
#include <functional>

/**
 * @brief 現在のシーンと次に切り替えるシーンを管理するクラス。
 *
 * シーン切り替え要求は即時生成せず、次の update 内で反映することで、
 * 1 フレーム中の多重切り替えを防ぐ。
 * `currentScene` の所有権は SceneManager が持ち、破棄も SceneManager が行う。
 */
class SceneManager : public Manager<SceneManager> {
    /**
     * @brief Manager から private コンストラクタへアクセスさせる。
     */
    friend class Manager<SceneManager>;

public:
    /**
     * @brief 現在のシーンを更新し、予約されたシーン切り替えを反映する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void update(float deltaTime);

    /**
     * @brief 現在のシーンを描画する。
     */
    void draw();

    /**
     * @brief 現在のシーンを破棄し、シーンを持たない状態に戻す。
     */
    void clearScene();

    /**
     * @brief 次フレームで指定シーンへ切り替える。
     * @tparam T 切り替え先のシーンクラス。
     * @tparam Args シーン生成時に渡す引数型。
     * @param[in] args シーン生成時に渡す引数。
     * @note すでに切り替え予約がある場合、新しい要求は無視される。
     */
    template<typename T, typename... Args>
    void changeScene(Args... args) {
        if (nextSceneFactory) return;
        nextSceneFactory = [=]() -> SceneBase* { return new T(args...); };
    }

private:
    /**
     * @brief シーンを持たない状態で初期化する。
     */
    SceneManager() : currentScene(nullptr), nextSceneFactory(nullptr) {}

    /**
     * @brief 現在シーンと予約済み状態を破棄する。
     */
    ~SceneManager();

    SceneBase* currentScene; ///< 現在実行中のシーン。SceneManager が所有し、nullptr の場合はシーンなし。
    std::function<SceneBase*()> nextSceneFactory; ///< 次回 update で生成するシーンの factory。未予約時は空。
};

#endif
