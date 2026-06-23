#ifndef _TITLE_SCENE_H_
#define _TITLE_SCENE_H_


/**
 * @file
 * @brief タイトル画面シーンを定義する。
 */


#include "scene_base.h"

/**
 * @brief タイトル画面の入力、メニュー選択、画面描画を担当するシーン。
 *
 * SceneManager により生成・破棄され、DxLib の画像ハンドルをシーン内で保持する。
 */
class TitleScene : public SceneBase
{
public:
    /**
     * @brief タイトル画面に必要な画像リソースと初期選択状態を準備する。
     */
    TitleScene();

    /**
     * @brief タイトル画面の派生破棄用デストラクタ。
     */
    virtual ~TitleScene() = default;

    /**
     * @brief メニュー入力とクリック演出を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) override;

    /**
     * @brief 背景、ロゴ、メニューボタン、ポインタを描画する。
     */
    virtual void draw() override;

private:
    /**
     * @brief 指定スクリーン座標が開始ボタン内か判定する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return ボタン内なら true。
     */
    bool isInsideStartButton(int x, int y) const;

    /**
     * @brief 指定スクリーン座標が設定ボタン内か判定する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return ボタン内なら true。
     */
    bool isInsideSettingButton(int x, int y) const;

    /**
     * @brief 指定スクリーン座標が終了ボタン内か判定する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return ボタン内なら true。
     */
    bool isInsideExitButton(int x, int y) const;

    /**
     * @brief 指定スクリーン座標上のメニュー番号を取得する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return ホバー中のメニュー番号。該当なしの場合の値は実装に従う。
     */
    int getHoveredMenu(int x, int y) const;

    /**
     * @brief メニュー決定演出を開始する。
     * @param[in] menuIndex 決定したメニュー番号。
     */
    void startMenuAction(int menuIndex);

    /**
     * @brief 演出完了後に対応するメニュー処理を実行する。
     * @param[in] menuIndex 実行するメニュー番号。
     */
    void executeMenuAction(int menuIndex);

    /**
     * @brief メニューボタン画像を描画する。
     * @param[in] graphHandle ボタン画像の DxLib グラフィックハンドル。
     * @param[in] menuIndex メニュー番号。
     * @param[in] y 描画 Y 座標（pixel）。
     */
    void drawMenuButton(int graphHandle, int menuIndex, int y) const;

    int mTitleGraphHandle;      ///< タイトルロゴ画像の DxLib グラフィックハンドル。
    int mBackgroundGraphHandle; ///< 背景画像の DxLib グラフィックハンドル。
    int mStartGraphHandle;      ///< 開始ボタン画像の DxLib グラフィックハンドル。
    int mSettingGraphHandle;    ///< 設定ボタン画像の DxLib グラフィックハンドル。
    int mExitGraphHandle;       ///< 終了ボタン画像の DxLib グラフィックハンドル。
    int mPointerGraphHandle;    ///< 選択ポインタ画像の DxLib グラフィックハンドル。
    int mSelectedMenu;          ///< 現在選択中のメニュー番号。
    int mClickedMenu;           ///< クリック演出中のメニュー番号。
    int mPendingMenu;           ///< 演出完了後に実行するメニュー番号。
    int mLastMouseX;            ///< 前回記録したマウス X 座標（pixel）。
    int mLastMouseY;            ///< 前回記録したマウス Y 座標（pixel）。
    float mClickEffectTimer;    ///< クリック視覚効果の経過時間（秒）。
    float mClickActionTimer;    ///< メニュー処理実行までの待機時間（秒）。
    float mPointerAnimTimer;    ///< 選択ポインタのアニメーション時間（秒）。
};

#endif
