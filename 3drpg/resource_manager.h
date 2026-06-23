#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_


/**
 * @file
 * @brief モデル、画像、サウンドの読み込みとキャッシュを管理する ResourceManager を定義する。
 */


#include "manager.h"
#include <map>
#include <string>

/**
 * @brief モデル、画像、サウンドをパス単位で読み込み、DxLib ハンドルをキャッシュする管理クラス。
 *
 * 共有リソースのハンドルは ResourceManager が管理する。`createModelInstance()` は
 * キャッシュ済みモデルを複製して返すため、呼び出し側が複製ハンドルを解放する必要がある。
 */
class ResourceManager : public Manager<ResourceManager> {

    /**
     * @brief Manager から private コンストラクタへアクセスさせる。
     */
    friend class Manager<ResourceManager>;

public:
    /**
     * @brief リソースパスを実行ファイル基準の実パスへ解決する。
     * @param[in] path 解決したい相対または絶対パス。
     * @return 解決後のパス文字列。
     */
    std::string resolvePath(const std::string& path);

    /**
     * @brief モデルを取得する。未読み込みの場合は読み込んでキャッシュする。
     * @param[in] path モデルリソースのパス。
     * @return DxLib モデルハンドル。
     */
    int getModel(const std::string& path);

    /**
     * @brief キャッシュ済みモデルから独立したモデルインスタンスを作成する。
     * @param[in] path 複製元モデルリソースのパス。
     * @return 複製された DxLib モデルハンドル。呼び出し側が解放する。
     */
    int createModelInstance(const std::string& path);

    /**
     * @brief 画像を取得する。未読み込みの場合は読み込んでキャッシュする。
     * @param[in] path 画像リソースのパス。
     * @return DxLib グラフィックハンドル。
     */
    int getGraph(std::string path);

    /**
     * @brief 2D サウンドを取得する。未読み込みの場合は読み込んでキャッシュする。
     * @param[in] path サウンドリソースのパス。
     * @return DxLib サウンドハンドル。
     */
    int getSound(std::string path);

    /**
     * @brief 3D サウンドを取得する。未読み込みの場合は読み込んでキャッシュする。
     * @param[in] path サウンドリソースのパス。
     * @return DxLib サウンドハンドル。
     */
    int get3DSound(std::string path);

    /**
     * @brief キャッシュ中のすべてのリソースハンドルを解放する。
     */
    void clearAll();

private:
    /**
     * @brief 空のキャッシュで生成する。
     */
    ResourceManager() = default;

    /**
     * @brief キャッシュ中のリソースを解放する。
     */
    ~ResourceManager();

    /**
     * @brief シングルトンの複製を禁止する。
     */
    ResourceManager(const ResourceManager&) = delete;

    /**
     * @brief シングルトンの代入を禁止する。
     * @return 代入結果は使用しない。宣言は削除済み。
     */
    ResourceManager& operator=(const ResourceManager&) = delete;

    std::map<std::string, int> mModels;   ///< パス別にキャッシュした共有モデルハンドル。
    std::map<std::string, int> mGraphs;   ///< パス別にキャッシュした画像ハンドル。
    std::map<std::string, int> mSounds;   ///< パス別にキャッシュした 2D サウンドハンドル。
    std::map<std::string, int> m3DSounds; ///< パス別にキャッシュした 3D サウンドハンドル。
};

#endif
