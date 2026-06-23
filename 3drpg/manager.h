#ifndef _MANAGER_H_
#define _MANAGER_H_


/**
 * @file
 * @brief CRTP 形式のシングルトン管理クラスを定義する。
 */


/**
 * @brief 管理系クラスへ `instance()` アクセスを提供する CRTP 基底クラス。
 * @tparam T 派生側の管理クラス型。
 *
 * 各 `T` につき関数ローカル static のインスタンスを 1 つ保持する。
 * コピー代入は禁止されており、所有権は呼び出し側へ渡さない。
 *
 * @note インスタンス生成タイミングは最初の `instance()` 呼び出し時。
 * @todo Verify: ゲーム全体での破棄順序に依存する管理クラスがないか確認する。
 */
template<typename T>
class Manager
{
public:

    
    /**
     * @brief 管理クラスの共有インスタンスを取得する。
     * @return `T` 型インスタンスへの非所有ポインタ。nullptr は返さない。
     */
    static T* instance()
    {
        static T instance;
        return &instance;
    }

protected:

    /**
     * @brief 派生クラスからのみ生成できるようにする。
     */
    Manager() = default;

    /**
     * @brief 派生クラスのライフタイム終了時にのみ破棄される。
     */
    ~Manager() = default;

    /**
     * @brief シングルトンの複製を禁止する。
     */
    Manager(const Manager&) = delete;

    /**
     * @brief シングルトンの代入による複製を禁止する。
     * @return 代入結果は使用しない。宣言は削除済み。
     */
    Manager& operator=(const Manager&) = delete;
};

#endif // !_MANAGER_H_
