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

#include "ScriptMgr.h"
#include "deadmines.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellMgr.h"

enum Spells
{
    SPELL_DUAL_WIELD        = 674,
    SPELL_THRASH            = 12787,
    SPELL_VANCLEEFS_ALLIES  = 5200,
    SPELL_APPLY_POISON      = 81187,
    SPELL_INSTANT_POISON    = 81188,
    SPELL_WOUND_POISON      = 81190,
    SPELL_DEADLY_POISON     = 81192,
    SPELL_LEACHING_POISON   = 81194,
    SPELL_SHADOWSTEP        = 81196,
    SPELL_CLOAK_OF_SHADOWS  = 81197,
    SPELL_EVASION           = 81198,
    SPELL_VANISH            = 81199,
    SPELL_BLIND             = 81200,
    SPELL_BACKSTAB          = 81201,
    SPELL_AMBUSH            = 81202,
    SPELL_GARROTE           = 81203,
    SPELL_SAP               = 81204,
    SPELL_FAN_OF_KNIVES     = 81205
};

enum Speech
{
    SAY_AGGRO  = 0,    // None may challenge the Brotherhood!
    SAY_ONE    = 1,    // Lapdogs, all of you!
    SAY_SUMMON = 2,    // calls more of his allies out of the shadows.
    SAY_TWO    = 3,    // Fools! Our cause is righteous!
    SAY_KILL   = 4,    // And stay down!
    SAY_THREE  = 5,    // The Brotherhood shall prevail!
    SAY_DEATH  = 6
};

// TDB coords
Position const BlackguardPositions[] =
{
    { -78.2791f, -824.784f, 40.0007f, 2.93215f },
    { -77.8071f, -815.097f, 40.0188f, 3.26377f }
};

struct boss_vancleef : public BossAI
{
    public:
        boss_vancleef(Creature* creature) : BossAI(creature, DATA_VANCLEEF), _guardsCalled(false), _health25(false), _health33(false), _health75(false) { }

        void Reset() override
        {
            BossAI::Reset();

            _guardsCalled = false;
            _health25 = false;
            _health33 = false;
            _health75 = false;

            DoCastSelf(SPELL_DUAL_WIELD, true);
            DoCastSelf(SPELL_THRASH, true);
            DoCastSelf(SPELL_INSTANT_POISON, true);

            SummonBlackguards();

            me->SetReactState(REACT_DEFENSIVE);
        }

