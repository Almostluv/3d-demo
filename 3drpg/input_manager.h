#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_


/**
 * @file
 * @brief キーボード、マウス、ゲームパッド入力を管理する InputManager を定義する。
 */


#include "manager.h"
#include "DxLib.h"

/**
 * @brief キーボード、マウス、XInput ゲームパッドの現在/前フレーム状態を保持する管理クラス。
 *
 * 毎フレーム `update()` を 1 回呼び、`isPush()` などの差分判定を同じフレーム内で利用する。
 */
class InputManager : public Manager<InputManager> {
    /**
     * @brief Manager から private コンストラクタへアクセスさせる。
     */
    friend class Manager<InputManager>;

public:
    /**
     * @brief 入力デバイスの状態を現在フレーム用に更新する。
     */
    void update();

    /**
     * @brief キーがこのフレームで押されたか確認する。
     * @param[in] keyCode DxLib のキーコード。
     * @return 押した瞬間なら true。
     */
    bool isPush(int keyCode) const;

    /**
     * @brief キーが押され続けているか確認する。
     * @param[in] keyCode DxLib のキーコード。
     * @return 押下中なら true。
     */
    bool isPress(int keyCode) const;

    /**
     * @brief キーがこのフレームで離されたか確認する。
     * @param[in] keyCode DxLib のキーコード。
     * @return 離した瞬間なら true。
     */
    bool isRelease(int keyCode) const;

    /**
     * @brief マウスボタンがこのフレームで押されたか確認する。
     * @param[in] mouseButton DxLib のマウスボタンビット。
     * @return 押した瞬間なら true。
     */
    bool isMousePush(int mouseButton) const;

    /**
     * @brief マウスボタンが押され続けているか確認する。
     * @param[in] mouseButton DxLib のマウスボタンビット。
     * @return 押下中なら true。
     */
    bool isMousePress(int mouseButton) const;

    /**
     * @brief マウスボタンがこのフレームで離されたか確認する。
     * @param[in] mouseButton DxLib のマウスボタンビット。
     * @return 離した瞬間なら true。
     */
    bool isMouseRelease(int mouseButton) const;

    /**
     * @brief ゲームパッドボタンがこのフレームで押されたか確認する。
     * @param[in] button XInput のボタンビット。
     * @return 押した瞬間なら true。
     */
    bool isPadPush(int button) const;

    /**
     * @brief ゲームパッドボタンが押され続けているか確認する。
     * @param[in] button XInput のボタンビット。
     * @return 押下中なら true。
     */
    bool isPadPress(int button) const;

    /**
     * @brief ゲームパッドボタンがこのフレームで離されたか確認する。
     * @param[in] button XInput のボタンビット。
     * @return 離した瞬間なら true。
     */
    bool isPadRelease(int button) const;

    /**
     * @brief 左トリガーがこのフレームで押されたか確認する。
     * @return 押した瞬間なら true。
     */
    bool isPadLeftTriggerPush() const;

    /**
     * @brief 左トリガーが押され続けているか確認する。
     * @return 押下中なら true。
     */
    bool isPadLeftTriggerPress() const;

    /**
     * @brief 右トリガーがこのフレームで押されたか確認する。
     * @return 押した瞬間なら true。
     */
    bool isPadRightTriggerPush() const;

    /**
     * @brief 右トリガーが押され続けているか確認する。
     * @return 押下中なら true。
     */
    bool isPadRightTriggerPress() const;

    /**
     * @brief ウィンドウ基準のマウス X 座標を取得する。
     * @return X 座標（pixel）。
     */
    int getMouseX() const { return mMouseX; }

    /**
     * @brief ウィンドウ基準のマウス Y 座標を取得する。
     * @return Y 座標（pixel）。
     */
    int getMouseY() const { return mMouseY; }

    /**
     * @brief 画面基準のマウス X 座標を取得する。
     * @return X 座標（pixel）。
     */
    int getMouseScreenX() const { return mMouseScreenX; }

    /**
     * @brief 画面基準のマウス Y 座標を取得する。
     * @return Y 座標（pixel）。
     */
    int getMouseScreenY() const { return mMouseScreenY; }

    /**
     * @brief 左スティック X 入力を取得する。
     * @return 正規化された軸入力。
     */
    float getPadLeftStickX() const { return mPadLeftStickX; }

    /**
     * @brief 左スティック Y 入力を取得する。
     * @return 正規化された軸入力。
     */
    float getPadLeftStickY() const { return mPadLeftStickY; }

    /**
     * @brief 右スティック X 入力を取得する。
     * @return 正規化された軸入力。
     */
    float getPadRightStickX() const { return mPadRightStickX; }

    /**
     * @brief 右スティック Y 入力を取得する。
     * @return 正規化された軸入力。
     */
    float getPadRightStickY() const { return mPadRightStickY; }

    /**
     * @brief 最後に検出した主入力がゲームパッドかどうかを取得する。
     * @return ゲームパッド入力が最後なら true。
     */
    bool isLastInputPad() const { return mLastInputPad; }

    /**
     * @brief マウスカーソルを画面中央へ固定するか設定する。
     * @param[in] lock true の場合はマウスロックを有効にする。
     */
    void setMouseLock(bool lock);

private:
    /**
     * @brief 入力状態バッファを初期化する。
     */
    InputManager();

    /**
     * @brief 入力デバイスを所有しないため明示解放は行わない。
     */
    ~InputManager() = default;

    char mKeyStateCurr[256]; ///< 現在フレームのキーボード状態。
    char mKeyStatePrev[256]; ///< 前フレームのキーボード状態。

    int mMouseInputCurr;       ///< 現在フレームのマウスボタン状態ビット。
    int mMouseInputPrev;       ///< 前フレームのマウスボタン状態ビット。
    int mMouseX, mMouseY;      ///< ウィンドウ基準のマウス座標（pixel）。
    int mMouseScreenX, mMouseScreenY; ///< 画面基準のマウス座標（pixel）。
    bool mMouseLock;           ///< マウスカーソルを固定するかどうか。
    XINPUT_STATE mPadStateCurr; ///< 現在フレームの XInput 状態。
    XINPUT_STATE mPadStatePrev; ///< 前フレームの XInput 状態。
    float mPadLeftStickX;      ///< 左スティック X 軸の正規化入力。
    float mPadLeftStickY;      ///< 左スティック Y 軸の正規化入力。
    float mPadRightStickX;     ///< 右スティック X 軸の正規化入力。
    float mPadRightStickY;     ///< 右スティック Y 軸の正規化入力。
    bool mLastInputPad;        ///< 最後に使われた入力デバイスがゲームパッドかどうか。
};

#endif
