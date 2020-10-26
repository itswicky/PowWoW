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
    SPELL_DISTRACTING_PAIN              = 81017,
    SPELL_TERRIFY                       = 81018,
    SPELL_SAWBLADE                      = 81020,
    SPELL_SELF_REPAIR                   = 81015,
    SPELL_INITIATE_DECIMATION_PROTOCOL  = 81019,
    SPELL_EJECT_SNEED                   = 5141,
    SPELL_DISARM                        = 6713,
    SPELL_AXE_TOSS                      = 81013,
    SPELL_SUIT_UP                       = 81021
};

enum Events
{
    EVENT_DISTRACTING_PAIN,
    EVENT_TERRIFY,
    EVENT_SAWBLADE,
    EVENT_INITIATE_DECIMATION_PROTOCOL,
    EVENT_DISARM,
    EVENT_AXE_TOSS
};

class boss_sneeds_shredder : public CreatureScript
{
public:
    boss_sneeds_shredder() : CreatureScript("boss_sneeds_shredder") { }

    struct boss_sneeds_shredderAI : public BossAI
    {
        boss_sneeds_shredderAI(Creature* creature) : BossAI(creature, DATA_SNEEDS_SHREDDER)
        {
            _health25 = false;
            _health50 = false;
            _health75 = false;
        }

        void Reset() override
        {
            BossAI::Reset();

            _health25 = false;
            _health50 = false;
            _health75 = false;
        }

        void JustEngagedWith(Unit* victim) override
        {
            BossAI::JustEngagedWith(victim);

            events.ScheduleEvent(EVENT_DISTRACTING_PAIN, 7s);
            events.ScheduleEvent(EVENT_TERRIFY, 12s);
            events.ScheduleEvent(EVENT_SAWBLADE, 3s);
            events.ScheduleEvent(EVENT_INITIATE_DECIMATION_PROTOCOL, 3min);

            me->SetRespawnCompatibilityMode(true);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) override
        {
            if (!_health25 && HealthBelowPct(25))
            {
                DoCastSelf(SPELL_SELF_REPAIR);
                _health25 = true;
            }
            else if (!_health50 && HealthBelowPct(50))
            {
                DoCastSelf(SPELL_SELF_REPAIR);
                _health50 = true;
            }
            else if (!_health75 && HealthBelowPct(75))
            {
                DoCastSelf(SPELL_SELF_REPAIR);
                _health75 = true;
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
                    case EVENT_DISTRACTING_PAIN:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true, false))
                            DoCast(target, SPELL_DISTRACTING_PAIN);
                        events.Repeat(10s);
                        break;
                    case EVENT_TERRIFY:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 10.0f, true))
                            DoCast(target, SPELL_TERRIFY);
                        events.Repeat(12s);
                        break;
                    case EVENT_SAWBLADE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true, false))
                            DoCast(target, SPELL_SAWBLADE);
                        events.Repeat(8s);
                        break;
                    case EVENT_INITIATE_DECIMATION_PROTOCOL:
                        DoCastSelf(SPELL_INITIATE_DECIMATION_PROTOCOL);
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
            DoCastSelf(SPELL_EJECT_SNEED);
            me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);            
        }

    protected:
        bool _health25;
        bool _health50;
        bool _health75;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDeadminesAI<boss_sneeds_shredderAI>(creature);
    };
};

class boss_sneed : public CreatureScript
{
public:
    boss_sneed() : CreatureScript("boss_sneed") { }

    struct boss_sneedAI : public BossAI
    {
        boss_sneedAI(Creature* creature) : BossAI(creature, DATA_SNEED)
        {
            SneedsShredderData = DATA_SNEEDS_SHREDDER;
        }

        void Reset() override
        {
            BossAI::Reset();
        }

        void JustReachedHome() override
        {
            _JustReachedHome();
            me->DespawnOrUnsummon();

            if (Creature* sneedsShredder = ObjectAccessor::GetCreature(*me, instance->GetGuidData(SneedsShredderData)))
            {
                if (sneedsShredder->isDead())
                {
                    sneedsShredder->Respawn();
                }
            }
        }

        void JustEngagedWith(Unit* victim) override
        {
            BossAI::JustEngagedWith(victim);

            events.ScheduleEvent(EVENT_DISARM, 8s);
            events.ScheduleEvent(EVENT_AXE_TOSS, 3s);
        }

        void JustSummoned(Creature* summon) override
        {
            if (summon->GetEntry() == NPC_SNEEDS_SHREDDER)
            {
                summon->AI()->AttackStart(SelectTarget(SELECT_TARGET_RANDOM));
                DoZoneInCombat(summon);
                me->DespawnOrUnsummon();
            }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) override
        {
            if (!_health15 && HealthBelowPct(15))
            {
                DoCastSelf(SPELL_SUIT_UP);
                _health15 = true;
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
                case EVENT_DISARM:
                    DoCastVictim(SPELL_DISARM);
                    events.Repeat(15s);
                    break;
                case EVENT_AXE_TOSS:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, true))
                        DoCast(target, SPELL_AXE_TOSS);
                    events.Repeat(10s);
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
            if (Creature* sneedsShredder = ObjectAccessor::GetCreature(*me, instance->GetGuidData(SneedsShredderData)))
            {
                if (sneedsShredder->isDead())
                {
                    sneedsShredder->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                }
            }
        }

    protected:
        uint32 SneedsShredderData;
        bool _health15;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDeadminesAI<boss_sneedAI>(creature);
    };
};

void AddSC_boss_sneed()
{
    new boss_sneeds_shredder();
    new boss_sneed();
}
