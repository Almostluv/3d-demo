/**
 * @file
 * @brief threat_component.cpp 実装を定義する。
 */

#include "threat_component.h"

namespace
{
constexpr float PlayerBaseThreat = 100.0f; ///< プレイヤーに初期付与する基本脅威値。
constexpr float CompanionBaseThreat = 30.0f; ///< 相棒に初期付与する基本脅威値。
constexpr float ThreatDecayPerSecond = 6.0f; ///< 脅威値が自然減少する速度。単位は threat/second。
constexpr float DamageThreatScale = 12.0f; ///< ダメージ量を脅威値へ変換する倍率。

float getBaseThreat(ThreatSource source)
{
    /// プレイヤーは基本的に優先ターゲットにしたいため、相棒より高い基礎脅威を持つ。
    return source == ThreatSource::Player
        ? PlayerBaseThreat
        : CompanionBaseThreat;
}
}

ThreatComponent::ThreatComponent()
{
    /// 生成直後からターゲット選択に使えるよう、基礎脅威値を設定する。
    reset();
}

void ThreatComponent::reset()
{
    /// 各ターゲットの脅威値をシーン開始時の基準値へ戻す。
    mThreat[toIndex(ThreatSource::Player)] = PlayerBaseThreat;
    mThreat[toIndex(ThreatSource::Companion)] = CompanionBaseThreat;
}

void ThreatComponent::update(float deltaTime)
{
    for (int i = 0; i < SourceCount; ++i)
    {
        /// 一時的に上がった脅威値だけを、時間経過で基礎値へ戻す。
        ThreatSource source = static_cast<ThreatSource>(i);
        float baseThreat = getBaseThreat(source);

        if (mThreat[i] > baseThreat)
        {
            mThreat[i] -= ThreatDecayPerSecond * deltaTime;
            if (mThreat[i] < baseThreat)
            {
                mThreat[i] = baseThreat;
            }
        }
    }
}

void ThreatComponent::addThreat(ThreatSource source, float value)
{
    if (value <= 0.0f)
    {
        /// 0 以下の加算はターゲット選択に影響させない。
        return;
    }

    mThreat[toIndex(source)] += value;
}

void ThreatComponent::addDamageThreat(ThreatSource source, int damage)
{
    if (damage <= 0)
    {
        /// ダメージが発生していない場合は脅威値を増やさない。
        return;
    }

    /// 与ダメージ量を倍率で脅威値へ変換し、攻撃した対象へ注意を向ける。
    addThreat(source, static_cast<float>(damage) * DamageThreatScale);
}

float ThreatComponent::getThreat(ThreatSource source) const
{
    return mThreat[toIndex(source)];
}

ThreatSource ThreatComponent::getHighestSource() const
{
    /// 同値の場合はプレイヤーを優先し、相棒へターゲットが移りすぎないようにする。
    return mThreat[toIndex(ThreatSource::Companion)] > mThreat[toIndex(ThreatSource::Player)]
        ? ThreatSource::Companion
        : ThreatSource::Player;
}

int ThreatComponent::toIndex(ThreatSource source) const
{
    int index = static_cast<int>(source);
    if (index < 0 || index >= SourceCount)
    {
        /// 不正な source が渡された場合は Player 側へフォールバックする。
        return 0;
    }

    return index;
}
