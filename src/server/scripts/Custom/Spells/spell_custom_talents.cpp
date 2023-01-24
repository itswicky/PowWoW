#include "ScriptMgr.h"
#include "DBCStores.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Item.h"

class spell_talent_combulstibolt_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_combulstibolt_aura);

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        auto caster = GetCaster();
        auto target = eventInfo.GetProcTarget();
        if (!caster || !target || !eventInfo.GetDamageInfo())
            return;
        auto damage = eventInfo.GetDamageInfo()->GetDamage();
        auto bonusFire = caster->GetBonusSchoolModifierPct(SPELL_SCHOOL_FIRE);
        damage = damage * (bonusFire / 100);
        CastSpellExtraArgs args;
        args.AddSpellBP0(damage);
        target->CastSpell(target, 180186, args);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_combulstibolt_aura::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_talent_burningarmor_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_burningarmor_aura);

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        auto caster = GetCaster();
        auto target = eventInfo.GetProcTarget();
        if (!caster || !target)
            return;
        /*
        auto damage = eventInfo.GetDamageInfo()->GetDamage();
        auto bonusFire = caster->GetBonusSchoolModifierPct(SPELL_SCHOOL_FIRE);
        damage = damage * (bonusFire / 100);
        */
        int32 damage = caster->GetArmor() * 0.03;
        CastSpellExtraArgs args;
        args.AddSpellBP0(damage);
        caster->CastSpell(target, 180188, args);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_burningarmor_aura::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_talent_engulf_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_engulf_aura);

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        auto caster = GetCaster();
        auto target = eventInfo.GetProcTarget();
        PreventDefaultAction();
        if (!caster || !target)
            return;
        caster->CastSpell(target, 180193, true);
        // 180193 Engulfing Flames
        if (target->HasAura(180193))
        {
            auto aura = target->GetAura(180193);
            if (aura && aura->GetStackAmount() == 10)
            {
                // Spread 180195 Engulfing Flames (triggers 180193 on nearby ally)
                target->CastSpell(target, 180195, true);
            }
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_engulf_aura::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_talent_engulfing_flames_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_engulfing_flames_aura);

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        auto target = GetTarget();
        if (!target)
            return;
        // 180193 Engulfing Flames
        if (target->HasAura(180193))
        {
            if (target->GetAura(180193)->GetStackAmount() >= 10)
            {
                // Engulf
                target->CastSpell(target, 180194, true);
            }
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_talent_engulfing_flames_aura::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_CHANGE_AMOUNT);
        OnEffectApply += AuraEffectApplyFn(spell_talent_engulfing_flames_aura::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAPPLY);
    }
};

class spell_talent_fire_ward_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_fire_ward_aura);

    void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        auto caster = GetCaster();
        if (!caster || !caster->ToPlayer())
            return;
        // 180172 Heated Temperament
        if (caster->HasAura(180172))
        {
            auto player = caster->ToPlayer();
            auto points = caster->GetComboPoints(caster->GetComboTargetGUID());
            if (points > 0)
            {
                PreventDefaultAction();
                player->ClearComboPoints();
                CastSpellExtraArgs args;
                args.AddSpellBP0((player->GetMaxHealth() * 0.01) * points);
                // Magic Ward any magic school, based on max hp
                player->CastSpell(player, 180198, args);
                auto aura = aurEff->GetBase();
                if (aura)
                {
                    aura->Remove();
                }
            }
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_talent_fire_ward_aura::OnApply, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

// 180199 From The Ashes
class spell_talent_from_the_ashes_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_from_the_ashes_aura);

    void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        auto caster = GetCaster();
        auto target = GetTarget();
        if (!target || !caster || target == caster)
            return;
        if (target->getDeathState() == JUST_DIED)
        {
            // Pile of Ash cannot be affected by Solar Flare
            if (target->ToCreature() && target->ToCreature()->GetEntry() == 52206)
                return;
            // Resurrection
            if (target->IsPlayer() && caster->IsFriendlyTo(target))
            {
                auto summon = target->SummonCreature(52206, target->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                if (summon)
                {
                    target->CastSpell(summon, 180200);
                }
            }
            // Phoenix
            else
            {
                auto summon = target->SummonCreature(52206, target->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                if (summon)
                {
                    caster->CastSpell(summon, 180201);
                }
            }
        }
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_talent_from_the_ashes_aura::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 180201 From The Ashes (Guardian Phoenix)
class spell_talent_from_the_ashes_phoenix_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_from_the_ashes_phoenix_aura);

    void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        auto caster = GetCaster();
        auto target = GetTarget();
        if (!target || !caster || target == caster)
            return;
        if (target->getDeathState() == JUST_DIED)
        {

            /*auto summon = caster->SummonCreature(52207, target->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
            if (summon)
            {

            }*/
            // Summon Phoenix
            caster->CastSpell(target, 180203);
        }
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_talent_from_the_ashes_phoenix_aura::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 180200 From The Ashes (Resurrection)
class spell_talent_from_the_ashes_resurrection_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_from_the_ashes_resurrection_aura);

    void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        auto caster = GetCaster();
        auto target = GetTarget();
        if (!target || !caster || target == caster)
            return;
        if (target->getDeathState() == JUST_DIED)
        {
            // 180204 Solar Flare Immune (5min debuff after ress)
            if (caster->isDead() && caster->ToPlayer() && !caster->HasAura(180204))
            {
                caster->ToPlayer()->ResurrectPlayer(0.2f);
                caster->CastSpell(caster, 24171);
                caster->CastSpell(caster, 180204);
            }
        }
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_talent_from_the_ashes_resurrection_aura::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_talent_icy_veins_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_icy_veins_aura);

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        auto caster = GetCaster();
        if (!caster || !caster->ToPlayer())
            return;
        // 180205 Winter's Mercy
        if (caster->HasAura(180205))
        {
            auto player = caster->ToPlayer();
            auto points = caster->GetComboPoints(caster->GetComboTargetGUID());
            if (points > 0)
            {
                player->ClearComboPoints();
                CastSpellExtraArgs args1;
                args1.AddSpellBP0(20 + (5 * points));
                // Icy Veins bonus speed bonus
                player->CastSpell(player, 180207, args1);
                CastSpellExtraArgs args2;
                args2.AddSpellBP0(5 * points);
                // Icy Veins damage taken bonus
                player->CastSpell(player, 180208, args1);
            }
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_talent_icy_veins_aura::OnApply, EFFECT_0, SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

// 180216 Heart of the Glacier
class spell_talent_heart_glacier_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_heart_glacier_aura);

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        auto caster = GetCaster();
        auto target = eventInfo.GetProcTarget();
        if (!caster || !target)
            return;
        // 180217 Frost Chill (up to 4 targets in area)
        caster->CastSpell(target, 180217);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_heart_glacier_aura::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 180219 Polar Affliction
