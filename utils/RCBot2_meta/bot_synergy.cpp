#include "engine_wrappers.h"

#include "bot.h"
#include "bot_cvars.h"
#include "ndebugoverlay.h"
#include "bot_squads.h"
#include "bot_synergy.h"
#include "in_buttons.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_mods.h"
#include "bot_task.h"
#include "bot_schedule.h"
#include "bot_weapons.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_navigator.h"
#include "bot_perceptron.h"
#include "bot_waypoint_visibility.h"

bool CBotSynergy::isEnemy(edict_t *pEdict, bool bCheckWeapons)
{
    if(m_pEdict == pEdict) // Not self
        return false;

    if(ENTINDEX(pEdict) <= CBotGlobals::maxClients()) // Coop mod, don't attack players
        return false;

    const char* szclassname = pEdict->GetClassName();

    if(strncmp(szclassname, "npc_", 4) == 0) // Attack NPCs
    {// TODO: Filter NPCs
        return true;
    }

    return false;
}