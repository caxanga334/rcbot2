/*
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
#include "server_class.h"

#include "bot.h"

#include "in_buttons.h"

#include "bot_mods.h"
#include "bot_globals.h"
#include "bot_weapons.h"
#include "bot_configfile.h"
#include "bot_getprop.h"
#include "bot_css_bot.h"
#include "bot_navigator.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_perceptron.h"

#include "rcbot/logging.h"

// For debug messages
const char *szMapTypes[CS_MAP_MAX+1] =
{
    "DEATHMATCH",
    "BOMB DEFUSAL",
    "HOSTAGE RESCUE",
    "MAP TYPE MAX"
};

eCSSMapType CCounterStrikeSourceMod::m_MapType = CS_MAP_DEATHMATCH;

void CCounterStrikeSourceMod::initMod()
{
    CWeapons::loadWeapons((m_szWeaponListName == NULL) ? "CSS" : m_szWeaponListName, CSSWeaps); // Load weapon list
    logger->Log(LogLevel::TRACE, "CCounterStrikeSourceMod::initMod()");
}

void CCounterStrikeSourceMod::mapInit()
{
	const string_t mapname = gpGlobals->mapname;
	const char *szmapname = mapname.ToCStr();

    if(strncmp(szmapname, "de_", 3) == 0)
        m_MapType = CS_MAP_BOMBDEFUSAL;
    else if(strncmp(szmapname, "cs_", 3) == 0)
        m_MapType = CS_MAP_HOSTAGERESCUE;
    else
        m_MapType = CS_MAP_DEATHMATCH;

    logger->Log(LogLevel::TRACE, "CCounterStrikeSourceMod::mapInit()\nMap Type: %s", szMapTypes[m_MapType]);
}