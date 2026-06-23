/**
 * @file
 * @brief AttackEffect の DxLib 描画処理を定義する。
 */

#include "attack_effect.h"
#include "attack_effect_internal.h"

#include <vector>

using namespace AttackEffectInternal;

namespace
{
VECTOR sampleGroundPosition(int groundModelHandle, VECTOR pos)
{
    if (groundModelHandle == -1)
    {
        return pos;
    }

    /// 魔法陣は指定位置の真下へレイを飛ばし、ステージ表面へ合わせて表示する。
    VECTOR start = VAdd(pos, VGet(0.0f, 80.0f, 0.0f));
    VECTOR end = VAdd(pos, VGet(0.0f, -200.0f, 0.0f));
    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(groundModelHandle, -1, start, end);
    if (hit.HitFlag)
    {
        return VAdd(hit.HitPosition, VGet(0.0f, MagicCircleGroundOffset, 0.0f));
    }

    return pos;
}

VERTEX3D makeVertex(VECTOR pos, COLOR_U8 color, float u = 0.0f, float v = 0.0f)
{
    /// DxLib の DrawPolygon3D 系で使う頂点を、発光系エフェクト向けの標準値で作る。
    VERTEX3D vertex;
    vertex.pos = pos;
    vertex.norm = VGet(0.0f, 1.0f, 0.0f);
    vertex.dif = color;
    vertex.spc = GetColorU8(0, 0, 0, 0);
    vertex.u = u;
    vertex.v = v;
    vertex.su = 0.0f;
    vertex.sv = 0.0f;
    return vertex;
}
}

