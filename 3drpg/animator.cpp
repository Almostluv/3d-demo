/**
 * @file
 * @brief animator.cpp 実装を定義する。
 */

#include "animator.h"
#include "DxLib.h"

/// コンストラクタ
/// アニメーション管理に必要な変数を初期化する
Animator::Animator()
    : mModelHandle(-1)
    , mAttachAnimIndex(-1)
    , mPrevAttachAnimIndex(-1)
    , mCurrentAnimIndex(-1)
    , mCurrentAnimSourceModelHandle(-1)
    , mAnimTime(0.0f)
    , mAnimTotalTime(0.0f)
    , mPrevAnimTime(0.0f)
    , mPrevAnimTotalTime(0.0f)
    , mBlendDuration(0.12f)
    , mBlendTimer(0.0f)
    , mLoop(true)
    , mPrevLoop(false)
    , mIsFinished(false)
{
}

/// 使用するモデルを設定し、アニメーション状態を初期化する
void Animator::init(int modelHandle)
{
    mModelHandle = modelHandle;
    mAttachAnimIndex = -1;
    mPrevAttachAnimIndex = -1;
    mCurrentAnimIndex = -1;
    mCurrentAnimSourceModelHandle = -1;
    mAnimTime = 0.0f;
    mAnimTotalTime = 0.0f;
    mPrevAnimTime = 0.0f;
    mPrevAnimTotalTime = 0.0f;
    mBlendTimer = 0.0f;
    mLoop = true;
    mPrevLoop = false;
    mIsFinished = false;
}

/// 指定したアニメーションを再生する
void Animator::play(
    int animIndex,
    int animSourceModelHandle,
    bool loop,
    float blendDuration
)
{
    /// モデル、またはアニメーション元が無効なら何もしない
    if (mModelHandle == -1 || animSourceModelHandle == -1)
    {
        return;
    }

    /// 同じアニメーションと同じソースを再生中なら付け直さない
    /// 毎フレームアタッチし直すと、アニメーションが先頭に戻ってしまう
    if (mCurrentAnimIndex == animIndex &&
        mCurrentAnimSourceModelHandle == animSourceModelHandle)
    {
        return;
    }

    /// 以前のブレンドアウト対象が残っていれば先に解除する
    if (mPrevAttachAnimIndex != -1)
    {
        MV1DetachAnim(mModelHandle, mPrevAttachAnimIndex);
        mPrevAttachAnimIndex = -1;
    }

    /// 現在のアニメーションを旧アニメーションとして保持し、
    /// 新しいアニメーションへクロスフェードする準備を行う
    if (mAttachAnimIndex != -1)
    {
        if (blendDuration <= 0.0f)
        {
            /// 即時切り替え時は旧アニメーションを残さない
            /// 終端姿勢による引き戻しを防ぐ
            MV1DetachAnim(mModelHandle, mAttachAnimIndex);
            mAttachAnimIndex = -1;
        }
        else
        {
            /// 現在のアニメーションをブレンドアウト対象として保存
            mPrevAttachAnimIndex = mAttachAnimIndex;
            mPrevAnimTime = mAnimTime;
            mPrevAnimTotalTime = mAnimTotalTime;
            mPrevLoop = mLoop;

            /// ブレンド開始
            mBlendTimer = 0.0f;
            mBlendDuration = blendDuration;
        }
    }

    /// 別モデルに入っているアニメーションを本体モデルにアタッチする
    mAttachAnimIndex = MV1AttachAnim(
        mModelHandle,
        animIndex,
        animSourceModelHandle,
        TRUE
    );

    /// アタッチに失敗した場合
    if (mAttachAnimIndex == -1)
    {
        mCurrentAnimIndex = -1;
        mCurrentAnimSourceModelHandle = -1;
        mAnimTime = 0.0f;
        mAnimTotalTime = 0.0f;

        /// 旧アニメーションが残っていれば解除する
        if (mPrevAttachAnimIndex != -1)
        {
            MV1DetachAnim(mModelHandle, mPrevAttachAnimIndex);
            mPrevAttachAnimIndex = -1;
        }

        mLoop = true;
        mPrevLoop = false;
        mIsFinished = false;
        return;
    }

    /// アニメーション総時間を取得する
    mAnimTotalTime = MV1GetAttachAnimTotalTime(
        mModelHandle,
        mAttachAnimIndex
    );

    /// 現在アニメーション情報を更新する
    mCurrentAnimIndex = animIndex;
    mCurrentAnimSourceModelHandle = animSourceModelHandle;
    mAnimTime = 0.0f;
    mLoop = loop;
    mIsFinished = false;

    /// 総時間が極端に短い場合は、
    /// 実質的に1フレームしかないアニメーションとして扱う
    if (mAnimTotalTime <= 1.0f)
    {
        MV1SetAttachAnimBlendRate(
            mModelHandle,
            mAttachAnimIndex,
            1.0f
        );

        MV1SetAttachAnimTime(
            mModelHandle,
            mAttachAnimIndex,
            0.0f
        );

        return;
    }

    /// 新アニメーションは 0 から開始する
    /// 旧アニメーションがある場合は、最初は影響率0から始める
    MV1SetAttachAnimBlendRate(
        mModelHandle,
        mAttachAnimIndex,
        mPrevAttachAnimIndex != -1 ? 0.0f : 1.0f
    );

    /// 再生開始位置を先頭に設定する
    MV1SetAttachAnimTime(
        mModelHandle,
        mAttachAnimIndex,
        0.0f
    );
}

