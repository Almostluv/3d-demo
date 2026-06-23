/**
 * @file
 * @brief game_object.cpp 実装を定義する。
 */

#include "game_object.h"

GameObject::GameObject()
    : mModelHandle(-1)
    , mPos(VGet(0, 0, 0))
    , mRot(VGet(0, 0, 0))
    , mScale(1.0f)
{
    /// モデルを持たない無効状態から開始し、派生クラスが必要なリソースを設定する。
}

GameObject::~GameObject() {
    /// モデル解放は所有関係を持つ派生クラス側で行う。
}
