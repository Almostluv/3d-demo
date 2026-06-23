#ifndef _ATTACK_EFFECT_H_
#define _ATTACK_EFFECT_H_

/**
 * @file
 * @brief プレイヤーの攻撃、魔法、回復などの視覚エフェクトを管理する
 *        AttackEffect クラスを定義する。
 */

#include "DxLib.h"

#include <d3d11.h>
#include <vector>

 /**
  * @brief 攻撃、命中、魔法および回復エフェクトの生成、更新、描画を管理するクラス。
  *
  * 斬撃、剣の軌跡、各種命中エフェクト、魔法エフェクトおよび
  * 回復エフェクトの状態を保持し、DirectX 11 または DxLib を使用して描画する。
  */
class AttackEffect
{
public:

    /**
     * @brief DirectX 11 の描画処理で使用する頂点データ。
     */
    struct Dx11Vertex
    {
        float x, y, z;    ///< 頂点の三次元位置成分。

        float r, g, b, a; ///< 頂点カラーの RGBA 成分。

        float u, v;       ///< テクスチャ座標の UV 成分。
    };

public:

    /**
     * @brief AttackEffect を生成する。
     */
    AttackEffect();

    /**
     * @brief AttackEffect を破棄する。
     */
    ~AttackEffect();

    /**
     * @brief プレイヤーの位置と向きに基づいて斬撃エフェクトを開始する。
     *
     * @param[in] playerPos プレイヤーの位置。
     * @param[in] playerRotY プレイヤーの Y 軸回転値。
     * @param[in] attackStep 攻撃段階を表す値。
     */
    void startSlash(
        VECTOR playerPos,
        float playerRotY,
        int attackStep
    );

    /**
     * @brief 剣の軌跡を構成する位置サンプルを追加する。
     *
     * @param[in] basePos 剣の根元側の位置。
     * @param[in] tipPos 剣の先端側の位置。
     * @param[in] attackStep サンプルが属する攻撃段階。
     */
    void addSwordTrailSample(
        VECTOR basePos,
        VECTOR tipPos,
        int attackStep
    );

    /**
     * @brief 通常攻撃の命中エフェクトを開始する。
     *
     * @param[in] hitPos 攻撃が命中した位置。
     * @param[in] attackStep 命中した攻撃の段階。
     */
    void startHitImpact(
        VECTOR hitPos,
        int attackStep
    );

    /**
     * @brief 魔法攻撃の命中エフェクトを開始する。
     *
     * @param[in] hitPos 魔法攻撃が命中した位置。
     * @param[in] attackStep 命中した攻撃の段階。
     */
    void startMagicHitImpact(
        VECTOR hitPos,
        int attackStep
    );

    /**
     * @brief シールドに対する命中エフェクトを開始する。
     *
     * @param[in] hitPos 攻撃がシールドに命中した位置。
     * @param[in] attackStep 命中した攻撃の段階。
     */
    void startShieldHitImpact(
        VECTOR hitPos,
        int attackStep
    );

    /**
     * @brief カウンター攻撃の命中エフェクトを開始する。
     *
     * @param[in] hitPos カウンター攻撃が命中した位置。
     */
    void startCounterHitImpact(
        VECTOR hitPos
    );

    /**
     * @brief 指定位置で回復エフェクトを開始する。
     *
     * @param[in] pos 回復エフェクトを発生させる位置。
     */
    void startHealEffect(
        VECTOR pos
    );

    /**
     * @brief プレイヤーの向きに基づいて魔法エフェクトを開始する。
     *
     * @param[in] startPos 魔法エフェクトの開始位置。
     * @param[in] playerRotY プレイヤーの Y 軸回転値。
     * @param[in] attackStep 魔法攻撃の段階。
     */
    void startMagicEffect(
        VECTOR startPos,
        float playerRotY,
        int attackStep
    );

    /**
     * @brief 地面モデルを参照する魔法エフェクトを開始する。
     *
     * @param[in] startPos 魔法エフェクトの開始位置。
     * @param[in] playerRotY プレイヤーの Y 軸回転値。
     * @param[in] attackStep 魔法攻撃の段階。
     * @param[in] groundModelHandle 地面モデルとして使用するモデルハンドル。
     */
    void startMagicEffect(
        VECTOR startPos,
        float playerRotY,
        int attackStep,
        int groundModelHandle
    );

