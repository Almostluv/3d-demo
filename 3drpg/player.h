#ifndef _PLAYER_H_
#define _PLAYER_H_


/**
 * @file
 * @brief 操作キャラクターと AI 相棒として使う Player を定義する。
 */


#include "player_state.h"
#include "player_input.h"
#include "char_base.h"
#include "player_character_type.h"

class Camera;

/**
 * @brief プレイヤーキャラクターを表すクラス。
 *
 * Knight と Wizard の操作、アニメーション、攻撃、回避、ガード、
 * 被ダメージ、スタミナ、魔法用座標を管理する。AI 相棒としても利用できる。
 * Camera は非所有ポインタとして参照し、Player より長く有効である必要がある。
 */
class Player : public CharBase
{
public:

    /**
     * @brief カメラとキャラクター種別を指定して生成する。
     * @param[in] camera 移動方向やロックオン方向の基準にするカメラ。Player は所有しない。
     * @param[in] characterType 使用するキャラクター種別。
     */
    Player(
        Camera* camera,
        PlayerCharacterType characterType = PlayerCharacterType::Knight
    );

    /**
     * @brief モデル、アニメーション、ループ SE などを解放する。
     */
    virtual ~Player();


    /**
     * @brief 入力、状態遷移、アニメーション、各種タイマーを更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) override;

    /**
     * @brief 現在のモデル姿勢でプレイヤーを描画する。
     */
    virtual void draw() override;

    /**
     * @brief プレイヤー状態を確認するためのデバッグ HUD を描画する。
     */
    void drawDebugHud() const;


    /**
     * @brief このフレームで使用する入力情報を設定する。
     * @param[in] input 移動方向とアクション入力をまとめた構造体。
     */
    void setInput(const PlayerInput& input);

    /**
     * @brief ロックオン状態を設定する。
     * @param[in] isLockOn true の場合はロックオン向けの向き制御を使う。
     */
    void setLockOn(bool isLockOn);

    /**
     * @brief AI 操作に切り替えるかどうかを設定する。
     * @param[in] isAiControlled true の場合は外部から与えた AI 入力で動く。
     */
    void setAiControlled(bool isAiControlled);

    /**
     * @brief 内部座標と回転をモデルへ即座に反映する。
     */
    void syncModelTransform();


    /**
     * @brief 攻撃状態かどうかを取得する。
     * @return 攻撃中なら true。
     */
    bool isAttacking() const {
        return mState == PlayerState::Attack;
    }

    /**
     * @brief 現在の通常攻撃段階を取得する。
     * @return 攻撃段階。
     */
    int getAttackStep() const {
        return mAttackStep;
    }

    /**
     * @brief 攻撃判定を発生させる要求を取得し、同時に消費する。
     * @return このフレームで攻撃判定を出す必要がある場合は true。
     */
    bool consumeAttackHitRequest();

    /**
     * @brief ヒットストップを開始する。
     * @param[in] duration 停止させる時間（秒）。
     */
    void startHitStop(float duration);

    /**
     * @brief 範囲強化魔法の発動中かどうかを取得する。
     * @return 範囲強化の効果中なら true。
     */
    bool isAreaPowerUp() const;

    /**
     * @brief 強化効果が有効かどうかを取得する。
     * @return 強化効果中なら true。
     */
    bool isPowerUpActive() const;

    /**
     * @brief Wizard かどうかを取得する。
     * @return Wizard の場合は true。
     */
    bool isWizard() const {
        return mCharacterType == PlayerCharacterType::Wizard;
    }

    /**
     * @brief 範囲強化魔法の効果半径を取得する。
     * @return 効果半径。
     */
    float getAreaPowerUpRadius() const;

    /**
     * @brief 現在のスタミナを取得する。
     * @return スタミナ値。
     */
    float getStamina() const {
        return mStamina;
    }

    /**
     * @brief 魔法発射位置に使う手のワールド座標を取得する。
     */
    VECTOR getMagicHandPosition() const;

    /**
     * @brief 剣の根元付近のワールド座標を取得する。
     */
    VECTOR getSwordJointPosition() const;

    /**
     * @brief 剣先のワールド座標を取得する。
     */
    VECTOR getSwordTipPosition() const;

