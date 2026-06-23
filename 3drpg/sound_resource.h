#ifndef _SOUND_RESOURCE_H_
#define _SOUND_RESOURCE_H_


/**
 * @file
 * @brief サウンドファイルパスを定義する。
 */


/**
 * @brief BGM と SE のファイルパスをまとめる namespace。
 */
namespace SoundResource
{
    /**
     * @brief BGM リソースパス。
     */
    namespace Bgm
    {
        
        constexpr const char* TitleScene = "Resource/Sound/BGM/BGM_TitleScene.mp3";
        
        constexpr const char* MutantScene = "Resource/Sound/BGM/BGM_MutantScene.mp3";
        
        constexpr const char* VampireScene = "Resource/Sound/BGM/BGM_VampireScene.mp3";
        
        constexpr const char* Defeat = "Resource/Sound/BGM/BGM_Defeat.mp3";
    }

    /**
     * @brief SE リソースパス。
     */
    namespace Se
    {
        
        constexpr const char* Confirm = "Resource/Sound/SE/System/UI_Confirm.mp3";
        
        constexpr const char* Cancel = "Resource/Sound/SE/System/UI_Cancel.mp3";
       
        constexpr const char* Teleport = "Resource/Sound/SE/System/Teleport.mp3";
        
        constexpr const char* ValueChange = "Resource/Sound/SE/System/UI_ValueChange.mp3";
        
        constexpr const char* CharacterSelect = "Resource/Sound/SE/System/UI_CharacterSelect.mp3";

        /**
         * @brief 敵用 SE リソースパス。
         */
        namespace Enemy
        {
            
            constexpr const char* MutantDashFootstep = "Resource/Sound/SE/Enemy/Mutant_DashFootstep.mp3";
            
            constexpr const char* MutantDashRoar = "Resource/Sound/SE/Enemy/Mutant_DashRoar.mp3";
           
            constexpr const char* MutantJumpAttackLand = "Resource/Sound/SE/Enemy/Mutant_JumpAttackLand.mp3";
            
            constexpr const char* VampireOpening = "Resource/Sound/SE/Enemy/Vampire_Opening.mp3";
        }

        /**
         * @brief キャラクター用 SE リソースパス。
         */
        namespace Chara
        {
            
            constexpr const char* CharacterHit = "Resource/Sound/SE/Chara/Character_Hit.mp3";
            
            constexpr const char* KnightGuard = "Resource/Sound/SE/Chara/Knight_Guard.mp3";
            
            constexpr const char* KnightParry = "Resource/Sound/SE/Chara/Knight_Parry.mp3";
            
            constexpr const char* KnightParrySuccess = "Resource/Sound/SE/Chara/Knight_ParrySuccess.mp3";
            
            constexpr const char* KnightRolling = "Resource/Sound/SE/Chara/Knight_Rolling.mp3";
          
            constexpr const char* KnightSlash = "Resource/Sound/SE/Chara/Knight_Slash.mp3";
            
            constexpr const char* KnightSlashHitFlesh = "Resource/Sound/SE/Chara/Knight_SlashHitFlesh.mp3";
           
            constexpr const char* KnightSlashHitShield = "Resource/Sound/SE/Chara/Knight_SlashHitShield.mp3";
            
            constexpr const char* KnightWalking = "Resource/Sound/SE/Chara/Knight_Walking.mp3";
           
            constexpr const char* WizardBeamCharging = "Resource/Sound/SE/Chara/Wizard_BeamCharging.mp3";
            
            constexpr const char* WizardBeamEnd = "Resource/Sound/SE/Chara/Wizard_BeamEnd.mp3";
            
            constexpr const char* WizardBeamFire = "Resource/Sound/SE/Chara/Wizard_BeamFire.mp3";
          
            constexpr const char* WizardBeamLoop = "Resource/Sound/SE/Chara/Wizard_BeamLoop.mp3";
            
            constexpr const char* WizardMagicCircle = "Resource/Sound/SE/Chara/Wizard_MagicCircle.mp3";
            
            constexpr const char* WizardMagicMissile = "Resource/Sound/SE/Chara/Wizard_MagicMissile.mp3";
           
            constexpr const char* WizardRolling = "Resource/Sound/SE/Chara/Wizard_Rolling.mp3";
        }
    }
}

#endif // !_SOUND_RESOURCE_H_
