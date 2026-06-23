#ifndef _MAGIC_PROJECTILE_ENEMY_H_
#define _MAGIC_PROJECTILE_ENEMY_H_


/**
 * @file
 * @brief 魔法弾を使うボス敵を定義する。
 */


#include <memory>
#include <vector>

#include "animator.h"
#include "player.h"

/**
 * @brief ボスが発射する魔法弾の実行時状態。
 *
 * MagicProjectileEnemy が所有し、毎フレーム位置、誘導、寿命、軌跡を更新する。
 * 座標と速度はゲーム内のワールド空間で扱う。
 */
struct MagicProjectile
{
    VECTOR pos; ///< 現在位置。ワールド空間。
    VECTOR velocity; ///< 現在速度。ワールド空間での移動量。
    float life; ///< 残り寿命または経過管理用の時間。単位は seconds。
    float rotation; ///< 描画時の回転値。単位は実装側の描画処理に従う。
    bool introHoming; ///< イントロ演出中の誘導弾として扱うかどうか。
    float homingTimer; ///< 誘導処理の残り時間または経過時間。単位は seconds。
    bool waitingLaunch; ///< 発射待機状態かどうか。
    float launchTimer; ///< 発射までの待機時間。単位は seconds。
    int targetIndex; ///< 誘導対象の識別番号。実際の対応は更新処理側で決める。
    float arcTimer; ///< 弧を描く移動の時間管理値。単位は seconds。
    std::vector<VECTOR> trail; ///< 軌跡描画用に保持する過去位置。ワールド空間。
};

/**
 * @brief 魔法弾の着弾エフェクト状態。
 *
 * MagicProjectileEnemy が所有し、一定時間だけ描画される。
 */
struct MagicImpact
{
    VECTOR pos; ///< 着弾位置。ワールド空間。
    float timer; ///< エフェクト表示の時間管理値。単位は seconds。
};

/**
 * @brief 魔法弾攻撃を行うボス敵。
 *
 * VampireScene 側のボス戦で使われる敵キャラクターで、モデル、アニメーション、
 * 魔法弾、着弾エフェクト、HP フェーズ遷移をまとめて管理する。
 * Player ポインタは参照先を所有せず、プレイヤー側のライフサイクルに依存する。
 * このクラスの更新と描画は通常のゲームループ上で呼ばれる想定。
 */
class MagicProjectileEnemy
{
public:
    /**
     * @brief 指定位置にボス敵を生成する。
     * @param[in] pos 初期位置。ワールド空間。
     */
    explicit MagicProjectileEnemy(VECTOR pos);

    /**
     * @brief ボス本体、攻撃シーケンス、魔法弾を更新する。
     * @param[in] deltaTime 前回更新からの経過時間。単位は seconds。
     * @param[in] player 攻撃対象候補のプレイヤー。所有しない。
     * @param[in] companion 攻撃対象候補の相棒。所有しない。
     */
    void update(float deltaTime, Player* player, Player* companion);

    /**
     * @brief アニメーション再生を更新する。
     * @param[in] deltaTime 前回更新からの経過時間。単位は seconds。
     */
    void updateAnimation(float deltaTime);

    /**
     * @brief イントロ演出用の魔法弾を更新する。
     * @param[in] deltaTime 前回更新からの経過時間。単位は seconds。
     */
    void updateIntroProjectiles(float deltaTime);

    /**
     * @brief ボス本体、魔法弾、エフェクトを描画する。
     */
    void draw() const;

    /**
     * @brief 攻撃判定確認用のデバッグ表示を描画する。
     */
    void drawDebugHitCollision() const;

    /**
     * @brief HP が尽きて死亡状態かどうかを返す。
     * @return 死亡状態なら true。
     */
    bool isDead() const;

    /**
     * @brief 死亡アニメーションが最後まで再生されたかどうかを返す。
     * @return 死亡アニメーションが完了していれば true。
     */
    bool isDeadAnimationFinished() const;

    /**
     * @brief ボスの現在位置を取得する。
     * @return 現在位置。ワールド空間。
     */
    VECTOR getPosition() const;

    /**
     * @brief 頭部付近の位置を取得する。
     * @return 頭部位置。ワールド空間。
     */
    VECTOR getHeadPosition() const;

    /**
     * @brief ロックオンや照準に使う位置を取得する。
     * @return ロックポイント位置。ワールド空間。
     */
    VECTOR getLockPointPosition() const;

    /**
     * @brief 現在 HP を取得する。
     * @return 現在 HP。
     */
    int getHp() const;

    /**
     * @brief 最大 HP を取得する。
     * @return 最大 HP。
     */
    int getMaxHp() const;

    /**
     * @brief 現在管理している魔法弾数を取得する。
     * @return 魔法弾数。
     */
    int getProjectileCount() const;

