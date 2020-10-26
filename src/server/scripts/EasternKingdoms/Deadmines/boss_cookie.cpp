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
    SPELL_UNWAHSED_CLAWS            = 81163,
    SPELL_ACID_SPLASH               = 81165,
    SPELL_COOKIES_COOKING           = 81166,
    SPELL_TRY_THIS                  = 81167,
    SPELL_RANCID_FOOD               = 81168,
    SPELL_RAW_FOOD                  = 81170,
    SPELL_POISONED_FOOD             = 81171,
    SPELL_IRRADIATED_FOOD           = 81172,
    SPELL_BILE_RETCH                = 81169
};

enum Texts
{
    SAY_AGGRO = 0,
    SAY_PHASE_1 = 1,    
    SAY_TRY_THIS = 2,
    SAY_SLAY = 3,
    SAY_DEATH = 4
};

class boss_cookie : public CreatureScript
{
public:
    boss_cookie() : CreatureScript("boss_cookie") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDeadminesAI<boss_cookieAI>(creature);
    }

    struct boss_cookieAI : public ScriptedAI
    {
        boss_cookieAI(Creature* creature) : ScriptedAI(creature)
        {
            Initialize();
            instance = creature->GetInstanceScript();
        }

        void Initialize()
        {
            DoCast(me, SPELL_UNWAHSED_CLAWS);

            uiAcidSplashTimer = urand(6000, 8000);
            uiCookiesCookingTimer = 1000;
            uiTryThisTimer = urand(4000, 6000);

            uiPhase = 0;
        }

        InstanceScript* instance;

        uint32 uiAcidSplashTimer;
        uint32 uiCookiesCookingTimer;
        uint32 uiTryThisTimer;

        uint32 uiPhase;

        void Reset() override
        {
            Initialize();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
        }

        void UpdateAI(uint32 uiDiff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uiAcidSplashTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_MAXDISTANCE, 0, 30.0f, true))
                {
                    DoCast(target, SPELL_ACID_SPLASH);
                    if (uiPhase == 1)
                    {
                        uiAcidSplashTimer = 10000;
                    }
                    else
                    uiAcidSplashTimer = 16000;
                }
            }
            else uiAcidSplashTimer -= uiDiff;

            if (uiCookiesCookingTimer <= uiDiff)
            {
                if (uiPhase == 1)
                {
                    DoCast(me, SPELL_COOKIES_COOKING);
                    uiCookiesCookingTimer = urand(18000, 20000);
                }
            }
            else uiCookiesCookingTimer -= uiDiff;

            if (uiTryThisTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                {
                    Talk(SAY_TRY_THIS);
                    DoCast(target, SPELL_TRY_THIS);
                    if (uiPhase == 1)
                    {
                        uiTryThisTimer = 8000;
                    }
                    else
                    uiTryThisTimer = 12000;
                }
            }
            else uiTryThisTimer -= uiDiff;

            if ((!HealthAbovePct(50)))
            {
                uiPhase = 1;
            }
            else
                uiPhase = 0;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }
    };
};

class spell_cookie_try_this : public SpellScriptLoader
{
public:
    spell_cookie_try_this() : SpellScriptLoader("spell_cookie_try_this") { }

    class spell_cookie_try_this_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_cookie_try_this_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_TRY_THIS))
                return false;
            return true;
        }

        void HandleEffect(SpellEffIndex /*effIndex*/)
        {
            std::vector<uint32> spellList =
            {
                SPELL_RANCID_FOOD,
                SPELL_RAW_FOOD,
                SPELL_POISONED_FOOD,
                SPELL_IRRADIATED_FOOD
            };

            uint32 spellId = spellList[urand(0, spellList.size() - 1)];
            switch (spellId)
            {
             default:
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, spellId, true);
                break;
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_cookie_try_this_SpellScript::HandleEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_cookie_try_this_SpellScript();
    }
};

class spell_cookie_bile_retch : public SpellScriptLoader
{
public:
    spell_cookie_bile_retch() : SpellScriptLoader("spell_cookie_bile_retch") { }

    class spell_cookie_bile_retch_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_cookie_bile_retch_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_RANCID_FOOD))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* target = GetExplTargetUnit())
            {
                if (target->HasAura(SPELL_RANCID_FOOD))
                    return;
                else
                    GetCaster()->CastSpell(target, SPELL_RANCID_FOOD, true);
            }
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_cookie_bile_retch_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_cookie_bile_retch_SpellScript();
    }
};

void AddSC_boss_cookie()
{
    new boss_cookie();
    new spell_cookie_try_this();
    new spell_cookie_bile_retch();
}
