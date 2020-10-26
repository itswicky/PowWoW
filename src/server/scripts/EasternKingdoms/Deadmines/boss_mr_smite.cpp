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

enum Spells
{
    SPELL_THRASH                = 3391,
    SPELL_SMITE_STOMP           = 6432,
    SPELL_SMITE_SLAM            = 6435,
    SPELL_NIMBLE_REFLEXES       = 6264,
    SPELL_CANNON_STRIKE_SUMMON  = 81160,
    SPELL_CANNON_STRIKE_AURA    = 81158,
    SPELL_BLUNDERBUSS_SHOT      = 81156,
    SPELL_GROUND_SLAM           = 81157,
    SPELL_DUAL_WIELDING         = 81161,
    SPELL_SMITES_MIGHTY_HAMMER  = 81162
};

enum Equips
{
    EQUIP_SWORD             = 5191,
    EQUIP_AXE               = 5196,
    EQUIP_MACE              = 7230
};

enum Texts
{
    SAY_PHASE_1             = 2,
    SAY_PHASE_2             = 3,
    SAY_AGGRO               = 4,
    SAY_CANNON_STRIKE       = 5,
    SAY_SLAY                = 6,
    SAY_DEATH               = 7
};

class boss_mr_smite : public CreatureScript
{
public:
    boss_mr_smite() : CreatureScript("boss_mr_smite") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDeadminesAI<boss_mr_smiteAI>(creature);
    }

    struct boss_mr_smiteAI : public ScriptedAI
    {
        boss_mr_smiteAI(Creature* creature) : ScriptedAI(creature)
        {
            Initialize();
            instance = creature->GetInstanceScript();
        }

        void Initialize()
        {
            uiThrashTimer = urand(5000, 9000);
            uiSlamTimer = 7000;
            uiNimbleReflexesTimer = urand(15500, 19600);
            uiCannonStrikeTimer = urand(9000, 14500);
            uiBlunderbussShotTimer = urand(5000, 7500);
            uiGroundSlamTimer = urand(6200, 8200);

            uiHealth = 0;

            uiPhase = 0;
            uiTimer = 0;

            uiIsMoving = false;
        }

        InstanceScript* instance;

        uint32 uiThrashTimer;
        uint32 uiSlamTimer;
        uint32 uiNimbleReflexesTimer;
        uint32 uiCannonStrikeTimer;
        uint32 uiBlunderbussShotTimer;
        uint32 uiGroundSlamTimer;

        uint8 uiHealth;

        uint32 uiPhase;
        uint32 uiTimer;

        bool uiIsMoving;

        void Reset() override
        {
            Initialize();

            SetEquipmentSlots(false, EQUIP_SWORD, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveAura(SPELL_DUAL_WIELDING);
            me->RemoveAura(SPELL_SMITES_MIGHTY_HAMMER);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            uiPhase = 5;
        }

        void UpdateAI(uint32 uiDiff) override
        {
            if (!UpdateVictim())
                return;

            if (!uiIsMoving) // halt abilities in between phases
            {
                if (uiThrashTimer <= uiDiff)
                {
                    if (uiPhase == 6)
                    {
                        DoCast(me, SPELL_THRASH);
                        uiThrashTimer = urand(6000, 9000);
                    }
                }
                else uiThrashTimer -= uiDiff;

                if (uiSlamTimer <= uiDiff)
                {
                    DoCastVictim(SPELL_SMITE_SLAM);
                    uiSlamTimer = 11000;
                }
                else uiSlamTimer -= uiDiff;

                if (uiNimbleReflexesTimer <= uiDiff)
                {
                    DoCast(me, SPELL_NIMBLE_REFLEXES);
                    uiNimbleReflexesTimer = urand(20000, 24000);
                }
                else uiNimbleReflexesTimer -= uiDiff;

                if (uiCannonStrikeTimer <= uiDiff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                    {
                        DoCast(target, SPELL_CANNON_STRIKE_SUMMON);
                        Talk(SAY_CANNON_STRIKE);

                        if (Creature* trigger = me->FindNearestCreature(50012, 100.0f, true))
                            DoCast(trigger, SPELL_CANNON_STRIKE_AURA);

                        uiCannonStrikeTimer = urand(12000, 18000);
                    }                         
                }
                else uiCannonStrikeTimer -= uiDiff;

                if (uiBlunderbussShotTimer <= uiDiff)
                {
                    if (uiPhase == 5)
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true, false))
                    {
                        DoCast(target, SPELL_BLUNDERBUSS_SHOT);
                        uiBlunderbussShotTimer = urand(9000, 110000);
                    }
                }
                else uiBlunderbussShotTimer -= uiDiff;

                if (uiGroundSlamTimer <= uiDiff)
                {
                    if (uiPhase == 7)
                    {
                        DoCast(me, SPELL_GROUND_SLAM);
                        uiGroundSlamTimer = urand(16000, 18000);
                    }
                }
                else uiGroundSlamTimer -= uiDiff;
            }

            if ((uiHealth == 0 && !HealthAbovePct(66)) || (uiHealth == 1 && !HealthAbovePct(33)))
            {
                ++uiHealth;
                DoCastAOE(SPELL_SMITE_STOMP, false);
                SetCombatMovement(false);
                me->AttackStop();
                me->InterruptNonMeleeSpells(false);
                me->SetReactState(REACT_PASSIVE);
                uiTimer = 2500;
                uiPhase = 1;

                switch (uiHealth)
                {
                    case 1:
                        Talk(SAY_PHASE_1);
                        break;
                    case 2:
                        Talk(SAY_PHASE_2);
                        break;
                }
            }

            if (uiPhase)
            {
                if (uiTimer <= uiDiff)
                {
                    switch (uiPhase)
                    {
                        case 1:
                        {
                            if (uiIsMoving)
                                break;

                            if (GameObject* go = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(DATA_SMITE_CHEST)))
                            {
                                me->GetMotionMaster()->Clear();
                                me->GetMotionMaster()->MovePoint(1, go->GetPositionX() - 1.5f, go->GetPositionY() + 1.4f, go->GetPositionZ());
                                uiIsMoving = true;
                            }
                            break;
                        }
                        case 2:
                            if (uiHealth == 1)
                            {
                                SetEquipmentSlots(false, EQUIP_AXE, EQUIP_AXE, EQUIP_NO_CHANGE);
                                me->AddAura(SPELL_DUAL_WIELDING, me);
                            }
                            else
                            {
                                SetEquipmentSlots(false, EQUIP_MACE, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
                                me->RemoveAura(SPELL_DUAL_WIELDING);
                                me->AddAura(SPELL_SMITES_MIGHTY_HAMMER, me);
                            }
                            uiTimer = 500;
                            uiPhase = 3;
                            break;
                        case 3:
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            uiTimer = 750;
                            uiPhase = 4;
                            break;
                        case 4:
                            me->SetReactState(REACT_AGGRESSIVE);
                            SetCombatMovement(true);
                            me->GetMotionMaster()->MoveChase(me->GetVictim(), me->m_CombatDistance);
                            uiIsMoving = false;
                            if (uiHealth == 1)
                                uiPhase = 6;
                            else
                            {
                                uiPhase = 7;
                                uiGroundSlamTimer = 3500;
                            }
                            break;
                    }
                } else uiTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }

        void MovementInform(uint32 uiType, uint32 /*uiId*/) override
        {
            if (uiType != POINT_MOTION_TYPE)
                return;

            me->SetFacingTo(5.47f);
            me->SetStandState(UNIT_STAND_STATE_KNEEL);

            uiTimer = 2000;
            uiPhase = 2;
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

void AddSC_boss_mr_smite()
{
    new boss_mr_smite();
}