    /**
     * @brief 指定された対象位置に関連する魔法エフェクトを開始する。
     *
     * @param[in] startPos 魔法エフェクトの開始位置。
     * @param[in] playerRotY プレイヤーの Y 軸回転値。
     * @param[in] attackStep 魔法攻撃の段階。
     * @param[in] targetPos 魔法エフェクトの対象位置。
     */
    void startMagicEffect(
        VECTOR startPos,
        float playerRotY,
        int attackStep,
        VECTOR targetPos
    );

    /**
     * @brief 現在有効なエフェクトの状態を更新する。
     *
     * @param[in] deltaTime 前回の更新から経過した時間。
     */
    void update(float deltaTime);

    /**
     * @brief 現在有効なすべてのエフェクトを描画する。
     */
    void draw() const;

private:

    /**
     * @brief 進行中の斬撃エフェクトの状態。
     */
    struct Slash
    {
        VECTOR pos;       ///< 斬撃エフェクトの基準位置。

        float rotY;       ///< 斬撃エフェクトの Y 軸回転値。

        float timer;      ///< エフェクト開始後の経過時間。

        float duration;   ///< エフェクトの継続時間。

        int attackStep;   ///< 斬撃が属する攻撃段階。
    };

    /**
     * @brief 剣の軌跡を構成する単一の位置サンプル。
     */
    struct SwordTrailSample
    {
        VECTOR basePos;   ///< 剣の根元側の位置。

        VECTOR tipPos;    ///< 剣の先端側の位置。

        float age;        ///< サンプルが追加されてからの経過時間。

        int attackStep;   ///< サンプルが属する攻撃段階。
    };

    /**
     * @brief 命中エフェクトを構成する単一のパーティクル。
     */
    struct ImpactParticle
    {
        VECTOR pos;       ///< パーティクルの現在位置。

        VECTOR dir;       ///< パーティクルの移動方向。

        float speed;      ///< パーティクルの移動速度。

        float size;       ///< パーティクルの描画サイズ。

        float visibleTime; ///< パーティクルが表示される時間。

        float alpha;      ///< パーティクルの不透明度。
    };

    /**
     * @brief 進行中の命中エフェクトの状態。
     */
    struct Impact
    {
        VECTOR pos;       ///< 命中エフェクトの発生位置。

        float timer;      ///< エフェクト開始後の経過時間。

        float duration;   ///< エフェクトの継続時間。

        int attackStep;   ///< エフェクトが属する攻撃段階。

        bool isMagic;     ///< 魔法攻撃の命中エフェクトであるかどうか。

        bool isShield;    ///< シールドへの命中エフェクトであるかどうか。

        bool isCounter;   ///< カウンター攻撃の命中エフェクトであるかどうか。

        std::vector<ImpactParticle> particles; ///< 命中エフェクトを構成するパーティクル。
    };

    /**
     * @brief 進行中の魔法エフェクトの状態。
     */
    struct MagicEffect
    {
        VECTOR pos;       ///< 魔法エフェクトの現在位置。

        VECTOR startPos;  ///< 魔法エフェクトの開始位置。

        VECTOR forward;   ///< 魔法エフェクトの進行方向。

        VECTOR curvePos;  ///< 曲線状の移動または描画に使用する位置。

        VECTOR targetPos; ///< 魔法エフェクトの対象位置。

        bool hasTarget;   ///< 有効な対象位置が設定されているかどうか。

        float timer;      ///< エフェクト開始後の経過時間。

        float duration;   ///< エフェクトの継続時間。

        int attackStep;   ///< 魔法エフェクトが属する攻撃段階。

        int groundModelHandle; ///< 魔法処理で参照する地面モデルのハンドル。

        std::vector<VECTOR> trail; ///< 魔法エフェクトの軌跡を構成する位置履歴。
    };

    /**
     * @brief 魔法または回復エフェクトを構成する単一のパーティクル。
     */
    struct MagicParticle
    {
        VECTOR pos;       ///< パーティクルの現在位置。

