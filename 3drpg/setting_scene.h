#ifndef _SETTING_SCENE_H_
#define _SETTING_SCENE_H_


/**
 * @file
 * @brief 設定画面シーンを定義する。
 */


#include "game_settings.h"
#include "scene_base.h"

/**
 * @brief 設定画面を閉じた後の戻り先。
 */
enum SettingReturnMode
{
    SettingReturnTitle,       ///< タイトル画面へ戻る。
    SettingReturnCharaSelect, ///< キャラクター選択画面へ戻る。
    SettingReturnOverlay      ///< 呼び出し元シーン上のオーバーレイとして閉じる。
};

/**
 * @brief 音量、表示、操作設定を編集するシーン。
 *
 * `GameSettings` の値を読み書きし、戻り先に応じて単体シーンまたは
 * ポーズメニュー内オーバーレイとして使われる。
 */
class SettingScene : public SceneBase
{
public:
    /**
     * @brief 設定画面内のページ種別。
     */
    enum SettingPage
    {
        GamePage,          ///< ゲームプレイ設定ページ。
        ScreenPage,        ///< 画面設定ページ。
        SoundPage,         ///< 音量設定ページ。
        ControllerPage,    ///< ゲームパッド設定ページ。
        KeyboardMousePage, ///< キーボード・マウス設定ページ。
        SettingPageCount   ///< ページ数。
    };

    /**
     * @brief 戻り先を指定して設定シーンを生成する。
     * @param[in] returnMode 設定画面終了後の戻り先。
     */
    explicit SettingScene(SettingReturnMode returnMode = SettingReturnTitle);

    /**
     * @brief 設定シーンは外部リソースを明示所有しない。
     */
    virtual ~SettingScene() = default;

    /**
     * @brief オーバーレイ利用時に設定画面が閉じられたか確認する。
     * @return 終了済みなら true。
     */
    bool isFinished() const { return mIsFinished; }

    /**
     * @brief 入力、項目選択、割り当て待ち状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) override;

    /**
     * @brief 現在ページの設定項目と入力ヒントを描画する。
     */
    virtual void draw() override;

private:
    /**
     * @brief 入力割り当て編集モード。
     */
    enum ConfigMode
    {
        ConfigNone,          ///< 割り当て編集をしていない。
        ConfigController,    ///< ゲームパッド割り当て編集中。
        ConfigKeyboardMouse  ///< キーボード・マウス割り当て編集中。
    };

    /**
     * @brief 設定画面内で選択できる項目。
     */
    enum SettingItem
    {
        MasterVolume,     ///< マスター音量。
        BgmVolume,        ///< BGM 音量。
        SeVolume,         ///< SE 音量。
        MouseSensitivity, ///< マウス感度。
        InvertMouseY,     ///< マウス Y 軸反転。
        ButtonConfig,     ///< ゲームパッド割り当て編集。
        KeyConfig,        ///< キーボード・マウス割り当て編集。
        Fullscreen,       ///< フルスクリーン設定。
        AutoLockTarget,   ///< 自動ロックオン設定。
        Back,             ///< 前画面へ戻る。
        ItemCount         ///< 項目数。
    };

    /**
     * @brief 現在ページ内の選択項目を移動する。
     * @param[in] direction 移動方向。
     */
    void moveSelection(int direction);

    /**
     * @brief 現在選択中の項目値を増減する。
     * @param[in] direction 増減方向。
     * @param[in] playSound true の場合は操作 SE を再生する。
     */
    void adjustSelected(int direction, bool playSound = true);

    /**
     * @brief 現在選択中の項目を決定する。
     */
    void decideSelected();

    /**
     * @brief 表示ページを切り替える。
     * @param[in] page 切り替え先ページ番号。
     */
    void switchPage(int page);

    /**
     * @brief 1 つのページタブを描画する。
     * @param[in] page ページ番号。
     * @param[in] x 描画 X 座標（pixel）。
     * @param[in] label 表示ラベル。呼び出し中は有効である必要がある。
     */
    void drawPageTab(int page, int x, const char* label) const;

    /**
     * @brief ページタブ一覧を描画する。
     */
    void drawPageTabs() const;

    /**
     * @brief ゲームパッド設定ページの操作ガイドを描画する。
     */
    void drawControllerGuide() const;

