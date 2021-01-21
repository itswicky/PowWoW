#include "ScriptMgr.h"
#include "Containers.h"
#include "DBCStores.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.cpp"
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

//Spellscripts here to begin with spell_custom_

enum Spells
{
    FIXATE                  = 81043,
    GLYPH_OF_MIND_BLAST     = 81047,
    SPELL_MIND_WRACK        = 81048,
    SHADOW_BLADES           = 81049,
    SHADOW_BLADES_DAMAGE    = 81050,
    EARTHSTRIKE             = 81074,
    GIFT_OF_THE_EARTHMOTHER = 81085,
    DISCHARGE               = 81130,
    DUMMY_FINISHER          = 81152,
    GRONNS_POWER            = 81254,
    GRONNS_FURY             = 81255,
    SEAL_OF_SHADOWS         = 81259
};

class spell_custom_fixate : public SpellScriptLoader
{
public: spell_custom_fixate() : SpellScriptLoader("spell_custom_fixate") { }

        class spell_custom_fixate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_custom_fixate_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ FIXATE });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                GetHitUnit()->CastSpell(GetCaster(), FIXATE, true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_custom_fixate_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_custom_fixate_SpellScript();
        }
};

class spell_custom_glyph_of_mind_blast : public SpellScriptLoader
{
public:
    spell_custom_glyph_of_mind_blast() : SpellScriptLoader("spell_custom_glyph_of_mind_blast") { }

    class spell_custom_glyph_of_mind_blast_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_custom_glyph_of_mind_blast_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ GLYPH_OF_MIND_BLAST });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            Unit* caster = eventInfo.GetActor();
            Unit* target = eventInfo.GetProcTarget();

            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo || !damageInfo->GetDamage() || !damageInfo->GetVictim())
                return;

            int32 amount = CalculatePct(static_cast<int32>(damageInfo->GetDamage()), 50);
            // Divide damage into each tick of DoT effect
            amount /= 4;

            CastSpellExtraArgs args(aurEff);
            args.AddSpellBP0(amount);
            caster->CastSpell(target, SPELL_MIND_WRACK, args);

            //caster->CastSpell(SPELL_MIND_WRACK, SPELLVALUE_BASE_POINT0, amount, damageInfo->GetVictim(), true, nullptr, aurEff); commented outdated structure for reference
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_custom_glyph_of_mind_blast_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_custom_glyph_of_mind_blast_AuraScript();
    }
};

class spell_custom_shadow_blades : public SpellScriptLoader
{
public:
    spell_custom_shadow_blades() : SpellScriptLoader("spell_custom_shadow_blades") { }

    class spell_custom_shadow_blades_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_custom_shadow_blades_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SHADOW_BLADES });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            Unit* caster = eventInfo.GetActor();
            Unit* target = eventInfo.GetProcTarget();

            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo || !damageInfo->GetDamage() || !damageInfo->GetVictim())
                return;

            int32 amount = CalculatePct(static_cast<int32>(damageInfo->GetDamage()), 10);

            CastSpellExtraArgs args(aurEff);
            args.AddSpellBP0(amount);
            caster->CastSpell(target, SHADOW_BLADES_DAMAGE, args);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_custom_shadow_blades_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_custom_shadow_blades_AuraScript();
    }
};

class spell_custom_earthstrike : public SpellScriptLoader
{
public:
    spell_custom_earthstrike() : SpellScriptLoader("spell_custom_earthstrike") { }

    class spell_custom_earthstrike_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_custom_earthstrike_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) override
        {
            if (!sSpellMgr->GetSpellInfo(EARTHSTRIKE))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            GetCaster()->GetSpellHistory()->ModifyCooldown(17364 /* Stormstrike*/, -(2 * IN_MILLISECONDS));
        }

        void HandleEffect(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            Unit* target = GetHitUnit();
            if (!target)
                return;

            caster->CastSpell(target, 81075, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_custom_earthstrike_SpellScript::HandleDummy, EFFECT_2, SPELL_EFFECT_DUMMY);
            OnEffectHitTarget += SpellEffectFn(spell_custom_earthstrike_SpellScript::HandleEffect, EFFECT_1, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_custom_earthstrike_SpellScript();
    }
};

class spell_custom_gotem : public SpellScriptLoader
{
public:
    spell_custom_gotem() : SpellScriptLoader("spell_custom_gotem") { }

    class spell_custom_gotem_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_custom_gotem_AuraScript);

        bool Validate(SpellInfo const* /*spell*/) override
        {
            if (!sSpellMgr->GetSpellInfo(GIFT_OF_THE_EARTHMOTHER))
                return false;
            return true;
        }

        void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
        {
            isPeriodic = true;
            amplitude = 1 * IN_MILLISECONDS;
        }

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            PreventDefaultAction();
            if (!aurEff->GetAmount())
                return;

            Unit* caster = GetCaster();
            if (!caster)
                return;

            uint8 rank = GetSpellInfo()->GetRank();

            if (AuraEffect const* aurEff = caster->GetAuraEffectOfRankedSpell(GIFT_OF_THE_EARTHMOTHER, EFFECT_0))
                return;

            if (caster->GetAuraEffectOfRankedSpell(51179, EFFECT_2))
                caster->CastSpell(caster, sSpellMgr->GetSpellWithRank(GIFT_OF_THE_EARTHMOTHER, rank), true);
        }

        void HandleUpdatePeriodic(AuraEffect* aurEff)
        {
            aurEff->CalculatePeriodic(GetCaster());
        }

        void Register() override
        {
            DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_custom_gotem_AuraScript::CalcPeriodic, EFFECT_2, SPELL_AURA_DUMMY);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_custom_gotem_AuraScript::HandleDummyTick, EFFECT_2, SPELL_AURA_DUMMY);
            OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_custom_gotem_AuraScript::HandleUpdatePeriodic, EFFECT_2, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_custom_gotem_AuraScript();
    }
};

