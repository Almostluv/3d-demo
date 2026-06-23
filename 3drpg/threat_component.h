#ifndef _THREAT_COMPONENT_H_
#define _THREAT_COMPONENT_H_


/**
 * @file
 * @brief 敵 AI が攻撃対象を選ぶための脅威値コンポーネントを定義する。
 */

/**
 * @brief 脅威値を集計する攻撃元の種類。
 */
enum class ThreatSource
{
    Player,    ///< 操作プレイヤー。
    Companion, ///< AI 相棒。
    Count      ///< 配列サイズ計算用の番兵値。通常の攻撃元としては使わない。
};

/**
 * @brief 攻撃元ごとの脅威値を蓄積・減衰し、最も高い対象を返すコンポーネント。
 *
 * Enemy が値メンバとして所有し、毎フレーム `update()` で時間減衰させる。
 */
class ThreatComponent
{
public:
    /**
     * @brief 初期脅威値を設定する。
     */
    ThreatComponent();

    /**
     * @brief 脅威値を初期状態へ戻す。
     */
    void reset();

    /**
     * @brief 脅威値を時間経過で更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void update(float deltaTime);

    /**
     * @brief 指定した攻撃元へ脅威値を加算する。
     * @param[in] source 加算対象。
     * @param[in] value 加算する脅威値。
     */
    void addThreat(ThreatSource source, float value);

    /**
     * @brief ダメージ量に応じた脅威値を加算する。
     * @param[in] source 加算対象。
     * @param[in] damage 与えたダメージ量。
     */
    void addDamageThreat(ThreatSource source, int damage);

    /**
     * @brief 指定した攻撃元の現在脅威値を取得する。
     * @param[in] source 取得対象。
     * @return 現在の脅威値。
     */
    float getThreat(ThreatSource source) const;

    /**
     * @brief 最も脅威値が高い攻撃元を取得する。
     * @return 最大脅威値を持つ攻撃元。
     */
    ThreatSource getHighestSource() const;

private:
    static constexpr int SourceCount = static_cast<int>(ThreatSource::Count); ///< `mThreat` の要素数。

    /**
     * @brief ThreatSource を配列インデックスへ変換する。
     * @param[in] source 変換する攻撃元。
     * @return `mThreat` 用インデックス。
     */
    int toIndex(ThreatSource source) const;

    float mThreat[SourceCount]; ///< 攻撃元ごとの現在脅威値。
};

#endif
