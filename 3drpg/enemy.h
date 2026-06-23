#ifndef _ENEMY_H_
#define _ENEMY_H_


/**
 * @file
 * @brief Mutant 系ボス敵を表す Enemy を定義する。
 */


#include "char_base.h"
#include "enemy_state.h"
#include "combat_target.h"
#include "threat_component.h"
#include "enemy_param.h"

/**
 * @brief Enemy へのダメージ適用結果。
 */
enum class EnemyDamageResult
{
    None,   ///< ダメージが入らなかった。
    Hp,     ///< HP にダメージが入った。
    Shield  ///< ボスシールドにダメージが入った。
};

/**
 * @brief mutant 系ボス敵を表すクラス。
 *
 * 通常攻撃、ジャンプ攻撃、突進、回転攻撃、ボスシールド、脅威値による
 * ターゲット選択を管理する。
 * シーンから毎フレーム更新され、モデルとアニメーションハンドルを自身で管理する。
 */
class Enemy : public CharBase
{
public:
    /**
     * @brief 初期位置を指定して敵を生成する。
     * @param[in] startPos 出現位置。
     */
    Enemy(VECTOR startPos);

    /**
     * @brief モデルとアニメーションを解放する。
     */
    virtual ~Enemy();

    /**
     * @brief 旧形式の更新処理。必要最低限の単体更新を行う。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime);

    /**
     * @brief プレイヤー位置を基準に敵 AI を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     * @param[in] playerPos プレイヤーのワールド座標。
     */
    void update(float deltaTime, VECTOR playerPos);

    /**
     * @brief 複数ターゲット候補と脅威値を使って敵 AI を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     * @param[in] targets ターゲット候補配列。Enemy は所有しない。
     * @param[in] targetCount ターゲット候補数。
     */
    void update(float deltaTime, const CombatTarget* targets, int targetCount);

    /**
     * @brief 敵モデル、攻撃予告、攻撃軌跡を描画する。
     */
    virtual void draw() override;

    /**
     * @brief 登場時の着地演出を開始する。
     * @param[in] landingPos 着地目標位置。
     */
    void startIntroLanding(VECTOR landingPos);

    /**
     * @brief ジャンプ攻撃中のステージ衝突を解決する。
     * @param[in] stageCollisionModelHandle ステージ衝突用モデルハンドル。Enemy は所有しない。
     */
    void resolveJumpAttackStageCollision(int stageCollisionModelHandle);

    /**
     * @brief 敵へダメージを与える。
     * @param[in] value ダメージ量。
     * @param[in] playerAttackStep プレイヤー攻撃段階。
     * @param[in] attackerPos 攻撃元のワールド座標。
     * @param[in] source 脅威値を加算する攻撃元。
     * @param[in] shieldBreaker true の場合はボスシールド破壊を優先する。
     * @return HP またはシールドのどちらへダメージが入ったか。
     */
    EnemyDamageResult damage(int value, int playerAttackStep, VECTOR attackerPos, ThreatSource source = ThreatSource::Player, bool shieldBreaker = false);

    /**
     * @brief カウンター攻撃に対するリアクションを開始する。
     * @param[in] attackerPos 攻撃元のワールド座標。
     * @return リアクションが発生した場合は true。
     */
    bool counterReact(VECTOR attackerPos);

    /**
     * @brief 指定した攻撃元の脅威値を加算する。
     * @param[in] source 脅威値の発生元。
     * @param[in] value 加算する値。
     */
    void addThreat(ThreatSource source, float value) {
        mThreat.addThreat(source, value);
    }

    /**
     * @brief 指定した攻撃元の現在の脅威値を取得する。
     */
    float getThreat(ThreatSource source) const {
        return mThreat.getThreat(source);
    }

    /**
     * @brief 最も脅威値が高い攻撃元を取得する。
     */
    ThreatSource getHighestThreatSource() const {
        return mThreat.getHighestSource();
    }

