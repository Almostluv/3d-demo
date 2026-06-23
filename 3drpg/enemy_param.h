#ifndef _ENEMY_PARAM_H_
#define _ENEMY_PARAM_H_


/**
 * @file
 * @brief Enemy の調整用パラメータ定数を定義する。
 */


/**
 * @brief Mutant 系 Enemy の HP、当たり判定、AI、攻撃、リアクションに関する調整値。
 *
 * 時間値は秒、距離や半径は DxLib ワールド単位として使用される。
 */
namespace EnemyParam
{

    constexpr int Hp = 50; ///< Enemy の初期 HP。
    constexpr int AttackPower = 2; ///< Enemy の基礎攻撃力。
    constexpr int BossShieldHp = 6; ///< ボスシールドの最大 HP。
    constexpr float BossShieldBreakDuration = 10.0f; ///< ボスシールド破壊後の無効時間（秒）。


    constexpr float Scale = 0.24f; ///< Enemy モデルへ適用する一様スケール。

    constexpr float HitRadius = 17.0f; ///< Enemy の基本被弾判定半径。DxLib ワールド単位。
    constexpr int HitCircleCount = 6; ///< 複数円で近似する被弾判定の円数。
    constexpr float HitCircleRadius = 8.5f; ///< 複数円被弾判定の各円半径。DxLib ワールド単位。
    constexpr float HitCircleOffset = 10.0f; ///< 複数円被弾判定の前後オフセット。DxLib ワールド単位。
    constexpr float CollisionRadius = 9.0f; ///< Enemy 押し戻し衝突半径。DxLib ワールド単位。
    constexpr float CollisionHeight = 40.0f; ///< Enemy 押し戻し衝突高さ。DxLib ワールド単位。
    constexpr float AttackRadius = 26.0f; ///< 通常攻撃判定半径。DxLib ワールド単位。
    constexpr float AttackStartDistance = 38.0f; ///< 通常攻撃を開始する距離。DxLib ワールド単位。
    constexpr float AttackWidth = 12.0f; ///< 腕攻撃判定の幅。DxLib ワールド単位。


    constexpr float DetectRadius = 90.0f; ///< ターゲット検出半径。DxLib ワールド単位。
    constexpr float MoveSpeed = 13.0f; ///< Enemy の移動速度。DxLib ワールド単位/秒。
    constexpr float TargetSwitchCooldown = 1.5f; ///< ターゲット切り替え後の再切り替え待機時間（秒）。
    constexpr float FarTargetPenaltyStartRate = 1.15f; ///< 遠距離ターゲットへスコアペナルティを掛け始める距離倍率。
    constexpr float FarTargetInvalidRate = 1.65f; ///< 遠距離ターゲットを無効扱いにする距離倍率。
    constexpr float FarTargetPenaltyScale = 1.6f; ///< 遠距離ターゲットのスコアペナルティ倍率。
    constexpr float BossPlayerThreatBonus = 80.0f; ///< ボス時にプレイヤーへ加える脅威値ボーナス。
    constexpr int BossCompanionTargetChance = 25; ///< ボスが相棒を狙う抽選値。実際の確率計算は使用箇所に従う。


    constexpr float HitDuration = 0.35f; ///< 被弾リアクションの継続時間（秒）。
    constexpr float CounterHitStopDuration = 0.10f; ///< カウンター被弾時のヒットストップ時間（秒）。
    constexpr float DyingDuration = 1.2f; ///< 死亡状態の基準継続時間（秒）。


    constexpr float KnockbackSpeed = 40.0f; ///< 被弾ノックバック速度。DxLib ワールド単位/秒。
    constexpr float KnockbackDuration = 0.28f; ///< 被弾ノックバック継続時間（秒）。
    constexpr float KnockbackTotalDistance = 10.0f; ///< 被弾ノックバック総距離。DxLib ワールド単位。

    constexpr float KnockbackEndCompensationDistance = 9.0f; ///< ノックバック終了時の位置補正距離。DxLib ワールド単位。


    constexpr float AttackCooldown = 0.6f; ///< 通常攻撃後の基本クールダウン時間（秒）。
    constexpr float AttackRecoveryMin = 1.0f; ///< 通常攻撃後硬直の最短時間（秒）。
    constexpr float AttackRecoveryMax = 2.0f; ///< 通常攻撃後硬直の最長時間（秒）。
    constexpr float RamTriggerMinDistance = 38.0f; ///< 突進攻撃を選択する最短距離。DxLib ワールド単位。
    constexpr float RamTriggerMaxDistance = 115.0f; ///< 突進攻撃を選択する最長距離。DxLib ワールド単位。
    constexpr float RamChargeDuration = 0.75f; ///< 突進攻撃の予備動作時間（秒）。
    constexpr float RamDuration = 1.05f; ///< 突進攻撃の継続時間（秒）。
    constexpr float RamSpeed = 145.0f; ///< 突進攻撃の移動速度。DxLib ワールド単位/秒。
    constexpr float RamLength = 120.0f; ///< 突進攻撃の移動距離。DxLib ワールド単位。
    constexpr float RamCooldown = 4.0f; ///< 突進攻撃のクールダウン時間（秒）。
    constexpr float SpinAttackRadius = 26.0f; ///< 回転攻撃の判定半径。DxLib ワールド単位。
    constexpr float SpinAttackCooldown = 3.2f; ///< 回転攻撃のクールダウン時間（秒）。
    constexpr float CloseContactDistance = 15.0f; ///< 近距離接触扱いにする距離。DxLib ワールド単位。
    constexpr float CloseContactDuration = 1.0f; ///< 近距離接触が続いた時に行動判断へ使う時間（秒）。


    constexpr float JumpAttackRadius = 41.0f; ///< ジャンプ攻撃の判定半径。DxLib ワールド単位。

    constexpr float JumpAttackEffectiveDistance = 45.0f; ///< ジャンプ攻撃を有効に使いやすい距離。DxLib ワールド単位。

    constexpr float JumpAttackCooldown = 8.0f; ///< ジャンプ攻撃のクールダウン時間（秒）。

    constexpr float JumpAttackStartupDuration = 0.45f; ///< ジャンプ攻撃の予備動作時間（秒）。

    constexpr float JumpAttackRecoveryDuration = 0.55f; ///< ジャンプ攻撃着地後の硬直時間（秒）。

    constexpr float JumpAttackLandingOffsetDistance = 8.5f; ///< ジャンプ攻撃着地位置の補正距離。DxLib ワールド単位。

    constexpr float JumpAttackHeight = 17.0f; ///< ジャンプ攻撃の基準高さ。DxLib ワールド単位。

    constexpr float JumpAttackMinHeight = 12.0f; ///< ジャンプ攻撃高さの下限。DxLib ワールド単位。

    constexpr float JumpAttackMaxHeight = 26.0f; ///< ジャンプ攻撃高さの上限。DxLib ワールド単位。

    constexpr float JumpAttackLandTiming = 0.72f; ///< ジャンプ攻撃中に着地扱いへ移る正規化タイミング。

    constexpr float JumpAttackDuration = 1.1f; ///< ジャンプ攻撃の基準継続時間（秒）。

    constexpr float JumpAttackMinDuration = 0.8f; ///< ジャンプ攻撃継続時間の下限（秒）。

    constexpr float JumpAttackMaxDuration = 1.6f; ///< ジャンプ攻撃継続時間の上限（秒）。
}

#endif // !_ENEMY_PARAM_H_