class spell_talent_polar_affliction_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_polar_affliction_aura);

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        auto caster = GetCaster();
        auto target = eventInfo.GetProcTarget();
        if (!caster || !target || !caster->ToPlayer())
            return;
        // Calculate whether the target has a Frost debuff
        bool hasFrostDebuff = false;
        auto& targetAuras = target->GetAppliedAuras();
        for (auto itr = targetAuras.begin(); itr != targetAuras.end(); itr++)
        {
            auto aura = itr->second;
            auto base = aura->GetBase();
            if (!aura->IsPositive() && base->GetSpellInfo() && base->GetSpellInfo()->GetSchoolMask() & SPELL_SCHOOL_MASK_FROST)
            {
                hasFrostDebuff = true;
                break;
            }
        }
        // Do nothing if no Frost debuff
        if (!hasFrostDebuff)
            return;
        // If immune to Freeze/Stun effects, increase Frost damage taken instead
        if (target->IsImmunedToSpellEffect(sSpellMgr->GetSpellInfo(180230), 0, caster) ||
            ((target->GetMechanicImmunityMask() & MECHANIC_STUN) > 0) ||
            ((target->GetMechanicImmunityMask() & MECHANIC_FREEZE) > 0))
        {
            CastSpellExtraArgs args;
            // (1000 + spellPower) * 0.25
            args.AddSpellBP0((1000 + caster->ToPlayer()->GetBaseSpellPowerBonus()) * 0.25);
            caster->CastSpell(target, 180231, args);
        }
        // Freeze (stun) the target and increase Frost crit damage
        else
        {
            caster->CastSpell(target, 180230);
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_polar_affliction_aura::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 180232 Hammer of the North
class spell_talent_hammer_of_the_north_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_hammer_of_the_north_aura);

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        auto caster = GetCaster();
        auto target = eventInfo.GetProcTarget();
        if (!caster || !target || !caster->ToPlayer())
            return;
        auto player = caster->ToPlayer();
        if (Item* mainItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
        {
            player->ApplyEnchantment(mainItem, TEMP_ENCHANTMENT_SLOT, false);
            mainItem->SetEnchantment(TEMP_ENCHANTMENT_SLOT, 2500, 5000, 0, caster->GetGUID());
            player->ApplyEnchantment(mainItem, TEMP_ENCHANTMENT_SLOT, true);
        }
        if (Item* offHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
        {
            player->ApplyEnchantment(offHand, TEMP_ENCHANTMENT_SLOT, false);
            offHand->SetEnchantment(TEMP_ENCHANTMENT_SLOT, 2500, 5000, 0, caster->GetGUID());
            player->ApplyEnchantment(offHand, TEMP_ENCHANTMENT_SLOT, true);
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_hammer_of_the_north_aura::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_talent_congelation_trigger_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_congelation_trigger_aura);

    void OnProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        auto caster = GetCaster();
        if (!caster)
        {
            PreventDefaultAction();
            return;
        }
        if (caster->GetHealthPct() > 35)
        {
            PreventDefaultAction();
        }
        // Trigger
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_congelation_trigger_aura::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_talent_congelation_actual_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_congelation_actual_aura);

    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        if (GetCaster() && GetCaster()->GetHealthPct() > 35)
        {
            auto base = aurEff->GetBase();
            if (base)
            {
                base->Remove();
            }
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_talent_congelation_actual_aura::PeriodicTick, EFFECT_2, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class spell_talent_frozenheart_trigger_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_frozenheart_trigger_aura);

    void OnProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        auto caster = GetCaster();
        if (!caster)
        {
            PreventDefaultAction();
            return;
        }
        if (caster->GetHealthPct() <= 75)
        {
            PreventDefaultAction();
        }
        // Trigger
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_frozenheart_trigger_aura::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_talent_frozenheart_actual_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_frozenheart_actual_aura);

    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        if (GetCaster() && GetCaster()->GetHealthPct() <= 75)
        {
            auto base = aurEff->GetBase();
            if (base)
            {
                base->Remove();
            }
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_talent_frozenheart_actual_aura::PeriodicTick, EFFECT_2, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class spell_talent_coldtempered_aura : public AuraScript
{
    PrepareAuraScript(spell_talent_coldtempered_aura);

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        auto base = aurEff->GetBase();
        if (base)
            base->Remove();
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_talent_coldtempered_aura::OnProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_cold_steel_aura : public SpellScriptLoader
{
public:
    spell_cold_steel_aura() : SpellScriptLoader("spell_cold_steel_aura") { }

    class spell_cold_steel_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_cold_steel_aura_AuraScript);


        bool CheckProc(ProcEventInfo& eventInfo)
        {
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo)
                return false;

            if (eventInfo.GetActor()->GetTypeId() != TYPEID_PLAYER)
                return false;

            Player* player = eventInfo.GetActor()->ToPlayer();
            if (Item* weapon = player->GetWeaponForAttack(eventInfo.GetDamageInfo()->GetAttackType()))
                if (weapon->GetTemplate()->SubClass != ITEM_SUBCLASS_WEAPON_SWORD)
                    return false;
                else
                    return true;

            return false;
        }

        void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* actor = eventInfo.GetActor();
            float damage = 0.f;

            if (eventInfo.GetDamageInfo()->GetAttackType() == OFF_ATTACK)
                damage = (actor->GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE) + actor->GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE)) / 2.f;
            else
                damage = (actor->GetFloatValue(UNIT_FIELD_MINDAMAGE) + actor->GetFloatValue(UNIT_FIELD_MAXDAMAGE)) / 2.f;

            CastSpellExtraArgs args(aurEff);
            args.AddSpellBP0(damage * 0.05);
            actor->CastSpell(eventInfo.GetProcTarget(), GetSpellInfo()->Effects[EFFECT_0].TriggerSpell, args);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_cold_steel_aura_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_cold_steel_aura_AuraScript::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_cold_steel_aura_AuraScript();
    }
};