class spell_custom_discharge : public SpellScriptLoader
{
public:
    spell_custom_discharge() : SpellScriptLoader("spell_custom_discharge") { }

    class spell_custom_discharge_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_custom_discharge_SpellScript);

        SpellCastResult CheckCast()
        {
            Unit* caster = GetCaster();

            if (!caster->HasAura(81129))
            {
                GetCaster()->ToPlayer()->GetSession()->SendNotification("Requires at least 1 charge");
                return SPELL_FAILED_DONT_REPORT;
            }

            return SPELL_CAST_OK;
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            uint32 stacks = GetCaster()->GetAura(81129)->GetStackAmount();
            uint32 base = GetEffectValue();

            SetEffectValue(stacks * base);
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            GetCaster()->RemoveAura(81129);
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_custom_discharge_SpellScript::CheckCast);
            OnEffectLaunchTarget += SpellEffectFn(spell_custom_discharge_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            OnEffectHit += SpellEffectFn(spell_custom_discharge_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_custom_discharge_SpellScript();
    }
};

class spell_custom_dummy_finisher : public SpellScriptLoader
{
public:
    spell_custom_dummy_finisher() : SpellScriptLoader("spell_custom_dummy_finisher") { }

    class spell_custom_dummy_finisher_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_custom_dummy_finisher_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) override
        {
            if (!sSpellMgr->GetSpellInfo(DUMMY_FINISHER))
                return false;
            return true;
        }

        void HandleEffect(SpellEffIndex /*effIndex*/)
        {
            std::vector<uint32> spellList =
            {
                81153,               //Eviscerate
                81154,               //Rupture
                81155                //Slice and Dice
            };

            uint32 spellId = spellList[urand(0, spellList.size() - 1)];
            switch (spellId)
            {
            case 81155:
                GetCaster()->CastSpell(GetCaster(), spellId, true);
                break;
            default:
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, spellId, true);
                break;
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_custom_dummy_finisher_SpellScript::HandleEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_custom_dummy_finisher_SpellScript();
    }
};

class spell_custom_dragonslayers_skull : public SpellScriptLoader
{
public:
    spell_custom_dragonslayers_skull() : SpellScriptLoader("spell_custom_dragonslayers_skull") { }

    class spell_custom_dragonslayers_skull_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_custom_dragonslayers_skull_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ GRONNS_POWER });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            if (GetTarget()->HasAura(GRONNS_FURY)) // cant collect shards while under effect of Chaos Bane buff
                return false;
            return eventInfo.GetProcTarget() && eventInfo.GetProcTarget()->IsAlive();
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            GetTarget()->CastSpell(GetTarget(), GRONNS_POWER);

            // this can't be handled in AuraScript of SoulFragments because we need to know victim
            if (Aura* gronnsPower = GetTarget()->GetAura(GRONNS_POWER))
            {
                if (gronnsPower->GetStackAmount() >= 21)
                {
                    GetTarget()->CastSpell(eventInfo.GetProcTarget(), GRONNS_FURY, aurEff);
                    gronnsPower->Remove();
                }
            }
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            GetTarget()->RemoveAurasDueToSpell(GRONNS_POWER);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_custom_dragonslayers_skull_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_custom_dragonslayers_skull_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(spell_custom_dragonslayers_skull_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }

    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_custom_dragonslayers_skull_AuraScript();
    }
};

class spell_custom_seal_of_shadows : public SpellScriptLoader
{
public:
    spell_custom_seal_of_shadows() : SpellScriptLoader("spell_custom_seal_of_shadows") { }

    class spell_custom_seal_of_shadows_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_custom_seal_of_shadows_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SEAL_OF_SHADOWS });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            Unit* caster = eventInfo.GetActor();
            Unit* target = eventInfo.GetProcTarget();

            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo || !damageInfo->GetDamage() || !damageInfo->GetVictim())
                return;

            int32 amount = CalculatePct(static_cast<int32>(damageInfo->GetDamage()), 5);

            CastSpellExtraArgs args(aurEff);
            args.AddSpellBP0(amount);
            caster->CastSpell(target, SEAL_OF_SHADOWS, args);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_custom_seal_of_shadows_AuraScript::HandleProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_custom_seal_of_shadows_AuraScript();
    }
};

void AddSC_custom_spell_scripts()
{
    new spell_custom_fixate();
    new spell_custom_glyph_of_mind_blast();
    new spell_custom_shadow_blades();
    new spell_custom_earthstrike();
    new spell_custom_gotem();
    new spell_custom_discharge();
    new spell_custom_dummy_finisher();
    new spell_custom_dragonslayers_skull();
    new spell_custom_seal_of_shadows();
}