    /**
     * @brief 盾のワールド座標を取得する。
     */
    VECTOR getShieldJointPosition() const;

    /**
     * @brief カウンターエフェクトの発生位置を取得する。
     */
    VECTOR getCounterEffectPosition() const;

    /**
     * @brief 範囲強化効果を適用する。
     */
    void applyAreaPowerUp();

    /**
     * @brief 攻撃方向を指定してダメージを与える。
     * @param[in] value ダメージ量。
     * @param[in] attackerPos 攻撃元のワールド座標。
     */
    void damage(int value, VECTOR attackerPos);

    /**
     * @brief ノックバック方向と距離を指定してダメージを与える。
     * @param[in] value ダメージ量。
     * @param[in] attackerPos 攻撃元のワールド座標。
     * @param[in] knockbackDirection ノックバック方向。
     * @param[in] knockbackDistance ノックバック距離。DxLib ワールド単位。
     */
    void damage(int value, VECTOR attackerPos, VECTOR knockbackDirection, float knockbackDistance);

    /**
     * @brief ガード衝撃エフェクト要求を取得し、同時に消費する。
     * @return エフェクトを出す必要がある場合は true。
     */
    bool consumeGuardImpactEffectRequest();

    /**
     * @brief シーン遷移時に保存済み HP を復元する。
     * @param[in] hp 復元する HP。
     */
    void restoreHpForScene(int hp);

    /**
     * @brief 即座に死亡状態へ移行する。
     */
    void forceDead();

    /**
     * @brief 死亡状態かどうかを取得する。
     * @return 死亡状態なら true。
     */
    bool isDead() const {
        return mState == PlayerState::Dead;
    }

    /**
     * @brief 死亡アニメーションが終了したかどうかを取得する。
     * @return 死亡演出が完了していれば true。
     */
    bool isDeadAnimationFinished() const;

    /**
     * @brief 回避状態かどうかを取得する。
     * @return 回避中なら true。
     */
    bool isDodging() const {
        return mState == PlayerState::Dodge;
    }

    /**
     * @brief カウンターの攻撃判定が有効なタイミングかどうかを取得する。
     * @return 有効なら true。
     */
    bool isCounterActive() const;

    /**
     * @brief 地面へのスナップ補正が可能かどうかを取得する。
     * @return ジャンプ中でなければ true。
     */
    bool canSnapToGround() const {
        return mState != PlayerState::Jump;
    }

    /**
     * @brief 現在の Y 軸回転角を取得する。
     * @return Y 軸回転角（ラジアン）。
     */
    float getRotationY() const {
        return mRot.y;
    }

private:


    /**
     * @brief 入力ベクトルとカメラ方向から移動処理を行う。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void handleMove(float deltaTime);

    /**
     * @brief 現在状態に対応する更新処理へ分岐する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateState(float deltaTime);


    /**
     * @brief 待機状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateIdle(float deltaTime);

    /**
     * @brief 移動状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateMove(float deltaTime);

    /**
     * @brief 攻撃状態と攻撃判定タイミングを更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateAttack(float deltaTime);

    /**
     * @brief 回避状態と移動補間を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateDodge(float deltaTime);

    /**
     * @brief ガード状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateGuard(float deltaTime);

    /**
     * @brief ガード成功リアクションを更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateGuardImpact(float deltaTime);

    /**
     * @brief カウンター状態を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateCounter(float deltaTime);

    /**
     * @brief ジャンプ状態と垂直速度を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateJump(float deltaTime);

    /**
     * @brief 被弾状態とノックバック補間を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateHit(float deltaTime);

    /**
     * @brief 死亡状態と死亡演出を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateDead(float deltaTime);


    /**
     * @brief 現在キャラクターに応じた攻撃開始処理を行う。
     */
    void startAttack();

    /**
     * @brief 回避状態へ遷移する。
     */
    void startDodge();

    /**
     * @brief ガード状態へ遷移する。
     */
    void startGuard();

    /**
     * @brief カウンター状態へ遷移する。
     */
    void startCounter();

    /**
     * @brief ジャンプ状態へ遷移する。
     */
    void startJump();

    /**
     * @brief Wizard のビーム魔法攻撃を開始する。
     */
    void startWizardMagicAttack2();