class spell_glaciation_aura : public AuraScript
{
    PrepareAuraScript(spell_glaciation_aura);


    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetHealInfo() && eventInfo.GetHealInfo()->GetEffectiveHeal() > 0;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        HealInfo* healInfo = eventInfo.GetHealInfo();
        if (!healInfo || !healInfo->GetHeal())
            return;
        // Non stacking.
        if (eventInfo.GetProcTarget()->HasAura(180252, eventInfo.GetActor()->GetGUID()))
            return;

        int32 absorb = int32(CalculatePct(healInfo->GetHeal(), 20.0f));

        CastSpellExtraArgs args(aurEff);
        args.AddSpellBP0(absorb);
        eventInfo.GetActor()->CastSpell(eventInfo.GetProcTarget(), 180252, args);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_glaciation_aura::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_glaciation_aura::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_frostfire_bolt_combo_spender : public SpellScript
{
    PrepareSpellScript(spell_frostfire_bolt_combo_spender);

    void OnHit()
    {
        if (!GetCaster()->HasAura(SPELL_ICY_HOT))
            return;
        uint8 comboPoints = GetCaster()->GetComboPoints(GetHitUnit());
        if (!comboPoints)
            return;
        uint32 slowAmount = comboPoints * 5;
        uint32 damageAmount = GetHitDamage() * (comboPoints * 0.05);
        CastSpellExtraArgs args(TRIGGERED_FULL_MASK);
        args.AddSpellBP0(-slowAmount);
        args.AddSpellBP1(damageAmount);

        GetCaster()->CastSpell(GetHitUnit(), 180254, args);
        GetCaster()->ClearComboPoints();
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_frostfire_bolt_combo_spender::OnHit);
    }
};


class spell_ice_barrier_combo_spender : public SpellScript
{
    PrepareSpellScript(spell_ice_barrier_combo_spender);

    void OnHit()
    {
        if (!GetCaster()->HasAura(180205))
            return;

        uint8 comboPoints = GetCaster()->GetComboPoints();
        if (!comboPoints)
            return;

        CastSpellExtraArgs args(TRIGGERED_FULL_MASK);
        args.AddSpellBP0(comboPoints);

        GetCaster()->CastSpell(GetHitUnit(), 180255, args);
        GetCaster()->ClearComboPoints();
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_ice_barrier_combo_spender::OnHit);
    }
};

class spell_second_wind_health_aura : public AuraScript
{
    PrepareAuraScript(spell_second_wind_health_aura);

    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        return uint32(std::floor(GetTarget()->GetHealthPct())) <= 30;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_second_wind_health_aura::CheckProc);
    }
};

class spell_perseverance_health_aura : public AuraScript
{
    PrepareAuraScript(spell_perseverance_health_aura);

    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        return uint32(std::floor(GetTarget()->GetHealthPct())) <= 20;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_perseverance_health_aura::CheckProc);
    }
};

