#ifndef _ANIMATOR_H_
#define _ANIMATOR_H_


/**
 * @file
 * @brief モデルアニメーション再生を管理する Animator を定義する。
 */


/**
 * @brief DxLib モデルへアニメーションをアタッチし、ブレンド付きで再生するクラス。
 *
 * モデルハンドルやアニメーション元モデルは所有しない。呼び出し側は `init()` に渡した
 * 本体モデルと `play()` に渡すアニメーションソースを Animator の利用中に有効に保つ必要がある。
 */
class Animator
{
public:
    /**
     * @brief 無効ハンドルと停止状態で初期化する。
     */
    Animator();

    /**
     * @brief Animator 自身は DxLib リソースを所有しないため、明示解放は行わない。
     */
    ~Animator() = default;

    /**
     * @brief アニメーションを適用する本体モデルを登録する。
     * @param[in] modelHandle DxLib モデルハンドル。Animator は所有しない。
     */
    void init(int modelHandle);

    /**
     * @brief 現在アニメーションとブレンド状態を 1 フレーム分更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void update(float deltaTime);

    /**
     * @brief 指定アニメーションを本体モデルへアタッチして再生する。
     * @param[in] animIndex アニメーション番号。
     * @param[in] animSourceModelHandle アニメーションを持つ DxLib モデルハンドル。Animator は所有しない。
     * @param[in] loop true の場合はループ再生する。
     * @param[in] blendDuration 直前アニメーションからのブレンド時間（秒）。
     */
    void play(int animIndex, int animSourceModelHandle, bool loop = true, float blendDuration = 0.12f);

    /**
     * @brief 現在のアタッチアニメーション番号を取得する。
     * @return DxLib のアタッチアニメーション番号。未再生時は実装上の無効値。
     */
    int getAttachAnimIndex() const { return mAttachAnimIndex; }

    /**
     * @brief 現在再生中のアニメーション番号を取得する。
     * @return アニメーション番号。
     */
    int getCurrentAnimIndex() const { return mCurrentAnimIndex; }

    /**
     * @brief 現在のアニメーションソースモデルを取得する。
     * @return DxLib モデルハンドル。Animator は所有しない。
     */
    int getCurrentAnimSourceModelHandle() const { return mCurrentAnimSourceModelHandle; }

    /**
     * @brief 非ループ再生が最後まで進んだかどうかを取得する。
     * @return 再生終了済みなら true。
     */
    bool isFinished() const { return mIsFinished; }

    /**
     * @brief 現在アニメーションの再生時間を取得する。
     * @return 再生時間（秒）。
     */
    float getAnimTime() const { return mAnimTime; }

    /**
     * @brief 現在アニメーションの総時間を取得する。
     * @return 総時間（秒）。
     */
    float getAnimTotalTime() const { return mAnimTotalTime; }

private:
    int   mModelHandle;                  ///< アニメーションを適用する本体 DxLib モデルハンドル。非所有。
    int   mAttachAnimIndex;              ///< 現在アニメーションの DxLib アタッチ番号。
    int   mPrevAttachAnimIndex;          ///< ブレンドアウト中の旧アニメーションのアタッチ番号。
    int   mCurrentAnimIndex;             ///< 現在再生中のアニメーション番号。
    int   mCurrentAnimSourceModelHandle; ///< 現在のアニメーションソースモデルハンドル。非所有。
    float mAnimTime;                     ///< 現在アニメーションの再生時間（秒）。
    float mAnimTotalTime;                ///< 現在アニメーションの総時間（秒）。
    float mPrevAnimTime;                 ///< 旧アニメーションの再生時間（秒）。
    float mPrevAnimTotalTime;            ///< 旧アニメーションの総時間（秒）。
    float mBlendDuration;                ///< ブレンドに掛ける時間（秒）。
    float mBlendTimer;                   ///< 現在のブレンド進行時間（秒）。
    bool  mLoop;                         ///< 現在アニメーションをループ再生するかどうか。
    bool  mPrevLoop;                     ///< 旧アニメーションをループ再生していたかどうか。
    bool  mIsFinished;                   ///< 非ループ再生が終了したかどうか。
};

#endif