void AttackEffect::drawHitEffectImage() const
{
    /// ヒット系はライティングを切り、常に見える発光エフェクトとして描画する。
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);

    for (const auto& impact : mImpacts)
    {

        float rate = clamp01(impact.timer / impact.duration);
        float fade = 1.0f - rate;
        VECTOR pos = VAdd(impact.pos, VGet(0.0f, impact.isMagic ? 7.0f : 4.0f, 0.0f));

        if (impact.isMagic)
        {
            /// 魔法ヒットは球と放射ラインで、物理ヒットの粒子とは別表現にする。
            int alpha = static_cast<int>(230.0f * fade);
            float coreSize = 3.0f + rate * 4.5f;
            float ringSize = 5.0f + rate * 15.0f;
            int blue = static_cast<int>(180.0f + 75.0f * fade);
            int color = GetColor(80, blue, 255);
            int coreColor = GetColor(220, 250, 255);

            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            DrawSphere3D(pos, coreSize, 12, coreColor, color, TRUE);
            DrawSphere3D(pos, ringSize * 0.45f, 12, color, coreColor, FALSE);

            for (int i = 0; i < 12; ++i)
            {
                float angle = DX_TWO_PI_F * static_cast<float>(i) / 12.0f;
                VECTOR dir = VGet(cosf(angle), sinf(angle) * 0.35f, sinf(angle));
                VECTOR start = VAdd(pos, VScale(dir, ringSize * 0.25f));
                VECTOR end = VAdd(pos, VScale(dir, ringSize));
                DrawLine3D(start, end, color);
            }
        }
        else
        {
            /// 通常、シールド、カウンターは同じ粒子処理で色を切り替える。
            for (const auto& particle : impact.particles)
            {
                if (particle.alpha <= 0.0f)
                {
                    continue;
                }

                int alpha = static_cast<int>(particle.alpha * 230.0f);
                VECTOR particlePos = VAdd(particle.pos, VGet(0.0f, 4.0f, 0.0f));
                VECTOR tailPos = VSub(particlePos, VScale(particle.dir, particle.size * 0.9f));
                int darkRed = impact.isCounter ? GetColor(150, 80, 0) : impact.isShield ? GetColor(5, 30, 120) : GetColor(95, 0, 0);
                int brightRed = impact.isCounter ? GetColor(255, 245, 160) : impact.isShield ? GetColor(60, 185, 255) : GetColor(255, 24, 18);

                SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                DrawSphere3D(particlePos, particle.size * 0.18f, 6, brightRed, darkRed, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_ADD, alpha / 2);
                DrawLine3D(tailPos, particlePos, brightRed);
            }
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawBright(255, 255, 255);
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
}
void AttackEffect::drawMagicEffects() const
{
    /// 魔法系はビルボードと床ポリゴンを使うため、ライティングを切って描く。
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);

    for (const auto& effect : mMagicEffects)
    {
        float rate = clamp01(effect.timer / effect.duration);
        float fade = 1.0f - rate;

        if (effect.attackStep == 1 && mMagicMissileGraphHandle != -1)
        {
            /// 魔法弾は現在位置の本体と、過去位置のトレイルを別サイズで描く。
            if (effect.trail.size() > 1)
            {
                for (int i = static_cast<int>(effect.trail.size()) - 2; i >= 0; --i)
                {
                    float t = static_cast<float>(i + 1) / static_cast<float>(effect.trail.size());
                    float alpha = t * fade;
                    SetDrawBright(175, 235, 255);
                    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(150.0f * alpha));
                    DrawBillboard3D(effect.trail[i], 0.5f, 0.5f, 7.5f + 8.0f * alpha, effect.timer * 6.0f + i * 0.35f, mMagicMissileGraphHandle, TRUE);
                }
            }
            else
            {
                for (int i = 6; i >= 1; --i)
                {
                    float t = static_cast<float>(i) / 6.0f;
                    VECTOR pos = VAdd(effect.pos, VScale(effect.forward, -5.0f * i));
                    SetDrawBright(175, 235, 255);
                    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(165.0f * fade * (1.0f - t * 0.12f)));
                    DrawBillboard3D(pos, 0.5f, 0.5f, 12.5f * (1.0f - t * 0.10f), 0.0f, mMagicMissileGraphHandle, TRUE);
                }
            }

            SetDrawBright(205, 245, 255);
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255.0f * fade));
            DrawBillboard3D(effect.pos, 0.5f, 0.5f, 24.0f, -effect.timer * 5.0f, mMagicMissileGraphHandle, TRUE);
            DrawBillboard3D(effect.pos, 0.5f, 0.5f, 17.0f, effect.timer * 9.0f, mMagicMissileGraphHandle, TRUE);
        }
        else if (effect.attackStep == 2 && mMagicBeamGraphHandle != -1)
        {
            /// ビームは射程方向に複数ビルボードを並べ、脈動で太さを変える。
            float charge = sinf(rate * DX_PI_F);
            float coreAlpha = fade * (0.72f + 0.28f * charge);
            for (int i = 0; i < MagicBeamSegmentCount; ++i)
            {
                float t = static_cast<float>(i) / static_cast<float>(MagicBeamSegmentCount - 1);
                VECTOR pos = VAdd(effect.pos, VScale(effect.forward, MagicBeamVisualRange * t));
                float pulse = 0.5f + 0.5f * sinf(effect.timer * 24.0f + t * 10.0f);

                SetDrawBright(95, 185, 255);
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(135.0f * coreAlpha * (1.0f - t * 0.18f)));
                DrawBillboard3D(pos, 0.5f, 0.5f, 43.0f + pulse * 7.0f, effect.timer * 4.5f + t * 2.0f, mMagicBeamGraphHandle, TRUE);

                SetDrawBright(235, 250, 255);
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(245.0f * coreAlpha * (1.0f - t * 0.08f)));
                DrawBillboard3D(pos, 0.5f, 0.5f, 19.0f + pulse * 3.0f, -effect.timer * 12.0f - t * 4.0f, mMagicBeamGraphHandle, TRUE);
            }

            for (int i = 0; i < 5; ++i)
            {
                float t = static_cast<float>(i) / 4.0f;
                VECTOR pos = VAdd(effect.pos, VScale(effect.forward, MagicBeamVisualRange * t));
                SetDrawBright(160, 230, 255);
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(95.0f * coreAlpha));
                DrawBillboard3D(pos, 0.5f, 0.5f, 64.0f + i * 3.0f, effect.timer * 8.0f + i, mMagicBeamGraphHandle, TRUE);
            }
        }
        else if (effect.attackStep == 3 && mMagicCircleGraphHandle != -1)
        {
            /// 魔法陣はリングごとに分割し、各頂点を地面へサンプルして床の傾きに追従させる。
            float halfSize = 42.0f + rate * 5.0f;
            int alpha = static_cast<int>(255.0f * fade);
            COLOR_U8 color = GetColorU8(35, 165, 255, alpha);
            VECTOR center = effect.pos;
            std::vector<VERTEX3D> vertices;
            vertices.reserve(MagicCircleRingCount * MagicCircleSegmentCount * 6);

            for (int ring = 0; ring < MagicCircleRingCount; ++ring)
            {
                float innerRadius = halfSize * static_cast<float>(ring) / MagicCircleRingCount;
                float outerRadius = halfSize * static_cast<float>(ring + 1) / MagicCircleRingCount;

                for (int segment = 0; segment < MagicCircleSegmentCount; ++segment)
                {
                    float angle0 = DX_TWO_PI_F * static_cast<float>(segment) / MagicCircleSegmentCount;
                    float angle1 = DX_TWO_PI_F * static_cast<float>(segment + 1) / MagicCircleSegmentCount;
                    VECTOR inner0 = VGet(cosf(angle0) * innerRadius, 0.0f, sinf(angle0) * innerRadius);
                    VECTOR outer0 = VGet(cosf(angle0) * outerRadius, 0.0f, sinf(angle0) * outerRadius);
                    VECTOR outer1 = VGet(cosf(angle1) * outerRadius, 0.0f, sinf(angle1) * outerRadius);
                    VECTOR inner1 = VGet(cosf(angle1) * innerRadius, 0.0f, sinf(angle1) * innerRadius);
                    VECTOR p0 = sampleGroundPosition(effect.groundModelHandle, VAdd(center, inner0));
                    VECTOR p1 = sampleGroundPosition(effect.groundModelHandle, VAdd(center, outer0));
                    VECTOR p2 = sampleGroundPosition(effect.groundModelHandle, VAdd(center, outer1));
                    VECTOR p3 = sampleGroundPosition(effect.groundModelHandle, VAdd(center, inner1));
                    float u0 = 0.5f + inner0.x / (halfSize * 2.0f);
                    float v0 = 0.5f + inner0.z / (halfSize * 2.0f);
                    float u1 = 0.5f + outer0.x / (halfSize * 2.0f);
                    float v1 = 0.5f + outer0.z / (halfSize * 2.0f);
                    float u2 = 0.5f + outer1.x / (halfSize * 2.0f);
                    float v2 = 0.5f + outer1.z / (halfSize * 2.0f);
                    float u3 = 0.5f + inner1.x / (halfSize * 2.0f);
                    float v3 = 0.5f + inner1.z / (halfSize * 2.0f);

                    vertices.push_back(makeVertex(p0, color, u0, v0));
                    vertices.push_back(makeVertex(p1, color, u1, v1));
                    vertices.push_back(makeVertex(p2, color, u2, v2));
                    vertices.push_back(makeVertex(p0, color, u0, v0));
                    vertices.push_back(makeVertex(p2, color, u2, v2));
                    vertices.push_back(makeVertex(p3, color, u3, v3));
                }
            }

            SetUseZBuffer3D(TRUE);
            SetWriteZBuffer3D(FALSE);
            SetDrawBright(120, 210, 255);
            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            DrawPolygon3D(vertices.data(), static_cast<int>(vertices.size() / 3), mMagicCircleGraphHandle, TRUE);
            SetUseZBuffer3D(FALSE);
        }
    }

    int particleGraph = mMagicMissileGraphHandle;
    if (particleGraph != -1)
    {
        /// 魔法の発生粒子は短命なビルボードとしてまとめて描画する。
        for (const auto& particle : mMagicParticles)
        {
            float fade = 1.0f - clamp01(particle.timer / particle.duration);
            SetDrawBright(140, 220, 255);
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(230.0f * fade));
            DrawBillboard3D(particle.pos, 0.5f, 0.5f, particle.size, particle.timer * 5.0f, particleGraph, TRUE);
        }
    }
    for (const auto& particle : mHealParticles)
    {
        /// 回復粒子は緑の小球で上昇させ、攻撃エフェクトと色で区別する。
        float rate = clamp01(particle.timer / particle.duration);
        float fade = 1.0f - rate;
        int alpha = static_cast<int>(220.0f * fade);
        int color = GetColor(20, 255, 60);
        int subColor = GetColor(120, 255, 140);
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
        DrawSphere3D(particle.pos, particle.size * (0.75f + rate * 0.25f), 6, subColor, color, TRUE);
    }


    SetDrawBright(255, 255, 255);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawBright(255, 255, 255);
    SetWriteZBuffer3D(TRUE);
    SetUseZBuffer3D(TRUE);
    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
}
void AttackEffect::drawSwordTrailDxLib() const
{
    if (mSwordTrailSamples.size() < 2)
    {
        return;
    }

    /// DxLib fallback でも DirectX 11 版と同じサンプル補間で剣軌跡を描く。
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

    for (std::size_t i = 0; i + 1 < mSwordTrailSamples.size(); ++i)
    {
        const SwordTrailSample& p0 = mSwordTrailSamples[i == 0 ? i : i - 1];
        const SwordTrailSample& p1 = mSwordTrailSamples[i];
        const SwordTrailSample& p2 = mSwordTrailSamples[i + 1];
        const SwordTrailSample& p3 = mSwordTrailSamples[i + 2 < mSwordTrailSamples.size() ? i + 2 : i + 1];

        for (int j = 0; j < SwordTrailSubdivisions; ++j)
        {
            float t0 = static_cast<float>(j) / SwordTrailSubdivisions;
            float t1 = static_cast<float>(j + 1) / SwordTrailSubdivisions;
            VECTOR base0 = catmullRomVector(p0.basePos, p1.basePos, p2.basePos, p3.basePos, t0);
            VECTOR tip0 = catmullRomVector(p0.tipPos, p1.tipPos, p2.tipPos, p3.tipPos, t0);
            VECTOR base1 = catmullRomVector(p0.basePos, p1.basePos, p2.basePos, p3.basePos, t1);
            VECTOR tip1 = catmullRomVector(p0.tipPos, p1.tipPos, p2.tipPos, p3.tipPos, t1);
            float ageRate0 = clamp01(lerpVector(VGet(p1.age, 0.0f, 0.0f), VGet(p2.age, 0.0f, 0.0f), t0).x / SwordTrailDuration);
            float ageRate1 = clamp01(lerpVector(VGet(p1.age, 0.0f, 0.0f), VGet(p2.age, 0.0f, 0.0f), t1).x / SwordTrailDuration);
            int alpha0 = static_cast<int>((1.0f - ageRate0) * 235.0f);
            int alpha1 = static_cast<int>((1.0f - ageRate1) * 235.0f);

            VERTEX3D vertices[6];
            vertices[0] = makeVertex(base0, GetColorU8(120, 190, 255, alpha0 / 5), 0.0f, ageRate0);
            vertices[1] = makeVertex(tip0, GetColorU8(220, 245, 255, alpha0), 1.0f, ageRate0);
            vertices[2] = makeVertex(tip1, GetColorU8(220, 245, 255, alpha1), 1.0f, ageRate1);
            vertices[3] = vertices[0];
            vertices[4] = vertices[2];
            vertices[5] = makeVertex(base1, GetColorU8(120, 190, 255, alpha1 / 5), 0.0f, ageRate1);
            DrawPolygon3D(vertices, 2, DX_NONE_GRAPH, TRUE);
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
}
void AttackEffect::drawDxLibFallback() const
{
    /// シェーダーが使えない環境向けに、DxLib の DrawPolygon3D だけで攻撃表現を描く。
    drawSwordTrailDxLib();

    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

    if (mPixelShaderHandle != -1)
    {
        SetUsePixelShader(mPixelShaderHandle);
    }

    for (const auto& slash : mSlashes)
    {
        /// fallback の斬撃も DirectX 11 版と同じ扇形ポリゴン構成にする。
        float rate = clamp01(slash.timer / slash.duration);
        float fade = 1.0f - rate;
        float stepScale = slash.attackStep == 3 ? 1.35f : 1.0f;
        float innerRadius = 6.0f * stepScale;
        float outerRadius = 24.0f * stepScale;
        float height = 7.0f + (slash.attackStep * 1.5f);
        float startAngle = -1.10f + (rate * 0.45f);
        float endAngle = 1.10f + (rate * 0.45f);
        int alpha = static_cast<int>(120.0f * fade);
        COLOR_U8 innerColor = GetColorU8(0, 45, 180, alpha / 2);
        COLOR_U8 outerColor = GetColorU8(15, 130, 255, alpha);
        FLOAT4 params = { slash.timer, fade, static_cast<float>(slash.attackStep), 0.0f };
        FLOAT4 tint = { 0.08f, 0.42f, 1.0f, 0.75f };
        VERTEX3D vertices[SlashSegmentCount * 6];
        int vertexIndex = 0;

        if (mPixelShaderHandle != -1)
        {
            SetPSConstF(0, params);
            SetPSConstF(1, tint);
        }

        for (int i = 0; i < SlashSegmentCount; ++i)
        {
            float t0 = static_cast<float>(i) / SlashSegmentCount;
            float t1 = static_cast<float>(i + 1) / SlashSegmentCount;
            float a0 = startAngle + ((endAngle - startAngle) * t0);
            float a1 = startAngle + ((endAngle - startAngle) * t1);
            VECTOR i0 = VAdd(slash.pos, rotateLocal(sinf(a0) * innerRadius, cosf(a0) * innerRadius, slash.rotY));
            VECTOR o0 = VAdd(slash.pos, rotateLocal(sinf(a0) * outerRadius, cosf(a0) * outerRadius, slash.rotY));
            VECTOR i1 = VAdd(slash.pos, rotateLocal(sinf(a1) * innerRadius, cosf(a1) * innerRadius, slash.rotY));
            VECTOR o1 = VAdd(slash.pos, rotateLocal(sinf(a1) * outerRadius, cosf(a1) * outerRadius, slash.rotY));

            i0.y += height;
            o0.y += height + sinf(t0 * DX_PI_F) * 5.0f;
            i1.y += height;
            o1.y += height + sinf(t1 * DX_PI_F) * 5.0f;

            vertices[vertexIndex++] = makeVertex(i0, innerColor, t0, 0.0f);
            vertices[vertexIndex++] = makeVertex(o0, outerColor, t0, 1.0f);
            vertices[vertexIndex++] = makeVertex(o1, outerColor, t1, 1.0f);
            vertices[vertexIndex++] = makeVertex(i0, innerColor, t0, 0.0f);
            vertices[vertexIndex++] = makeVertex(o1, outerColor, t1, 1.0f);
            vertices[vertexIndex++] = makeVertex(i1, innerColor, t1, 0.0f);
        }

        DrawPolygon3D(vertices, SlashSegmentCount * 2, DX_NONE_GRAPH, TRUE);
    }

    for (const auto& impact : mImpacts)
    {

        float rate = clamp01(impact.timer / impact.duration);
        float fade = 1.0f - rate;
        float stepScale = impact.attackStep == 3 ? 1.5f : 1.0f;
        float innerRadius = (3.0f + rate * 18.0f) * stepScale;
        float outerRadius = innerRadius + (4.0f * stepScale);
        int alpha = static_cast<int>(230.0f * fade);
        COLOR_U8 innerColor = impact.isCounter ? GetColorU8(255, 210, 70, alpha / 2) : impact.isShield ? GetColorU8(0, 55, 190, alpha / 2) : GetColorU8(120, 0, 0, alpha / 2);
        COLOR_U8 outerColor = impact.isCounter ? GetColorU8(255, 245, 175, alpha) : impact.isShield ? GetColorU8(20, 160, 255, alpha) : GetColorU8(255, 45, 25, alpha);
        FLOAT4 params = { impact.timer, fade, static_cast<float>(impact.attackStep), 1.0f };
        FLOAT4 tint = impact.isCounter ? FLOAT4{ 1.0f, 0.78f, 0.20f, 1.0f } : impact.isShield ? FLOAT4{ 0.08f, 0.45f, 1.0f, 1.0f } : FLOAT4{ 1.0f, 0.10f, 0.04f, 1.0f };
        VERTEX3D vertices[ImpactSegmentCount * 6];
        int vertexIndex = 0;

        if (mPixelShaderHandle != -1)
        {
            SetPSConstF(0, params);
            SetPSConstF(1, tint);
        }

        for (int i = 0; i < ImpactSegmentCount; ++i)
        {
            float a0 = DX_TWO_PI_F * static_cast<float>(i) / ImpactSegmentCount;
            float a1 = DX_TWO_PI_F * static_cast<float>(i + 1) / ImpactSegmentCount;
            VECTOR i0 = VAdd(impact.pos, VGet(sinf(a0) * innerRadius, 0.25f, cosf(a0) * innerRadius));
            VECTOR o0 = VAdd(impact.pos, VGet(sinf(a0) * outerRadius, 0.25f, cosf(a0) * outerRadius));
            VECTOR i1 = VAdd(impact.pos, VGet(sinf(a1) * innerRadius, 0.25f, cosf(a1) * innerRadius));
            VECTOR o1 = VAdd(impact.pos, VGet(sinf(a1) * outerRadius, 0.25f, cosf(a1) * outerRadius));

            float t0 = static_cast<float>(i) / ImpactSegmentCount;
            float t1 = static_cast<float>(i + 1) / ImpactSegmentCount;
            vertices[vertexIndex++] = makeVertex(i0, innerColor, t0, 0.0f);
            vertices[vertexIndex++] = makeVertex(o0, outerColor, t0, 1.0f);
            vertices[vertexIndex++] = makeVertex(o1, outerColor, t1, 1.0f);
            vertices[vertexIndex++] = makeVertex(i0, innerColor, t0, 0.0f);
            vertices[vertexIndex++] = makeVertex(o1, outerColor, t1, 1.0f);
            vertices[vertexIndex++] = makeVertex(i1, innerColor, t1, 0.0f);
        }

        DrawPolygon3D(vertices, ImpactSegmentCount * 2, DX_NONE_GRAPH, TRUE);

        float burstHalfWidth = (2.0f + rate * 7.0f) * stepScale;
        float burstHeight = (10.0f + rate * 13.0f) * stepScale;
        COLOR_U8 burstCenter = impact.isCounter ? GetColorU8(255, 245, 190, alpha) : impact.isShield ? GetColorU8(25, 170, 255, alpha) : GetColorU8(255, 70, 35, alpha);
        COLOR_U8 burstEdge = impact.isCounter ? GetColorU8(180, 90, 0, alpha / 4) : impact.isShield ? GetColorU8(0, 40, 180, alpha / 4) : GetColorU8(120, 0, 0, alpha / 4);
        VECTOR center = impact.pos;

        VERTEX3D burstX[6];
        burstX[0] = makeVertex(VAdd(center, VGet(-burstHalfWidth, -burstHeight * 0.5f, 0.0f)), burstEdge, 0.0f, 0.0f);
        burstX[1] = makeVertex(VAdd(center, VGet(0.0f, 0.0f, 0.0f)), burstCenter, 0.5f, 0.5f);
        burstX[2] = makeVertex(VAdd(center, VGet(burstHalfWidth, -burstHeight * 0.5f, 0.0f)), burstEdge, 1.0f, 0.0f);
        burstX[3] = burstX[0];
        burstX[4] = makeVertex(VAdd(center, VGet(0.0f, burstHeight * 0.5f, 0.0f)), burstCenter, 0.5f, 1.0f);
        burstX[5] = burstX[2];
        DrawPolygon3D(burstX, 2, DX_NONE_GRAPH, TRUE);

        VERTEX3D burstZ[6];
        burstZ[0] = makeVertex(VAdd(center, VGet(0.0f, -burstHeight * 0.5f, -burstHalfWidth)), burstEdge, 0.0f, 0.0f);
        burstZ[1] = makeVertex(VAdd(center, VGet(0.0f, 0.0f, 0.0f)), burstCenter, 0.5f, 0.5f);
        burstZ[2] = makeVertex(VAdd(center, VGet(0.0f, -burstHeight * 0.5f, burstHalfWidth)), burstEdge, 1.0f, 0.0f);
        burstZ[3] = burstZ[0];
        burstZ[4] = makeVertex(VAdd(center, VGet(0.0f, burstHeight * 0.5f, 0.0f)), burstCenter, 0.5f, 1.0f);
        burstZ[5] = burstZ[2];
        DrawPolygon3D(burstZ, 2, DX_NONE_GRAPH, TRUE);
    }

    SetUsePixelShader(-1);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
}
