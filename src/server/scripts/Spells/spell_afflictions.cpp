/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

 /*
  * Scripts for spells with SPELLFAMILY_GENERIC which cannot be included in AI script file
  * of creature using it or can't be bound to any player class.
  * Ordered alphabetically using scriptname.
  * Scriptnames of files in this file should be prefixed with "spell_gen_"
  */

#include "ScriptMgr.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "InstanceScript.h"
#include "Item.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"

enum Afflictions
{
    AFFLICTION_BEES_AURA        = 82002,
    AFFLICTION_BEES_DAMAGE      = 82003,
};

class spell_aff_bees_dummy : public AuraScript
{
    PrepareAuraScript(spell_aff_bees_dummy);

    void UpdatePeriodic(AuraEffect* aurEff)
    {
        Player* caster = aurEff->GetCaster()->ToPlayer();
        if (!caster)
            return;

        bool isCombat = caster->IsInCombat();
        if (!isCombat)
            return;

        if (caster->GetSpellHistory()->HasCooldown(AFFLICTION_BEES_AURA))
            return;

        SpellInfo const* beesAura = sSpellMgr->GetSpellInfo(AFFLICTION_BEES_AURA);
        uint32 stackSize = beesAura->StackAmount;
        uint32 cooldown = urand(30, 300);

        if (frand(0.0f,1.0f) >= 0.4f)
        {
            caster->AddAura(AFFLICTION_BEES_AURA, caster);
            caster->SetAuraStack(AFFLICTION_BEES_AURA, caster, stackSize);
            caster->GetSpellHistory()->AddCooldown(AFFLICTION_BEES_AURA, 0, std::chrono::seconds(cooldown));
        }        
    }

    void Register() override
    {
        OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_aff_bees_dummy::UpdatePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

class spell_aff_bees_aura : public AuraScript
{
    PrepareAuraScript(spell_aff_bees_aura);

    void UpdatePeriodic(AuraEffect* aurEff)
    {
        Player* caster = aurEff->GetCaster()->ToPlayer();
        if (!caster)
            return;

        uint32 damage = (caster->GetMaxHealth() / 100) * aurEff->GetAmount(); // don't need to multiply by stack size since it's calculated in bp0 already
        CastSpellExtraArgs args(aurEff);
        args.AddSpellBP0(damage);
        caster->CastSpell(caster, AFFLICTION_BEES_DAMAGE, args);
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        uint32 schoolMask = eventInfo.GetSchoolMask();
        if (!spellInfo)
            return false;

        if (schoolMask != SPELL_SCHOOL_MASK_FIRE)
            return false;

        PreventDefaultAction();

        ModStackAmount(-1, AURA_REMOVE_BY_DEFAULT);
    }

    //void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    //{
    //
    //}

    void Register() override
    {
        OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_aff_bees_aura::UpdatePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        DoCheckProc += AuraCheckProcFn(spell_aff_bees_aura::CheckProc);
        //OnEffectRemove += AuraEffectRemoveFn(spell_aff_bees_aura::OnRemove, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_affliction_spell_scripts()
{
    RegisterSpellScript(spell_aff_bees_dummy);
    RegisterSpellScript(spell_aff_bees_aura);
}
