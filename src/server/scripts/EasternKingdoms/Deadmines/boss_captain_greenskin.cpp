/* ScriptData
SDName: Boss Mr.Smite
SD%Complete:
SDComment: Timers and say taken from acid script
EndScriptData */

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
    SPELL_POISONED_HARPOON  = 81176,
    SPELL_BOOT              = 81178,
    SPELL_CLEAVE            = 40505,
    SPELL_CAPTAIN_ENRAGE    = 81183,
};

enum Texts
{
    SAY_AGGRO   = 0,
    SAY_SUMMON  = 1,
    SAY_SLAY    = 2,
    SAY_ENRAGE  = 3,
    SAY_DEATH   = 4,
    SAY_DEATH2  = 5
};

enum Events
{
    EVENT_POISONED_HARPOON      = 1,
    EVENT_CALL_REINFORCEMENTS   = 2,
    EVENT_CLEAVE                = 3,
    EVENT_BOOT                  = 4,
};

class boss_captain_greenskin : public CreatureScript
{
    public:
        boss_captain_greenskin() : CreatureScript("boss_captain_greenskin") { }

    struct boss_captain_greenskinAI : public BossAI
    {
        boss_captain_greenskinAI(Creature* creature) : BossAI(creature, DATA_CAPTAIN_GREENSKIN)
        {
            PollyData = DATA_POLLY;
        }

        bool pollyDeath;

        void Reset() override
        {
            if (Creature* polly = ObjectAccessor::GetCreature(*me, instance->GetGuidData(PollyData)))
            {
                if (polly && polly->isDead())
                {
                    polly->SetRespawnCompatibilityMode(true);
                    polly->Respawn(true);
                    polly->GetMotionMaster()->MoveTargetedHome();
                    me->GetMotionMaster()->MoveTargetedHome();
                }

                else if (polly->GetVictim())
                    AddThreat(polly->GetVictim(), 0.0f);
            }

            if (!me->IsInCombat())
            {
                BossAI::Reset();
                summons.DespawnAll();
            }

            pollyDeath = false;
        }

        void JustEngagedWith(Unit* victim) override
        {
            BossAI::JustEngagedWith(victim);

            events.ScheduleEvent(EVENT_POISONED_HARPOON, 7s);
            events.ScheduleEvent(EVENT_CALL_REINFORCEMENTS, 30s);
            events.ScheduleEvent(EVENT_CLEAVE, 5s);
            events.ScheduleEvent(EVENT_BOOT, 10s);

            Talk(SAY_AGGRO);
            me->SetRespawnCompatibilityMode(true);

            if (Creature* polly = ObjectAccessor::GetCreature(*me, instance->GetGuidData(PollyData)))
            {
                if (polly && polly->IsAlive() && !polly->GetVictim())
                {
                    polly->AI()->AttackStart(victim);
                }
            }

            pollyDeath = false;
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (!pollyDeath)
            {
                if (Creature* polly = ObjectAccessor::GetCreature(*me, instance->GetGuidData(PollyData)))
                {
                    if (polly && polly->isDead())
                    {
                        Talk(SAY_ENRAGE);
                        DoCast(me, SPELL_CAPTAIN_ENRAGE);
                        pollyDeath = true;
                    }
                }
            }

            events.Update(diff);

            summons.DoZoneInCombat();

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            const Position ReinforcementsPosition = { -47.0277f, -806.7373f, 42.8095f, 4.02808f };
            uint32 roll = urand(0, 99);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_POISONED_HARPOON:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                            DoCast(target, SPELL_POISONED_HARPOON);
                        events.ScheduleEvent(EVENT_POISONED_HARPOON, 15s);
                        break;
                    case EVENT_CALL_REINFORCEMENTS:
                        if (roll < 33)
                            DoSummon(NPC_DEFIAS_PRIATE, ReinforcementsPosition, 1min, TEMPSUMMON_CORPSE_TIMED_DESPAWN);
                        else if (roll < 66)
                            DoSummon(NPC_DEFIAS_SQUALLSHAPER, ReinforcementsPosition, 1min, TEMPSUMMON_CORPSE_TIMED_DESPAWN);
                        else
                            DoSummon(NPC_GOLBIN_SHIPBUILDER, ReinforcementsPosition, 1min, TEMPSUMMON_CORPSE_TIMED_DESPAWN);
                        Talk(SAY_SUMMON);
                        events.ScheduleEvent(EVENT_CALL_REINFORCEMENTS, 45s);
                        break;
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, 8s);
                        break;
                    case EVENT_BOOT:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 5.0f, true))
                            DoCast(target, SPELL_BOOT);
                        events.ScheduleEvent(EVENT_BOOT, 12s);
                        break;
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }
            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->SetRespawnCompatibilityMode(false);

            if (Creature* polly = ObjectAccessor::GetCreature(*me, instance->GetGuidData(PollyData)))
            {
                if (polly && polly->IsAlive())
                {
                    Talk(SAY_DEATH);
                    me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                }
                else
                    Talk(SAY_DEATH2);
            }
            else
                Talk(SAY_DEATH2);
        }

        protected:
            uint32 PollyData;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDeadminesAI<boss_captain_greenskinAI>(creature);
    };
};

void AddSC_boss_captain_greenskin()
{
    new boss_captain_greenskin();
}