    /**
     * @brief 現在の入力割り当て一覧を描画する。
     */
    void drawBindingList() const;

    /**
     * @brief 1 アクション分の入力割り当て表示を描画する。
     * @param[in] action `GameInputAction` の値。
     * @param[in] binding 表示する入力割り当て。
     * @param[in] x 描画 X 座標（pixel）。
     * @param[in] y 描画 Y 座標（pixel）。
     * @param[in] color DxLib のカラー値。
     * @param[in] conflict 競合中なら true。
     */
    void drawBindingValue(int action, InputBinding binding, int x, int y, int color, bool conflict) const;

    /**
     * @brief ゲームパッド割り当てに対応するボタン画像を取得する。
     * @param[in] binding 入力割り当て。
     * @return DxLib グラフィックハンドル。該当なしの場合の値は実装に従う。
     */
    int getPadBindingGraphHandle(InputBinding binding) const;

    /**
     * @brief 1 つの設定項目行を描画する。
     * @param[in] item 項目番号。
     * @param[in] y 描画 Y 座標（pixel）。
     * @param[in] label 項目ラベル。
     * @param[in] value 表示値。
     */
    void drawItem(int item, int y, const char* label, const char* value) const;

    /**
     * @brief ボタン画像付きの操作ヒントを描画する。
     * @param[in] handle ボタン画像の DxLib グラフィックハンドル。
     * @param[in] x 描画 X 座標（pixel）。
     * @param[in] y 描画 Y 座標（pixel）。
     * @param[in] text 説明文字列。
     * @param[in] color DxLib のカラー値。
     */
    void drawButtonHint(int handle, int x, int y, const char* text, int color) const;

    /**
     * @brief 入力割り当て待ち状態の案内を描画する。
     */
    void drawWaitingPrompt() const;

    /**
     * @brief 入力割り当て競合の警告を描画する。
     */
    void drawConflictPrompt() const;

    /**
     * @brief 指定項目の表示値テキストを作成する。
     * @param[in] item 項目番号。
     * @param[out] value 出力先バッファ。
     * @param[in] valueSize 出力先バッファサイズ。
     */
    void makeValueText(int item, char* value, int valueSize) const;

    /**
     * @brief 現在選択中の設定項目を初期値へ戻す。
     */
    void resetSelectedDefault();

    /**
     * @brief 割り当て待ち中のアクションを初期割り当てへ戻す。
     */
    void resetWaitingBindingDefault();

    /**
     * @brief 指定項目が決定操作を持つか判定する。
     * @param[in] item 項目番号。
     * @return 決定操作を持つなら true。
     */
    bool isConfirmItem(int item) const;

    /**
     * @brief 指定項目が初期値リセット可能か判定する。
     * @param[in] item 項目番号。
     * @return リセット可能なら true。
     */
    bool isResettableItem(int item) const;

    /**
     * @brief キーボードまたはマウスの新しい割り当て入力を読み取る。
     * @param[out] binding 読み取った入力割り当て。
     * @return 入力を読み取れた場合は true。
     */
    bool readPressedBinding(InputBinding* binding) const;

    /**
     * @brief ゲームパッドの新しい割り当て入力を読み取る。
     * @param[out] binding 読み取った入力割り当て。
     * @return 入力を読み取れた場合は true。
     */
    bool readPressedPadBinding(InputBinding* binding) const;

    /**
     * @brief 現在のページに入力割り当て競合があるか確認する。
     * @return 競合があるなら true。
     */
    bool hasBindingConflict() const;

    /**
     * @brief キーボード・マウス割り当てに競合があるか確認する。
     * @return 競合があるなら true。
     */
    bool hasKeyboardBindingConflict() const;

    /**
     * @brief ゲームパッド割り当てに競合があるか確認する。
     * @return 競合があるなら true。
     */
    bool hasPadBindingConflict() const;

    /**
     * @brief 指定アクションのキーボード・マウス割り当てが競合しているか確認する。
     * @param[in] action `GameInputAction` の値。
     * @return 競合していれば true。
     */
    bool isKeyboardBindingConflictAction(int action) const;

    /**
     * @brief 指定アクションのゲームパッド割り当てが競合しているか確認する。
     * @param[in] action `GameInputAction` の値。
     * @return 競合していれば true。
     */
    bool isPadBindingConflictAction(int action) const;

