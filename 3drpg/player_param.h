#ifndef _PLAYER_PARAM_H_
#define _PLAYER_PARAM_H_


/**
 * @file
 * @brief Player の調整用パラメータ定数を定義する。
 */


/**
 * @brief Player の HP、スタミナ、移動、攻撃、回避、ガード、被弾に関する調整値。
 *
 * 時間値は秒、距離や半径は DxLib ワールド単位として使用される。
 */
namespace PlayerParam
{

    constexpr int Hp = 20; ///< プレイヤーの初期 HP。
    constexpr int AttackPower = 1; ///< 通常攻撃の基礎ダメージ。

    constexpr float Stamina = 100.0f; ///< スタミナ最大値。

    constexpr float StaminaRegenDelay = 0.80f; ///< スタミナ消費後、回復開始までの待機時間（秒）。

    constexpr float StaminaRegenSpeed = 70.0f; ///< 1 秒あたりのスタミナ回復量。

    constexpr float KnightAttack1Stamina = 30.0f; ///< Knight 1 段目攻撃のスタミナ消費量。

    constexpr float KnightAttack2Stamina = 30.0f; ///< Knight 2 段目攻撃のスタミナ消費量。

    constexpr float KnightAttack3Stamina = 40.0f; ///< Knight 3 段目攻撃のスタミナ消費量。

    constexpr float DodgeStamina = 30.0f; ///< 回避のスタミナ消費量。

    constexpr float GuardImpactStamina = 30.0f; ///< ガード成功リアクションのスタミナ消費量。

    constexpr float CounterStamina = 20.0f; ///< カウンターのスタミナ消費量。

    constexpr float WizardMagicAttack1Stamina = 30.0f; ///< Wizard 魔法弾攻撃のスタミナ消費量。

    constexpr float WizardMagicAttack2StartStamina = 50.0f; ///< Wizard ビーム魔法開始時のスタミナ消費量。

    constexpr float WizardAreaPowerUpStamina = 60.0f; ///< Wizard 範囲強化魔法のスタミナ消費量。


    constexpr float MoveSpeed = 30.0f; ///< プレイヤーの移動速度。DxLib ワールド単位/秒。
    constexpr float Scale = 0.14f; ///< Knight モデルへ適用する一様スケール。

    constexpr float WizardScale = 0.10f; ///< Wizard モデルへ適用する一様スケール。


    constexpr float AttackDuration = 1.20f; ///< 通常攻撃状態の基本継続時間（秒）。
    constexpr float HitStopDuration = 0.155f; ///< 通常ヒット時に停止させる時間（秒）。
    constexpr float CounterHitStopDuration = 0.10f; ///< カウンターヒット時に停止させる時間（秒）。

    constexpr float AttackRadius = 18.0f; ///< プレイヤー攻撃判定の基本半径。DxLib ワールド単位。

    constexpr float WizardMagicAttack2Interval = 0.30f; ///< Wizard ビーム魔法の連続ヒット間隔（秒）。

    constexpr float WizardMagicAttack2StartDelay = 1.00f; ///< Wizard ビーム魔法開始から判定発生までの待機時間（秒）。

    constexpr float WizardAreaPowerUpRadius = 42.0f; ///< Wizard 範囲強化魔法の効果半径。DxLib ワールド単位。

    constexpr int WizardAreaPowerUpHeal = 5; ///< Wizard 範囲強化魔法で回復する HP 量。

    constexpr int WizardAreaPowerUpAttackPower = 2; ///< Wizard 範囲強化中の攻撃力補正値。

    constexpr float WizardAreaPowerUpDuration = 8.0f; ///< Wizard 範囲強化効果の継続時間（秒）。

    constexpr float AttackFrontDotThreshold = 0.0f; ///< 前方攻撃判定に使う内積しきい値。


    constexpr float DodgeDuration = 1.0f; ///< 回避状態の継続時間（秒）。

    constexpr float DodgeTotalDistance = 28.0f; ///< 回避で移動する総距離。DxLib ワールド単位。

    constexpr float DodgeMoveEndRate = 0.95f; ///< 回避移動を終了する正規化時間。

    constexpr float DodgeAnimEndRate = 0.72f; ///< 回避アニメーションを終了扱いにする正規化時間。


    constexpr float HitRadius = 10.0f; ///< プレイヤー被弾判定半径。DxLib ワールド単位。
    constexpr float CollisionRadius = 4.0f; ///< プレイヤー押し戻し衝突半径。DxLib ワールド単位。
    constexpr float CollisionHeight = 22.0f; ///< Knight 押し戻し衝突高さ。DxLib ワールド単位。
    constexpr float WizardCollisionHeight = 18.0f; ///< Wizard 押し戻し衝突高さ。DxLib ワールド単位。


    constexpr float GuardKnockbackDistance = 8.0f; ///< ガード成功時のノックバック距離。DxLib ワールド単位。

    constexpr float GuardFrontDotThreshold = 0.0f; ///< ガード正面判定に使う内積しきい値。

    constexpr float GuardEnterDuration = 0.10f; ///< ガード入力開始からガード成立までの時間（秒）。

    constexpr float GuardImpactDuration = 1.0f; ///< ガード成功リアクションの継続時間（秒）。


    constexpr float CounterDuration = 0.80f; ///< カウンター状態の継続時間（秒）。


    constexpr float JumpDuration = 0.60f; ///< ジャンプ状態の継続時間（秒）。

    constexpr float JumpSpeed = 6.0f; ///< ジャンプ初速度。DxLib ワールド単位/秒。

    constexpr float JumpGravity = 18.0f; ///< ジャンプ中に適用する重力加速度。DxLib ワールド単位/秒^2。


    constexpr float HitDuration = 0.40f; ///< 被弾リアクションの継続時間（秒）。

    constexpr float RamKnockbackDistance = 18.0f; ///< 突進攻撃を受けた時のノックバック距離。DxLib ワールド単位。

    constexpr float DeadDuration = 1.20f; ///< 死亡状態の基準継続時間（秒）。
}

#endif // !_PLAYER_PARAM_H_
