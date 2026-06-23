#ifndef _AUDIO_MANAGER_H_
#define _AUDIO_MANAGER_H_


/**
 * @file
 * @brief BGM と SE の再生を管理する AudioManager を定義する。
 */


#include "manager.h"
#include "DxLib.h"
#include <string>

/**
 * @brief BGM、SE、3D SE の再生と音量反映を担当する管理クラス。
 *
 * `Manager<AudioManager>` 経由で共有される。サウンドハンドルの生成と再生は DxLib に委譲し、
 * 現在再生中の BGM だけを内部状態として追跡する。
 */
class AudioManager : public Manager<AudioManager>
{
    /**
     * @brief Manager から private コンストラクタへアクセスさせる。
     */
    friend class Manager<AudioManager>;

public:
    /**
     * @brief 指定パスの BGM を再生する。
     * @param[in] path サウンドリソースのパス。
     */
    void playBgm(const std::string& path);

    /**
     * @brief 現在再生中の BGM を停止する。
     */
    void stopBgm();

    /**
     * @brief 指定パスの SE を 1 回再生する。
     * @param[in] path サウンドリソースのパス。
     */
    void playSe(const std::string& path);

    /**
     * @brief 指定パスの SE をループ再生する。
     * @param[in] path サウンドリソースのパス。
     * @return 停止に使う DxLib サウンドハンドル。失敗時の値は DxLib の戻り値に従う。
     */
    int playLoopSe(const std::string& path);

    /**
     * @brief ループ再生中の SE を停止する。
     * @param[in] handle 停止対象の DxLib サウンドハンドル。
     */
    void stopSe(int handle);

    /**
     * @brief 3D サウンドのリスナー位置と向きを更新する。
     * @param[in] position リスナーのワールド座標。
     * @param[in] frontPosition リスナー前方を示すワールド座標。
     */
    void updateListener(VECTOR position, VECTOR frontPosition);

    /**
     * @brief 指定位置で 3D SE を再生する。
     * @param[in] path サウンドリソースのパス。
     * @param[in] position 発音位置のワールド座標。
     * @param[in] radius 聞こえる範囲。DxLib ワールド単位。
     */
    void playSe3D(const std::string& path, VECTOR position, float radius = 360.0f);

    /**
     * @brief 現在再生中の BGM に最新の音量設定を反映する。
     */
    void updateCurrentBgmVolume();

private:
    /**
     * @brief 初期 BGM 状態で生成する。
     */
    AudioManager() = default;

    /**
     * @brief 管理中の BGM を停止し、必要なサウンド状態を破棄する。
     */
    ~AudioManager();

    /**
     * @brief シングルトンの複製を禁止する。
     */
    AudioManager(const AudioManager&) = delete;

    /**
     * @brief シングルトンの代入を禁止する。
     * @return 代入結果は使用しない。宣言は削除済み。
     */
    AudioManager& operator=(const AudioManager&) = delete;

    std::string mCurrentBgmPath; ///< 現在再生中の BGM パス。未再生時は空。
    int mCurrentBgmHandle = -1;  ///< 現在再生中の BGM ハンドル。-1 は未再生または無効状態。
};

#endif
