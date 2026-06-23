#ifndef _MUTANT_SCENE_H_
#define _MUTANT_SCENE_H_


/**
 * @file
 * @brief Mutant 戦シーンを定義する。
 */


#include <memory>
#include <vector>

#include "scene_base.h"
#include "player.h"
#include "camera.h"
#include "input_manager.h"
#include "enemy.h"
#include "attack_effect.h"
#include "player_character_type.h"


/**
 * @brief ポーズ中に表示する設定シーンの前方宣言。
 */
class SettingScene;

/**
 * @brief mutant 戦を中心にしたメイン戦闘シーン。
 *
 * プレイヤー、相棒 AI、敵、ステージ、UI、ポーズメニュー、ボス選択用の
 * 転送マーカーをまとめて管理する。処理の実体は用途ごとに分割した
 * mutant_scene_*.cpp に配置している。
 */
class MutantScene : public SceneBase
{
public:

    /**
     * @brief 使用キャラクターを指定して mutant 戦シーンを生成する。
     * @param[in] playerType プレイヤーが操作するキャラクター種別。
     */
    explicit MutantScene(
        PlayerCharacterType playerType = PlayerCharacterType::Knight
    );

    /**
     * @brief シーン内で確保したモデル、UI、エフェクトなどを解放する。
     */
    virtual ~MutantScene();

    /**
     * @brief 戦闘、AI、入力、衝突、ポーズ状態を 1 フレーム分更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) override;

    /**
     * @brief ステージ、キャラクター、エフェクト、UI を描画する。
     */
    virtual void draw() override;

private:


    std::unique_ptr<Player> mPlayer; ///< 操作プレイヤー。MutantScene が所有する。

    std::unique_ptr<Player> mCompanion; ///< AI 相棒。MutantScene が所有する。

    std::unique_ptr<Camera> mCamera; ///< 戦闘用三人称カメラ。MutantScene が所有する。

    std::vector<std::unique_ptr<Enemy>> mEnemies; ///< シーン内の Mutant 系敵リスト。各 Enemy を所有する。

    AttackEffect mAttackEffect; ///< 近接攻撃、魔法、ヒット時の視覚エフェクト。


    int mStageModelHandle; ///< 表示用ステージモデルの DxLib ハンドル。

    int mStageCollisionModelHandle; ///< 衝突判定用ステージモデルの DxLib ハンドル。

    int mSkyModelHandle; ///< 空背景モデルの DxLib ハンドル。


    int mOverheadLightHandle; ///< 上方ライトの DxLib ライトハンドル。

    int mCenterSpotLightHandle; ///< 中央スポットライトの DxLib ライトハンドル。

    int mShadowMapHandle; ///< シャドウマップ用 DxLib ハンドル。

    int mMagicBossTeleportCircleGraphHandle; ///< VampireScene 方面マーカー画像の DxLib グラフィックハンドル。
    int mFinalBossTeleportCircleGraphHandle; ///< FinalBoss 方面マーカー画像の DxLib グラフィックハンドル。
    int mAGraphHandle; ///< 操作ヒント用 A ボタン画像の DxLib グラフィックハンドル。


    int mStageMaterialNum; ///< ステージモデルのマテリアル数。

    int mStageNoDiffuseTextureNum; ///< Diffuse テクスチャを持たないステージマテリアル数。


    bool mIsEnemyLocked; ///< プレイヤーが敵へロックオンしているかどうか。

    PlayerCharacterType mPlayerType; ///< 操作プレイヤーのキャラクター種別。

    float mIntroTimer; ///< 戦闘導入演出の経過時間（秒）。
    float mEnemyHpBarRevealTimer; ///< 敵 HP バー表示開始までの管理タイマー（秒）。
    float mPlayerDelayedHp; ///< UI 表示用に遅延追従するプレイヤー HP。
    float mCompanionDelayedHp; ///< UI 表示用に遅延追従する相棒 HP。
    bool mEnemyIntroStarted; ///< 敵登場演出を開始済みかどうか。

    float mCompanionAttackTimer; ///< 相棒 AI の攻撃判断タイマー（秒）。
    float mCompanionSpellTimer; ///< 相棒 AI の魔法使用判断タイマー（秒）。
    float mCompanionDodgeTimer; ///< 相棒 AI の回避判断タイマー（秒）。
    float mCompanionGuardHoldTimer; ///< 相棒 AI のガード維持時間（秒）。
    float mCompanionSpecialTimer; ///< 相棒 AI の特殊行動判断タイマー（秒）。
    float mCompanionDefenseReactionTimer; ///< 相棒 AI の防御反応タイマー（秒）。
    float mCompanionDeadTimer; ///< 相棒死亡後の経過時間（秒）。
    const char* mCompanionDebugState; ///< 相棒 AI のデバッグ表示用状態名。文字列は所有しない。

    bool mIsPaused; ///< ポーズメニュー表示中かどうか。
    bool mShowDebugHitCollision; ///< デバッグ用当たり判定表示を有効にするかどうか。
    bool mEnemyMovementEnabled; ///< 敵 AI の移動処理を有効にするかどうか。
    int mPauseSelectedItem; ///< ポーズメニューで選択中の項目番号。
    int mPauseFontHandle; ///< ポーズメニュー用 DxLib フォントハンドル。
    std::unique_ptr<SettingScene> mPauseSettingScene; ///< ポーズ中に開く設定シーン。存在する場合は所有する。
    float mTeleportEffectTimer; ///< 転送エフェクトの経過時間（秒）。
    bool mTeleportToFinalBoss; ///< FinalBoss 方面へ転送中かどうか。
    bool mTeleportReturnToBossSelect; ///< ボス選択地点へ戻る転送中かどうか。
    bool mTeleportResourcesLoaded; ///< 転送マーカー関連リソースを読み込み済みかどうか。
    VECTOR mReturnMarkerPos; ///< ボス選択地点へ戻るマーカー位置。

