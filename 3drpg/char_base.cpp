/**
 * @file
 * @brief char_base.cpp 実装を定義する。
 */

#include "char_base.h"

CharBase::CharBase(const CharBaseParam& param)
    : mHp(param.hp)
    , mHitRadius(param.hitRadius)
    , mCollisionRadius(param.collisionRadius)
    , mCollisionHeight(param.collisionHeight)
    , mAttackRadius(param.attackRadius)
    , mMoveSpeed(param.moveSpeed)
    , mAttackPower(param.attackPower)
{
    /// 派生キャラクター共通の HP、当たり判定、移動、攻撃パラメータを受け取る。
}

CharBase::~CharBase()
{
}

void CharBase::syncModelTransform(float addRotY)
{
    if (mModelHandle == -1)
    {
        return;
    }

    VECTOR drawRot = mRot;

    /// モデルによって正面向きが異なる場合に、追加 Y 回転で描画だけ補正する。
    drawRot.y += addRotY;

    /// 論理位置と論理回転を DxLib モデルへ反映する。
    MV1SetPosition(mModelHandle, mPos);

    MV1SetRotationXYZ(mModelHandle, drawRot);
}

void CharBase::drawModel() const
{
    if (mModelHandle != -1)
    {
        /// モデル未生成の場合は描画せず、派生側の呼び出しを安全にする。
        MV1DrawModel(mModelHandle);
    }
}

void CharBase::deleteModel()
{
    if (mModelHandle != -1)
    {
        /// GameObject が所有しているモデルインスタンスを解放し、二重解放を防ぐため無効化する。
        MV1DeleteModel(mModelHandle);
        mModelHandle = -1;
    }
}