    /**
     * @brief 指定位置から最も近い移動中の魔法弾までの距離を取得する。
     * @param[in] pos 基準位置。ワールド空間。
     * @return 最も近い魔法弾までの距離。
     */
    float getNearestMovingProjectileDistance(VECTOR pos) const;

    /**
     * @brief 指定位置に最も近い移動中の魔法弾情報を取得する。
     * @param[in] pos 基準位置。ワールド空間。
     * @param[out] projectilePos 見つかった魔法弾の位置を受け取るポインタ。
     * @param[out] velocity 見つかった魔法弾の速度を受け取るポインタ。
     * @return 情報を取得できた場合は true。
     */
    bool getNearestMovingProjectileInfo(VECTOR pos, VECTOR* projectilePos, VECTOR* velocity) const;

    /**
     * @brief ボス本体の当たり判定半径を取得する。
     * @return 当たり判定半径。
     */
    float getHitRadius() const;

    /**
     * @brief ボス本体の当たり判定高さを取得する。
     * @return 当たり判定高さ。
     */
    float getHitHeight() const;

    /**
     * @brief 攻撃カプセル内にボスが含まれるか判定する。
     * @param[in] attackerPos 攻撃者の位置。ワールド空間。
     * @param[in] forward 攻撃方向。
     * @param[in] attackRadius 攻撃判定の半径。
     * @return 攻撃範囲内なら true。
     */
    bool isInsideAttackCapsule(VECTOR attackerPos, VECTOR forward, float attackRadius) const;

    /**
     * @brief ボスにダメージを与える。
     * @param[in] value 減少させる HP 値。
     */
    void damage(int value);

    /**
     * @brief デバッグ用に HP を 1 にする。
     */
    void debugSetHpToOne();

    /**
     * @brief 待機アニメーションを再生する。
     */
    void playIdle();

    /**
     * @brief 咆哮アニメーションを開始する。
     */
    void startRoaring();

    /**
     * @brief 咆哮アニメーションが完了したかどうかを返す。
     * @return 咆哮が完了していれば true。
     */
    bool isRoaringFinished() const;

    /**
     * @brief イントロ演出用の魔法弾数を設定する。
     * @param[in] count 生成または保持する魔法弾数。
     */
    void setIntroMagicBallCount(int count);

    /**
     * @brief 待機中のイントロ魔法弾を発射状態へ移行する。
     */
    void releaseIntroMagicBalls();

private:
    /**
     * @brief 攻撃対象にするプレイヤーを選ぶ。
     * @param[in] player 候補となるプレイヤー。所有しない。
     * @param[in] companion 候補となる相棒。所有しない。
     * @return 選択された対象。対象がない場合の扱いは実装に従う。
     */
    Player* chooseTarget(Player* player, Player* companion) const;

    /**
     * @brief 誘導弾を順番に撃つ攻撃シーケンスを開始する。
     */
    void startHomingShotSequence();

    /**
     * @brief 発射待機状態の誘導弾を生成する。
     * @param[in] targetIndex 誘導対象の識別番号。
     */
    void spawnWaitingHomingShot(int targetIndex);

    /**
     * @brief 四方向へ魔法弾を放つ攻撃を実行する。
     */
    void shootFourWayBurst();

    /**
     * @brief 対象位置を挟み込む魔法弾攻撃を実行する。
     * @param[in] targetPos 攻撃対象位置。ワールド空間。
     */
    void shootPincer(VECTOR targetPos);

    /**
     * @brief 魔法弾を生成して管理リストに追加する。
     * @param[in] pos 初期位置。ワールド空間。
     * @param[in] velocity 初期速度。ワールド空間。
     */
    void spawnProjectile(VECTOR pos, VECTOR velocity);

    /**
     * @brief 怯み中の状態と派生攻撃を更新する。
     * @param[in] deltaTime 前回更新からの経過時間。単位は seconds。
     * @param[in] player 攻撃対象候補のプレイヤー。所有しない。
     * @param[in] companion 攻撃対象候補の相棒。所有しない。
     */
    void updateStagger(float deltaTime, Player* player, Player* companion);

    /**
     * @brief 中央テレポート予告状態を更新する。
     * @param[in] deltaTime 前回更新からの経過時間。単位は seconds。
     * @param[in] player 攻撃対象候補のプレイヤー。所有しない。
     * @param[in] companion 攻撃対象候補の相棒。所有しない。
     */
    void updateCenterTeleportMarker(float deltaTime, Player* player, Player* companion);

    /**
     * @brief 保留中のテレポートを確定し、必要な攻撃へつなげる。
     * @param[in] player 攻撃対象候補のプレイヤー。所有しない。
     * @param[in] companion 攻撃対象候補の相棒。所有しない。
     */
    void finishPendingTeleport(Player* player, Player* companion);

