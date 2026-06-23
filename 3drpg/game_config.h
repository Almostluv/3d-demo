#ifndef _GAME_CONFIG_H_
#define _GAME_CONFIG_H_


/**
 * @file
 * @brief 画面サイズやゲーム全体の設定定数を定義する。
 */



constexpr int kWindowWidth = 1280; ///< ウィンドウ幅（pixel）。

constexpr int kWindowHeight = 720; ///< ウィンドウ高さ（pixel）。


/**
 * @brief MutantScene のステージ、ライト、シャドウ、UI リソースに関する設定値。
 *
 * 位置・距離は DxLib ワールド単位、角度値は DxLib のライト API に渡す値として扱う。
 */
namespace MutantSceneConfig
{

    constexpr float StagePositionY = -10.0f; ///< ステージ基準位置の Y 座標。DxLib ワールド単位。

    constexpr float StageScale = 0.2f; ///< 共通ステージモデルへ適用する一様スケール。

    constexpr float MutantStageScale = 0.8f; ///< Mutant 戦ステージへ適用する一様スケール。

    constexpr float MutantArenaBaseScale = 0.24f; ///< アリーナ制限半径計算の基準スケール。

    constexpr float MutantArenaBaseLimitRadius = 52.0f; ///< 基準スケール時のアリーナ制限半径。DxLib ワールド単位。

    constexpr float MutantArenaLimitRadius = MutantArenaBaseLimitRadius * (MutantStageScale / MutantArenaBaseScale); ///< 実スケール適用後の Mutant 戦アリーナ制限半径。DxLib ワールド単位。


    constexpr float OverheadLightY = 85.0f; ///< 上方ライトの Y 座標。DxLib ワールド単位。

    constexpr float OverheadLightRange = 260.0f; ///< 上方ライトの有効範囲。DxLib ワールド単位。

    constexpr float OverheadLightAtten0 = 0.35f; ///< 上方ライトの距離減衰係数 0。

    constexpr float OverheadLightAtten1 = 0.002f; ///< 上方ライトの距離減衰係数 1。

    constexpr float OverheadLightDrawY = 1000.0f; ///< 上方ライト表示用位置の Y 座標。DxLib ワールド単位。

    constexpr float OverheadLightDrawRange = 10000.0f; ///< 上方ライト表示用の広い描画範囲。DxLib ワールド単位。


    constexpr float CenterSpotLightX = 0.0f; ///< 中央スポットライトの X 座標。DxLib ワールド単位。
    constexpr float CenterSpotLightY = 95.0f; ///< 中央スポットライトの Y 座標。DxLib ワールド単位。
    constexpr float CenterSpotLightZ = 0.0f; ///< 中央スポットライトの Z 座標。DxLib ワールド単位。

    constexpr float CenterSpotLightRange = 180.0f; ///< 中央スポットライトの有効範囲。DxLib ワールド単位。

    constexpr float CenterSpotLightAtten0 = 0.45f; ///< 中央スポットライトの距離減衰係数 0。
    constexpr float CenterSpotLightAtten1 = 0.002f; ///< 中央スポットライトの距離減衰係数 1。

    constexpr float CenterSpotLightOutAngle = 0.95f; ///< 中央スポットライト外側角度。DxLib ライト API 用の角度値。

    constexpr float CenterSpotLightInAngle = 0.45f; ///< 中央スポットライト内側角度。DxLib ライト API 用の角度値。

    constexpr float PlayerArmorLightOffsetX = -34.0f; ///< プレイヤー鎧用補助ライト位置の X オフセット。DxLib ワールド単位。
    constexpr float PlayerArmorLightOffsetY = 54.0f; ///< プレイヤー鎧用補助ライト位置の Y オフセット。DxLib ワールド単位。
    constexpr float PlayerArmorLightOffsetZ = -42.0f; ///< プレイヤー鎧用補助ライト位置の Z オフセット。DxLib ワールド単位。

    constexpr float PlayerArmorLightTargetY = 18.0f; ///< プレイヤー鎧用補助ライトの注視点 Y 座標。DxLib ワールド単位。


    constexpr float MainLightX = -0.35f; ///< メインライト方向ベクトルの X 成分。
    constexpr float MainLightY = -0.80f; ///< メインライト方向ベクトルの Y 成分。
    constexpr float MainLightZ = 0.45f; ///< メインライト方向ベクトルの Z 成分。


    constexpr int ShadowMapSize = 2048; ///< シャドウマップ解像度（pixel）。

    constexpr float ShadowAreaHalfSize = 90.0f; ///< シャドウ描画範囲の半径。DxLib ワールド単位。

    constexpr float ShadowGroundRayTop = 160.0f; ///< 影用地面探索レイの上方向距離。DxLib ワールド単位。

    constexpr float ShadowGroundRayBottom = 260.0f; ///< 影用地面探索レイの下方向距離。DxLib ワールド単位。

    constexpr float ShadowAreaBottomMargin = 8.0f; ///< シャドウ範囲下端へ加える余白。DxLib ワールド単位。

    constexpr float ShadowAreaTopMargin = 90.0f; ///< シャドウ範囲上端へ加える余白。DxLib ワールド単位。

    constexpr float ShadowAdjustDepth = 0.006f; ///< シャドウの深度補正値。


    /**
     * @brief プレイヤー HP フレーム画像のパス。
     */
    constexpr const char* PlayerHpFrame =
        "Resource/GameUI/playerFrame.png";

    /**
     * @brief 敵 HP フレーム画像のパス。
     */
    constexpr const char* EnemyHpFrame =
        "Resource/GameUI/enemyFrame.png";

    /**
     * @brief スタミナフレーム画像のパス。
     */
    constexpr const char* StaminaFrame =
        "Resource/GameUI/staminaFrame.png";
}

#endif // !_GAME_CONFIG_H_
