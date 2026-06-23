/**
 * @file
 * @brief audio_manager.cpp 実装を定義する。
 */

#include "audio_manager.h"
#include "game_settings.h"
#include "resource_manager.h"
#include "DxLib.h"

namespace
{
    int dxVolumeFromPercent(int master, int channel)
    {
        /// 設定値 0..100 同士を合成し、DxLib の 0..255 音量へ変換する。
        int volume = 255 * master * channel / 10000;
        if (volume < 0) return 0;
        if (volume > 255) return 255;
        return volume;
    }

    int currentBgmVolume()
    {
        GameSettings* settings = GameSettings::instance();
        return dxVolumeFromPercent(settings->getMasterVolume(), settings->getBgmVolume());
    }

    int currentSeVolume()
    {
        GameSettings* settings = GameSettings::instance();
        return dxVolumeFromPercent(settings->getMasterVolume(), settings->getSeVolume());
    }
}

AudioManager::~AudioManager()
{
    /// 再生中 BGM が残らないよう、終了時に停止しておく。
    stopBgm();
}

void AudioManager::playBgm(const std::string& path)
{
    if (path.empty())
    {
        return;
    }

    if (path == mCurrentBgmPath && mCurrentBgmHandle != -1)
    {
        /// 同じ BGM の再指定ではロードし直さず、音量だけ最新設定へ同期する。
        if (CheckSoundMem(mCurrentBgmHandle) == 1)
        {
            updateCurrentBgmVolume();
            return;
        }

        updateCurrentBgmVolume();
        PlaySoundMem(mCurrentBgmHandle, DX_PLAYTYPE_LOOP);
        return;
    }

    stopBgm();

    /// BGM は ResourceManager の通常サウンドキャッシュから取得してループ再生する。
    int handle = ResourceManager::instance()->getSound(path);
    if (handle == -1)
    {
        return;
    }

    mCurrentBgmPath = path;
    mCurrentBgmHandle = handle;
    updateCurrentBgmVolume();
    PlaySoundMem(mCurrentBgmHandle, DX_PLAYTYPE_LOOP);
}

void AudioManager::stopBgm()
{
    if (mCurrentBgmHandle != -1)
    {
        /// ハンドル自体は ResourceManager が所有するため、ここでは再生停止だけ行う。
        StopSoundMem(mCurrentBgmHandle);
    }

    mCurrentBgmPath.clear();
    mCurrentBgmHandle = -1;
}

void AudioManager::playSe(const std::string& path)
{
    if (path.empty())
    {
        return;
    }

    int handle = ResourceManager::instance()->getSound(path);
    if (handle == -1)
    {
        return;
    }

    /// SE は重ねて鳴らすため DX_PLAYTYPE_BACK を使う。
    ChangeVolumeSoundMem(currentSeVolume(), handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}


int AudioManager::playLoopSe(const std::string& path)
{
    if (path.empty())
    {
        return -1;
    }

    int handle = ResourceManager::instance()->getSound(path);
    if (handle == -1)
    {
        return -1;
    }

    /// ループ SE は停止用にハンドルを呼び出し側へ返す。
    ChangeVolumeSoundMem(currentSeVolume(), handle);
    PlaySoundMem(handle, DX_PLAYTYPE_LOOP);
    return handle;
}

void AudioManager::stopSe(int handle)
{
    if (handle != -1)
    {
        StopSoundMem(handle);
    }
}
void AudioManager::updateListener(VECTOR position, VECTOR frontPosition)
{
    /// 3D SE の定位は現在カメラ位置と注視点に合わせる。
    Set3DSoundListenerPosAndFrontPos_UpVecY(position, frontPosition);
}

void AudioManager::playSe3D(const std::string& path, VECTOR position, float radius)
{
    if (path.empty())
    {
        return;
    }

    int handle = ResourceManager::instance()->get3DSound(path);
    if (handle == -1)
    {
        return;
    }

    /// 3D SE は再生前に位置と聞こえる半径を設定する。
    ChangeVolumeSoundMem(currentSeVolume(), handle);
    Set3DPositionSoundMem(position, handle);
    Set3DRadiusSoundMem(radius, handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

void AudioManager::updateCurrentBgmVolume()
{
    if (mCurrentBgmHandle != -1)
    {
        /// 設定画面で音量を変更した場合、再生中 BGM へ即時反映する。
        ChangeVolumeSoundMem(currentBgmVolume(), mCurrentBgmHandle);
    }
}
