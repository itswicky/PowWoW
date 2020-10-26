#include "ScriptMgr.h"
#include "Containers.h"
#include "DBCStores.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerAI.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Unit.h"

enum Spells
{
    PHOENIX_FLAMES  = 78722,
    RHONINS_UNSTABLE_BINDINGS   = 81004
};

class spell_leg_phoenix_flames_vis : public SpellScriptLoader
{
public:
    spell_leg_phoenix_flames_vis() : SpellScriptLoader("spell_leg_phoenix_flames_vis") { }

    class spell_leg_phoenix_flames_vis_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_leg_phoenix_flames_vis_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({PHOENIX_FLAMES});
        }

        void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
        {
            isPeriodic = true;
            amplitude = 1 * IN_MILLISECONDS;
        }

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            Aura * phoenixFlames = caster->GetAura(81001);
            int32 stacks = phoenixFlames->GetStackAmount();

            if (stacks == 25)
                caster->AddAura(81002, caster);
        }

        void HandleUpdatePeriodic(AuraEffect* aurEff)
        {
            aurEff->CalculatePeriodic(GetCaster());
        }

        void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            caster->RemoveAura(81002);
        }

        void Register() override
        {
            DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_leg_phoenix_flames_vis_AuraScript::CalcPeriodic, EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_leg_phoenix_flames_vis_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER);
            OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_leg_phoenix_flames_vis_AuraScript::HandleUpdatePeriodic, EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER);
            AfterEffectRemove += AuraEffectRemoveFn(spell_leg_phoenix_flames_vis_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_leg_phoenix_flames_vis_AuraScript();
    }
};

class spell_leg_unstable_bindings : public SpellScriptLoader
{
public:
    spell_leg_unstable_bindings() : SpellScriptLoader("spell_leg_unstable_bindings") { }

    class spell_leg_unstable_bindings_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_leg_unstable_bindings_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ RHONINS_UNSTABLE_BINDINGS });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            int32 cd = sSpellMgr->AssertSpellInfo(RHONINS_UNSTABLE_BINDINGS)->Effects[EFFECT_2].CalcValue();

            if ((eventInfo.GetHitMask() & PROC_HIT_CRITICAL))
            {
                cd *= 2;

                eventInfo.GetActor()->GetSpellHistory()->ModifyCooldown(11129, -(cd * IN_MILLISECONDS));

                return true;
            }

            eventInfo.GetActor()->GetSpellHistory()->ModifyCooldown(11129, -(cd * IN_MILLISECONDS));

            return true;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_leg_unstable_bindings_AuraScript::CheckProc);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_leg_unstable_bindings_AuraScript();
    }
};

void AddSC_legendary_spell_scripts()
{
    new spell_leg_phoenix_flames_vis();
    new spell_leg_unstable_bindings();
}
