#ifndef _GAME_MANAGER_H_
#define _GAME_MANAGER_H_


/**
 * @file
 * @brief アプリケーション全体の初期化、メインループ、終了処理を管理する GameManager を定義する。
 */

#include "manager.h"

/**
 * @brief アプリケーション全体の実行ループを管理するクラス。
 *
 * DxLib の初期化、メインループ、現在シーンの更新と描画、終了処理を担当する。
 * インスタンスは `Manager<GameManager>` 経由で取得し、呼び出し側は所有しない。
 */
class GameManager : public Manager<GameManager>
{
    /**
     * @brief Manager から private コンストラクタへアクセスさせる。
     */
    friend class Manager<GameManager>;

public:

    /**
     * @brief ゲームを起動し、終了要求が出るまでメインループを実行する。
     * @return 正常終了時は 0、初期化失敗時は -1。
     */
    int run();

    /**
     * @brief メインループへ終了要求を送る。
     */
    void requestExit();

private:

    /**
     * @brief 終了要求フラグを初期状態へ設定する。
     */
    GameManager();

    /**
     * @brief メインループ終了後の破棄に備える。
     */
    ~GameManager();

    /**
     * @brief DxLib とゲーム全体の初期化を行う。
     * @return 成功時は 0、失敗時は負値。
     */
    int init();

    /**
     * @brief 1 フレーム分のゲーム状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void update(float deltaTime);

    /**
     * @brief 現在フレームを描画する。
     */
    void draw();

    /**
     * @brief DxLib とゲーム全体の終了処理を行う。
     */
    void finalize();

    bool mShouldExit; ///< メインループ終了要求が出ているかどうか。
};

#endif
