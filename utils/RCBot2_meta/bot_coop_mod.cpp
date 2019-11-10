/*
 *    part of https://rcbot2.svn.sourceforge.net/svnroot/rcbot2
 *
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include "bot.h"
#include "bot_coop.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "ndebugoverlay.h"
#include "bot_squads.h"
#include "in_buttons.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_task.h"
#include "bot_schedule.h"
#include "bot_weapons.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_navigator.h"
#include "bot_perceptron.h"
#include "bot_waypoint_visibility.h"
#include "bot_mods.h"

#include "vstdlib/random.h" // for random functions

int CSynergyMod::m_iMapGlobal = SYN_MAPGLOBAL_NONE;

void CSynergyMod::initMod()
{
	CBotGlobals::botMessage(NULL, 0, "Synergy Init.");
	// load weapons
	CWeapons::loadWeapons((m_szWeaponListName == NULL) ? "SYNERGY" : m_szWeaponListName, SYNERGYWeaps);
}

void CSynergyMod::mapInit()
{
	CBotGlobals::botMessage(NULL, 0, "Synergy: Map Init.");
	static char *szmapname = CBotGlobals::getMapName();
	// hard coding these until another solution is found.
	if (strncmp(szmapname, "d1_trainstation", 15) == 0)
	{
		m_iMapGlobal = SYN_MAPGLOBAL_PRECRIMINAL; // TRAIN STATION
	}
	else if (strncmp(szmapname, "d2_coast_12", 11) == 0)
	{
		m_iMapGlobal = SYN_MAPGLOBAL_ANTLIONSALLY;
	}
	else if (strncmp(szmapname, "d2_prison_01", 11) == 0)
	{
		m_iMapGlobal = SYN_MAPGLOBAL_ANTLIONSALLY;
	}
	else if (strncmp(szmapname, "d3_citadel", 10) == 0)
	{
		m_iMapGlobal = SYN_MAPGLOBAL_SUPER_PHYSGUN;
	}
	else
	{
		m_iMapGlobal = SYN_MAPGLOBAL_NONE;
	}
	CBotGlobals::botMessage(NULL, 0, "Synergy m_iMapGlobal %d.", m_iMapGlobal);
}

// Gets a random Player
edict_t *CSynergyMod::GetRandomPlayer(edict_t *pIgnore)
{
	// Client Edict
	edict_t* pRandom;
	// Random Client Index
	int iRCI;

	// 20 search attempts
	for (int i = 0; i < 20; i++)
	{
		iRCI = RandomInt(1, gpGlobals->maxClients);
		pRandom = engine->PEntityOfEntIndex(iRCI);
		
		if (CBotGlobals::entityIsValid(pRandom) && (!pRandom->IsFree()) && (pRandom != pIgnore)) // is valid entity and is not being ignored
		{
			if (CBotGlobals::entityIsAlive(pRandom)) // is alive
			{
				return pRandom;
			}
		}
	}

	return NULL;
}

void CSynergyMod::addWaypointFlags(edict_t* pPlayer, edict_t* pEdict, int* iFlags, int* iArea, float* fMaxDistance)
{
	if (strncmp(pEdict->GetClassName(), "trigger_hurt", 12) == 0)
		*iFlags |= CWaypointTypes::W_FL_TRIGGER_HURT;
}