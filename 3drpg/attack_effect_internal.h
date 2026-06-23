/**
 * @file
 * @brief AttackEffect の内部共有ヘルパーを定義する。
 */

#ifndef ATTACK_EFFECT_INTERNAL_H_
#define ATTACK_EFFECT_INTERNAL_H_

#include "attack_effect.h"

#include <cmath>

namespace AttackEffectInternal
{
constexpr int SlashSegmentCount = 16; ///< 斬撃扇形を構成する分割数。
constexpr float SwordTrailDuration = 0.30f; ///< 剣軌跡が残る時間。単位は seconds。
constexpr int SwordTrailMaxSamples = 24; ///< 剣軌跡として保持する最大サンプル数。
constexpr int SwordTrailSubdivisions = 5; ///< 剣軌跡の曲線補間に使う分割数。
constexpr int ImpactSegmentCount = 32; ///< 命中リングを構成する分割数。
constexpr float ImpactDuration = 0.24f; ///< 命中エフェクトの表示時間。単位は seconds。
constexpr int HitParticleCount = 64; ///< 通常命中で生成する粒子数。
constexpr float HitParticleMaxSpeed = 92.0f; ///< 命中パーティクルの最大初速。
constexpr float HitParticleMinSpeed = 28.0f; ///< 命中パーティクルの最小初速。
constexpr float HitParticleDeceleration = 310.0f; ///< 命中パーティクルの減速量。
constexpr float HitParticleFadeSpeed = 12.0f; ///< 命中パーティクルのフェード速度。
constexpr float MagicMissileDuration = 0.62f; ///< 魔法弾エフェクトの移動時間。単位は seconds。
constexpr int MagicMissileTrailMax = 14; ///< 魔法弾の軌跡として保持する最大位置数。
constexpr float MagicBeamDuration = 0.56f; ///< 魔法ビーム表示の残存時間。単位は seconds。
constexpr float MagicBeamVisualRange = 118.0f; ///< 魔法ビームの描画距離。ワールド空間。
constexpr int MagicBeamSegmentCount = 18; ///< 魔法ビームを構成する描画分割数。
constexpr float MagicCircleDuration = 2.90f; ///< 魔法陣エフェクトの表示時間。単位は seconds。
constexpr int MagicParticleCount = 16; ///< 魔法発生時に出す補助粒子数。
constexpr int MagicCircleRingCount = 6; ///< 魔法陣を放射状に分割するリング数。
constexpr int MagicCircleSegmentCount = 32; ///< 魔法陣 1 リングあたりの円周分割数。
constexpr float MagicCircleGroundOffset = 0.08f; ///< 魔法陣を地面から少し浮かせる Y オフセット。

inline float clamp01(float value)
{
    /// エフェクトの進行率やフェード率を 0..1 に収める。
    if (value < 0.0f)
    {
        return 0.0f;
    }

    if (value > 1.0f)
    {
        return 1.0f;
    }

    return value;
}

inline VECTOR lerpVector(VECTOR a, VECTOR b, float t)
{
    /// 魔法弾のカーブ移動や補間描画で使う単純な線形補間。
    return VGet(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t);
}

inline VECTOR catmullRomVector(VECTOR p0, VECTOR p1, VECTOR p2, VECTOR p3, float t)
{
    /// 剣軌跡を直線ではなく滑らかな曲線として補間する。
    float t2 = t * t;
    float t3 = t2 * t;
    return VGet(
        0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
        0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3),
        0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3));
}

inline float randomRange(float minValue, float maxValue)
{
    /// DxLib の整数乱数から、指定範囲の float 乱数を作る。
    float rate = static_cast<float>(GetRand(10000)) / 10000.0f;
    return minValue + (maxValue - minValue) * rate;
}

inline VECTOR randomDirection()
{
    /// ヒットパーティクルを上方向寄りに散らし、地面方向へ出すぎないようにする。
    VECTOR dir = VGet(randomRange(-1.0f, 1.0f), randomRange(-0.35f, 0.85f), randomRange(-1.0f, 1.0f));
    if (VSize(dir) <= 0.0001f)
    {
        return VGet(0.0f, 1.0f, 0.0f);
    }
    return VNorm(dir);
}

inline VECTOR forwardFromRot(float rotY)
{
    /// Y 軸回転から XZ 平面上の前方向ベクトルを作る。
    return VGet(sinf(rotY), 0.0f, cosf(rotY));
}

inline VECTOR rotateLocal(float x, float z, float rotY)
{
    /// ローカル XZ オフセットをキャラクターの Y 回転に合わせて回す。
    float s = sinf(rotY);
    float c = cosf(rotY);
    return VGet((x * c) + (z * s), 0.0f, (-x * s) + (z * c));
}
}

#endif
