/**
 * @file
 * @brief resource_manager.cpp 実装を定義する。
 */

#include "resource_manager.h"
#include "DxLib.h"
#include <Windows.h>

namespace
{
    bool isAbsolutePath(const std::string& path)
    {
        /// Windows のドライブ指定と UNC パスを絶対パスとして扱う。
        return (path.size() >= 2 && path[1] == ':') ||
            (path.size() >= 2 && path[0] == '\\' && path[1] == '\\');
    }

    bool fileExists(const std::string& path)
    {
        return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
    }

    std::string parentPath(const std::string& path)
    {
        /// 実行ファイル位置から相対探索するため、パス文字列の親だけを取り出す。
        size_t slash = path.find_last_of("\\/");
        return slash == std::string::npos ? std::string() : path.substr(0, slash);
    }

    std::string joinPath(const std::string& base, const std::string& path)
    {
        if (base.empty())
        {
            return path;
        }

        char last = base[base.size() - 1];
        if (last == '\\' || last == '/')
        {
            /// base 側が区切り文字で終わる場合は、そのまま連結して重複区切りを避ける。
            return base + path;
        }

        return base + "\\" + path;
    }

    std::string getExeDir()
    {
        char path[MAX_PATH] = {};
        /// 実行ファイルの場所を基準に、Visual Studio 実行時と配布時の両方でリソースを探す。
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return parentPath(path);
    }

    std::string resolveResourcePath(const std::string& path)
    {
        if (isAbsolutePath(path) || fileExists(path))
        {
            return path;
        }

        std::string exeDir = getExeDir();
        std::string candidate = joinPath(exeDir, path);
        if (fileExists(candidate))
        {
            /// exe と同じ階層から起動された配布配置を優先する。
            return candidate;
        }

        candidate = joinPath(exeDir, "..\\..\\3drpg\\" + path);
        if (fileExists(candidate))
        {
            /// Visual Studio の x64/Debug などから実行した場合のプロジェクト内 Resource を探す。
            return candidate;
        }

        candidate = joinPath(exeDir, "..\\..\\" + path);
        if (fileExists(candidate))
        {
            /// 作業ディレクトリ構造が浅い場合の相対パス候補も確認する。
            return candidate;
        }

        return path;
    }
}

std::string ResourceManager::resolvePath(const std::string& path)
{
    return resolveResourcePath(path);
}
ResourceManager::~ResourceManager()
{
    clearAll();
}

int ResourceManager::getModel(const std::string& path)
{
    std::string resolvedPath = resolvePath(path);
    auto it = mModels.find(resolvedPath);

    if (it != mModels.end())
    {
        /// 同じモデルは再ロードせず、共有元モデルハンドルを返す。
        return it->second;
    }

    int handle = MV1LoadModel(resolvedPath.c_str());

    if (handle != -1)
    {
        mModels[resolvedPath] = handle;
    }

    return handle;
}

int ResourceManager::createModelInstance(const std::string& path)
{
    int sourceHandle = getModel(path);

    if (sourceHandle == -1)
    {
        return -1;
    }

    /// 個別に位置やアニメーションを変えるモデルは、共有元から複製して使う。
    return MV1DuplicateModel(sourceHandle);
}

int ResourceManager::getGraph(std::string path)
{
    std::string resolvedPath = resolvePath(path);

    if (mGraphs.count(resolvedPath) > 0)
    {
        /// 画像リソースはパス解決後のキーでキャッシュする。
        return mGraphs[resolvedPath];
    }

    int handle = LoadGraph(resolvedPath.c_str());

    if (handle != -1)
    {
        mGraphs[resolvedPath] = handle;
    }

    return handle;
}

int ResourceManager::getSound(std::string path)
{
    std::string resolvedPath = resolvePath(path);

    if (mSounds.count(resolvedPath) > 0)
    {
        /// 通常 SE/BGM は 2D サウンドとしてキャッシュする。
        return mSounds[resolvedPath];
    }

    int handle = LoadSoundMem(resolvedPath.c_str());

    if (handle != -1)
    {
        mSounds[resolvedPath] = handle;
    }

    return handle;
}

int ResourceManager::get3DSound(std::string path)
{
    std::string resolvedPath = resolvePath(path);

    if (m3DSounds.count(resolvedPath) > 0)
    {
        /// 3D SE は通常 SE と生成フラグが異なるため、別キャッシュで管理する。
        return m3DSounds[resolvedPath];
    }

    /// DxLib はロード時のフラグで 3D サウンド化するため、ロード直前だけ有効にする。
    SetCreate3DSoundFlag(TRUE);
    int handle = LoadSoundMem(resolvedPath.c_str());
    SetCreate3DSoundFlag(FALSE);

    if (handle != -1)
    {
        m3DSounds[resolvedPath] = handle;
    }

    return handle;
}
void ResourceManager::clearAll()
{
    /// DxLib の各リソース種別に対応する削除 API で、保持中のハンドルをまとめて解放する。
    for (auto& pair : mModels)
    {
        MV1DeleteModel(pair.second);
    }

    mModels.clear();

    for (auto& pair : mGraphs)
    {
        DeleteGraph(pair.second);
    }

    mGraphs.clear();

    for (auto& pair : mSounds)
    {
        DeleteSoundMem(pair.second);
    }

    mSounds.clear();

    for (auto& pair : m3DSounds)
    {
        DeleteSoundMem(pair.second);
    }

    m3DSounds.clear();
}