    /**
     * @brief 現在狙っているターゲット種別を取得する。
     */
    ThreatSource getCurrentTargetSource() const {
        return mCurrentTargetSource;
    }

    /**
     * @brief ボス扱いにするかどうかを設定する。
     * @param[in] isBoss true の場合はボス用の HP 表示やシールドを使う。
     */
    void setBoss(bool isBoss);

    /**
     * @brief ボスシールド機能が有効かどうかを取得する。
     */
    bool isBossShieldEnabled() const { return mBossShieldEnabled; }

    /**
     * @brief 現在シールド HP が残っているかどうかを取得する。
     */
    bool hasBossShield() const { return mBossShieldEnabled && mBossShieldHp > 0; }

    /**
     * @brief ボスシールド残量を 0.0 から 1.0 の割合で取得する。
     */
    float getBossShieldRate() const {
        return EnemyParam::BossShieldHp > 0 ? static_cast<float>(mBossShieldHp) / static_cast<float>(EnemyParam::BossShieldHp) : 0.0f;
    }

    /**
     * @brief HP 残量を 0.0 から 1.0 の割合で取得する。
     */
    float getHpRate() const {
        return static_cast<float>(mHp) / static_cast<float>(EnemyParam::Hp);
    }

    /**
     * @brief 死亡演出まで完了したかどうかを取得する。
     */
    bool isDead() const {
        if (mHp > 0 || mState != EnemyState::Dying)
        {
            return false;
        }

        return mDyingAnimPlaying
            ? mAnimator.isFinished()
            : mStateTimer >= mDyingDuration;
    }

    /**
     * @brief 攻撃判定を発生させる要求を取得し、同時に消費する。
     * @return このフレームで攻撃判定を出す必要がある場合は true。
     */
    bool consumeAttackHitRequest();

    /**
     * @brief 現在の攻撃判定を処理済みにする。
     */
    void markAttackHitDone() {
        mAttackHitDone = true;
        mAttackHitRequest = false;
    }

    /**
     * @brief 現在有効な攻撃半径を取得する。
     */
    float getAttackRadius() const {
        return mAttackHitRequest
            ? mCurrentAttackRadius
            : mAttackRadius;
    }

    /**
     * @brief 腕攻撃などで使う攻撃幅を取得する。
     */
    float getAttackWidth() const;

    /**
     * @brief 敵の前方向ベクトルを取得する。
     */
    VECTOR getForward() const;

    /**
     * @brief 攻撃判定の中心位置を取得する。
     */
    VECTOR getAttackHitPosition() const;

    /**
     * @brief 攻撃判定の半径を取得する。
     */
    float getAttackHitRadius() const;

    /**
     * @brief 現在の攻撃中心を取得する。
     */
    VECTOR getAttackCenter() const {
        return mAttackHitRequest && mState == EnemyState::JumpAttack
            ? mJumpAttackHitCenter
            : mPos;
    }

    /**
     * @brief 頭部のワールド座標を取得する。
     */
    VECTOR getHeadPosition() const;

    /**
     * @brief ロックオン用の注視点を取得する。
     */
    VECTOR getLockPointPosition() const;

    /**
     * @brief 被弾判定用の円数を取得する。
     */
    int getHitCircleCount() const;

    /**
     * @brief 指定インデックスの被弾円中心を取得する。
     */
    VECTOR getHitCircleCenter(int index) const;

    /**
     * @brief 指定インデックスの被弾円半径を取得する。
     */
    float getHitCircleRadius(int index) const;

    /**
     * @brief プレイヤーへ脅威となる攻撃状態かどうかを取得する。
     */
    bool isAttackThreatening() const;

    /**
     * @brief カウンター可能なタイミングかどうかを取得する。
     */
    bool isCounterTiming() const;

    /**
     * @brief 回避推奨タイミングかどうかを取得する。
     */
    bool isDodgeTiming() const;

    /**
     * @brief ジャンプ攻撃中かどうかを取得する。
     */
    bool isJumpAttacking() const {
        return mState == EnemyState::JumpAttack;
    }

