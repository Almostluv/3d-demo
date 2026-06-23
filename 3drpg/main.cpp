/**
 * @file
 * @brief main.cpp 実装を定義する。
 */


#include "game_manager.h"


int main(int argv, char** argc)
{
	/// ゲーム全体の初期化、メインループ、終了処理は GameManager に集約する。
	return GameManager::instance()->run();
}
