#ifndef _VAMPIRE_SCENE_H_
#define _VAMPIRE_SCENE_H_


/**
 * @file
 * @brief Vampire 戦シーンを定義する。
 */


#include <memory>
#include <vector>

#include "attack_effect.h"
#include "camera.h"
#include "magic_projectile_enemy.h"
#include "player.h"
#include "player_character_type.h"
#include "scene_base.h"

/**
 * @brief ポーズ中に表示する設定シーンの前方宣言。
 */
class SettingScene;

/**
 * @brief vampire 戦を行うボス専用シーン。
 *
 * プレイヤーと相棒 AI、魔法弾を使うボス、イントロ演出、戦闘 UI、
 * クリア後の退出マーカーを管理する。
 * 処理の実体は用途ごとに分割した vampire_scene_*.cpp に配置している。
 */
class VampireScene : public SceneBase
{
public:
    /**
     * @brief 使用キャラクターを指定して vampire 戦シーンを生成する。
     * @param[in] playerType プレイヤーが操作するキャラクター種別。
     */
    explicit VampireScene(PlayerCharacterType playerType);

    /**
     * @brief シーン内リソースを解放する。
     */
    virtual ~VampireScene();

    /**
     * @brief イントロ、戦闘、AI、ポーズ、退出処理を 1 フレーム分更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) override;

    /**
     * @brief ボス部屋、キャラクター、攻撃エフェクト、UI を描画する。
     */
    virtual void draw() override;

private:
    /**
     * @brief Vampire 戦イントロ演出の状態。
     */
    enum IntroState
    {
        IntroMoveToBoss, ///< カメラまたは演出がボスへ寄る状態。
        IntroRoaring,    ///< ボス咆哮演出中。
        IntroMoveBack,   ///< 通常戦闘カメラへ戻る状態。
        IntroDone        ///< イントロ完了後の通常戦闘状態。
    };

    /**
     * @brief イントロ演出の状態遷移とカメラ補間を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateIntro(float deltaTime);

    /**
     * @brief イントロ用カメラを DxLib へ反映する。
     */
    void drawIntroCamera() const;

    /**
     * @brief 通常戦闘カメラの位置を取得する。
     * @return カメラ位置のワールド座標。
     */
    VECTOR getNormalCameraEye() const;

    /**
     * @brief 通常戦闘カメラの注視点を取得する。
     * @return 注視点のワールド座標。
     */
    VECTOR getNormalCameraTarget() const;

    /**
     * @brief ボス演出用カメラの位置を取得する。
     * @return カメラ位置のワールド座標。
     */
    VECTOR getBossCameraEye() const;

    /**
     * @brief ボス演出用カメラの注視点を取得する。
     * @return 注視点のワールド座標。
     */
    VECTOR getBossCameraTarget() const;

    /**
     * @brief 2 点間を線形補間する。
     * @param[in] a 開始座標。
     * @param[in] b 終了座標。
     * @param[in] t 補間率。
     * @return 補間後の座標。
     */
    VECTOR lerpVector(VECTOR a, VECTOR b, float t) const;

    /**
     * @brief プレイヤー入力、状態、カメラ連動を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updatePlayer(float deltaTime);

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
     * @brief プレイヤー攻撃とボスへのヒット処理を行う。
     */
    void processPlayerAttack();

    /**
     * @brief 相棒攻撃とボスへのヒット処理を行う。
     */
    void processCompanionAttack();

    /**
     * @brief 遅延予約された魔法弾ヒットを更新・適用する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updatePendingMagicMissileHits(float deltaTime);

    /**
     * @brief 指定 Player を魔法ボスの方向へ向ける。
     * @param[in,out] player 向きを変更する Player。nullptr の扱いは実装に従う。
     */
    void faceEnemy(Player* player);

    /**
     * @brief 指定 Player がボスへ攻撃可能な距離にいるか判定する。
     * @param[in] player 判定対象。VampireScene は所有しない。
     * @return 攻撃可能範囲なら true。
     */
    bool isEnemyInAttackRange(Player* player) const;