    /**
     * @brief Wizard の範囲強化魔法を開始する。
     */
    void startWizardAreaPowerUp();

    /**
     * @brief 指定段数の通常攻撃を開始する。
     * @param[in] step 攻撃段数。
     */
    void startAttackStep(int step);

    /**
     * @brief 入力に応じて次段攻撃の予約を試みる。
     */
    void tryQueueNextAttack();

    /**
     * @brief 強化効果の残り時間を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updatePowerUp(float deltaTime);

    /**
     * @brief スタミナ回復と回復遅延を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateStamina(float deltaTime);

    /**
     * @brief 遅延再生する SE タイマーを更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateDelayedSe(float deltaTime);

    /**
     * @brief Wizard ビームのループ SE を停止する。
     */
    void stopWizardBeamSe();

    /**
     * @brief スタミナを消費できる場合だけ減算する。
     * @param[in] value 消費スタミナ量。
     * @retval true 消費に成功した。
     * @retval false スタミナ不足で消費しなかった。
     */
    bool consumeStamina(float value);

    /**
     * @brief 通常攻撃段数に応じたスタミナ消費量を取得する。
     * @param[in] step 攻撃段数。
     * @return 消費スタミナ量。
     */
    float getAttackStepStaminaCost(int step) const;

    /**
     * @brief 入力方向に応じた移動アニメーションを再生する。
     */
    void playMoveAnimation();

private:


    PlayerInput mInput; ///< このフレームで使用する入力スナップショット。

    bool mIsLockOn;       ///< ロックオン向けの移動・向き制御を使うかどうか。
    bool mIsAiControlled; ///< 外部 AI 入力で制御されているかどうか。

    PlayerCharacterType mCharacterType; ///< 使用中のキャラクター種別。


    int mIdleAnimIndex; ///< 待機アニメーション番号。
    int mMoveAnimIndex; ///< 移動アニメーション番号。

    int mAttack1AnimIndex; ///< 1 段目攻撃アニメーション番号。
    int mAttack2AnimIndex; ///< 2 段目攻撃アニメーション番号。
    int mAttack3AnimIndex; ///< 3 段目攻撃アニメーション番号。

    int mDodgeAnimIndex;       ///< 回避アニメーション番号。
    int mGuardAnimIndex;       ///< ガードアニメーション番号。
    int mGuardImpactAnimIndex; ///< ガード成功リアクションアニメーション番号。
    int mCounterAnimIndex;     ///< カウンターアニメーション番号。
    int mJumpAnimIndex;        ///< ジャンプアニメーション番号。
    int mHitAnimIndex;         ///< 被弾アニメーション番号。
    int mKnockbackAnimIndex;   ///< ノックバックアニメーション番号。
    int mDeadAnimIndex;        ///< 死亡アニメーション番号。


    int mIdleAnimSourceHandle;      ///< 待機アニメーション元モデルハンドル。
    int mMoveAnimSourceHandle;      ///< 前進アニメーション元モデルハンドル。
    int mMoveLeftAnimSourceHandle;  ///< 左移動アニメーション元モデルハンドル。
    int mMoveRightAnimSourceHandle; ///< 右移動アニメーション元モデルハンドル。
    int mMoveBackAnimSourceHandle;  ///< 後退アニメーション元モデルハンドル。

    int mAttack1AnimSourceHandle; ///< 1 段目攻撃アニメーション元モデルハンドル。
    int mAttack2AnimSourceHandle; ///< 2 段目攻撃アニメーション元モデルハンドル。
    int mAttack3AnimSourceHandle; ///< 3 段目攻撃アニメーション元モデルハンドル。

    int mDodgeAnimSourceHandle;       ///< 回避アニメーション元モデルハンドル。
    int mGuardAnimSourceHandle;       ///< ガードアニメーション元モデルハンドル。
    int mGuardImpactAnimSourceHandle; ///< ガード成功リアクションアニメーション元モデルハンドル。
    int mCounterAnimSourceHandle;     ///< カウンターアニメーション元モデルハンドル。

    int mJumpAnimSourceHandle;      ///< ジャンプアニメーション元モデルハンドル。
    int mHitAnimSourceHandle;       ///< 被弾アニメーション元モデルハンドル。
    int mKnockbackAnimSourceHandle; ///< ノックバックアニメーション元モデルハンドル。
    int mDeadAnimSourceHandle;      ///< 死亡アニメーション元モデルハンドル。


