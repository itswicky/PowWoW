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

// This is where scripts' loading functions should be declared:

void AddSC_GOMove_commandscript();
void AddSC_FallOfDalaran();
void AddSC_DragonIsles();
void AddSC_WorldsEnd();
void AddSC_chromie_script();
void AddSC_Spells_Custom_Items();
void AddSC_Spells_Custom_Talents();
void AddSC_Spells_Custom_Generic();
void AddSC_NPCS_Custom_Pets();
void AddSC_Instance_Dummies();
void AddSC_TbsBullshit_commandscript();

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddCustomScripts()
{
    AddSC_GOMove_commandscript();
    AddSC_FallOfDalaran();
    AddSC_DragonIsles();
    AddSC_WorldsEnd();
    AddSC_chromie_script();
    AddSC_Spells_Custom_Items();
    AddSC_Spells_Custom_Talents();
    AddSC_Spells_Custom_Generic();
    AddSC_NPCS_Custom_Pets();
    AddSC_Instance_Dummies();
    AddSC_TbsBullshit_commandscript();
}