class spell_verdant_dreamer_periodic_aura : public AuraScript
{
    PrepareAuraScript(spell_verdant_dreamer_periodic_aura);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ spellInfo->Effects[EFFECT_0].TriggerSpell });
    }

    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        Player* player = GetTarget()->ToPlayer();
        if (!player)
            return;

        int32 reduction = (player->GetStat(STAT_SPIRIT) * 0.05f) * -1;

        CastSpellExtraArgs args(aurEff);
        args.OriginalCaster = GetCasterGUID();
        args.AddSpellBP0(reduction);
        player->CastSpell(player, 180136, args);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_verdant_dreamer_periodic_aura::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class spell_warlords_charge_periodic_aura : public AuraScript
{
    PrepareAuraScript(spell_warlords_charge_periodic_aura);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ spellInfo->Effects[EFFECT_0].TriggerSpell });
    }

    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        Player* player = GetTarget()->ToPlayer();
        if (!player)
            return;

        int32 bonus = (player->GetStat(STAT_STAMINA) * 0.1f);

        CastSpellExtraArgs args(aurEff);
        args.OriginalCaster = GetCasterGUID();
        args.AddSpellBP0(bonus);
        player->CastSpell(player, 180150, args);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warlords_charge_periodic_aura::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class spell_point_blank_periodic_aura : public AuraScript
{
    PrepareAuraScript(spell_point_blank_periodic_aura);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ spellInfo->Effects[EFFECT_0].TriggerSpell });
    }

    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        Player* caster = GetTarget()->ToPlayer();
        Unit* target = caster->GetSelectedUnit();
        if (!target)
            return;
        if (target == caster)
            return;

        float distance = caster->GetExactDist(target);
        int32 amount = ((100.0f - (distance * 5.0f)) * 0.5f);
        amount = std::max(-50, amount);
        amount = std::min(50, amount);

        CastSpellExtraArgs args(aurEff);
        args.OriginalCaster = GetCasterGUID();
        args.AddSpellBP0(amount);
        caster->RemoveAura(180156);
        caster->CastSpell(caster, 180156, args);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_point_blank_periodic_aura::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class spell_dead_eye_periodic_aura : public AuraScript
{
    PrepareAuraScript(spell_dead_eye_periodic_aura);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ spellInfo->Effects[EFFECT_0].TriggerSpell });
    }

    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        Player* caster = GetTarget()->ToPlayer();
        Unit* target = caster->GetSelectedUnit();
        if (!target)
            return;
        if (target == caster)
            return;

        float distance = caster->GetExactDist(target);
        int32 amount = ((100.0f - (distance * 5.0f)) * 0.5f) * -1;
        amount = std::max(-50, amount);
        amount = std::min(50, amount);

        CastSpellExtraArgs args(aurEff);
        args.OriginalCaster = GetCasterGUID();
        args.AddSpellBP0(amount);
        caster->RemoveAura(180158);
        caster->CastSpell(caster, 180158, args);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_dead_eye_periodic_aura::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};


class spell_dmg_proc_aura : public SpellScriptLoader
{
public:
    spell_dmg_proc_aura() : SpellScriptLoader("spell_dmg_proc_aura") { }

    class spell_dmg_proc_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_dmg_proc_aura_AuraScript);

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
            {
                // Aura spells and triggers cannot proc this
                if (spellInfo->Id >= 180037 && spellInfo->Id <= 180039)
                {
                    return false;
                }
            }
            return true;
        }
        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
            {
                PreventDefaultAction();
                if (!eventInfo.GetDamageInfo())
                    return;
                uint32 spell = spellInfo->Effects[0].TriggerSpell;
                uint32 proc_dmg = (float(eventInfo.GetDamageInfo()->GetDamage()) * (float(aurEff->GetAmount()) / 100.0));
                CastSpellExtraArgs args(aurEff);
                args.OriginalCaster = GetCasterGUID();
                args.AddSpellBP0(proc_dmg);
                GetTarget()->CastSpell(GetTarget(), spell, args);
            }
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dmg_proc_aura_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_dmg_proc_aura_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_dmg_proc_aura_AuraScript();
    }
};


class spell_respiratory_pause : public SpellScriptLoader
{
public:
    spell_respiratory_pause() : SpellScriptLoader("spell_respiratory_pause") { }

    class spell_respiratory_pause_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_respiratory_pause_AuraScript);

        void HandlePeriodic(AuraEffect const* aurEff)
        {
            PreventDefaultAction();
            if (aurEff->GetAmount() <= 0)
            {
                Unit* target = GetTarget();
                target->CastSpell(target, 180097, aurEff);
                if (Player* playerTarget = GetUnitOwner()->ToPlayer())
                {
                    int32 baseAmount = aurEff->GetBaseAmount();
                    int32 amount = playerTarget->CalculateSpellDamage(GetSpellInfo(), aurEff->GetEffIndex(), &baseAmount);
                    GetEffect(EFFECT_0)->SetAmount(amount);
                }
            }
        }

        void HandleUpdatePeriodic(AuraEffect* aurEff)
        {
            if (Player* playerTarget = GetUnitOwner()->ToPlayer())
            {
                int32 baseAmount = aurEff->GetBaseAmount();
                int32 amount = playerTarget->isMoving() ?
                    playerTarget->CalculateSpellDamage(GetSpellInfo(), aurEff->GetEffIndex(), &baseAmount) :
                    aurEff->GetAmount() - 1;
                aurEff->SetAmount(amount);
            }
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_respiratory_pause_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_respiratory_pause_AuraScript::HandleUpdatePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_respiratory_pause_AuraScript();
    }
};

