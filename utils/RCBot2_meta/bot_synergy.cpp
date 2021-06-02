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
#define HL2_DLL
#include "baseentity.h"
#undef HL2_DLL

bool CBotSynergy::isEnemy(edict_t *pEdict, bool bCheckWeapons)
{
    if(m_pEdict == pEdict) // Not self
        return false;

    if(ENTINDEX(pEdict) <= CBotGlobals::maxClients()) // Coop mod, don't attack players
        return false;

    const char* szclassname = pEdict->GetClassName();

    if(strncmp(szclassname, "npc_", 4) == 0) // Attack NPCs
    {// TODO: Filter NPCs
        CBaseEntity* pEntity = pEdict->GetUnknown()->GetBaseEntity();
        
        switch (pEntity->Classify())
        {
        case CLASS_NONE:
        case CLASS_BULLSEYE:
        case CLASS_PLAYER:
        case CLASS_PLAYER_ALLY:
        case CLASS_PLAYER_ALLY_VITAL:
        case CLASS_VORTIGAUNT:
        case CLASS_CITIZEN_PASSIVE:
        case CLASS_CITIZEN_REBEL:
            return false;
            break;
        case CLASS_COMBINE:
        case CLASS_COMBINE_GUNSHIP:
        case CLASS_COMBINE_HUNTER:
        case CLASS_METROPOLICE:
        case CLASS_ANTLION:
        case CLASS_BARNACLE:
        case CLASS_MANHACK:
        case CLASS_ZOMBIE:
        case CLASS_STALKER:
        case CLASS_HEADCRAB:
        case CLASS_SCANNER:
            return true;
            break;
        default:
            return false;
            break;
        }
    }

    return false;
}