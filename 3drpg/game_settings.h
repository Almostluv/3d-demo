#ifndef _GAME_SETTINGS_H_
#define _GAME_SETTINGS_H_


/**
 * @file
 * @brief ユーザー設定と入力割り当てを管理する GameSettings を定義する。
 */


#include "manager.h"

/**
 * @brief ゲームプレイ中に割り当て可能な入力アクション。
 */
enum GameInputAction
{
    GameActionMoveForward, ///< 前進。
    GameActionMoveBack,    ///< 後退。
    GameActionMoveLeft,    ///< 左移動。
    GameActionMoveRight,   ///< 右移動。
    GameActionPrimary,     ///< 主攻撃または決定系アクション。
    GameActionDodge,       ///< 回避。
    GameActionSecondary,   ///< ガードなどの副アクション。
    GameActionSpecial,     ///< キャラクター固有の特殊アクション。
    GameActionLockOn,      ///< ロックオン切り替え。
    GameActionCount        ///< 配列サイズ用の番兵値。通常のアクションとしては使わない。
};

/**
 * @brief 入力割り当てのデバイス種別。
 */
enum InputBindingType
{
    InputBindingKey,   ///< キーボードキー。
    InputBindingMouse, ///< マウスボタン。
    InputBindingPad    ///< ゲームパッドボタンまたはトリガー。
};

/**
 * @brief ゲームパッド左トリガーを binding code として扱うための特殊値。
 */
constexpr int PadBindingLeftTrigger = 100;

/**
 * @brief ゲームパッド右トリガーを binding code として扱うための特殊値。
 */
constexpr int PadBindingRightTrigger = 101;

/**
 * @brief 1 つのアクションに割り当てる入力種別とコード。
 */
struct InputBinding
{
    int type; ///< `InputBindingType` の値。
    int code; ///< キーコード、マウスボタン、ゲームパッドボタン、または特殊トリガー値。
};

/**
 * @brief 音量、表示、カメラ、入力割り当てのユーザー設定を保持する管理クラス。
 *
 * 現在の実装ではメモリ上の設定値を保持する。ファイル保存の有無はこのヘッダーからは確認できない。
 */
class GameSettings : public Manager<GameSettings>
{
    /**
     * @brief Manager から private コンストラクタへアクセスさせる。
     */
    friend class Manager<GameSettings>;

public:
    /**
     * @brief マスター音量を取得する。
     * @return 音量パーセント。
     */
    int getMasterVolume() const { return mMasterVolume; }

    /**
     * @brief BGM 音量を取得する。
     * @return 音量パーセント。
     */
    int getBgmVolume() const { return mBgmVolume; }

    /**
     * @brief SE 音量を取得する。
     * @return 音量パーセント。
     */
    int getSeVolume() const { return mSeVolume; }

    /**
     * @brief マウス感度設定値を取得する。
     * @return 感度設定値。
     */
    int getMouseSensitivity() const { return mMouseSensitivity; }

    /**
     * @brief マウス Y 軸反転が有効か取得する。
     * @return 有効なら true。
     */
    bool isInvertMouseY() const { return mInvertMouseY; }

    /**
     * @brief フルスクリーン設定が有効か取得する。
     * @return 有効なら true。
     */
    bool isFullscreen() const { return mFullscreen; }

    /**
     * @brief 自動ロックオンが有効か取得する。
     * @return 有効なら true。
     */
    bool isAutoLockTarget() const { return mAutoLockTarget; }

    /**
     * @brief 指定ロールとアクションの入力割り当てを取得する。
     * @param[in] role 入力ロール番号。
     * @param[in] action `GameInputAction` の値。
     * @return 入力割り当て。
     */
    InputBinding getInputBinding(int role, int action) const;

    /**
     * @brief 指定アクションのゲームパッド割り当てを取得する。
     * @param[in] action `GameInputAction` の値。
     * @return ゲームパッド入力割り当て。
     */
    InputBinding getPadBinding(int action) const;

    /**
     * @brief マスター音量へ差分を加える。
     * @param[in] value 加算する差分。
     */
    void addMasterVolume(int value);

    /**
     * @brief BGM 音量へ差分を加える。
     * @param[in] value 加算する差分。
     */
    void addBgmVolume(int value);

    /**
     * @brief SE 音量へ差分を加える。
     * @param[in] value 加算する差分。
     */
    void addSeVolume(int value);

    /**
     * @brief マウス感度へ差分を加える。
     * @param[in] value 加算する差分。
     */
    void addMouseSensitivity(int value);

    /**
     * @brief 指定ロールとアクションの入力割り当てを設定する。
     * @param[in] role 入力ロール番号。
     * @param[in] action `GameInputAction` の値。
     * @param[in] binding 設定する入力割り当て。
     */
    void setInputBinding(int role, int action, InputBinding binding);

    /**
     * @brief すべてのロールへ同じ入力割り当てを設定する。
     * @param[in] action `GameInputAction` の値。
     * @param[in] binding 設定する入力割り当て。
     */
    void setInputBindingAllRoles(int action, InputBinding binding);

    /**
     * @brief 指定アクションのゲームパッド割り当てを設定する。
     * @param[in] action `GameInputAction` の値。
     * @param[in] binding 設定する入力割り当て。
     */
    void setPadBinding(int action, InputBinding binding);

    /**
     * @brief 入力割り当てを表示用テキストへ変換する。
     * @param[in] binding 変換する入力割り当て。
     * @param[out] text 出力先バッファ。
     * @param[in] textSize 出力先バッファサイズ。
     */
    void getInputBindingText(InputBinding binding, char* text, int textSize) const;

    /**
     * @brief 操作設定を初期値へ戻す。
     */
    void resetControlSettings();

    /**
     * @brief マウス Y 軸反転設定を切り替える。
     */
    void toggleInvertMouseY();

    /**
     * @brief フルスクリーン設定を切り替える。
     */
    void toggleFullscreen();

    /**
     * @brief 自動ロックオン設定を切り替える。
     */
    void toggleAutoLockTarget();

    /**
     * @brief カメラ感度に適用する倍率を取得する。
     * @return 感度倍率。
     */
    float getCameraSensitivityScale() const;

private:
    /**
     * @brief 設定値を初期値で生成する。
     */
    GameSettings();

    /**
     * @brief 外部リソースを所有しないため明示解放は行わない。
     */
    ~GameSettings() = default;

    /**
     * @brief パーセント値を有効範囲へ丸める。
     * @param[in] value 丸める値。
     * @return 丸め後の値。
     */
    int clampPercent(int value) const;

    int mMasterVolume; ///< マスター音量パーセント。
    int mBgmVolume; ///< BGM 音量パーセント。
    int mSeVolume; ///< SE 音量パーセント。
    int mMouseSensitivity; ///< マウス感度設定値。
    bool mInvertMouseY; ///< マウス Y 軸を反転するかどうか。
    bool mFullscreen; ///< フルスクリーン表示を使うかどうか。
    bool mAutoLockTarget; ///< 自動ロックオンを使うかどうか。
    InputBinding mInputBindings[2][GameActionCount]; ///< ロール別キーボード/マウス入力割り当て。
    InputBinding mPadBindings[GameActionCount]; ///< ゲームパッド入力割り当て。
};

#endif