class spell_from_the_ashes_proc_engulf : public SpellScriptLoader
{
public:
    spell_from_the_ashes_proc_engulf() : SpellScriptLoader("spell_from_the_ashes_proc_engulf") { }

    class spell_from_the_ashes_proc_engulf_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_from_the_ashes_proc_engulf_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            auto caster = GetCaster();
            auto target = eventInfo.GetProcTarget();
            auto damageInfo = eventInfo.GetDamageInfo();
            auto healInfo = eventInfo.GetHealInfo();
            if (!caster || !target ||
                ((!damageInfo || damageInfo->GetDamage() == 0) &&
                (!healInfo || healInfo->GetHeal() == 0)))
            {
                PreventDefaultAction();
            }
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_from_the_ashes_proc_engulf_AuraScript::HandleProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_from_the_ashes_proc_engulf_AuraScript();
    }
};

// 180413 - Defiance
class hot_defiance : public SpellScriptLoader
{
public:
    hot_defiance() : SpellScriptLoader("hot_defiance") { }

    class hot_defiance_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_defiance_AuraScript);

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();
            if (target->HealthBelowPct(50))
            {
                if (!target->HasAura(180414))
                    target->CastSpell(target, 180414, true);
            }
            else
            {
                if (target->HasAura(180414))
                    target->RemoveAurasDueToSpell(180414);
            }
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(hot_defiance_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_defiance_AuraScript();
    }
};

// 180415,  180417, 180418- Life Leech, Life Leech, Vampiric Aspect
class hot_life_leech : public SpellScriptLoader
{
public:
    hot_life_leech() : SpellScriptLoader("hot_life_leech") { }

    class hot_life_leech_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_life_leech_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo || !damageInfo->GetDamage())
                return;

            int32 healamount = ((float)damageInfo->GetDamage() * ((float)GetSpellInfo()->Effects[EFFECT_0].BasePoints / 100.f)) + 0.5f;
            if (healamount > 0)
            {
                Unit* actor = eventInfo.GetActor();
                CastSpellExtraArgs args(aurEff);
                args.AddSpellMod(SPELLVALUE_BASE_POINT0, healamount);
                actor->CastSpell(actor, 180416, args);
            }
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(hot_life_leech_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_life_leech_AuraScript();
    }
};

// 180426 - Enfeeble
class hot_enfeeble : public SpellScriptLoader
{
public:
    hot_enfeeble() : SpellScriptLoader("hot_enfeeble") { }

    class hot_enfeeble_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_enfeeble_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo)
                return;

            if (damageInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_SHADOW)
                if (Unit* victim = damageInfo->GetVictim())
                    eventInfo.GetActor()->CastSpell(victim, 180427);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(hot_enfeeble_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_enfeeble_AuraScript();
    }
};

// 180430 - One with the Shadow
class hot_one_with_the_shadow : public SpellScriptLoader
{
public:
    hot_one_with_the_shadow() : SpellScriptLoader("hot_one_with_the_shadow") { }

    class hot_one_with_the_shadow_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_one_with_the_shadow_AuraScript);

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();
            if (target->HasStealthAura())
            {
                if (!target->HasAura(180431))
                    target->CastSpell(target, 180431, true);
            }
            else
            {
                if (target->HasAura(180431))
                    target->RemoveAurasDueToSpell(180431);
            }
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(hot_one_with_the_shadow_AuraScript::HandleDummyTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_one_with_the_shadow_AuraScript();
    }
};

// 180432 - Violent Bastion
class hot_violent_bastion : public SpellScriptLoader
{
public:
    hot_violent_bastion() : SpellScriptLoader("hot_violent_bastion") { }

    class hot_violent_bastion_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_violent_bastion_AuraScript);

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();
            if (Player* target_player = target->ToPlayer())
            {
                if (Item* offweapon = target_player->GetShield())
                {
                    if (!target->HasAura(180433))
                        target->CastSpell(target, 180433, true);
                }
                else
                {
                    if (target->HasAura(180433))
                        target->RemoveAurasDueToSpell(180433);
                }
            }
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(hot_violent_bastion_AuraScript::HandleDummyTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_violent_bastion_AuraScript();
    }
};

// 180440 - Blood Siphon
class hot_blood_siphon : public SpellScriptLoader
{
public:
    hot_blood_siphon() : SpellScriptLoader("hot_blood_siphon") { }

    class hot_blood_siphon_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_blood_siphon_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* actor = eventInfo.GetActor();
            actor->CastSpell(actor, 180441, true);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(hot_blood_siphon_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_blood_siphon_AuraScript();
    }
};

// 180442 - Soul Siphon, 180461 - Soul Drinker
class hot_soul_siphon : public SpellScriptLoader
{
public:
    hot_soul_siphon() : SpellScriptLoader("hot_soul_siphon") { }

    class hot_soul_siphon_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_soul_siphon_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* actor = eventInfo.GetActor();
            actor->CastSpell(actor, 180443, true);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(hot_soul_siphon_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_soul_siphon_AuraScript();
    }
};

