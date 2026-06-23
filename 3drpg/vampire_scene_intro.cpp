/**
 * @file
 * @brief vampire_scene_intro.cpp 実装を定義する。
 */

#include "vampire_scene.h"
#include "audio_manager.h"
#include "DxLib.h"

void VampireScene::updateIntro(float deltaTime)
{
    constexpr float CameraMoveTime = 2.0f; ///< イントロカメラが目標位置へ移動する時間。単位は seconds。
    constexpr float RoaringTime = 3.0f; ///< ボス咆哮演出の再生時間。単位は seconds。
    constexpr float BallAppearInterval = 0.42f; ///< 魔法弾を順番に出す間隔。単位は seconds。
    constexpr float BallAppearDelay = 1.5f; ///< 咆哮開始から魔法弾出現までの遅延時間。単位は seconds。

    /// イントロ中もキャラクターとボスのアニメーションだけは進め、停止画に見えないようにする。
    mIntroTimer += deltaTime;
    mPlayer->update(deltaTime);
    if (mCompanion != nullptr)
    {
        mCompanion->update(deltaTime);
    }
    mMagicEnemy->updateAnimation(deltaTime);
    mMagicEnemy->updateIntroProjectiles(deltaTime);

    if (mIntroState == IntroMoveToBoss)
    {
        if (mIntroTimer >= CameraMoveTime)
        {
            /// ボスへ寄るカメラ移動が終わったら咆哮演出へ切り替える。
            mIntroState = IntroRoaring;
            mIntroTimer = 0.0f;
        }
        return;
    }

    if (mIntroState == IntroRoaring)
    {
        /// 咆哮中はリスナー位置もボス寄りカメラに合わせ、SE の定位を演出と一致させる。
        AudioManager::instance()->updateListener(getBossCameraEye(), getBossCameraTarget());
        if (!mRoaringStarted)
        {
            /// 咆哮開始は一度だけ送る。毎フレーム呼ぶとアニメーションが先頭へ戻る。
            mMagicEnemy->startRoaring();
            mRoaringStarted = true;
        }

        /// 魔法弾は一斉出現ではなく、咆哮後に時間差で増やして威圧感を作る。
        int ballCount = mIntroTimer >= BallAppearDelay
            ? static_cast<int>((mIntroTimer - BallAppearDelay) / BallAppearInterval) + 1
            : 0;
        mMagicEnemy->setIntroMagicBallCount(ballCount);

        if (mIntroTimer >= RoaringTime)
        {
            /// 咆哮時間が終わったら、操作カメラへ戻る状態へ進む。
            mIntroState = IntroMoveBack;
            mIntroTimer = 0.0f;
        }
        return;
    }

    if (mIntroState == IntroMoveBack && mIntroTimer >= CameraMoveTime)
    {
        if (!mIntroBallsReleased)
        {
            /// 操作可能になる直前に魔法弾を解放し、戦闘開始の合図にする。
            mMagicEnemy->releaseIntroMagicBalls();
            mIntroBallsReleased = true;
        }
        mMagicEnemy->playIdle();
        mIntroState = IntroDone;
        mIntroTimer = 0.0f;
    }
}

void VampireScene::drawIntroCamera() const
{
    constexpr float CameraMoveTime = 2.0f; ///< イントロ終了時に操作カメラへ戻す時間。単位は seconds。
    VECTOR eye = getNormalCameraEye();
    VECTOR target = getNormalCameraTarget();

    if (mIntroState == IntroMoveToBoss)
    {
        /// 通常視点からボス寄り視点へ補間し、シーン開始時の注目点を明確にする。
        float t = mIntroTimer / CameraMoveTime;
        if (t > 1.0f) t = 1.0f;
        eye = lerpVector(getNormalCameraEye(), getBossCameraEye(), t);
        target = lerpVector(getNormalCameraTarget(), getBossCameraTarget(), t);
    }
    else if (mIntroState == IntroRoaring)
    {
        /// 咆哮中は固定カメラでボスを見せる。
        eye = getBossCameraEye();
        target = getBossCameraTarget();
    }
    else if (mIntroState == IntroMoveBack)
    {
        /// 戦闘操作へ戻すため、ボス視点から通常視点へ逆向きに補間する。
        float t = mIntroTimer / CameraMoveTime;
        if (t > 1.0f) t = 1.0f;
        eye = lerpVector(getBossCameraEye(), getNormalCameraEye(), t);
        target = lerpVector(getBossCameraTarget(), getNormalCameraTarget(), t);
    }

    /// DxLib のカメラを直接設定するため、通常 Camera::draw はこの間呼ばない。
    SetCameraPositionAndTarget_UpVecY(eye, target);
    SetCameraNearFar(1.0f, 100000.0f);
}

VECTOR VampireScene::getNormalCameraEye() const
{
    /// 通常戦闘視点はプレイヤーの少し上後方から見る。
    VECTOR target = getNormalCameraTarget();
    return VAdd(target, VGet(0.0f, 18.0f, 58.0f));
}

VECTOR VampireScene::getNormalCameraTarget() const
{
    /// プレイヤー前方を注視点にして、進行方向側を見やすくする。
    return VAdd(mPlayer->getPosition(), VGet(0.0f, 13.0f, -16.0f));
}

VECTOR VampireScene::getBossCameraEye() const
{
    /// ボス紹介用の視点は敵の斜め上から配置する。
    return VAdd(mMagicEnemy->getPosition(), VGet(26.0f, 34.0f, 42.0f));
}

VECTOR VampireScene::getBossCameraTarget() const
{
    /// ボスの胴体付近を狙い、足元ではなく本体が中央に入るようにする。
    return VAdd(mMagicEnemy->getPosition(), VGet(0.0f, 13.0f, 0.0f));
}

VECTOR VampireScene::lerpVector(VECTOR a, VECTOR b, float t) const
{
    /// カメラ補間用の線形補間。t は呼び出し側で 0.0f から 1.0f に丸める。
    return VGet(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t);
}