    /**
     * @brief 突進攻撃中かどうかを取得する。
     */
    bool isRamAttacking() const {
        return mState == EnemyState::Ram;
    }

    /**
     * @brief 回転攻撃中かどうかを取得する。
     */
    bool isSpinAttacking() const {
        return mState == EnemyState::Attack360;
    }

    /**
     * @brief 通常攻撃中かどうかを取得する。
     */
    bool isNormalAttacking() const {
        return mState == EnemyState::Attack;
    }

    /**
     * @brief 突進準備中かどうかを取得する。
     */
    bool isCharging() const {
        return mState == EnemyState::Charging;
    }

    /**
     * @brief 突進開始位置を取得する。
     */
    VECTOR getRamAttackStart() const { return mRamStartPos; }

    /**
     * @brief 突進方向を取得する。
     */
    VECTOR getRamAttackForward() const { return mRamDirection; }

    /**
     * @brief 突進距離を取得する。
     */
    float getRamAttackLength() const { return mRamLength; }

    /**
     * @brief 回転攻撃の半径を取得する。
     */
    float getSpinAttackRadius() const { return EnemyParam::SpinAttackRadius; }

    /**
     * @brief 腕攻撃が指定ターゲットに当たっているか判定する。
     */
    bool isArmAttackHitting(VECTOR targetPos, float targetRadius, float targetHeight) const;

    /**
     * @brief 魔法ビームが敵に当たっているか判定する。
     */
    bool isMagicBeamHitting(VECTOR startPos, VECTOR forward, float range, float radius) const;

    /**
     * @brief 魔法ビームのヒット位置を取得する。
     */
    VECTOR getMagicBeamHitPosition(VECTOR startPos, VECTOR forward, float range) const;

    /**
     * @brief 腕攻撃のデバッグ当たり判定を描画する。
     */
    void drawAttackArmHitBoxes() const;

    /**
     * @brief 左手攻撃軌跡のデバッグ点を描画する。
     */
    void drawLeftHandTrailDebugPoints() const;

    /**
     * @brief 突進攻撃の予告マーカーを描画する。
     */
    void drawRamMarker() const;

    /**
     * @brief 敵の移動処理を有効化または無効化する。
     */
    void setMovementEnabled(bool enabled) { mMovementEnabled = enabled; }

    /**
     * @brief デバッグ用にジャンプ攻撃を即時開始する。
     */
    void debugStartJumpAttack(VECTOR targetPos) { startJumpAttack(targetPos); }

    /**
     * @brief 地面へのスナップ補正が可能かどうかを取得する。
     */
    bool canSnapToGround() const {
        return mState != EnemyState::JumpAttack
            || mJumpAttackLanded;
    }

private:

    /**
     * @brief 待機状態を更新し、ターゲット距離に応じて次状態へ遷移する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     * @param[in] distanceToPlayer ターゲットまでの距離。DxLib ワールド単位。
     */
    void updateIdle(float deltaTime, float distanceToPlayer);

    /**
     * @brief ターゲットへ接近する追跡状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     * @param[in] toPlayer ターゲット方向ベクトル。
     * @param[in] distanceToPlayer ターゲットまでの距離。DxLib ワールド単位。
     */
    void updateChase(float deltaTime, VECTOR toPlayer, float distanceToPlayer);

    /**
     * @brief 通常攻撃状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateAttack(float deltaTime);

    /**
     * @brief 突進準備状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     * @param[in] toTarget ターゲット方向ベクトル。
     * @param[in] distanceToTarget ターゲットまでの距離。DxLib ワールド単位。
     */
    void updateCharging(float deltaTime, VECTOR toTarget, float distanceToTarget);

    /**
     * @brief 突進攻撃状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateRam(float deltaTime);

    /**
     * @brief 回転攻撃状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateAttack360(float deltaTime);

    /**
     * @brief ジャンプ攻撃状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateJumpAttack(float deltaTime);

    /**
     * @brief 被弾リアクション状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     * @param[in] playerPos 攻撃元の基準にするプレイヤー座標。
     */
    void updateHit(float deltaTime, VECTOR playerPos);