/// アニメーションを更新する
void Animator::update(float deltaTime)
{
    /// モデル、またはアタッチ中アニメーションが無効なら何もしない
    if (mModelHandle == -1 || mAttachAnimIndex == -1)
    {
        return;
    }

    /// 総時間が無効なら更新しない
    if (mAnimTotalTime <= 0.0f)
    {
        return;
    }

    /// DxLibのMV1アニメーション時間は秒ではなく独自単位のため、
    /// deltaTime に係数を掛けて進行速度を調整する
    float animStep = deltaTime * 40.0f;

    /// 現在アニメーションを進める
    mAnimTime += animStep;

    if (mLoop)
    {
        /// ループ再生の場合、終端を超えたら先頭へ戻す
        while (mAnimTime >= mAnimTotalTime)
        {
            mAnimTime -= mAnimTotalTime;
        }
    }
    else
    {
        /// 非ループ再生の場合、終端で停止する
        if (mAnimTime >= mAnimTotalTime)
        {
            mAnimTime = mAnimTotalTime;
            mIsFinished = true;
        }
    }

    /// 現在アニメーション時間をモデルへ反映する
    MV1SetAttachAnimTime(
        mModelHandle,
        mAttachAnimIndex,
        mAnimTime
    );

    /// 旧アニメーションがある場合はクロスフェードしながら時間を進める
    if (mPrevAttachAnimIndex != -1)
    {
        if (mPrevAnimTotalTime > 0.0f)
        {
            /// ブレンドアウト中の旧アニメーションも進める
            mPrevAnimTime += animStep;

            if (mPrevLoop)
            {
                while (mPrevAnimTime >= mPrevAnimTotalTime)
                {
                    mPrevAnimTime -= mPrevAnimTotalTime;
                }
            }
            else if (mPrevAnimTime >= mPrevAnimTotalTime)
            {
                mPrevAnimTime = mPrevAnimTotalTime;
            }

            MV1SetAttachAnimTime(
                mModelHandle,
                mPrevAttachAnimIndex,
                mPrevAnimTime
            );
        }

        /// ブレンド進行率を計算する
        mBlendTimer += deltaTime;

        float blendRate =
            mBlendDuration > 0.0f
            ? mBlendTimer / mBlendDuration
            : 1.0f;

        if (blendRate > 1.0f)
        {
            blendRate = 1.0f;
        }

        /// 新旧二つのアニメーションの影響率を補間して、
        /// 遷移を滑らかにする
        MV1SetAttachAnimBlendRate(
            mModelHandle,
            mAttachAnimIndex,
            blendRate
        );

        MV1SetAttachAnimBlendRate(
            mModelHandle,
            mPrevAttachAnimIndex,
            1.0f - blendRate
        );

        /// ブレンド完了後、旧アニメーションを解除する
        if (blendRate >= 1.0f)
        {
            MV1DetachAnim(mModelHandle, mPrevAttachAnimIndex);
            mPrevAttachAnimIndex = -1;
            mPrevAnimTime = 0.0f;
            mPrevAnimTotalTime = 0.0f;
            mPrevLoop = false;
        }
    }
}