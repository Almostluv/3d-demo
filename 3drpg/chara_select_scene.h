#ifndef _CHARA_SELECT_SCENE_H_
#define _CHARA_SELECT_SCENE_H_


/**
 * @file
 * @brief キャラクター選択シーンを定義する。
 */


#include "animator.h"
#include "player_character_type.h"
#include "scene_base.h"
#include "DxLib.h"

#include <cstddef>
#include <vector>

/**
 * @brief タイトル画面中に先読みするリソース種別。
 */
enum class TitlePreloadType
{
    Model, ///< DxLib モデルリソース。
    Graph  ///< DxLib 画像リソース。
};

/**
 * @brief タイトル画面中の非同期風プリロード 1 件分の情報。
 *
 * `path` は静的なリソースパス文字列を指す非所有ポインタ。
 */
struct TitlePreloadTask
{
    TitlePreloadType type; ///< 先読み対象のリソース種別。
    const char* path;      ///< 先読み対象リソースのパス。
};

/**
 * @brief タイトル表示とキャラクター選択を担当するシーン。
 *
 * Knight / Wizard のプレビュー、選択確定ダイアログ、開始演出、
 * MutantScene 用リソースの先読みを管理する。
 */
class CharaSelectScene : public SceneBase
{
public:

    /**
     * @brief キャラクター選択に必要なモデル、UI、ライトを準備する。
     */
    CharaSelectScene();

    /**
     * @brief シーンで保持する DxLib リソースを解放する。
     */
    virtual ~CharaSelectScene();

    /**
     * @brief 入力、選択状態、モデル演出、開始遷移を更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    virtual void update(float deltaTime) override;

    /**
     * @brief キャラクタープレビュー、背景、UI を描画する。
     */
    virtual void draw() override;

private:


    /**
     * @brief キャラクタープレビューモデルと演出タイマーを更新する。
     * @param[in] deltaTime 前フレームからの経過時間（秒）。
     */
    void updateModel(float deltaTime);

    /**
     * @brief 入力とマウス位置から選択中キャラクターを更新する。
     */
    void updateSelection();

    /**
     * @brief MutantScene で使う主要リソースの先読みキューを作成する。
     */
    void setupMutantScenePreloadQueue();

    /**
     * @brief モデルリソースを先読みキューへ追加する。
     * @param[in] path モデルリソースパス。呼び出し側は文字列を所有し続ける。
     */
    void addPreloadModel(const char* path);

    /**
     * @brief 画像リソースを先読みキューへ追加する。
     * @param[in] path 画像リソースパス。呼び出し側は文字列を所有し続ける。
     */
    void addPreloadGraph(const char* path);

    /**
     * @brief 先読みキューを指定件数まで進める。
     * @param[in] maxCount 1 回の更新で処理する最大件数。
     */
    void updatePreloadQueue(int maxCount);

    /**
     * @brief 先読みキューが完了しているか確認する。
     * @return すべて処理済みなら true。
     */
    bool isPreloadFinished() const;


    /**
     * @brief キャラクタープレビュー用のシャドウマップを描画する。
     */
    void renderShadowMap();

    /**
     * @brief タイトル画面 UI を描画する。
     */
    void drawTitleUi();

    /**
     * @brief 戦闘開始遷移中の仮 UI を描画する。
     * @param[in] rate 遷移率。
     */
    void drawTransitionGameUi(float rate);


    /**
     * @brief 指定キャラクター位置にマウスカーソルが重なっているか判定する。
     * @param[in] pos キャラクターのワールド座標。
     * @return カーソルが重なっていれば true。
     */
    bool isMouseOverCharacter(VECTOR pos) const;


    /**
     * @brief ホバーまたは選択中のキャラクターモデルへ発光表現を適用する。
     * @param[in] modelHandle 対象モデルの DxLib ハンドル。
     * @param[in] hovered ホバー中なら true。
     * @param[in] selected 選択中なら true。
     * @param[in] timer 発光アニメーション用タイマー（秒）。
     */
    void applyCharacterGlow(
        int modelHandle,
        bool hovered,
        bool selected,
        float timer
    ) const;