        VECTOR velocity;  ///< パーティクルの移動速度ベクトル。

        float timer;      ///< パーティクル生成後の経過時間。

        float duration;   ///< パーティクルの継続時間。

        float size;       ///< パーティクルの描画サイズ。
    };

private:

    /**
     * @brief DirectX 11 描画で使用するリソースを初期化する。
     *
     * @return 初期化処理の結果。
     */
    bool setupDx11();

    /**
     * @brief DirectX 11 描画で使用しているリソースを解放する。
     */
    void releaseDx11();

    /**
     * @brief DirectX 11 による描画が可能な状態か確認する。
     *
     * @return DirectX 11 描画に必要なリソースが利用可能な場合は true。
     */
    bool isDx11Ready() const;

    /**
     * @brief DirectX 11 を使用してエフェクトを描画する。
     *
     * @return DirectX 11 描画処理の実行結果。
     */
    bool drawDx11() const;

    /**
     * @brief DirectX 11 を使用して剣の軌跡を描画する。
     *
     * @return 剣の軌跡の描画処理結果。
     */
    bool drawSwordTrailDx11() const;

    /**
     * @brief 指定された頂点データを DirectX 11 で一括描画する。
     *
     * @param[in] vertices 描画する頂点データ。
     * @param[in] params シェーダーに渡す描画パラメーター。
     * @param[in] tint 描画時に適用する色調整値。
     *
     * @return 一括描画処理の実行結果。
     */
    bool drawDx11Batch(
        const std::vector<Dx11Vertex>& vertices,
        FLOAT4 params,
        FLOAT4 tint
    ) const;

    /**
     * @brief DirectX 11 描画を使用できない場合の DxLib 描画を実行する。
     */
    void drawDxLibFallback() const;

    /**
     * @brief DxLib を使用して剣の軌跡を描画する。
     */
    void drawSwordTrailDxLib() const;

    /**
     * @brief 命中エフェクト用画像を描画する。
     */
    void drawHitEffectImage() const;

    /**
     * @brief 現在有効な魔法エフェクトを描画する。
     */
    void drawMagicEffects() const;

private:

    std::vector<Slash> mSlashes; ///< 現在有効な斬撃エフェクトの一覧。

    std::vector<SwordTrailSample> mSwordTrailSamples; ///< 剣の軌跡を構成する位置サンプルの一覧。

    std::vector<Impact> mImpacts; ///< 現在有効な命中エフェクトの一覧。

    std::vector<MagicEffect> mMagicEffects; ///< 現在有効な魔法エフェクトの一覧。

    std::vector<MagicParticle> mMagicParticles; ///< 魔法エフェクト用パーティクルの一覧。

    std::vector<MagicParticle> mHealParticles; ///< 回復エフェクト用パーティクルの一覧。

    int mPixelShaderHandle; ///< DxLib で使用するピクセルシェーダーのハンドル。

    int mHitEffectGraphHandle; ///< 命中エフェクト画像のグラフィックハンドル。

    int mMagicMissileGraphHandle; ///< 魔法弾画像のグラフィックハンドル。

    int mMagicBeamGraphHandle; ///< 魔法ビーム画像のグラフィックハンドル。

    int mMagicCircleGraphHandle; ///< 魔法陣画像のグラフィックハンドル。

    ID3D11VertexShader* mDx11VertexShader; ///< エフェクト描画で使用する頂点シェーダー。

    ID3D11PixelShader* mDx11PixelShader; ///< エフェクト描画で使用するピクセルシェーダー。

    ID3D11InputLayout* mDx11InputLayout; ///< 頂点データの入力レイアウト。

    ID3D11Buffer* mDx11VertexBuffer; ///< エフェクト描画用の頂点バッファー。

    ID3D11Buffer* mDx11ConstantBuffer; ///< シェーダーパラメーターを保持する定数バッファー。

    ID3D11BlendState* mDx11BlendState; ///< エフェクト描画に適用するブレンドステート。

    ID3D11DepthStencilState* mDx11DepthStencilState; ///< エフェクト描画に適用する深度ステンシルステート。

    ID3D11RasterizerState* mDx11RasterizerState; ///< エフェクト描画に適用するラスタライザーステート。
};

#endif