        void JustEngagedWith(Unit* victim) override
        {
            BossAI::JustEngagedWith(victim);
            summons.DoZoneInCombat();

            Shadowstep_Timer = 16000;
            Cloak_Evasion_Timer = 14000;
            Blind_Timer = 12000;
            Fan_of_Knives_Timer = 8000;
            Wait_Timer = 0;

            InVanish = false;

            Talk(SAY_AGGRO);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_KILL);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            summons.DespawnAll();
            _DespawnAtEvade();
        }

        void SummonBlackguards()
        {
            for (Position BlackguardPosition : BlackguardPositions)
                DoSummon(NPC_BLACKGUARD, BlackguardPosition, 1min, TEMPSUMMON_CORPSE_TIMED_DESPAWN);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) override
        {
            if (!_guardsCalled && HealthBelowPct(50))
            {
                Talk(SAY_SUMMON);
                Talk(SAY_TWO);
                SummonBlackguards();
                DoCastSelf(SPELL_APPLY_POISON);
                DoCastSelf(SPELL_VANISH);
                Wait_Timer = 4000;
                InVanish = true;
                _guardsCalled = true;
            }

            if (!_health25 && HealthBelowPct(25))
            {
                Talk(SAY_THREE);
                DoCastSelf(SPELL_APPLY_POISON);
                DoCastSelf(SPELL_VANISH);
                Wait_Timer = 4000;
                InVanish = true;
                _health25 = true;
            }
            else if (!_health75 && HealthBelowPct(75))
            {
                Talk(SAY_ONE);
                DoCastSelf(SPELL_APPLY_POISON);
                DoCastSelf(SPELL_VANISH);
                Wait_Timer = 4000;
                InVanish = true;
                _health75 = true;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            summons.DoZoneInCombat();

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            uint32 roll = urand(0, 99);

            if (!InVanish)
            {
                if (Shadowstep_Timer <= diff)
                {
                    if (Unit* target = SelectTarget(SelectTargetMethod::MaxDistance, 0, 60.0f, true, false))
                    {
                        DoStopAttack();
                        DoCast(target, SPELL_SHADOWSTEP);
                        DoCast(target, SPELL_BACKSTAB);
                        Shadowstep_Timer = 20000;
                    }
                }
                else Shadowstep_Timer -= diff;

                if (Cloak_Evasion_Timer <= diff)
                {
                    if (roll < 50)
                        DoCastSelf(SPELL_CLOAK_OF_SHADOWS);
                    else
                        DoCastSelf(SPELL_EVASION);
                    Cloak_Evasion_Timer = 22000;
                }
                else Cloak_Evasion_Timer -= diff;

                if (Blind_Timer <= diff)
                {
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 20.0f, true, false))
                        DoCast(target, SPELL_BLIND);
                    Blind_Timer = 24000;
                }
                else Blind_Timer -= diff;

                if (Fan_of_Knives_Timer <= diff)
                {
                    DoStopAttack();
                    DoCastSelf(SPELL_FAN_OF_KNIVES);
                    Fan_of_Knives_Timer = urand(10000, 14000);
                }
                else Fan_of_Knives_Timer -= diff;
            }

            if (InVanish)
            {
                if (Wait_Timer <= diff)
                {
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true, false))
                    {
                        DoCast(target, SPELL_SHADOWSTEP);

                        if (roll < 33)
                            DoCast(target, SPELL_AMBUSH);
                        else if (roll < 66)
                            DoCast(target, SPELL_GARROTE);
                        else
                        {
                            DoCast(target, SPELL_SAP);
                        }

                        InVanish = false;
                    }
                }
                else Wait_Timer -= diff;
            }

            if (!InVanish)
                DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
        }

    private:
        bool _guardsCalled;
        bool _health25;
        bool _health33;
        bool _health75;
        bool InVanish;
        uint32 Wait_Timer;
        uint32 Shadowstep_Timer;
        uint32 Cloak_Evasion_Timer;
        uint32 Blind_Timer;
        uint32 Fan_of_Knives_Timer;
};

class spell_apply_poison : public SpellScriptLoader
{
public:
    spell_apply_poison() : SpellScriptLoader("spell_apply_poison") { }

    class spell_apply_poison_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_apply_poison_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_APPLY_POISON))
                return false;
            return true;
        }

        void HandleEffect(SpellEffIndex /*effIndex*/)
        {
            std::vector<uint32> spellList =
            {
                SPELL_INSTANT_POISON,
                SPELL_WOUND_POISON,
                SPELL_DEADLY_POISON,
                SPELL_LEACHING_POISON
            };

            uint32 spellId = spellList[urand(0, spellList.size() - 1)];
            switch (spellId)
            {
                default:
                    GetCaster()->RemoveAura(SPELL_INSTANT_POISON);
                    GetCaster()->RemoveAura(SPELL_WOUND_POISON);
                    GetCaster()->RemoveAura(SPELL_DEADLY_POISON);
                    GetCaster()->RemoveAura(SPELL_LEACHING_POISON);
                    GetCaster()->CastSpell(GetCaster(), spellId, true);
                    break;
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_apply_poison_SpellScript::HandleEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_apply_poison_SpellScript();
    }
};

void AddSC_boss_vancleef()
{
    RegisterDeadminesCreatureAI(boss_vancleef);
    new spell_apply_poison();
}