// 180444 - Mana on Kill
class hot_mana_on_kill : public SpellScriptLoader
{
public:
    hot_mana_on_kill() : SpellScriptLoader("hot_mana_on_kill") { }

    class hot_mana_on_kill_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_mana_on_kill_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* actor = eventInfo.GetActor();
            actor->CastSpell(actor, 180445, true);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(hot_mana_on_kill_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_mana_on_kill_AuraScript();
    }
};

// 180446 - Life on Kill
class hot_life_on_kill : public SpellScriptLoader
{
public:
    hot_life_on_kill() : SpellScriptLoader("hot_life_on_kill") { }

    class hot_life_on_kill_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_life_on_kill_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* actor = eventInfo.GetActor();
            actor->CastSpell(actor, 180447, true);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(hot_life_on_kill_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_life_on_kill_AuraScript();
    }
};

// 180465 - Blood Drinker
class hot_blood_drinker : public SpellScriptLoader
{
public:
    hot_blood_drinker() : SpellScriptLoader("hot_blood_drinker") { }

    class hot_blood_drinker_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_blood_drinker_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* actor = eventInfo.GetActor();
            actor->CastSpell(actor, 180466, true);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(hot_blood_drinker_AuraScript::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_blood_drinker_AuraScript();
    }
};

// 93999 - Shield Superiority Dummy
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

        caster->AddAura(93999, caster);
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return;

        caster->RemoveAura(93999);
        caster->UpdateShieldBlockValue();
        caster->UpdateDamagePhysical(BASE_ATTACK);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_gen_shield_sup_dummy::AfterApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_gen_shield_sup_dummy::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 93998 - Shield Superiority
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

// 180475 - Blind on Hit
class hot_blind_on_hit : public SpellScriptLoader
{
public:
    hot_blind_on_hit() : SpellScriptLoader("hot_blind_on_hit") { }

    class hot_blind_on_hit_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_blind_on_hit_AuraScript);

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            // Enshroud
            if (eventInfo.GetActor()->HasAura(180477))
                return urand(1, 100) < 26;
            else return urand(1, 100) < 11;
        }

        void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            Unit* attacker = eventInfo.GetProcTarget();
            Unit* actor = eventInfo.GetActor();
            if (!attacker || !actor)
                return;

            if (actor->HasAura(180477)) // Enshroud
                actor->CastSpell(attacker, 180474);
            else
                if (actor->HasAura(180476)) // Blind Effect
                    actor->CastSpell(attacker, 180473);
                else
                    actor->CastSpell(attacker, 180472);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(hot_blind_on_hit_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(hot_blind_on_hit_AuraScript::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_blind_on_hit_AuraScript();
    }
};

// 180478 - Maligant Deterioration
class hot_maligant_deterioration : public SpellScriptLoader
{
public:
    hot_maligant_deterioration() : SpellScriptLoader("hot_maligant_deterioration") { }

    class hot_maligant_deterioration_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_maligant_deterioration_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (!target->HasAura(180479))
            {
                CastSpellExtraArgs args;
                args.AddSpellBP0((int32)(target->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_HASTE_SPELL)) / -2);
                target->CastSpell(target, 180479, args);
            }
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (target->HasAura(180479))
                target->RemoveAurasDueToSpell(180479);
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(hot_maligant_deterioration_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            AfterEffectRemove += AuraEffectRemoveFn(hot_maligant_deterioration_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_maligant_deterioration_AuraScript();
    }
};

// 180482 - Nathrezim Pact
class hot_nathrezim_pact : public SpellScriptLoader
{
public:
    hot_nathrezim_pact() : SpellScriptLoader("hot_nathrezim_pact") { }

    class hot_nathrezim_pact_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_nathrezim_pact_AuraScript);

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo || !damageInfo->GetDamage())
                return;

            int32 healamount = ((float)damageInfo->GetDamage() * (5.f / 100.f)) + 0.5f;
            if (healamount > 0)
            {
                Unit* actor = eventInfo.GetActor();
                CastSpellExtraArgs args(aurEff);
                args.AddSpellMod(SPELLVALUE_BASE_POINT0, healamount);
                actor->CastSpell(actor, 180483, args);
            }
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(hot_nathrezim_pact_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_nathrezim_pact_AuraScript();
    }
};

// 180503 - Companionship
class hot_companionship : public SpellScriptLoader
{
public:
    hot_companionship() : SpellScriptLoader("hot_companionship") { }

    class hot_companionship_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_companionship_AuraScript);

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();

            if (!target->HasAura(180504))
            {
                for (Unit::ControlList::iterator itr = target->m_Controlled.begin(); itr != target->m_Controlled.end();)
                {
                    Unit* unit = *itr;
                    ++itr;
                    if (unit->GetTypeId() == TYPEID_UNIT && unit->IsGuardian())
                    {
                        target->AddAura(180504, target);
                        break;
                    }
                }
            }
            else
            {
                bool hasguardian = false;
                for (Unit::ControlList::iterator itr = target->m_Controlled.begin(); itr != target->m_Controlled.end();)
                {
                    Unit* unit = *itr;
                    ++itr;
                    if (unit->GetTypeId() == TYPEID_UNIT && unit->IsGuardian())
                    {
                        hasguardian = true;
                        break;
                    }
                }

                if (!hasguardian)
                    target->RemoveAurasDueToSpell(180504);
            }
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (target->HasAura(180504))
                target->RemoveAurasDueToSpell(180504);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(hot_companionship_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(hot_companionship_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_companionship_AuraScript();
    }
};

