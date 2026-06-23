#ifndef _RESULT_SCENE_H_
#define _RESULT_SCENE_H_


/**
 * @file
 * @brief リザルト画面シーンを定義する。
 */


#include "player_character_type.h"
#include "scene_base.h"

/**
 * @brief 勝敗結果を表示し、リトライまたはタイトル復帰を選択するシーン。
 *
 * 前回の使用キャラクターとリトライ先を保持し、入力に応じて次シーンへ遷移する。
 */
class ResultScene : public SceneBase
{
public:
    /**
     * @brief リザルト表示内容とリトライ先を指定して生成する。
     * @param[in] isVictory 勝利リザルトなら true。
     * @param[in] playerType リトライ時に使用するキャラクター種別。
     * @param[in] retryVampireScene true の場合は VampireScene へリトライする。
     */
    ResultScene(bool isVictory, PlayerCharacterType playerType, bool retryVampireScene = false);

    /**
     * @brief リザルト画面で確保した DxLib リソースを解放する。
     */
    virtual ~ResultScene();

    /**
     * @brief メニュー選択と決定入力を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) override;

    /**
     * @brief リザルト文言、選択肢、入力ヒントを描画する。
     */
    virtual void draw() override;

private:
    /**
     * @brief リザルト画面の選択項目。
     */
    enum ResultItem
    {
        Retry,     ///< 現在のボス戦をやり直す。
        Title,     ///< タイトル画面へ戻る。
        ItemCount  ///< 選択項目数。
    };

    /**
     * @brief 選択項目を上下に移動する。
     * @param[in] direction 移動方向。正負で上下を表す。
     */
    void moveSelection(int direction);

    /**
     * @brief 現在選択中の項目を実行する。
     */
    void decideSelected();

    /**
     * @brief リザルト用テキストを描画する。
     * @param[in] x 描画 X 座標（pixel）。
     * @param[in] y 描画 Y 座標（pixel）。
     * @param[in] text 表示文字列。呼び出し中は有効である必要がある。
     * @param[in] color DxLib のカラー値。
     */
    void drawText(int x, int y, const char* text, int color) const;

    /**
     * @brief ゲームパッド入力ヒントを描画する。
     * @param[in] handle ボタン画像の DxLib グラフィックハンドル。
     * @param[in] x 描画 X 座標（pixel）。
     * @param[in] y 描画 Y 座標（pixel）。
     * @param[in] text 説明文字列。
     * @param[in] color DxLib のカラー値。
     */
    void drawPadHint(int handle, int x, int y, const char* text, int color) const;

    /**
     * @brief キーボード入力ヒントを描画する。
     * @param[in] x 描画 X 座標（pixel）。
     * @param[in] y 描画 Y 座標（pixel）。
     * @param[in] key キー名。
     * @param[in] text 説明文字列。
     * @param[in] color DxLib のカラー値。
     */
    void drawKeyHintBox(int x, int y, const char* key, const char* text, int color) const;

    /**
     * @brief 指定スクリーン座標上の項目番号を取得する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return 項目番号。該当なしの場合の値は実装に従う。
     */
    int getMouseItem(int x, int y) const;

    /**
     * @brief 項目番号に対応する描画 Y 座標を取得する。
     * @param[in] item 項目番号。
     * @return Y 座標（pixel）。
     */
    int getItemY(int item) const;

    bool mIsVictory;                 ///< 勝利リザルトかどうか。
    PlayerCharacterType mPlayerType; ///< リトライ時に引き継ぐキャラクター種別。
    bool mRetryVampireScene;         ///< リトライ先が VampireScene かどうか。
    int mSelectedItem;               ///< 現在選択中の `ResultItem`。
    int mFontHandle;                 ///< リザルト文字に使う DxLib フォントハンドル。
    int mAGraphHandle;               ///< 決定ボタン表示用のグラフィックハンドル。
    int mDpadUpGraphHandle;          ///< 上方向キー表示用のグラフィックハンドル。
    int mDpadDownGraphHandle;        ///< 下方向キー表示用のグラフィックハンドル。
};

#endif