    /**
     * @brief 現在ページに表示する項目数を取得する。
     * @return 項目数。
     */
    int getPageItemCount() const;

    /**
     * @brief 現在ページ内インデックスから設定項目番号を取得する。
     * @param[in] index ページ内インデックス。
     * @return `SettingItem` の値。
     */
    int getPageItem(int index) const;

    /**
     * @brief 指定項目のページ内行番号を取得する。
     * @param[in] item 項目番号。
     * @return 行番号。
     */
    int getItemRow(int item) const;

    /**
     * @brief 指定項目の描画 Y 座標を取得する。
     * @param[in] item 項目番号。
     * @return Y 座標（pixel）。
     */
    int getItemY(int item) const;

    /**
     * @brief 指定スクリーン座標上の設定項目を取得する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return 項目番号。該当なしの場合の値は実装に従う。
     */
    int getMouseItem(int x, int y) const;

    /**
     * @brief 指定スクリーン座標上の入力割り当てアクションを取得する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return アクション番号。該当なしの場合の値は実装に従う。
     */
    int getMouseBindingAction(int x, int y) const;

    /**
     * @brief 指定スクリーン座標が値調整ボタン上なら調整方向を返す。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return 調整方向。該当なしの場合の値は実装に従う。
     */
    int getMouseAdjustDirection(int x, int y) const;

    /**
     * @brief 指定スクリーン座標上のページタブを取得する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return ページ番号。該当なしの場合の値は実装に従う。
     */
    int getMousePage(int x, int y) const;

    /**
     * @brief 指定スクリーン座標が戻るボタン内か判定する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return ボタン内なら true。
     */
    bool isInsideBackButton(int x, int y) const;

    /**
     * @brief 戻り先に応じて設定画面を閉じる。
     */
    void closeSetting();

    int mCurrentPage; ///< 現在表示中の `SettingPage`。
    int mConfigMode; ///< 現在の `ConfigMode`。
    int mSelectedItem; ///< 現在選択中の `SettingItem`。
    bool mIsWaitingBinding; ///< 新しい入力割り当て待ち状態かどうか。
    int mHoveredBindingAction; ///< マウスホバー中の入力割り当てアクション。
    int mWaitingAction; ///< 割り当て待ち中の `GameInputAction`。
    float mConflictPromptTimer; ///< 割り当て競合警告の表示タイマー（秒）。
    SettingReturnMode mReturnMode; ///< 設定画面終了後の戻り先。
    bool mIsFinished; ///< オーバーレイ利用時に閉じられたかどうか。
    int mControllerGraphHandle; ///< コントローラー画像の DxLib グラフィックハンドル。
    int mLbGraphHandle; ///< LB ボタン画像の DxLib グラフィックハンドル。
    int mRbGraphHandle; ///< RB ボタン画像の DxLib グラフィックハンドル。
    int mLeftStickGraphHandle; ///< 左スティック画像の DxLib グラフィックハンドル。
    int mRightStickGraphHandle; ///< 右スティック画像の DxLib グラフィックハンドル。
    int mLtGraphHandle; ///< LT ボタン画像の DxLib グラフィックハンドル。
    int mRtGraphHandle; ///< RT ボタン画像の DxLib グラフィックハンドル。
    int mAGraphHandle; ///< A ボタン画像の DxLib グラフィックハンドル。
    int mBGraphHandle; ///< B ボタン画像の DxLib グラフィックハンドル。
    int mYGraphHandle; ///< Y ボタン画像の DxLib グラフィックハンドル。
    int mXGraphHandle; ///< X ボタン画像の DxLib グラフィックハンドル。
    int mStartGraphHandle; ///< START ボタン画像の DxLib グラフィックハンドル。
    int mViewGraphHandle; ///< VIEW ボタン画像の DxLib グラフィックハンドル。
    int mDpadUpGraphHandle; ///< 十字キー上画像の DxLib グラフィックハンドル。
    int mDpadRightGraphHandle; ///< 十字キー右画像の DxLib グラフィックハンドル。
    int mDpadDownGraphHandle; ///< 十字キー下画像の DxLib グラフィックハンドル。
    int mDpadLeftGraphHandle; ///< 十字キー左画像の DxLib グラフィックハンドル。
};

#endif
