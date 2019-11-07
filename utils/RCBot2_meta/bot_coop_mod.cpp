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

void CSynergyMod::initMod()
{
	CBotGlobals::botMessage(NULL, 0, "Synergy Init.");
	// load weapons
	CWeapons::loadWeapons((m_szWeaponListName == NULL) ? "SYNERGY" : m_szWeaponListName, SYNERGYWeaps);
}

void CSynergyMod::mapInit()
{
	// read env_global state
	extern IServerGameEnts* servergameents;
	edict_t* pGlobal = CClassInterface::FindEntityByClassnameNearest(Vector(0, 0, 0), "env_global", 65535);
	CBaseEntity* pGlobalEntity = servergameents->EdictToBaseEntity(pGlobal);
	
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