    /**
     * @brief 死亡演出状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateDying(float deltaTime);

    /**
     * @brief ジャンプ攻撃の着地点予告を描画する。
     */
    void drawJumpAttackMarker() const;

    /**
     * @brief ジャンプ攻撃の判定中心を取得する。
     * @return 攻撃判定中心のワールド座標。
     */
    VECTOR getJumpAttackHitCenter() const;

    /**
     * @brief ジャンプ攻撃中の本体接地位置を取得する。
     * @return 接地位置のワールド座標。
     */
    VECTOR getJumpAttackBodyGroundPosition() const;

    /**
     * @brief ジャンプ攻撃予告マーカーの中心を取得する。
     * @return マーカー中心のワールド座標。
     */
    VECTOR getJumpAttackMarkerCenter() const;

    /**
     * @brief 通常攻撃を開始する。
     */
    void startAttack();

    /**
     * @brief 突進準備を開始する。
     * @param[in] toTarget ターゲット方向ベクトル。
     * @param[in] distanceToTarget ターゲットまでの距離。DxLib ワールド単位。
     */
    void startCharging(VECTOR toTarget, float distanceToTarget);

    /**
     * @brief 突進攻撃を開始する。
     */
    void startRam();

    /**
     * @brief 回転攻撃を開始する。
     */
    void startAttack360();

    /**
     * @brief ジャンプ攻撃を開始する。
     * @param[in] playerPos 狙うプレイヤー座標。
     */
    void startJumpAttack(VECTOR playerPos);

    /**
     * @brief 現在条件でジャンプ攻撃を開始すべきか判定する。
     * @param[in] playerPos プレイヤー座標。
     * @return 開始すべきなら true。
     */
    bool shouldStartJumpAttack(VECTOR playerPos) const;

    /**
     * @brief 被弾時にリアクションへ遷移すべきか判定する。
     * @param[in] playerAttackStep プレイヤー攻撃段階。
     * @return リアクションすべきなら true。
     */
    bool shouldReactToHit(int playerAttackStep) const;

    /**
     * @brief 攻撃軌跡の蓄積点をクリアする。
     */
    void clearAttackTrail();

    /**
     * @brief 攻撃後硬直時間をランダム範囲から取得する。
     * @return 硬直時間（秒）。
     */
    float getRandomAttackRecovery() const;

    /**
     * @brief 腕攻撃用カプセル近似ボックスを取得する。
     * @param[in] index 取得するボックス番号。
     * @param[out] start ボックス開始点。
     * @param[out] end ボックス終了点。
     * @param[out] halfWidth 幅の半分。
     * @param[out] halfHeight 高さの半分。
     * @return 取得できた場合は true。
     */
    bool getAttackArmBox(int index, VECTOR* start, VECTOR* end, float* halfWidth, float* halfHeight) const;

    /**
     * @brief 攻撃軌跡の点列とフェード時間を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateAttackTrail(float deltaTime);

    /**
     * @brief 攻撃軌跡を描画する。
     */
    void drawAttackTrail() const;

    /**
     * @brief 攻撃軌跡の内側基準点を取得する。
     * @return ワールド座標。
     */
    VECTOR getAttackTrailInnerPosition() const;

    /**
     * @brief 攻撃軌跡の外側基準点を取得する。
     * @return ワールド座標。
     */
    VECTOR getAttackTrailOuterPosition() const;

    /**
     * @brief 左手軌跡の先端位置を取得する。
     * @return ワールド座標。
     */
    VECTOR getLeftHandTrailTipPosition() const;

    /**
     * @brief 被弾状態を開始する。
     * @param[in] playerAttackStep プレイヤー攻撃段階。
     * @param[in] attackerPos 攻撃元のワールド座標。
     */
    void startHit(int playerAttackStep, VECTOR attackerPos);

    /**
     * @brief 死亡状態を開始する。
     */
    void startDying();

