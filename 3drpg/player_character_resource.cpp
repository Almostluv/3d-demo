/**
 * @file
 * @brief player_character_resource.cpp 実装を定義する。
 */

#include "player_character_resource.h"
#include "model_resource.h"

namespace
{
    constexpr const char* PlayerNormalMap =
        "Resource/Model/Player/knight/Paladin WProp J Nordstrom.fbm/Paladin_normal.png";

    constexpr const char* PlayerSpecularMap =
        "Resource/Model/Player/knight/Paladin WProp J Nordstrom.fbm/Paladin_specular.png";

    /// Knight は近接、防御、カウンターを持つため、専用の防御系アニメーションを割り当てる。
    constexpr PlayerCharacterResourceSet KnightResources = {
        ModelResource::Player::Model,
        ModelResource::Player::Idle,
        ModelResource::Player::Run,
        ModelResource::Player::WalkLeft,
        ModelResource::Player::WalkRight,
        ModelResource::Player::WalkBack,
        ModelResource::Player::Attack1,
        ModelResource::Player::Attack2,
        ModelResource::Player::Attack3,
        ModelResource::Player::Dodge,
        ModelResource::Player::Guard,
        ModelResource::Player::GuardImpact,
        ModelResource::Player::Counter,
        nullptr,
        ModelResource::Player::Hit,
        ModelResource::Player::KnockBack,
        ModelResource::Player::Dead,
        PlayerNormalMap,
        PlayerSpecularMap
    };

    /// Wizard は魔法攻撃中心のため、未使用の防御/カウンター枠は nullptr で明示する。
    constexpr PlayerCharacterResourceSet WizardResources = {
        ModelResource::Wizard::Model,
        ModelResource::Wizard::Idle,
        ModelResource::Wizard::Walk,
        ModelResource::Wizard::WalkLeft,
        ModelResource::Wizard::WalkRight,
        ModelResource::Wizard::WalkBack,
        ModelResource::Wizard::MagicAttack1,
        ModelResource::Wizard::MagicAttack2,
        ModelResource::Wizard::AreaPowerUp,
        ModelResource::Wizard::Dodge,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        ModelResource::Wizard::Hit,
        ModelResource::Wizard::KnockBack,
        ModelResource::Wizard::Dead,
        nullptr,
        nullptr
    };
}

const PlayerCharacterResourceSet& getPlayerCharacterResourceSet(
    PlayerCharacterType characterType
)
{
    /// Player 生成時にキャラクター種別から参照する静的リソースセットを選ぶ。
    return characterType == PlayerCharacterType::Wizard
        ? WizardResources
        : KnightResources;
}
