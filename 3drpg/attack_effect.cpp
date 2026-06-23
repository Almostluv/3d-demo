/**
 * @file
 * @brief AttackEffect の生成、更新、描画入口を定義する。
 */

#include "attack_effect.h"
#include "attack_effect_internal.h"
#include "resource_manager.h"

#include <algorithm>

using namespace AttackEffectInternal;

AttackEffect::AttackEffect()
    : mPixelShaderHandle(-1)
    , mHitEffectGraphHandle(-1)
    , mMagicMissileGraphHandle(-1)
    , mMagicBeamGraphHandle(-1)
    , mMagicCircleGraphHandle(-1)
    , mDx11VertexShader(nullptr)
    , mDx11PixelShader(nullptr)
    , mDx11InputLayout(nullptr)
    , mDx11VertexBuffer(nullptr)
    , mDx11ConstantBuffer(nullptr)
    , mDx11BlendState(nullptr)
    , mDx11DepthStencilState(nullptr)
    , mDx11RasterizerState(nullptr)
{
    /// 2D 画像エフェクトと DirectX 11 用リソースをまとめて初期化する。
    mHitEffectGraphHandle = ResourceManager::instance()->getGraph("Resource/Effect/KnightHitEffect.png");
    mMagicMissileGraphHandle = ResourceManager::instance()->getGraph("Resource/Effect/magicMissile.png");
    mMagicBeamGraphHandle = ResourceManager::instance()->getGraph("Resource/Effect/magicBeam.png");
    mMagicCircleGraphHandle = ResourceManager::instance()->getGraph("Resource/Effect/magicCircle.png");
    setupDx11();
}
AttackEffect::~AttackEffect()
{
    if (mPixelShaderHandle != -1)
    {
        DeleteShader(mPixelShaderHandle);
        mPixelShaderHandle = -1;
    }

    releaseDx11();
}
void AttackEffect::update(float deltaTime)
{
    /// 各エフェクトのタイマーを進め、寿命切れの要素を最後にまとめて削除する。
    for (auto& slash : mSlashes)
    {
        slash.timer += deltaTime;
    }

    for (auto& sample : mSwordTrailSamples)
    {
        sample.age += deltaTime;
    }

    for (auto& impact : mImpacts)
    {
        impact.timer += deltaTime;
        for (auto& particle : impact.particles)
        {
            if (particle.alpha <= 0.0f)
            {
                continue;
            }
            if (particle.speed > 0.0f)
            {
                particle.pos = VAdd(particle.pos, VScale(particle.dir, particle.speed * deltaTime));
                particle.speed -= HitParticleDeceleration * deltaTime;
                if (particle.speed < 0.0f)
                {
                    particle.speed = 0.0f;
                }
            }
            if (particle.visibleTime > 0.0f)
            {
                particle.visibleTime -= deltaTime;
            }
            else
            {
                particle.alpha -= HitParticleFadeSpeed * deltaTime;
                if (particle.alpha < 0.0f)
                {
                    particle.alpha = 0.0f;
                }
            }
        }
    }

    for (auto& effect : mMagicEffects)
    {
        effect.timer += deltaTime;
        if (effect.attackStep == 1)
        {
            /// 魔法弾だけは発射後も位置と軌跡を更新する。ビームと魔法陣は発生位置固定。
            VECTOR prevPos = effect.pos;
            if (effect.hasTarget)
            {
                float t = clamp01(effect.timer / effect.duration);
                float eased = t * t * (3.0f - 2.0f * t);
                VECTOR a = lerpVector(effect.startPos, effect.curvePos, eased);
                VECTOR b = lerpVector(effect.curvePos, effect.targetPos, eased);
                effect.pos = lerpVector(a, b, eased);
                VECTOR move = VSub(effect.pos, prevPos);
                if (VSize(move) > 0.0001f)
                {
                    effect.forward = VNorm(move);
                }
            }
            else
            {
                effect.pos = VAdd(effect.pos, VScale(effect.forward, 58.0f * deltaTime));
            }

            effect.trail.push_back(effect.pos);
            if (effect.trail.size() > MagicMissileTrailMax)
            {
                /// 軌跡は固定長に抑え、長時間の追尾でも配列が増え続けないようにする。
                effect.trail.erase(effect.trail.begin());
            }
        }
    }

    for (auto& particle : mMagicParticles)
    {
        particle.timer += deltaTime;
        particle.pos = VAdd(particle.pos, VScale(particle.velocity, deltaTime));
    }

    for (auto& particle : mHealParticles)
    {
        particle.timer += deltaTime;
        particle.pos = VAdd(particle.pos, VScale(particle.velocity, deltaTime));
    }

    /// erase-remove で寿命を過ぎた一時エフェクトを回収する。
    mSlashes.erase(
        std::remove_if(mSlashes.begin(), mSlashes.end(), [](const Slash& slash) { return slash.timer >= slash.duration; }),
        mSlashes.end());

    mSwordTrailSamples.erase(
        std::remove_if(mSwordTrailSamples.begin(), mSwordTrailSamples.end(), [](const SwordTrailSample& sample) { return sample.age >= SwordTrailDuration; }),
        mSwordTrailSamples.end());

    mImpacts.erase(
        std::remove_if(mImpacts.begin(), mImpacts.end(), [](const Impact& impact) { return impact.timer >= impact.duration; }),
        mImpacts.end());

    mMagicEffects.erase(
        std::remove_if(mMagicEffects.begin(), mMagicEffects.end(), [](const MagicEffect& effect) { return effect.timer >= effect.duration; }),
        mMagicEffects.end());

    mMagicParticles.erase(
        std::remove_if(mMagicParticles.begin(), mMagicParticles.end(), [](const MagicParticle& particle) { return particle.timer >= particle.duration; }),
        mMagicParticles.end());

    mHealParticles.erase(
        std::remove_if(mHealParticles.begin(), mHealParticles.end(), [](const MagicParticle& particle) { return particle.timer >= particle.duration; }),
        mHealParticles.end());

}
void AttackEffect::draw() const
{
    if (!drawDx11())
    {
        /// DirectX 11 描画に失敗した場合でも、DxLib のポリゴン描画で同じ効果を出す。
        drawDxLibFallback();
    }

    drawMagicEffects();
    drawHitEffectImage();
}
