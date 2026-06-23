/**
 * @file
 * @brief AttackEffect の各種エフェクト生成処理を定義する。
 */

#include "attack_effect.h"
#include "attack_effect_internal.h"

using namespace AttackEffectInternal;

void AttackEffect::startSlash(VECTOR playerPos, float playerRotY, int attackStep)
{
    /// 現在の斬撃表示は剣軌跡サンプルで表現するため、この API は互換用に残している。
    (void)playerPos;
    (void)playerRotY;
    (void)attackStep;
}
void AttackEffect::addSwordTrailSample(VECTOR basePos, VECTOR tipPos, int attackStep)
{
    if (VSize(VSub(tipPos, basePos)) <= 0.0001f)
    {
        return;
    }

    /// 剣の根元を少し先端側へ寄せ、太すぎる軌跡にならないようにする。
    VECTOR tipToJoint = VSub(basePos, tipPos);
    basePos = VAdd(tipPos, VScale(tipToJoint, 0.42f));

    SwordTrailSample sample;
    sample.basePos = basePos;
    sample.tipPos = tipPos;
    sample.age = 0.0f;
    sample.attackStep = attackStep;
    mSwordTrailSamples.push_back(sample);

    if (mSwordTrailSamples.size() > SwordTrailMaxSamples)
    {
        /// 古いサンプルを捨てて、軌跡用配列が増え続けないようにする。
        mSwordTrailSamples.erase(mSwordTrailSamples.begin());
    }
}
void AttackEffect::startHitImpact(VECTOR hitPos, int attackStep)
{
    Impact impact;
    impact.pos = hitPos;
    impact.timer = 0.0f;
    impact.duration = ImpactDuration;
    impact.attackStep = attackStep;
    impact.isMagic = false;
    impact.isShield = false;
    impact.isCounter = false;
    impact.particles.reserve(HitParticleCount);
    /// 命中点から複数の粒子を飛ばし、攻撃段ごとのヒット感を補強する。
    for (int i = 0; i < HitParticleCount; ++i)
    {
        ImpactParticle particle;
        particle.pos = hitPos;
        particle.dir = randomDirection();
        particle.speed = randomRange(HitParticleMinSpeed, HitParticleMaxSpeed) * (attackStep == 3 ? 1.25f : 1.0f);
        particle.size = randomRange(4.0f, 11.0f) * (attackStep == 3 ? 1.20f : 1.0f);
        particle.visibleTime = randomRange(0.01f, 0.05f);
        particle.alpha = 1.0f;
        impact.particles.push_back(particle);
    }
    mImpacts.push_back(impact);
}
void AttackEffect::startMagicHitImpact(VECTOR hitPos, int attackStep)
{
    /// 魔法ヒットは粒子ではなく青い発光球とリングで表現する。
    Impact impact;
    impact.pos = hitPos;
    impact.timer = 0.0f;
    impact.duration = ImpactDuration * 1.15f;
    impact.attackStep = attackStep;
    impact.isMagic = true;
    impact.isShield = false;
    impact.isCounter = false;
    mImpacts.push_back(impact);
}
void AttackEffect::startShieldHitImpact(VECTOR hitPos, int attackStep)
{
    /// シールド命中は通常ヒットと同じ粒子構造を使い、色だけを青系へ変える。
    Impact impact;
    impact.pos = hitPos;
    impact.timer = 0.0f;
    impact.duration = ImpactDuration;
    impact.attackStep = attackStep;
    impact.isMagic = false;
    impact.isShield = true;
    impact.isCounter = false;
    impact.particles.reserve(HitParticleCount);
    for (int i = 0; i < HitParticleCount; ++i)
    {
        ImpactParticle particle;
        particle.pos = hitPos;
        particle.dir = randomDirection();
        particle.speed = randomRange(HitParticleMinSpeed, HitParticleMaxSpeed) * (attackStep == 3 ? 1.25f : 1.0f);
        particle.size = randomRange(4.0f, 11.0f) * (attackStep == 3 ? 1.20f : 1.0f);
        particle.visibleTime = randomRange(0.01f, 0.05f);
        particle.alpha = 1.0f;
        impact.particles.push_back(particle);
    }
    mImpacts.push_back(impact);
}
void AttackEffect::startCounterHitImpact(VECTOR hitPos)
{
    constexpr int CounterParticleCount = 36;

    /// カウンター命中は短時間で強く散る粒子にして、通常ヒットとの差を出す。
    Impact impact;
    impact.pos = hitPos;
    impact.timer = 0.0f;
    impact.duration = ImpactDuration * 0.85f;
    impact.attackStep = 3;
    impact.isMagic = false;
    impact.isShield = false;
    impact.isCounter = true;
    impact.particles.reserve(CounterParticleCount);
    for (int i = 0; i < CounterParticleCount; ++i)
    {
        ImpactParticle particle;
        particle.pos = hitPos;
        particle.dir = randomDirection();
        particle.speed = randomRange(HitParticleMinSpeed, HitParticleMaxSpeed) * 1.45f;
        particle.size = randomRange(6.0f, 14.0f);
        particle.visibleTime = randomRange(0.01f, 0.04f);
        particle.alpha = 1.0f;
        impact.particles.push_back(particle);
    }
    mImpacts.push_back(impact);
}
void AttackEffect::startHealEffect(VECTOR pos)
{
    constexpr int HealParticleCount = 18;
    /// 回復演出は上昇する粒子だけで構成し、戦闘エフェクトより軽くする。
    for (int i = 0; i < HealParticleCount; ++i)
    {
        float angle = DX_TWO_PI_F * static_cast<float>(i) / HealParticleCount;
        float radius = 4.0f + static_cast<float>(i % 4) * 1.4f;
        MagicParticle particle;
        particle.pos = VAdd(pos, VGet(cosf(angle) * radius, 2.0f + static_cast<float>(i % 5), sinf(angle) * radius));
        particle.velocity = VGet(cosf(angle) * 1.2f, 12.0f + static_cast<float>(i % 4) * 1.5f, sinf(angle) * 1.2f);
        particle.timer = 0.0f;
        particle.duration = 0.9f + static_cast<float>(i % 4) * 0.12f;
        particle.size = 0.30f + static_cast<float>(i % 3) * 0.10f;
        mHealParticles.push_back(particle);
    }
}
void AttackEffect::startMagicEffect(VECTOR startPos, float playerRotY, int attackStep)
{
    startMagicEffect(startPos, playerRotY, attackStep, VGet(0.0f, 0.0f, 0.0f));
}
void AttackEffect::startMagicEffect(VECTOR startPos, float playerRotY, int attackStep, int groundModelHandle)
{
    /// 地面モデル付きの魔法は、内部生成後に最後のエフェクトへ床判定ハンドルを渡す。
    startMagicEffect(startPos, playerRotY, attackStep, VGet(0.0f, 0.0f, 0.0f));
    if (!mMagicEffects.empty())
    {
        mMagicEffects.back().groundModelHandle = groundModelHandle;
    }
}
void AttackEffect::startMagicEffect(VECTOR startPos, float playerRotY, int attackStep, VECTOR targetPos)
{
    /// attackStep によって魔法弾、ビーム、魔法陣の表示時間と移動方式を切り替える。
    MagicEffect effect;
    effect.forward = forwardFromRot(playerRotY);
    effect.targetPos = targetPos;
    effect.hasTarget = attackStep == 1 && VSize(targetPos) > 0.0001f;
    effect.timer = 0.0f;
    effect.attackStep = attackStep;
    effect.groundModelHandle = -1;
    effect.duration = attackStep == 1 ? MagicMissileDuration : attackStep == 2 ? MagicBeamDuration : MagicCircleDuration;
    effect.pos = startPos;
    effect.startPos = startPos;
    effect.curvePos = VAdd(startPos, VAdd(VScale(effect.forward, 14.0f), VGet(0.0f, 12.0f, 0.0f)));

    if (effect.hasTarget)
    {
        /// ターゲット付き魔法弾は中間制御点を使い、直線ではなく弧を描かせる。
        VECTOR toTarget = VSub(effect.targetPos, effect.pos);
        if (VSize(toTarget) > 0.0001f)
        {
            effect.forward = VNorm(toTarget);
            VECTOR right = VGet(effect.forward.z, 0.0f, -effect.forward.x);
            effect.curvePos = VAdd(effect.startPos, VAdd(VScale(effect.forward, 18.0f), VAdd(VScale(right, 9.0f), VGet(0.0f, 13.0f, 0.0f))));
        }
    }
    effect.trail.push_back(effect.pos);

    mMagicEffects.push_back(effect);

    /// 発生点の周囲に短命な粒子を出し、魔法の立ち上がりを見せる。
    VECTOR right = VGet(effect.forward.z, 0.0f, -effect.forward.x);
    for (int i = 0; i < MagicParticleCount; ++i)
    {
        float angle = DX_TWO_PI_F * static_cast<float>(i) / MagicParticleCount;
        float radius = 1.5f + static_cast<float>(i % 4) * 0.7f;
        MagicParticle particle;
        particle.pos = VAdd(effect.pos, VAdd(VScale(right, cosf(angle) * radius), VGet(0.0f, sinf(angle) * radius, 0.0f)));
        particle.velocity = VAdd(VScale(right, cosf(angle) * 3.5f), VAdd(VScale(effect.forward, attackStep == 3 ? 0.0f : -3.0f), VGet(0.0f, 3.0f + static_cast<float>(i % 3), 0.0f)));
        particle.timer = 0.0f;
        particle.duration = 0.45f + static_cast<float>(i % 3) * 0.08f;
        particle.size = 3.0f + static_cast<float>(i % 3);
        mMagicParticles.push_back(particle);
    }
}
