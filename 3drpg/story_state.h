#ifndef _STORY_STATE_H_
#define _STORY_STATE_H_


/**
 * @file
 * @brief ボス進行状態とシーン間の一時パーティ状態を管理する StoryState を定義する。
 */

#include "manager.h"

/**
 * @brief ゲーム進行上の大まかなフェーズ。
 */
enum class StoryPhase
{
    BossSelect,       ///< ボス選択エリアにいる状態。
    MagicBossCleared, ///< 魔法ボスを撃破済みの状態。
    FinalBoss,        ///< 最終ボス戦へ進んだ状態。
    HappyEnd          ///< 両ボス撃破後のエンディング状態。
};

/**
 * @brief シーンをまたぐストーリー進行フラグを保持する管理クラス。
 *
 * `Manager<StoryState>` 経由で共有される。ファイル保存はこのクラスからは確認できないため、
 * ここで保持する値は実行中セッション内のゲーム状態として扱う。
 */
class StoryState : public Manager<StoryState>
{
    /**
     * @brief Manager から private コンストラクタへアクセスさせる。
     */
    friend class Manager<StoryState>;

public:
    /**
     * @brief 現在のストーリーフェーズを取得する。
     * @return 現在フェーズ。
     */
    StoryPhase getPhase() const;

    /**
     * @brief 現在のストーリーフェーズを設定する。
     * @param[in] phase 新しいフェーズ。
     */
    void setPhase(StoryPhase phase);

    /**
     * @brief ボス選択フェーズかどうかを取得する。
     * @return ボス選択フェーズなら true。
     */
    bool isBossSelect() const;

    /**
     * @brief 魔法ボス撃破済みかどうかを取得する。
     * @return 撃破済みなら true。
     */
    bool isMagicBossCleared() const;

    /**
     * @brief 最終ボス撃破済みかどうかを取得する。
     * @return 撃破済みなら true。
     */
    bool isFinalBossCleared() const;

    /**
     * @brief 両方のボスを撃破済みかどうかを取得する。
     * @return 両方撃破済みなら true。
     */
    bool areBothBossesCleared() const;

    /**
     * @brief 現在フェーズが最終ボス戦かどうかを取得する。
     * @return 最終ボス戦なら true。
     */
    bool isFinalBoss() const;

    /**
     * @brief 魔法ボス撃破フラグを設定する。
     * @param[in] cleared 撃破済みなら true。
     */
    void setMagicBossCleared(bool cleared);

    /**
     * @brief 最終ボス撃破フラグを設定する。
     * @param[in] cleared 撃破済みなら true。
     */
    void setFinalBossCleared(bool cleared);

    /**
     * @brief VampireScene から戻る際に引き継ぐパーティ HP を一時保存する。
     * @param[in] playerHp プレイヤーの HP。
     * @param[in] companionHp 相棒の HP。
     */
    void saveVampirePartyState(int playerHp, int companionHp);

    /**
     * @brief VampireScene から戻る際のパーティ HP が保存されているか確認する。
     * @return 保存済みなら true。
     */
    bool hasVampirePartyState() const;

    /**
     * @brief VampireScene から戻る際に復元するプレイヤー HP を取得する。
     * @return 保存された HP。
     */
    int getVampirePlayerHp() const;

    /**
     * @brief VampireScene から戻る際に復元する相棒 HP を取得する。
     * @return 保存された HP。
     */
    int getVampireCompanionHp() const;

    /**
     * @brief 新規ゲーム開始時の状態へ戻す。
     */
    void resetForNewGame();

private:
    /**
     * @brief 初期ストーリー状態を設定する。
     */
    StoryState();

    StoryPhase mPhase;       ///< 現在のストーリーフェーズ。
    bool mMagicBossCleared;  ///< 魔法ボス撃破フラグ。
    bool mFinalBossCleared;  ///< 最終ボス撃破フラグ。
    bool mHasVampirePartyState; ///< VampireScene 復帰用 HP が保存されているかどうか。
    int mVampirePlayerHp;       ///< VampireScene 復帰時に復元するプレイヤー HP。
    int mVampireCompanionHp;    ///< VampireScene 復帰時に復元する相棒 HP。
};

#endif