    /**
     * @brief 指定モデルをキャラクタープレビューとして描画する。
     * @param[in] modelHandle 描画対象の DxLib モデルハンドル。
     */
    void drawCharacterModel(int modelHandle) const;

    /**
     * @brief 2 点間を線形補間する。
     * @param[in] a 開始座標。
     * @param[in] b 終了座標。
     * @param[in] t 補間率。
     * @return 補間後の座標。
     */
    VECTOR lerpVector(VECTOR a, VECTOR b, float t) const;

private:


    int mPlayerModelHandle;  ///< Knight プレビュー用 DxLib モデルハンドル。
    int mWizardModelHandle;  ///< Wizard プレビュー用 DxLib モデルハンドル。
    int mPointerModelHandle; ///< 選択ポインタ用 DxLib モデルハンドル。


    int mPlayerSittingAnimSourceHandle; ///< Knight 座りアニメーション元モデルハンドル。
    int mPlayerSitToStandAnimSourceHandle; ///< Knight 立ち上がりアニメーション元モデルハンドル。

    int mWizardSittingAnimSourceHandle; ///< Wizard 座りアニメーション元モデルハンドル。
    int mWizardSitToStandAnimSourceHandle; ///< Wizard 立ち上がりアニメーション元モデルハンドル。

    int mPlayerBarrelModelHandle; ///< Knight 側の演出用樽モデルハンドル。
    int mWizardBarrelModelHandle; ///< Wizard 側の演出用樽モデルハンドル。

    int mStageModelHandle; ///< キャラクター選択背景ステージの DxLib モデルハンドル。
    int mSkyModelHandle;   ///< 空背景モデルの DxLib ハンドル。


    int mOverheadLightHandle; ///< プレビュー用上方ライトの DxLib ライトハンドル。
    int mShadowMapHandle;     ///< プレビュー用シャドウマップハンドル。


    int mCharaSelectBannerGraphHandle; ///< キャラクター選択バナー画像の DxLib グラフィックハンドル。

    int mPlayerHpFrameGraphHandle; ///< 遷移中 UI 用プレイヤー HP フレーム画像ハンドル。
    int mEnemyHpFrameGraphHandle;  ///< 遷移中 UI 用敵 HP フレーム画像ハンドル。
    int mStaminaFrameGraphHandle;  ///< 遷移中 UI 用スタミナフレーム画像ハンドル。
    int mAGraphHandle;             ///< 決定ボタン表示用グラフィックハンドル。
    int mBGraphHandle;             ///< キャンセルボタン表示用グラフィックハンドル。
    int mDpadLeftGraphHandle;      ///< 左方向キー表示用グラフィックハンドル。
    int mDpadRightGraphHandle;     ///< 右方向キー表示用グラフィックハンドル。
    int mUiFontHandle;             ///< UI 文字表示用 DxLib フォントハンドル。


    Animator mPlayerAnimator; ///< Knight プレビューモデル用 Animator。
    Animator mWizardAnimator; ///< Wizard プレビューモデル用 Animator。


    PlayerCharacterType mSelectedCharacter; ///< 現在選択中のキャラクター種別。

    int mHoveredCharacter; ///< マウスホバー中のキャラクター番号。

    bool mHasSelectedCharacter; ///< キャラクター確定済みかどうか。


    float mTransitionTimer; ///< 戦闘開始遷移の経過時間（秒）。
    float mGlowTimer;       ///< キャラクター発光演出の経過時間（秒）。


    bool mIsStarting; ///< 戦闘開始遷移中かどうか。
    bool mConfirmCharacterDialog; ///< キャラクター確定ダイアログ表示中かどうか。

    std::vector<TitlePreloadTask> mPreloadTasks; ///< MutantScene 用リソースの先読みキュー。
    std::size_t mPreloadTaskIndex; ///< 次に処理する先読みタスクのインデックス。
};

#endif
