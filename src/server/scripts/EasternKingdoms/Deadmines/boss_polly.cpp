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
    SPELL_SQUAWK = 81180,
    SPELL_DIVE = 81181,
    SPELL_INSULT = 81182,
    SPELL_POLLY_ENRAGE = 81184
};

enum Texts
{
    SAY_AGGRO = 0,
    SAY_SLAY = 1,
    SAY_DEATH = 2,
    SAY_INSULT = 3,
    SAY_ENRAGE = 4
};

enum Events
{
    EVENT_SQUAWK = 1,
    EVENT_DIVE = 2,
    EVENT_INSULT = 3
};

class boss_polly : public CreatureScript
{
public:
    boss_polly() : CreatureScript("boss_polly") { }

    struct boss_pollyAI : public BossAI
    {
        boss_pollyAI(Creature* creature) : BossAI(creature, DATA_POLLY)
        {
            CaptainGreenskinData = DATA_CAPTAIN_GREENSKIN;
        }

        bool captainDeath;

        void Reset() override
        {
            if (Creature* CaptainGreenskin = ObjectAccessor::GetCreature(*me, instance->GetGuidData(CaptainGreenskinData)))
            {
                if (CaptainGreenskin->isDead())
                {
                    CaptainGreenskin->SetRespawnCompatibilityMode(true);
                    CaptainGreenskin->Respawn(true);
                    CaptainGreenskin->GetMotionMaster()->MoveTargetedHome();
                    me->GetMotionMaster()->MoveTargetedHome();
                }
                else if (CaptainGreenskin->GetVictim())
                    AddThreat(CaptainGreenskin->GetVictim(), 0.0f);
            }

            if (!me->IsInCombat())
                BossAI::Reset();

            captainDeath = false;
        }

        void JustEngagedWith(Unit* victim) override
        {
            BossAI::JustEngagedWith(victim);

            events.ScheduleEvent(EVENT_SQUAWK, 12s);
            events.ScheduleEvent(EVENT_DIVE, 8s);
            events.ScheduleEvent(EVENT_INSULT, 20s);

            Talk(SAY_AGGRO);
            me->SetRespawnCompatibilityMode(true);

            if (Creature* CaptainGreenskin = ObjectAccessor::GetCreature(*me, instance->GetGuidData(CaptainGreenskinData)))
            {
                if (CaptainGreenskin->IsAlive() && !CaptainGreenskin->GetVictim())
                {
                    CaptainGreenskin->AI()->AttackStart(victim);
                }

                captainDeath = false;
            }
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

            if (!captainDeath)
            {
                if (Creature* CaptainGreenskin = ObjectAccessor::GetCreature(*me, instance->GetGuidData(CaptainGreenskinData)))
                {
                    if (CaptainGreenskin->isDead())
                    {
                        Talk(SAY_ENRAGE);
                        DoCast(me, SPELL_POLLY_ENRAGE);
                        captainDeath = true;
                    }
                }
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SQUAWK:
                    DoCast(me, SPELL_SQUAWK);
                    if (captainDeath)
                        events.ScheduleEvent(EVENT_SQUAWK, 8s);
                    else
                        events.ScheduleEvent(EVENT_SQUAWK, 15s);
                    break;

                case EVENT_DIVE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::MaxDistance, 0, 40.0f, true))
                        DoCast(target, SPELL_DIVE);
                    events.ScheduleEvent(EVENT_DIVE, 12s);
                    break;
                case EVENT_INSULT:
                    DoCastVictim(SPELL_INSULT);
                    Talk(SAY_INSULT);
                    events.ScheduleEvent(EVENT_INSULT, 20s);
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
            Talk(SAY_DEATH);
            me->SetRespawnCompatibilityMode(false);

            if (Creature* CaptainGreenskin = ObjectAccessor::GetCreature(*me, instance->GetGuidData(CaptainGreenskinData)))
            {
                if (CaptainGreenskin->isDead())
                {
                    CaptainGreenskin->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                }
            }
        }

        protected:
            uint32 CaptainGreenskinData;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDeadminesAI<boss_pollyAI>(creature);
    };
};

void AddSC_boss_polly()
{
    new boss_polly();
}
