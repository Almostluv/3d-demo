#ifndef _DX11_CONTEXT_H_
#define _DX11_CONTEXT_H_


/**
 * @file
 * @brief DxLib から Direct3D 11 デバイス情報を取得する薄いラッパーを定義する。
 */

/**
 * @brief Direct3D 11 デバイスの前方宣言。
 */
struct ID3D11Device;

/**
 * @brief Direct3D 11 デバイスコンテキストの前方宣言。
 */
struct ID3D11DeviceContext;

/**
 * @brief DxLib 実行環境から Direct3D 11 の低レベル描画コンテキストを取得する。
 *
 * このクラスはデバイスを所有しない。返されるポインタの寿命は DxLib の初期化状態に依存する。
 */
class Dx11Context
{
public:

    /**
     * @brief 現在の Direct3D 11 デバイスを取得する。
     * @return 利用可能な場合は非所有ポインタ。利用不可の場合は nullptr。
     */
    static ID3D11Device* device();

    /**
     * @brief 現在の Direct3D 11 デバイスコンテキストを取得する。
     * @return 利用可能な場合は非所有ポインタ。利用不可の場合は nullptr。
     */
    static ID3D11DeviceContext* context();

    /**
     * @brief Direct3D 11 固有描画が利用できるか確認する。
     * @retval true DxLib が Direct3D 11 で動作しており、デバイスとコンテキストを取得できる。
     * @retval false Direct3D 11 固有描画を使えない。
     */
    static bool isAvailable();
};

#endif
