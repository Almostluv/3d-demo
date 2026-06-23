#ifndef _LOADING_SCENE_H_
#define _LOADING_SCENE_H_


/**
 * @file
 * @brief ローディング表示用シーンを定義する。
 */

#include "scene_base.h"

/**
 * @brief ロード中の背景とアニメーションを表示するシーン。
 *
 * `SceneBase` と同じく `SceneManager` から毎フレーム更新・描画される。
 * グラフィックとフォントのハンドルはシーンのライフタイム内で管理する。
 */
class LoadingScene : public SceneBase
{
public:
    /**
     * @brief ローディング用リソースと初期状態を準備する。
     */
    LoadingScene();

    /**
     * @brief シーンが保持する DxLib リソースを解放する。
     */
    virtual ~LoadingScene();

    /**
     * @brief ローディング演出を 1 フレーム進める。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) override;

    /**
     * @brief ローディング画面を描画する。
     */
    virtual void draw() override;

private:
    int mBackgroundGraphHandle; ///< 背景画像の DxLib グラフィックハンドル。
    int mFontHandle;            ///< ローディング文字に使う DxLib フォントハンドル。
    int mTaskIndex;             ///< 現在処理中または表示対象のロードタスク番号。
    float mAnimTimer;           ///< ローディング演出に使う経過時間（秒）。
};

#endif