    /**
     * @brief ボスシールドの回復・破壊状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateBossShield(float deltaTime);

    /**
     * @brief 脅威値と有効状態から狙うターゲット座標を選ぶ。
     * @param[in] targets ターゲット候補配列。Enemy は所有しない。
     * @param[in] targetCount ターゲット候補数。
     * @param[in] fallbackPos 有効候補がない場合に使う座標。
     * @return 選択したターゲット座標。
     */
    VECTOR chooseTargetPosition(const CombatTarget* targets, int targetCount, VECTOR fallbackPos);

    /**
     * @brief ターゲット候補の脅威値とターゲット切り替え状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     * @param[in] targets ターゲット候補配列。Enemy は所有しない。
     * @param[in] targetCount ターゲット候補数。
     */
    void updateTargetThreat(float deltaTime, const CombatTarget* targets, int targetCount);

    /**
     * @brief ターゲット候補の優先度スコアを計算する。
     * @param[in] target 評価対象。
     * @return 優先度スコア。
     */
    float getTargetScore(const CombatTarget& target) const;

    /**
     * @brief 指定した攻撃元に対応するターゲット候補を探す。
     * @param[in] targets ターゲット候補配列。Enemy は所有しない。
     * @param[in] targetCount ターゲット候補数。
     * @param[in] source 探す攻撃元。
     * @return 見つかった候補への非所有ポインタ。見つからない場合は nullptr。
     */
    const CombatTarget* findTarget(const CombatTarget* targets, int targetCount, ThreatSource source) const;

private:


    float mDetectRadius;  ///< ターゲット検出半径。DxLib ワールド単位。
    float mStateTimer;    ///< 現在状態に入ってからの経過時間（秒）。
    float mHitDuration;   ///< 被弾リアクション継続時間（秒）。
    float mDyingDuration; ///< 死亡演出の基準継続時間（秒）。


    float mKnockbackSpeed; ///< ノックバック速度。DxLib ワールド単位/秒。
    float mKnockbackDuration; ///< ノックバック継続時間（秒）。
    float mKnockbackTotalDistance; ///< ノックバック総距離。DxLib ワールド単位。
    float mKnockbackAppliedDistance; ///< 適用済みノックバック距離。DxLib ワールド単位。
    float mKnockbackEndCompensationDistance; ///< ノックバック終了時の補正距離。DxLib ワールド単位。


    float mAttackCooldown; ///< 通常攻撃の基本クールダウン時間（秒）。
    float mAttackCooldownTimer; ///< 通常攻撃の残りクールダウン時間（秒）。
    float mRamCooldownTimer; ///< 突進攻撃の残りクールダウン時間（秒）。
    float mSpinAttackCooldownTimer; ///< 回転攻撃の残りクールダウン時間（秒）。
    float mCloseContactTimer; ///< 近距離接触が続いている時間（秒）。
    float mCurrentAttackRadius; ///< 現在攻撃で使う攻撃半径。DxLib ワールド単位。


    float mJumpAttackRadius; ///< ジャンプ攻撃の判定半径。DxLib ワールド単位。
    float mJumpAttackEffectiveDistance; ///< ジャンプ攻撃を選びやすい距離。DxLib ワールド単位。
    float mJumpAttackCooldown; ///< ジャンプ攻撃の基本クールダウン時間（秒）。
    float mJumpAttackCooldownTimer; ///< ジャンプ攻撃の残りクールダウン時間（秒）。

    float mJumpAttackStartupDuration; ///< ジャンプ攻撃開始前の予備動作時間（秒）。
    float mJumpAttackRecoveryDuration; ///< ジャンプ攻撃着地後の硬直時間（秒）。

    float mJumpAttackLandingOffsetDistance; ///< 着地位置補正距離。DxLib ワールド単位。

    float mJumpAttackHeight; ///< ジャンプ攻撃の基本高さ。DxLib ワールド単位。
    float mJumpAttackMinHeight; ///< ジャンプ攻撃高さの下限。DxLib ワールド単位。
    float mJumpAttackMaxHeight; ///< ジャンプ攻撃高さの上限。DxLib ワールド単位。

