/**
 * @file
 * @brief SettingScene の cpp 間で共有する内部定数と表示 helper を定義する。
 */

#ifndef SETTING_SCENE_INTERNAL_H_
#define SETTING_SCENE_INTERNAL_H_

#include "setting_scene.h"

namespace SettingSceneInternal
{
constexpr int ItemX = 300; ///< 設定項目ラベルの描画 X 座標。単位は pixel。
constexpr int ValueX = 740; ///< 設定値の描画 X 座標。単位は pixel。
constexpr int ItemStartY = 210; ///< 先頭設定項目の描画 Y 座標。単位は pixel。
constexpr int ItemSpacing = 52; ///< 設定項目行の間隔。単位は pixel。
constexpr int BackY = 635; ///< 戻る行の描画 Y 座標。単位は pixel。
constexpr int ItemLeft = ItemX - 40; ///< 項目クリック判定の左端。単位は pixel。
constexpr int ItemRight = ValueX + 210; ///< 項目クリック判定の右端。単位は pixel。
constexpr int ItemTopOffset = -12; ///< 項目クリック判定の上端補正。単位は pixel。
constexpr int ItemBottomOffset = 42; ///< 項目クリック判定の下端補正。単位は pixel。
constexpr int ArrowHitPadding = 4; ///< 値調整矢印のクリック判定余白。単位は pixel。
constexpr int TabY = 120; ///< ページタブの描画 Y 座標。単位は pixel。
constexpr int TabStartX = 210; ///< 先頭ページタブの描画 X 座標。単位は pixel。
constexpr int TabWidth = 170; ///< ページタブのクリック幅。単位は pixel。
constexpr int TabHeight = 38; ///< ページタブのクリック高さ。単位は pixel。
constexpr int TabSpacing = 170; ///< ページタブ間隔。単位は pixel。
constexpr int KeyListY = 205; ///< 入力割り当て一覧の先頭 Y 座標。単位は pixel。
constexpr int KeyListSpacing = 42; ///< 入力割り当て一覧の行間隔。単位は pixel。

constexpr const char* ControllerGraph = "Resource/GameUI/UI_Controller_Keys_Thumbnail2.png"; ///< コントローラー説明画像。
constexpr const char* LbGraph = "Resource/GameUI/button_xbox_digital_bumper_light_1.png"; ///< LB ボタン画像。
constexpr const char* RbGraph = "Resource/GameUI/button_xbox_digital_bumper_light_2.png"; ///< RB ボタン画像。
constexpr const char* LeftStickGraph = "Resource/GameUI/button_xboxone_analog_l.png"; ///< 左スティック画像。
constexpr const char* RightStickGraph = "Resource/GameUI/button_xboxone_analog_r.png"; ///< 右スティック画像。
constexpr const char* LtGraph = "Resource/GameUI/button_xbox_analog_trigger_dark_1.png"; ///< LT ボタン画像。
constexpr const char* RtGraph = "Resource/GameUI/button_xbox_analog_trigger_dark_2.png"; ///< RT ボタン画像。
constexpr const char* AGraph = "Resource/GameUI/button_xbox_digital_a_3.png"; ///< A ボタン画像。
constexpr const char* BGraph = "Resource/GameUI/button_xbox_digital_b_3.png"; ///< B ボタン画像。
constexpr const char* YGraph = "Resource/GameUI/button_xbox_digital_y_3.png"; ///< Y ボタン画像。
constexpr const char* XGraph = "Resource/GameUI/button_xbox_digital_x_3.png"; ///< X ボタン画像。
constexpr const char* StartGraph = "Resource/GameUI/button_xbox_digital_menu_1.png"; ///< START ボタン画像。
constexpr const char* ViewGraph = "Resource/GameUI/button_xbox_digital_view_1.png"; ///< VIEW ボタン画像。
constexpr const char* DpadUpGraph = "Resource/GameUI/button_xbox_dpad_light_1.png"; ///< 十字キー上画像。
constexpr const char* DpadRightGraph = "Resource/GameUI/button_xbox_dpad_light_2.png"; ///< 十字キー右画像。
constexpr const char* DpadDownGraph = "Resource/GameUI/button_xbox_dpad_light_3.png"; ///< 十字キー下画像。
constexpr const char* DpadLeftGraph = "Resource/GameUI/button_xbox_dpad_light_4.png"; ///< 十字キー左画像。

inline const char* boolText(bool value)
{
    /// 設定画面では bool 値を ON/OFF 表記に統一する。
    return value ? "ON" : "OFF";
}

inline bool sameBinding(InputBinding a, InputBinding b)
{
    /// 入力種別とコードの両方が同じ場合だけ同一割り当てとみなす。
    return a.type == b.type && a.code == b.code;
}

inline const char* pageLabel(int page)
{
    /// ページ番号をタブ表示用の日本語ラベルへ変換する。
    switch (page)
    {
    case SettingScene::GamePage: return "ゲーム設定";
    case SettingScene::ScreenPage: return "画面設定";
    case SettingScene::SoundPage: return "サウンド設定";
    case SettingScene::ControllerPage: return "コントローラー設定";
    case SettingScene::KeyboardMousePage: return "キーボード・マウス設定";
    default: return "";
    }
}

inline const char* actionLabel(int action)
{
    /// アクション番号をボタン/キー設定画面の表示名へ変換する。
    switch (action)
    {
    case GameActionMoveForward: return "前進";
    case GameActionMoveBack: return "後退";
    case GameActionMoveLeft: return "左移動";
    case GameActionMoveRight: return "右移動";
    case GameActionPrimary: return "攻撃/魔法攻撃";
    case GameActionDodge: return "回避";
    case GameActionSecondary: return "ガード/魔法ビーム";
    case GameActionSpecial: return "カウンター/魔法杖";
    case GameActionLockOn: return "ロックオン";
    default: return "";
    }
}
}

#endif
