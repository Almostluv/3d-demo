/**
 * @file
 * @brief dx11_context.cpp 実装を定義する。
 */

#include "dx11_context.h"
#include "DxLib.h"
#include "DxFunctionWin.h"

ID3D11Device* Dx11Context::device()
{
    /// DxLib が内部で保持している Direct3D 11 デバイスを取得する。所有権は DxLib 側にある。
    return static_cast<ID3D11Device*>(
        const_cast<void*>(GetUseDirect3D11Device())
        );
}

ID3D11DeviceContext* Dx11Context::context()
{
    /// DxLib が内部で保持している immediate context を取得する。呼び出し側では解放しない。
    return static_cast<ID3D11DeviceContext*>(
        const_cast<void*>(GetUseDirect3D11DeviceContext())
        );
}

bool Dx11Context::isAvailable()
{
    /// Direct3D 11 で動作しており、デバイスとコンテキストが両方取得できる場合だけ利用可能とする。
    return GetUseDirect3DVersion() == DX_DIRECT3D_11
        && device() != nullptr
        && context() != nullptr;
}