    Camera* mCamera; ///< 移動方向とロックオン計算に使う非所有カメラポインタ。

    PlayerState mState; ///< 現在の行動状態。

    float mStateTimer; ///< 現在状態に入ってからの経過時間（秒）。


    float mAttackDuration; ///< 攻撃状態の基本継続時間（秒）。
    float mDodgeDuration;  ///< 回避状態の継続時間（秒）。

    float mGuardEnterDuration; ///< ガード入力開始からガード成立までの時間（秒）。
    float mCounterDuration;    ///< カウンター状態の継続時間（秒）。

    float mJumpDuration; ///< ジャンプ状態の継続時間（秒）。
    float mHitDuration;  ///< 被弾状態の継続時間（秒）。
    float mDeadDuration; ///< 死亡演出の基準継続時間（秒）。


    bool mAttackHitRequest; ///< シーン側へ攻撃判定発生を要求しているかどうか。
    bool mAttackHitDone; ///< 現在攻撃で攻撃判定処理済みかどうか。
    bool mGuardImpactEffectRequest; ///< ガード成功エフェクト発生を要求しているかどうか。
    float mHitStopTimer; ///< ヒットストップ残り時間（秒）。

    int mAttackStep; ///< 現在の通常攻撃段数。

    bool mNextAttackQueued; ///< 次段攻撃入力が予約されているかどうか。

    float mWizardMagicHitTimer; ///< Wizard 魔法ヒット処理用タイマー（秒）。
    bool mWizardBeamFireSePlayed; ///< Wizard ビーム発射 SE を再生済みかどうか。
    int mWizardBeamLoopSeHandle; ///< Wizard ビームループ SE の DxLib サウンドハンドル。
    float mMagicMissileSeTimer; ///< 魔法弾 SE の再生間隔タイマー（秒）。
    float mMagicCircleSeTimer; ///< 魔法陣 SE の再生間隔タイマー（秒）。
    float mKnightParrySeTimer; ///< Knight パリィ SE の再生間隔タイマー（秒）。

    int mWizardMagicHandFrameIndex; ///< 魔法発射位置に使う手フレーム番号。
    int mSwordFrameIndex; ///< 剣位置取得に使うフレーム番号。
    int mShieldFrameIndex; ///< 盾位置取得に使うフレーム番号。
    int mCounterEffectFrameIndex; ///< カウンターエフェクト位置に使うフレーム番号。
    int mBodyFrameIndex; ///< 本体位置補正に使うフレーム番号。

    float mPowerUpTimer; ///< 範囲強化効果の残り時間（秒）。

    float mStamina; ///< 現在スタミナ値。

    float mStaminaRegenDelayTimer; ///< スタミナ回復再開までの残り時間（秒）。


    float mDodgeTotalDistance; ///< 回避で移動する総距離。DxLib ワールド単位。

    VECTOR mDodgeDirection; ///< 回避移動方向のワールド空間ベクトル。
    VECTOR mDodgeStartPos;  ///< 回避開始時のワールド座標。
    VECTOR mDodgeEndPos;    ///< 回避終了予定のワールド座標。

    float mPrevDodgeProgress; ///< 前フレームまでに適用済みの回避補間率。

    VECTOR mHitKnockbackDirection; ///< 被弾ノックバック方向のワールド空間ベクトル。
    float mHitKnockbackDistance; ///< 被弾ノックバック距離。DxLib ワールド単位。
    float mPrevHitKnockbackProgress; ///< 前フレームまでに適用済みの被弾ノックバック補間率。
    bool mHitKnockbackActive; ///< 被弾ノックバックが有効かどうか。
    VECTOR mHitKnockbackStartBodyOffset; ///< 被弾開始時の本体フレーム補正量。


    float mJumpSpeed;     ///< ジャンプ初速度。DxLib ワールド単位/秒。
    float mJumpVelocityY; ///< 現在の垂直速度。DxLib ワールド単位/秒。

    bool mIsGrounded; ///< 地面に接地しているかどうか。
    float mFootstepSeTimer; ///< 足音 SE の再生間隔タイマー（秒）。
};

#endif