    float mJumpAttackLandTiming; ///< ジャンプ攻撃中に着地扱いへ移る正規化タイミング。

    float mJumpAttackDuration; ///< 現在ジャンプ攻撃の継続時間（秒）。
    float mJumpAttackMinDuration; ///< ジャンプ攻撃時間の下限（秒）。
    float mJumpAttackMaxDuration; ///< ジャンプ攻撃時間の上限（秒）。

    float mJumpAttackGroundY; ///< ジャンプ攻撃着地基準の地面 Y 座標。


    int mIdleAnimSourceHandle; ///< 待機アニメーション元モデルハンドル。
    int mWalkAnimSourceHandle; ///< 歩行アニメーション元モデルハンドル。
    int mAttackAnimSourceHandle; ///< 通常攻撃アニメーション元モデルハンドル。
    int mAttack2AnimSourceHandle; ///< 2 段目攻撃アニメーション元モデルハンドル。
    int mAttack3AnimSourceHandle; ///< 3 段目攻撃アニメーション元モデルハンドル。
    int mAttack360AnimSourceHandle; ///< 回転攻撃アニメーション元モデルハンドル。
    int mChargingAnimSourceHandle; ///< 突進準備アニメーション元モデルハンドル。
    int mRamAnimSourceHandle; ///< 突進攻撃アニメーション元モデルハンドル。
    int mJumpAttackAnimSourceHandle; ///< ジャンプ攻撃アニメーション元モデルハンドル。
    int mHit1AnimSourceHandle; ///< 被弾 1 アニメーション元モデルハンドル。
    int mHit2AnimSourceHandle; ///< 被弾 2 アニメーション元モデルハンドル。
    int mDyingAnimSourceHandle; ///< 死亡アニメーション元モデルハンドル。


    int mIdleAnimIndex; ///< 待機アニメーション番号。
    int mWalkAnimIndex; ///< 歩行アニメーション番号。
    int mAttackAnimIndex; ///< 通常攻撃アニメーション番号。
    int mAttack2AnimIndex; ///< 2 段目攻撃アニメーション番号。
    int mAttack3AnimIndex; ///< 3 段目攻撃アニメーション番号。
    int mAttack360AnimIndex; ///< 回転攻撃アニメーション番号。
    int mChargingAnimIndex; ///< 突進準備アニメーション番号。
    int mRamAnimIndex; ///< 突進攻撃アニメーション番号。
    int mJumpAttackAnimIndex; ///< ジャンプ攻撃アニメーション番号。
    int mHit1AnimIndex; ///< 被弾 1 アニメーション番号。
    int mHit2AnimIndex; ///< 被弾 2 アニメーション番号。
    int mDyingAnimIndex; ///< 死亡アニメーション番号。
    int mJumpAttackToeFrameIndex; ///< ジャンプ攻撃の足先位置取得に使うフレーム番号。
    int mJumpAttackBodyFrameIndex; ///< ジャンプ攻撃の本体位置取得に使うフレーム番号。
    int mHeadFrameIndex; ///< 頭部位置取得に使うフレーム番号。
    int mSpineFrameIndex; ///< 背骨位置取得に使うフレーム番号。
    int mLeftUpLegFrameIndex; ///< 左上脚位置取得に使うフレーム番号。
    int mRightUpLegFrameIndex; ///< 右上脚位置取得に使うフレーム番号。
    int mLeftLegFrameIndex; ///< 左脚位置取得に使うフレーム番号。
    int mRightLegFrameIndex; ///< 右脚位置取得に使うフレーム番号。
    int mLeftForeArmFrameIndex; ///< 左前腕位置取得に使うフレーム番号。
    int mLeftHandFrameIndex; ///< 左手位置取得に使うフレーム番号。
    int mRightForeArmFrameIndex; ///< 右前腕位置取得に使うフレーム番号。
    int mRightHandFrameIndex; ///< 右手位置取得に使うフレーム番号。
    int mRightHandIndex3FrameIndex; ///< 右手指先 3 の位置取得に使うフレーム番号。
    int mRightHandIndex4FrameIndex; ///< 右手指先 4 の位置取得に使うフレーム番号。


