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

#include "vstdlib/random.h" // for random functions


void CBotCoop :: modThink ()
{
	// find enemies and health stations / objectives etc
}

void CBotCoop::getTasks(unsigned int iIgnore)
{
	static CBotUtilities utils;

	// roam
	CWaypoint* pWaypoint = CWaypoints::getWaypoint(CWaypoints::randomFlaggedWaypoint(0));

	if (pWaypoint)
	{
		m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
	}

	ADD_UTILITY(BOT_UTIL_ROAM, !hasEnemy(), 1.0f);
}

bool CBotCoop :: isEnemy ( edict_t *pEdict,bool bCheckWeapons )
{
	const char *classname;

	if ( ENTINDEX(pEdict) == 0 ) 
		return false;

	// no shooting players
	if ( ENTINDEX(pEdict) <= CBotGlobals::maxClients() )
	{
		return false;
	}

	classname = pEdict->GetClassName();

	return this->IsNPCEnemy(classname);
}

bool CBotCoop :: startGame ()
{
	return true;
}

void CSynergyMod::initMod()
{
	//
}

void CSynergyMod::mapInit()
{
	//
}

// checks if an NPC is hostile
bool CBotCoop::IsNPCEnemy(const char *classname)
{
	if (strncmp(classname, "npc_", 4) == 0)
	{
		if (!strcmp(classname, "npc_barney") || !strcmp(classname, "npc_alyx") || !strcmp(classname, "npc_kleiner") ||
			!strcmp(classname, "npc_magnusson") || !strcmp(classname, "npc_citizen") || !strcmp(classname, "npc_vortigaunt") ||
			!strcmp(classname, "npc_monk") || !strcmp(classname, "npc_dog") || !strcmp(classname, "npc_eli") ||
			!strcmp(classname, "npc_mossman") || !strcmp(classname, "npc_gman") || !strcmp(classname, "npc_pigeon") || !strcmp(classname, "npc_seagull") ||
			!strcmp(classname, "npc_crow"))
		{
			return false;
		}
		else { return true; }
	}

	return false;
}