// 180505 - Lone Wolf
class hot_lone_wolf : public SpellScriptLoader
{
public:
    hot_lone_wolf() : SpellScriptLoader("hot_lone_wolf") { }

    class hot_lone_wolf_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_lone_wolf_AuraScript);

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();

            if (!target->HasAura(180506))
            {
                bool hasguardian = false;
                for (Unit::ControlList::iterator itr = target->m_Controlled.begin(); itr != target->m_Controlled.end();)
                {
                    Unit* unit = *itr;
                    ++itr;
                    if (unit->GetTypeId() == TYPEID_UNIT && unit->IsGuardian())
                    {
                        hasguardian = true;
                        break;
                    }
                }

                if (!hasguardian)
                    target->AddAura(180506, target);
            }
            else
            {
                for (Unit::ControlList::iterator itr = target->m_Controlled.begin(); itr != target->m_Controlled.end();)
                {
                    Unit* unit = *itr;
                    ++itr;
                    if (unit->GetTypeId() == TYPEID_UNIT && unit->IsGuardian())
                    {
                        target->RemoveAurasDueToSpell(180506);
                        break;
                    }
                }
            }
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (target->HasAura(180506))
                target->RemoveAurasDueToSpell(180506);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(hot_lone_wolf_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(hot_lone_wolf_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_lone_wolf_AuraScript();
    }
};

// 180518 - Shield Supperiosity
class hot_shield_supperiosity : public SpellScriptLoader
{
public:
    hot_shield_supperiosity() : SpellScriptLoader("hot_shield_supperiosity") { }

    class hot_shield_supperiosity_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_shield_supperiosity_AuraScript);

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            Player* target = GetTarget()->ToPlayer();

            if (!target->HasAura(180519))
            {
                if (target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) == nullptr)
                    if (Item* offweapon = target->GetShield())
                        target->AddAura(180519, target);
            }
            else
            {
                if ((target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) != nullptr) || (target->GetShield() == nullptr))
                    target->RemoveAurasDueToSpell(180519);
            }
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (target->HasAura(180519))
                target->RemoveAurasDueToSpell(180519);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(hot_shield_supperiosity_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(hot_shield_supperiosity_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_shield_supperiosity_AuraScript();
    }
};

// 180520 - Fists of Fury
class hot_fists_of_fury : public SpellScriptLoader
{
public:
    hot_fists_of_fury() : SpellScriptLoader("hot_fists_of_fury") { }

    class hot_fists_of_fury_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_fists_of_fury_AuraScript);

        void RemoveEffect(Unit* target)
        {
            target->RemoveAurasDueToSpell(180521);
            target->RemoveAurasDueToSpell(180522);
            target->RemoveAurasDueToSpell(180523);
            gloves_save = nullptr;
            agility_save = 0.f;
        }

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            Player* target = GetTarget()->ToPlayer();

            if (!target->HasAura(180521))
            {
                if (target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) == nullptr &&
                    target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND) == nullptr &&
                    target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED) == nullptr)
                {
                    float agility = target->GetStat(STAT_AGILITY);
                    int32 attackspeedbonus = (agility / 100.f);

                    CastSpellExtraArgs args(aurEff);
                    args.AddSpellMod(SPELLVALUE_BASE_POINT0, attackspeedbonus);
                    target->CastSpell(target, 180521, args);

                    agility_save = agility;

                    if (Item* gloves = target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_HANDS))
                    {
                        ItemTemplate const* itemplate = gloves->GetTemplate();
                        ScalingStatDistributionEntry const* ssd = target->GetScalingStatDistributionFor(*itemplate);
                        ScalingStatValuesEntry const* ssv = target->GetScalingStatValuesFor(*itemplate);

                        if (!itemplate)
                            return;

                        int32 strength = 0;
                        int32 agility = 0;
                        int32 stamina = 0;
                        int32 intellect = 0;
                        int32 spirit = 0;
                        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
                        {
                            uint32 statType = 0;
                            int32  val = 0;

                            if (ssd && ssv)
                            {
                                if (ssd->StatMod[i] < 0)
                                    continue;
                                statType = ssd->StatMod[i];
                                val = (ssv->getssdMultiplier(itemplate->ScalingStatValue) * ssd->Modifier[i]) / 10000;
                            }
                            else
                            {
                                if (i >= itemplate->StatsCount)
                                    continue;
                                statType = itemplate->ItemStat[i].ItemStatType;
                                val = itemplate->ItemStat[i].ItemStatValue;
                            }

                            if (val == 0)
                                continue;

                            switch (statType)
                            {
                            case ITEM_MOD_AGILITY:
                                agility += val;
                                break;
                            case ITEM_MOD_STRENGTH:
                                strength += val;
                                break;
                            case ITEM_MOD_INTELLECT:
                                intellect += val;
                                break;
                            case ITEM_MOD_SPIRIT:
                                spirit += val;
                                break;
                            case ITEM_MOD_STAMINA:
                                stamina += val;
                                break;
                            }
                        }

                        CastSpellExtraArgs args2(aurEff);
                        args2.AddSpellMod(SPELLVALUE_BASE_POINT0, strength * 2);
                        args2.AddSpellMod(SPELLVALUE_BASE_POINT1, agility * 2);
                        args2.AddSpellMod(SPELLVALUE_BASE_POINT2, stamina * 2);
                        target->CastSpell(target, 180522, args2);

                        CastSpellExtraArgs args3(aurEff);
                        args3.AddSpellMod(SPELLVALUE_BASE_POINT0, intellect * 2);
                        args3.AddSpellMod(SPELLVALUE_BASE_POINT1, spirit * 2);
                        target->CastSpell(target, 180523, args3);

                        gloves_save = gloves;
                        agility_save = target->GetStat(STAT_AGILITY);
                    }
                }
            }
            else
            {
                if (target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) != nullptr ||
                    target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND) != nullptr ||
                    target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED) != nullptr)
                {
                    RemoveEffect(target);
                    return;
                }

                Item* gloves = target->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_HANDS);
                if (gloves != gloves_save)
                {
                    RemoveEffect(target);
                    return;
                }

                if (target->GetStat(STAT_AGILITY) != agility_save)
                {
                    RemoveEffect(target);
                    return;
                }
            }
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (target->HasAura(180521))
                RemoveEffect(target);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(hot_fists_of_fury_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(hot_fists_of_fury_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }

        Item* gloves_save = nullptr;
        float agility_save = 0.f;
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_fists_of_fury_AuraScript();
    }
};