    EnemyState mState; ///< 現在の行動状態。

    ThreatComponent mThreat; ///< 攻撃元ごとの脅威値。
    ThreatSource mCurrentTargetSource; ///< 現在狙っているターゲット種別。
    float mTargetSwitchCooldownTimer; ///< ターゲット切り替えまでの残り待機時間（秒）。
    bool mIsBoss; ///< ボス扱いの UI とシールドを使うかどうか。
    bool mMovementEnabled; ///< AI 移動処理が有効かどうか。
    bool mBossShieldEnabled; ///< ボスシールド機能が有効かどうか。
    int mBossShieldHp; ///< 現在のボスシールド HP。
    float mBossShieldBreakTimer; ///< シールド破壊後の経過または復帰管理タイマー（秒）。


    bool mAttackHitRequest; ///< シーン側へ攻撃判定発生を要求しているかどうか。
    bool mAttackHitDone; ///< 現在攻撃で攻撃判定処理済みかどうか。
    int mAttackStep; ///< 現在攻撃段数。
    int mNextAttackStep; ///< 次に再生する攻撃段数。
    static const int AttackTrailPointMax = 18; ///< 攻撃軌跡として保持する最大点数。
    VECTOR mAttackTrailInnerPoints[AttackTrailPointMax]; ///< 攻撃軌跡内側点列。
    VECTOR mAttackTrailOuterPoints[AttackTrailPointMax]; ///< 攻撃軌跡外側点列。
    int mAttackTrailPointCount; ///< 現在保持している攻撃軌跡点数。
    float mAttackTrailFadeTimer; ///< 攻撃軌跡フェード用タイマー（秒）。

    bool mJumpAttackHitDone; ///< ジャンプ攻撃のヒット処理が完了したかどうか。
    bool mJumpAttackStarted; ///< ジャンプ攻撃の移動が開始済みかどうか。
    bool mJumpAttackLanded; ///< ジャンプ攻撃が着地済みかどうか。

    bool mHit2ReactionActive; ///< 2 種類目の被弾リアクションが有効かどうか。
    bool mHitAnimPlaying; ///< 被弾アニメーション再生中かどうか。
    float mHitStopTimer; ///< ヒットストップ残り時間（秒）。
    bool mDyingAnimPlaying; ///< 死亡アニメーション再生中かどうか。

    bool mKnockbackActive; ///< ノックバック移動が有効かどうか。
    bool mIsIntroLanding; ///< 登場時の着地演出中かどうか。

    VECTOR mRamDirection; ///< 突進方向のワールド空間ベクトル。
    VECTOR mRamStartPos; ///< 突進開始位置のワールド座標。
    float mRamLength; ///< 突進距離。DxLib ワールド単位。
    float mRamFootstepSeTimer; ///< 突進足音 SE の再生間隔タイマー（秒）。


    VECTOR mKnockbackDirection; ///< ノックバック方向のワールド空間ベクトル。

    VECTOR mJumpAttackStartPos; ///< ジャンプ攻撃開始位置のワールド座標。
    VECTOR mJumpAttackTargetPos; ///< ジャンプ攻撃目標位置のワールド座標。
    VECTOR mJumpAttackHitCenter; ///< ジャンプ攻撃判定中心のワールド座標。
    VECTOR mJumpAttackMarkerCenter; ///< ジャンプ攻撃予告マーカー中心のワールド座標。
    VECTOR mJumpAttackLandingBodyPos; ///< ジャンプ攻撃着地時の本体位置。
    int mJumpAttackStageCollisionModelHandle; ///< ジャンプ攻撃中の地面補正に使うステージ衝突モデル。非所有。
};

#endif
