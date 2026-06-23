/**
 * @file
 * @brief story_state.cpp 実装を定義する。
 */

#include "story_state.h"

StoryState::StoryState()
    : mPhase(StoryPhase::BossSelect)
    , mMagicBossCleared(false)
    , mFinalBossCleared(false)
    , mHasVampirePartyState(false)
    , mVampirePlayerHp(0)
    , mVampireCompanionHp(0)
{
    /// 新規起動時はボス選択状態から開始し、各ボス撃破フラグと引き継ぎ HP を未設定にする。
}

StoryPhase StoryState::getPhase() const
{
    return mPhase;
}

void StoryState::setPhase(StoryPhase phase)
{
    /// シーン遷移側から現在のストーリー段階を明示的に更新する。
    mPhase = phase;
}

bool StoryState::isBossSelect() const
{
    return mPhase == StoryPhase::BossSelect;
}

bool StoryState::isMagicBossCleared() const
{
    /// 互換用に、明示フラグとフェーズのどちらでも MagicBoss クリア扱いにする。
    return mMagicBossCleared || mPhase == StoryPhase::MagicBossCleared;
}

bool StoryState::isFinalBossCleared() const
{
    return mFinalBossCleared;
}

bool StoryState::areBothBossesCleared() const
{
    /// エンディング判定では Vampire 側と FinalBoss 側の両方の完了を確認する。
    return isMagicBossCleared() && isFinalBossCleared();
}

bool StoryState::isFinalBoss() const
{
    return mPhase == StoryPhase::FinalBoss;
}

void StoryState::setMagicBossCleared(bool cleared)
{
    mMagicBossCleared = cleared;
}

void StoryState::setFinalBossCleared(bool cleared)
{
    mFinalBossCleared = cleared;
}

void StoryState::saveVampirePartyState(int playerHp, int companionHp)
{
    /// VampireScene から戻った後に、MutantScene 側へ現在 HP を引き継ぐため保存する。
    mVampirePlayerHp = playerHp;
    mVampireCompanionHp = companionHp;
    mHasVampirePartyState = true;
}

bool StoryState::hasVampirePartyState() const
{
    return mHasVampirePartyState;
}

int StoryState::getVampirePlayerHp() const
{
    return mVampirePlayerHp;
}

int StoryState::getVampireCompanionHp() const
{
    return mVampireCompanionHp;
}

void StoryState::resetForNewGame()
{
    /// キャラクター選択から新しく始める場合は、進行状態と保存 HP を初期値へ戻す。
    mPhase = StoryPhase::BossSelect;
    mMagicBossCleared = false;
    mFinalBossCleared = false;
    mHasVampirePartyState = false;
    mVampirePlayerHp = 0;
    mVampireCompanionHp = 0;
}