    /**
     * @brief Wizard 魔法弾ヒットを遅延適用するための予約情報。
     */
    struct PendingMagicMissileHit
    {
        Enemy* target; ///< ダメージ適用先の非所有ポインタ。

        VECTOR hitPos; ///< ヒットエフェクトを出すワールド座標。

        float timer; ///< 適用までの残り時間（秒）。
        bool shieldBreaker; ///< ボスシールド破壊を優先するかどうか。
    };

    std::vector<PendingMagicMissileHit> mPendingMagicMissileHits; ///< 遅延適用待ちの魔法弾ヒット一覧。

private:


    /**
     * @brief シーンに出現する Enemy を生成・配置する。
     */
    void setupEnemies();

    /**
     * @brief Enemy 一覧を 1 フレーム分更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateEnemies(float deltaTime);

    /**
     * @brief 相棒 AI の入力生成と状態更新を行う。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateCompanion(float deltaTime);

    /**
     * @brief Knight の剣軌跡サンプルを更新する。
     */
    void updateKnightSwordTrails();

    /**
     * @brief UI 表示用の遅延 HP を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateDelayedPlayerHp(float deltaTime);


    /**
     * @brief プレイヤー攻撃と敵へのヒット処理を行う。
     */
    void processPlayerAttack();

    /**
     * @brief 相棒攻撃と敵へのヒット処理を行う。
     */
    void processCompanionAttack();

    /**
     * @brief 遅延予約された魔法弾ヒットを更新・適用する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updatePendingMagicMissileHits(float deltaTime);

    /**
     * @brief VampireScene 方面転送マーカーの入力と状態を更新する。
     */
    void updateVampireMarker();

    /**
     * @brief VampireScene 方面転送マーカーを描画する。
     */
    void drawVampireMarker() const;

    /**
     * @brief VampireScene 方面マーカーを使用できるか判定する。
     * @return 使用可能なら true。
     */
    bool canUseVampireMarker() const;

    /**
     * @brief FinalBoss 方面マーカーを使用できるか判定する。
     * @return 使用可能なら true。
     */
    bool canUseFinalBossMarker() const;

    /**
     * @brief ボス選択地点へ戻るマーカーを使用できるか判定する。
     * @return 使用可能なら true。
     */
    bool canUseReturnMarker() const;

    /**
     * @brief 転送エフェクトを更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateTeleportEffect(float deltaTime);

    /**
     * @brief 転送エフェクトを描画する。
     */
    void drawTeleportEffect() const;

    /**
     * @brief Enemy 攻撃とプレイヤー側へのヒット処理を行う。
     */
    void processEnemyAttack();

    /**
     * @brief プレイヤー・相棒と Enemy の押し戻し衝突を解決する。
     */
    void resolvePlayerEnemyCollision();

    /**
     * @brief プレイヤーとステージ地面の衝突を解決する。
     */
    void resolvePlayerStageGroundCollision();

    /**
     * @brief 相棒とステージ地面の衝突を解決する。
     */
    void resolveCompanionStageGroundCollision();

    /**
     * @brief Enemy 一覧とステージ地面の衝突を解決する。
     */
    void resolveEnemiesStageGroundCollision();


    /**
     * @brief デバッグ用 HUD を描画する。
     */
    void drawDebugHud();

    /**
     * @brief デバッグ用当たり判定を描画する。
     */
    void drawDebugHitCollision();


    /**
     * @brief ステージモデルと衝突モデルを準備する。
     */
    void setupStage();

    /**
     * @brief 初期配置時に FinalBoss 関連位置を地面へ合わせる。
     */
    void snapInitialFinalBossGround();

    /**
     * @brief シーン内ライトを設定する。
     */
    void setupLighting();

    /**
     * @brief シャドウマップ用リソースを設定する。
     */
    void setupShadowMap();

    /**
     * @brief シャドウマップを描画する。
     */
    void renderShadowMap();

    /**
     * @name UI helpers
     * @{
     */

    /**
     * @brief 戦闘 UI を描画する。
     */
    void drawGameUi();

    /**
     * @brief 戦闘 UI 用リソースを解放する。
     */
    void releaseGameUi();

    /**
     * @brief ポーズメニュー入力と設定画面を更新する。
     */
    void updatePauseMenu();

    /**
     * @brief ポーズメニューを描画する。
     */
    void drawPauseMenu() const;

    /**
     * @brief ポーズメニュー選択を移動する。
     * @param[in] direction 移動方向。
     */
    void movePauseSelection(int direction);

    /**
     * @brief ポーズメニューの現在項目を実行する。
     */
    void decidePauseSelection();

    /**
     * @brief 指定スクリーン座標上のポーズ項目を取得する。
     * @param[in] x スクリーン X 座標（pixel）。
     * @param[in] y スクリーン Y 座標（pixel）。
     * @return 項目番号。該当なしの場合の値は実装に従う。
     */
    int getPauseMouseItem(int x, int y) const;

    /**
     * @brief ポーズ項目の描画 Y 座標を取得する。
     * @param[in] item 項目番号。
     * @return Y 座標（pixel）。
     */
    int getPauseItemY(int item) const;

    /**
     * @brief ポーズ状態を設定する。
     * @param[in] paused true の場合はポーズする。
     */
    void setPaused(bool paused);
    /** @} */


    /**
     * @brief プレイヤーに最も近い Enemy を取得する。
     * @return 見つかった Enemy への非所有ポインタ。存在しない場合は nullptr。
     */
    Enemy* getNearestEnemy() const;
};

#endif
