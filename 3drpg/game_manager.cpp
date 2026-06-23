/**
 * @file
 * @brief game_manager.cpp 実装を定義する。
 */

#include "game_manager.h"
#include "scene_manager.h"
#include "input_manager.h"
#include "resource_manager.h" 
#include "mutant_scene.h"
#include "title_scene.h"
#include "game_config.h"
#include "DxLib.h"


#include <chrono>

GameManager::GameManager() : mShouldExit(false) {}

GameManager::~GameManager() {}

void GameManager::requestExit()
{
    /// メインループ条件で参照する終了フラグだけを立て、実際の破棄は finalize() に任せる。
    mShouldExit = true;
}

int GameManager::run() {
    if (init() == -1) return -1;

    using clock = std::chrono::steady_clock;
    auto prevTime = clock::now();

    while (ProcessMessage() == 0 && !mShouldExit) {
        /// steady_clock の差分を deltaTime として使い、各シーンの更新をフレーム時間に追従させる。
        auto currentTime = clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - prevTime).count();
        prevTime = currentTime;

        /// DxLib の裏画面へ描画し、1 フレームの最後に ScreenFlip() で表示を更新する。
        ClearDrawScreen();
        update(deltaTime);
        draw();
        ScreenFlip();
    }


    finalize();
    return 0;
}

int GameManager::init() {
    /// DxLib 初期化前にウィンドウ、D3D、文字コードを設定して実行環境を固定する。
    ChangeWindowMode(TRUE);
    SetUseDirect3DVersion(DX_DIRECT3D_11);
    SetUseDirect3D11MinFeatureLevel(DX_DIRECT3D_11_FEATURE_LEVEL_11_0);
    SetChangeScreenModeGraphicsSystemResetFlag(FALSE);
    SetGraphMode(kWindowWidth, kWindowHeight, 32);
    SetMainWindowText("3drpg");
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
    SetFontCharCodeFormat(DX_CHARCODEFORMAT_UTF8);

    if (DxLib_Init() == -1) return -1;

    /// 3D 描画とバックバッファ描画を有効にし、以降のシーン描画の前提を整える。
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetDrawScreen(DX_SCREEN_BACK);
    SetMouseDispFlag(FALSE);

    SceneManager::instance()->changeScene<TitleScene>();

    return 0;
}

void GameManager::update(float deltaTime) {
    /// 入力はシーン更新より先に確定させ、同じフレームの入力状態を全シーンで参照できるようにする。
    InputManager::instance()->update();

    SceneManager::instance()->update(deltaTime);
}

void GameManager::draw() {
    /// 描画の実体は現在のシーンへ委譲する。
    SceneManager::instance()->draw();
}

void GameManager::finalize() {
    /// DxLib を終了する前にシーンと共有リソースを明示的に解放する。
    SceneManager::instance()->clearScene();

    ResourceManager::instance()->clearAll();

    DxLib_End();
}
