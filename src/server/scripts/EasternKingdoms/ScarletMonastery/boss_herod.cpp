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

#include "scarlet_monastery.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptMgr.h"

enum HerodSays
{
    SAY_AGGRO = 0,
    SAY_WHIRLWIND = 1,
    SAY_ENRAGE = 2,
    SAY_KILL = 3,
    EMOTE_ENRAGE = 4,
    SAY_RAVAGER = 5,
    SAY_LEAP = 6,
    SAY_HARRY = 7,
    SAY_EXECUTE = 8,
    SAY_DEATH = 9
};

enum HerodSpells
{
    SPELL_RUSHINGCHARGE     = 8260,
    SPELL_CLEAVE            = 15496,
    SPELL_WHIRLWIND         = 8989,
    SPELL_WHIRLWIND_TRIGGER = 15578,
    SPELL_FRENZY            = 8269,
    SPELL_CHAIN_HOOK        = 81211,
    SPELL_SUMMON_RAVAGER    = 81212,
    SPELL_HEROIC_LEAP       = 81214,
    SPELL_HEROIC_LEAP_DAMAGE = 81219,
    SPELL_EXECUTE           = 81215,
    SPELL_BLOODTHIRST       = 81216,
    SPELL_BATTLE_STANCE     = 81217,
    SPELL_BERSERKER_STANCE  = 81218
};

enum HerodNpcs
{
    NPC_SCARLET_TRAINEE = 6575,
    NPC_SCARLET_MYRMIDON = 4295
};

enum HerodEvents
{
    EVENT_CLEAVE = 1,
    EVENT_WHIRLWIND,
    EVENT_CHAIN_HOOK,
    EVENT_SUMMON_RAVAGER,
    EVENT_HEROIC_LEAP,
    EVENT_HEROIC_LEAP_DAMAGE,
    EVENT_EXECUTE,
    EVENT_BLOODTHIRST,
    EVENT_RANGE_CHECK,
    EVENT_HEALTH_CHECK,
};

Position const ScarletTraineePos = { 1939.18f, -431.58f, 17.09f, 6.22f };

struct boss_herod : public BossAI
{
    boss_herod(Creature* creature) : BossAI(creature, DATA_HEROD)
    {
        me->AddAura(SPELL_BATTLE_STANCE, me);
    }

    void Reset() override
    {
        berserker_stance = false;
        _Reset();
        me->RemoveAura(SPELL_BERSERKER_STANCE);
        me->AddAura(SPELL_BATTLE_STANCE, me);
        instance->SetBossState(DATA_HEROD, NOT_STARTED);
    }

    void JustEngagedWith(Unit* who) override
    {
        Talk(SAY_AGGRO);
        DoCast(me, SPELL_BATTLE_STANCE);
        BossAI::JustEngagedWith(who);

        events.ScheduleEvent(EVENT_CLEAVE, 8s);
        events.ScheduleEvent(EVENT_CHAIN_HOOK, 30s);
        events.ScheduleEvent(EVENT_SUMMON_RAVAGER, 12s);

        instance->SetBossState(DATA_HEROD, IN_PROGRESS);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();

        Talk(SAY_DEATH);

        for (uint8 itr = 0; itr < 20; ++itr)
        {
            Position randomNearPosition = me->GetRandomPoint(ScarletTraineePos, 5.f);
            randomNearPosition.SetOrientation(ScarletTraineePos.GetOrientation());
            me->SummonCreature(NPC_SCARLET_TRAINEE, randomNearPosition, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 10min);
        }

        instance->SetBossState(DATA_HEROD, DONE);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage) override
    {
        if (!berserker_stance && me->HealthBelowPctDamaged(50, damage))
        {
            Talk(EMOTE_ENRAGE);
            Talk(SAY_ENRAGE);
            me->RemoveAura(SPELL_BATTLE_STANCE);
            me->AddAura(SPELL_BERSERKER_STANCE, me);            
            berserker_stance = true;
            events.RescheduleEvent(EVENT_HEROIC_LEAP, 15s);
            events.RescheduleEvent(EVENT_BLOODTHIRST, 8s);
            events.RescheduleEvent(EVENT_HEALTH_CHECK, 2s);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_CLEAVE:
                    DoCastVictim(SPELL_CLEAVE);
                    events.Repeat(10s);
                    break;
                case EVENT_CHAIN_HOOK:
                    DoCastSelf(SPELL_CHAIN_HOOK);
                    events.ScheduleEvent(EVENT_WHIRLWIND, 1s);
                    events.RescheduleEvent(EVENT_HEROIC_LEAP, 20s);
                    events.RescheduleEvent(EVENT_BLOODTHIRST, 12s);
                    events.RescheduleEvent(EVENT_HEALTH_CHECK, 12s);
                    events.ScheduleEvent(EVENT_CHAIN_HOOK, 40s);
                    break;
                case EVENT_WHIRLWIND:
                    Talk(SAY_WHIRLWIND);
                    DoCastSelf(SPELL_WHIRLWIND);
                    break;
                case EVENT_SUMMON_RAVAGER:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true, false))
                    {
                        Talk(SAY_RAVAGER);
                        DoCast(target, SPELL_SUMMON_RAVAGER);
                    }
                    events.Repeat(20s);
                    break;
                case EVENT_HEROIC_LEAP:
                    if (berserker_stance)
                        if (Unit* target = SelectTarget(SelectTargetMethod::MaxDistance, 0, 40.0f, true, false))
                        {
                            Talk(SAY_LEAP);
                            DoCast(target, SPELL_HEROIC_LEAP);
                            events.ScheduleEvent(EVENT_HEROIC_LEAP_DAMAGE, 1s);
                        }
                    events.Repeat(15s);
                    break;
                case EVENT_HEROIC_LEAP_DAMAGE:
                    if (berserker_stance)
                        DoCastSelf(SPELL_HEROIC_LEAP_DAMAGE);
                    break;
                case EVENT_BLOODTHIRST:
                    if (berserker_stance)
                        DoCastVictim(SPELL_BLOODTHIRST);
                    events.Repeat(12s);
                    break;
                case EVENT_HEALTH_CHECK:
                {
                    Map::PlayerList const& PlayerList = me->GetMap()->GetPlayers();
                    if (!PlayerList.isEmpty())
                        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                            if (i->GetSource()->HealthBelowPct(21) && i->GetSource()->IsAlive())
                                if (Unit* executeTarget = i->GetSource())
                                {
                                    if (!me->IsWithinMeleeRange(executeTarget))
                                    {
                                        DoCast(executeTarget, SPELL_HEROIC_LEAP);
                                        DoCast(executeTarget, SPELL_EXECUTE);
                                    }
                                    else
                                    {
                                        DoCast(executeTarget, SPELL_EXECUTE);
                                    }
                                }
                    events.Repeat(2s);
                    break;
                }
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
        DoMeleeAttackIfReady();
    }

private:
    bool berserker_stance;
};

struct npc_scarlet_trainee : public EscortAI
{
    npc_scarlet_trainee(Creature* creature) : EscortAI(creature)
    {
        _startTimer = urand(1000, 6000);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_startTimer)
        {
            if (_startTimer <= diff)
            {
                Start(true, true);
                _startTimer = 0;
            }
            else
                _startTimer -= diff;
        }

        EscortAI::UpdateAI(diff);
    }

private:
    uint32 _startTimer;
};

void AddSC_boss_herod()
{
    RegisterScarletMonasteryCreatureAI(boss_herod);
    RegisterScarletMonasteryCreatureAI(npc_scarlet_trainee);
}