// 180524 - Wild Quiver
class hot_wild_quiver : public SpellScriptLoader
{
public:
    hot_wild_quiver() : SpellScriptLoader("hot_wild_quiver") { }

    class hot_wild_quiver_AuraScript : public AuraScript
    {
        PrepareAuraScript(hot_wild_quiver_AuraScript);

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            return (urand(1, 100) <= 20);
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo || !damageInfo->GetDamage())
                return;

            Unit* actor = eventInfo.GetActor();
            actor->CastSpell(eventInfo.GetActionTarget(), 180525, true);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(hot_wild_quiver_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(hot_wild_quiver_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new hot_wild_quiver_AuraScript();
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


void AddSC_Spells_Custom_Talents()
{
    //new spell_dmg_proc_aura();
    new spell_respiratory_pause();
    RegisterAuraScript(spell_second_wind_health_aura);
    RegisterAuraScript(spell_perseverance_health_aura);
    RegisterAuraScript(spell_verdant_dreamer_periodic_aura);
    RegisterAuraScript(spell_warlords_charge_periodic_aura);
    RegisterAuraScript(spell_point_blank_periodic_aura);
    RegisterAuraScript(spell_dead_eye_periodic_aura);
    RegisterAuraScript(spell_talent_combulstibolt_aura);
    RegisterAuraScript(spell_talent_burningarmor_aura);
    RegisterAuraScript(spell_talent_engulf_aura);
    RegisterAuraScript(spell_talent_engulfing_flames_aura);
    RegisterAuraScript(spell_talent_fire_ward_aura);
    RegisterAuraScript(spell_talent_from_the_ashes_aura);
    RegisterAuraScript(spell_talent_from_the_ashes_phoenix_aura);
    RegisterAuraScript(spell_talent_from_the_ashes_resurrection_aura);
    RegisterAuraScript(spell_talent_icy_veins_aura);
    RegisterAuraScript(spell_talent_heart_glacier_aura);
    RegisterAuraScript(spell_talent_polar_affliction_aura);
    RegisterAuraScript(spell_talent_hammer_of_the_north_aura);
    RegisterAuraScript(spell_talent_congelation_trigger_aura);
    RegisterAuraScript(spell_talent_congelation_actual_aura);
    RegisterAuraScript(spell_talent_frozenheart_trigger_aura);
    RegisterAuraScript(spell_talent_frozenheart_actual_aura);
    RegisterAuraScript(spell_talent_coldtempered_aura);
    new spell_cold_steel_aura();
    RegisterAuraScript(spell_glaciation_aura);
    RegisterSpellScript(spell_frostfire_bolt_combo_spender);
    RegisterSpellScript(spell_ice_barrier_combo_spender);
    new spell_from_the_ashes_proc_engulf();
    new hot_defiance();
    new hot_life_leech();
    new hot_enfeeble();
    new hot_one_with_the_shadow();
    new hot_violent_bastion();
    new hot_blood_siphon();
    new hot_soul_siphon();
    new hot_mana_on_kill();
    new hot_life_on_kill();
    new hot_blood_drinker();
    RegisterAuraScript(spell_gen_shield_sup_dummy);
    RegisterAuraScript(spell_gen_shield_sup);
    new hot_blind_on_hit();
    new hot_maligant_deterioration();
    new hot_nathrezim_pact();
    new hot_companionship();
    new hot_lone_wolf();
    new hot_shield_supperiosity();
    new hot_fists_of_fury();
    new hot_wild_quiver();
    RegisterSpellScript(spell_gen_shield_sup_dummy);
    RegisterSpellScript(spell_gen_shield_sup);
    RegisterSpellScript(spell_firebrand_weapon);
    RegisterSpellScript(spell_combustibolt);
}
