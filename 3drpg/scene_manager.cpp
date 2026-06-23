/**
 * @file
 * @brief scene_manager.cpp 実装を定義する。
 */

#include "scene_manager.h"

SceneManager::~SceneManager()
{
    /// SceneManager が破棄されるタイミングでも、保持中のシーンを残さない。
    clearScene();
}

void SceneManager::clearScene()
{
    if (currentScene)
    {
        /// SceneBase はポリモーフィックに扱うため、所有している現在シーンをここで delete する。
        delete currentScene;
        currentScene = nullptr;
    }

    nextSceneFactory = nullptr;
}

void SceneManager::update(float deltaTime)
{
    if (nextSceneFactory)
    {
        /// changeScene() で予約された生成処理は update 冒頭で実行し、描画中の差し替えを避ける。
        SceneBase* newScene = nextSceneFactory();

        nextSceneFactory = nullptr;

        if (currentScene)
        {
            /// 新しいシーンへ切り替える前に、前シーンのリソース解放を完了させる。
            delete currentScene;
        }

        currentScene = newScene;
    }

    if (currentScene)
    {
        /// 現在有効なシーンだけを 1 フレーム分更新する。
        currentScene->update(deltaTime);
    }
}

void SceneManager::draw()
{
    if (currentScene)
    {
        /// 描画順序は各シーン内部で管理する。
        currentScene->draw();
    }
}
