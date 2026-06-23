#ifndef _MODEL_RESOURCE_H_
#define _MODEL_RESOURCE_H_


/**
 * @file
 * @brief モデルファイルパスを定義する。
 */


/**
 * @brief モデルとアニメーションのファイルパスをまとめる namespace。
 */
namespace ModelResource
{

    /**
     * @brief Knight 用モデルとアニメーションのパス。
     */
    namespace Player
    {
        
        constexpr const char* Model =
            "Resource/Model/Player/knight/player.mv1";

        constexpr const char* Idle =
            "Resource/Model/Player/knight/player_idle.mv1";

       
        constexpr const char* Run =
            "Resource/Model/Player/knight/player_Walk.mv1";

      
        constexpr const char* WalkLeft =
            "Resource/Model/Player/knight/player_WalkLeft.mv1";

        
        constexpr const char* WalkRight =
            "Resource/Model/Player/knight/player_WalkRight.mv1";

        
        constexpr const char* WalkBack =
            "Resource/Model/Player/knight/player_WalkBack.mv1";

        
        constexpr const char* Attack1 =
            "Resource/Model/Player/knight/player_attack1.mv1";

        
        constexpr const char* Attack2 =
            "Resource/Model/Player/knight/player_attack2.mv1";

        
        constexpr const char* Attack3 =
            "Resource/Model/Player/knight/player_attack3.mv1";

        
        constexpr const char* Dodge =
            "Resource/Model/Player/knight/player_dodge.mv1";

        
        constexpr const char* Guard =
            "Resource/Model/Player/knight/player_guard.mv1";

        
        constexpr const char* GuardImpact =
            "Resource/Model/Player/knight/player_guardimpact.mv1";

       
        constexpr const char* Counter =
            "Resource/Model/Player/knight/player_counter.mv1";

        
        constexpr const char* Jump =
            "Resource/Model/Player/knight/player_jump.mv1";

        
        constexpr const char* Hit =
            "Resource/Model/Player/knight/player_hit.mv1";

        constexpr const char* KnockBack =
            "Resource/Model/Player/knight/player_KnockBack.mv1";

        
        constexpr const char* Dead =
            "Resource/Model/Player/knight/player_dead.mv1";

       
        constexpr const char* Sitting =
            "Resource/Model/Player/knight/player_sitting.mv1";

        
        constexpr const char* SitToStand =
            "Resource/Model/Player/knight/player_SitToStand.mv1";
    }


    /**
     * @brief Wizard 用モデルとアニメーションのパス。
     */
    namespace Wizard
    {
       
        constexpr const char* Model =
            "Resource/Model/Player/Wizard/wizard.mv1";

        
        constexpr const char* Idle =
            "Resource/Model/Player/Wizard/wizard_idle.mv1";

        
        constexpr const char* Walk =
            "Resource/Model/Player/Wizard/wizard_walk.mv1";

       
        constexpr const char* WalkLeft =
            "Resource/Model/Player/Wizard/wizard_WalkLeft.mv1";

       
        constexpr const char* WalkRight =
            "Resource/Model/Player/Wizard/wizard_WalkRight.mv1";

       
        constexpr const char* WalkBack =
            "Resource/Model/Player/Wizard/wizard_WalkBack.mv1";

        
        constexpr const char* MagicAttack1 =
            "Resource/Model/Player/Wizard/wizard_MagicAttack1.mv1";

        constexpr const char* MagicAttack2 =
            "Resource/Model/Player/Wizard/wizard_MagicAttack2.mv1";

        constexpr const char* AreaPowerUp =
            "Resource/Model/Player/Wizard/wizard_AreaPowerUp.mv1";

       
        constexpr const char* Dodge =
            "Resource/Model/Player/Wizard/wizard_dodge.mv1";

        
        constexpr const char* Hit =
            "Resource/Model/Player/Wizard/wizard_hit.mv1";

        
        constexpr const char* KnockBack =
            "Resource/Model/Player/Wizard/wizard_KnockBack.mv1";

        
        constexpr const char* Dead =
            "Resource/Model/Player/Wizard/wizard_dead.mv1";

        
        constexpr const char* Sitting =
            "Resource/Model/Player/Wizard/wizard_sitting.mv1";

        
        constexpr const char* SitToStand =
            "Resource/Model/Player/Wizard/wizard_SitToStand.mv1";
    }


  
    /**
     * @brief Mutant 系 Enemy 用モデルとアニメーションのパス。
     */
    namespace Enemy
    {
       
        constexpr const char* Model =
            "Resource/Model/Enemy/Mutant/enemy.mv1";

       
        constexpr const char* Idle =
            "Resource/Model/Enemy/Mutant/enemy_idle.mv1";

        
        constexpr const char* Walk =
            "Resource/Model/Enemy/Mutant/enemy_walk.mv1";

        constexpr const char* Attack =
            "Resource/Model/Enemy/Mutant/enemy_attack1.mv1";

       
        constexpr const char* Attack2 =
            "Resource/Model/Enemy/Mutant/enemy_attack2.mv1";

       
        constexpr const char* Attack3 =
            "Resource/Model/Enemy/Mutant/enemy_attack3.mv1";

        
        constexpr const char* Attack360 =
            "Resource/Model/Enemy/Mutant/enemy_Attack360.mv1";

        
        constexpr const char* Charging =
            "Resource/Model/Enemy/Mutant/enemy_charging.mv1";

        
        constexpr const char* Ram =
            "Resource/Model/Enemy/Mutant/enemy_Ram.mv1";

        
        constexpr const char* JumpAttack =
            "Resource/Model/Enemy/Mutant/enemy_JumpAttack.mv1";

      
        constexpr const char* Hit1 =
            "Resource/Model/Enemy/Mutant/enemy_hit1.mv1";

       
        constexpr const char* Hit2 =
            "Resource/Model/Enemy/Mutant/enemy_hit2.mv1";

        
        constexpr const char* Dying =
            "Resource/Model/Enemy/Mutant/enemy_dying.mv1";
    }

    /**
     * @brief Vampire 用モデルとアニメーションのパス。
     */
    namespace Vampire
    {
        
        constexpr const char* Model =
            "Resource/Model/Enemy/Vampire/vampire.mv1";

        
        constexpr const char* Idle =
            "Resource/Model/Enemy/Vampire/vampire_Idle.mv1";

       
        constexpr const char* Roaring =
            "Resource/Model/Enemy/Vampire/vampire_Roaring.mv1";

        
        constexpr const char* Hit =
            "Resource/Model/Enemy/Vampire/vampire_hit.mv1";

        
        constexpr const char* Dying =
            "Resource/Model/Enemy/Vampire/vampire_dying.mv1";
    }

    /**
     * @brief Stage 用モデルのパス。
     */
    namespace Stage
    {
        
        constexpr const char* Arena =
            "Resource/Model/Stage/Stage00.mv1";

        
        constexpr const char* ArenaCollision =
            "Resource/Model/Stage/Stage00_c.mv1";


        
        constexpr const char* MutantArena =
            "Resource/Model/Stage/hell-arena2/source/Hell arena.mv1";

        
        constexpr const char* MutantArenaCollision =
            "Resource/Model/Stage/hell-arena2/source/Hell arena_c.mv1";
       
        constexpr const char* Sky =
            "Resource/Model/Stage/Stage00_sky.mv1";

        
        constexpr const char* Object08 =
            "Resource/Model/Stage/Stage_Obj008.mv1";
    }
}

#endif // !_MODEL_RESOURCE_H_