    /**
     * @brief ボス部屋の地面を描画する。
     */
    void drawGround() const;

    /**
     * @brief 戦闘 UI を描画する。
     */
    void drawGameUi() const;

    /**
     * @brief クリア後退出マーカーの入力と状態を更新する。
     */
    void updateExitMarker();

    /**
     * @brief クリア後退出マーカーを描画する。
     */
    void drawExitMarker() const;

    /**
     * @brief 退出マーカーを使用できるか判定する。
     * @return 使用可能なら true。
     */
    bool canUseExitMarker() const;

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

    PlayerCharacterType mPlayerType; ///< 操作プレイヤーのキャラクター種別。
    std::unique_ptr<Camera> mCamera; ///< 戦闘用三人称カメラ。VampireScene が所有する。
    std::unique_ptr<Player> mPlayer; ///< 操作プレイヤー。VampireScene が所有する。
    std::unique_ptr<Player> mCompanion; ///< AI 相棒。VampireScene が所有する。
    std::unique_ptr<MagicProjectileEnemy> mMagicEnemy; ///< 魔法弾を使うボス敵。VampireScene が所有する。
    std::unique_ptr<SettingScene> mSettingScene; ///< ポーズ中に開く設定シーン。存在する場合は所有する。
    AttackEffect mAttackEffect; ///< 近接攻撃、魔法、ヒット時の視覚エフェクト。
    float mPlayerDelayedHp; ///< UI 表示用に遅延追従するプレイヤー HP。
    float mCompanionDelayedHp; ///< UI 表示用に遅延追従する相棒 HP。
    float mCompanionAttackTimer; ///< 相棒 AI の攻撃判断タイマー（秒）。
    float mCompanionDodgeTimer; ///< 相棒 AI の回避判断タイマー（秒）。
    float mCompanionSpecialTimer; ///< 相棒 AI の特殊行動判断タイマー（秒）。
    float mCompanionStrafeTimer; ///< 相棒 AI の横移動継続タイマー（秒）。
    float mCompanionProjectileDecisionTimer; ///< 相棒 AI の弾回避判断タイマー（秒）。
    int mCompanionStrafeDir; ///< 相棒 AI の横移動方向。
    bool mCompanionIgnoreProjectile; ///< 相棒 AI が一時的に弾回避を無視するかどうか。
    bool mIsEnemyLocked; ///< プレイヤーがボスへロックオンしているかどうか。
    bool mShowDebugHitCollision; ///< デバッグ用当たり判定表示を有効にするかどうか。
    bool mIsPaused; ///< ポーズメニュー表示中かどうか。
    int mPauseSelectedItem; ///< ポーズメニューで選択中の項目番号。
    int mPauseFontHandle; ///< ポーズメニュー用 DxLib フォントハンドル。
    int mStageModelHandle; ///< ボス部屋表示用ステージモデルの DxLib ハンドル。
    int mStageCollisionModelHandle; ///< ボス部屋衝突用ステージモデルの DxLib ハンドル。
    int mExitTeleportCircleGraphHandle; ///< 退出マーカー画像の DxLib グラフィックハンドル。
    float mTeleportEffectTimer; ///< 転送エフェクトの経過時間（秒）。
    IntroState mIntroState; ///< 現在のイントロ演出状態。
    float mIntroTimer; ///< イントロ演出の経過時間（秒）。
    bool mRoaringStarted; ///< 咆哮演出を開始済みかどうか。
    bool mIntroBallsReleased; ///< イントロ用魔法弾を解放済みかどうか。

    /**
     * @brief Wizard 魔法弾ヒットを遅延適用するための予約情報。
     */
    struct PendingMagicMissileHit
    {
        int damage; ///< 適用するダメージ量。
        int attackStep; ///< ヒット元の攻撃段階。
        float timer; ///< 適用までの残り時間（秒）。
    };

    std::vector<PendingMagicMissileHit> mPendingMagicMissileHits; ///< 遅延適用待ちの魔法弾ヒット一覧。
};

#endif
