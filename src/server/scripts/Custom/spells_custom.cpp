#include "ScriptMgr.h"
#include "DBCStores.h"
#include "Item.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"

class spell_gen_aura : public SpellScript
{
    PrepareSpellScript(spell_gen_aura);

    SpellCastResult CheckRequirement()
    {
        Unit* caster = GetCaster();
        uint32 manaPercent = GetSpellInfo()->ManaCostPercentage;
        uint32 maxMana = caster->GetMaxPower(POWER_MANA);
        uint32 curMana = caster->GetPower(POWER_MANA);
        float percentCost = manaPercent / 100.0f;

        if (curMana < (percentCost * maxMana))
            return SPELL_FAILED_NO_POWER;
        else
            return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        uint32 manaPercent = GetSpellInfo()->ManaCostPercentage;
        uint32 maxMana = caster->GetMaxPower(POWER_MANA);
        uint32 baseCost = GetSpellInfo()->CalcPowerCost(caster, GetSpellInfo()->GetSchoolMask());
        float percentCost = manaPercent / 100.0f;
        float manaCostPercent = percentCost * maxMana;
        int32 remainder = manaCostPercent - baseCost;

        caster->ModifyPower(POWER_MANA, -remainder);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_gen_aura::CheckRequirement);
        OnEffectLaunch += SpellEffectFn(spell_gen_aura::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

class spell_gen_shield_sup_dummy : public AuraScript
{
    PrepareAuraScript(spell_gen_shield_sup_dummy);

    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return;

        Item const* mainHand = caster->GetWeaponForAttack(BASE_ATTACK, true);
        if (mainHand)
            return;

        Item* offHand = caster->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
        if (!offHand || offHand->GetTemplate()->InventoryType != INVTYPE_SHIELD)
            return;

        caster->AddAura(81001, caster);
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return;

        caster->RemoveAura(81001);
        caster->UpdateShieldBlockValue();
        caster->UpdateDamagePhysical(BASE_ATTACK);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_gen_shield_sup_dummy::AfterApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_gen_shield_sup_dummy::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_gen_shield_sup : public AuraScript
{
    PrepareAuraScript(spell_gen_shield_sup);

    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return;

        caster->UpdateShieldSuperiority();
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return;

        Item const* mainHand = caster->GetWeaponForAttack(BASE_ATTACK, true);
        if (!mainHand)
        {
            caster->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, BASE_MINDAMAGE);
            caster->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, BASE_MAXDAMAGE);
        }
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_gen_shield_sup::AfterApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_gen_shield_sup::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_firebrand_weapon : public AuraScript
{
    PrepareAuraScript(spell_firebrand_weapon);

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = eventInfo.GetActor();

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage() || !damageInfo->GetVictim())
            return;

        int32 bp = GetEffectInfo(EFFECT_0).CalcValue();
        CastSpellExtraArgs args(aurEff);
        args.AddSpellBP0(damageInfo->GetDamage() * bp / 100);
        caster->CastSpell(damageInfo->GetVictim(), 93006, args);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_firebrand_weapon::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_combustibolt : public AuraScript
{
    PrepareAuraScript(spell_combustibolt);

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = eventInfo.GetActor();

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage() || !damageInfo->GetVictim())
            return;

        int32 bp = GetEffectInfo(EFFECT_0).CalcValue();
        CastSpellExtraArgs args(aurEff);
        args.AddSpellBP0(damageInfo->GetDamage() * bp / 100);
        caster->CastSpell(damageInfo->GetVictim(), 93014, args);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_combustibolt::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_soothing_flame : public AuraScript
{
    PrepareAuraScript(spell_soothing_flame);

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = eventInfo.GetActor();

        HealInfo* healInfo = eventInfo.GetHealInfo();
        if (!healInfo || !healInfo->GetHeal() || !healInfo->GetTarget())
            return;

        int32 bp = GetEffectInfo(EFFECT_0).CalcValue();
        CastSpellExtraArgs args(aurEff);
        args.AddSpellBP0(healInfo->GetHeal() * bp / 300);
        caster->CastSpell(healInfo->GetTarget(), 93017, args);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_soothing_flame::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

void AddSC_Spells_Custom_()
{
    RegisterSpellScript(spell_gen_aura);
    RegisterSpellScript(spell_gen_shield_sup_dummy);
    RegisterSpellScript(spell_gen_shield_sup);
    RegisterSpellScript(spell_firebrand_weapon);
    RegisterSpellScript(spell_combustibolt);
    RegisterSpellScript(spell_soothing_flame);
}