    /**
     * @brief 被弾による怯みとテレポート後の攻撃予定を開始する。
     * @param[in] teleportPos テレポート先。ワールド空間。
     * @param[in] fourWayBurst 四方向攻撃を予約するかどうか。
     * @param[in] pincer 挟み込み攻撃を予約するかどうか。
     */
    void startHitStagger(VECTOR teleportPos, bool fourWayBurst, bool pincer);

    /**
     * @brief ボスを指定位置へ移動する。
     * @param[in] pos 移動先。ワールド空間。
     */
    void teleportTo(VECTOR pos);

    /**
     * @brief 次に使う側面テレポート位置を取得する。
     * @return テレポート先。ワールド空間。
     */
    VECTOR nextSideTeleportPosition();

    /**
     * @brief HP 変化に応じたフェーズ到達処理を確認する。
     * @param[in] oldHp ダメージ適用前の HP。
     */
    void checkPhaseThreshold(int oldHp);

    /**
     * @brief すべての魔法弾を移動、寿命、衝突の観点で更新する。
     * @param[in] deltaTime 前回更新からの経過時間。単位は seconds。
     * @param[in] player 衝突対象候補のプレイヤー。所有しない。
     * @param[in] companion 衝突対象候補の相棒。所有しない。
     */
    void updateProjectiles(float deltaTime, Player* player, Player* companion);

    /**
     * @brief 魔法弾とプレイヤーの衝突を処理する。
     * @param[in,out] projectile 判定対象の魔法弾。
     * @param[in] player 衝突対象のプレイヤー。所有しない。
     * @return 衝突した場合は true。
     */
    bool hitPlayer(MagicProjectile& projectile, Player* player);

    int mModelHandle; ///< ボスモデルのリソースハンドル。
    int mIdleAnimSourceHandle; ///< 待機アニメーションの元ハンドル。
    int mRoaringAnimSourceHandle; ///< 咆哮アニメーションの元ハンドル。
    int mHitAnimSourceHandle; ///< 被弾アニメーションの元ハンドル。
    int mDyingAnimSourceHandle; ///< 死亡アニメーションの元ハンドル。
    int mSpineFrameIndex; ///< 姿勢調整に使う背骨フレームのインデックス。
    Animator mAnimator; ///< ボス本体のアニメーション制御。
    VECTOR mPos; ///< ボスの現在位置。ワールド空間。
    int mHp; ///< 現在 HP。
    bool mDyingStarted; ///< 死亡アニメーション開始済みかどうか。
    std::vector<MagicProjectile> mProjectiles; ///< 現在有効な魔法弾。要素はこのクラスが所有する。
    std::vector<MagicImpact> mImpacts; ///< 現在有効な着弾エフェクト。要素はこのクラスが所有する。
    int mProjectileGraph; ///< 魔法弾描画用グラフィックハンドル。
    float mShotTimer; ///< 通常射撃までの時間管理値。単位は seconds。
    float mShotSequenceTimer; ///< 連続射撃シーケンスの時間管理値。単位は seconds。
    int mShotSequenceIndex; ///< 連続射撃シーケンス内の現在インデックス。
    float mPincerTimer; ///< 挟み込み攻撃までの時間管理値。単位は seconds。
    float mBurstTimer; ///< 四方向攻撃までの時間管理値。単位は seconds。
    float mStaggerTimer; ///< 怯み状態の時間管理値。単位は seconds。
    float mCenterTeleportMarkerTimer; ///< 中央テレポート予告の残り時間管理値。単位は seconds。
    float mCenterTeleportMarkerElapsed; ///< 中央テレポート予告の経過時間。単位は seconds。
    bool mHasPendingTeleport; ///< 実行待ちのテレポートがあるかどうか。
    bool mPendingFourWayBurst; ///< テレポート後に四方向攻撃を行う予定があるかどうか。
    bool mPendingPincer; ///< テレポート後に挟み込み攻撃を行う予定があるかどうか。
    bool mWaitingCenterTeleport; ///< 中央テレポート予告中かどうか。
    VECTOR mPendingTeleportPos; ///< 保留中のテレポート先。ワールド空間。
    int mTeleportIndex; ///< 側面テレポート位置の選択インデックス。
    bool mPhase80Triggered; ///< HP 80% フェーズ処理済みかどうか。
    bool mPhase60Triggered; ///< HP 60% フェーズ処理済みかどうか。
    bool mPhase40Triggered; ///< HP 40% フェーズ処理済みかどうか。
    bool mPhase20Triggered; ///< HP 20% フェーズ処理済みかどうか。
};

#endif
