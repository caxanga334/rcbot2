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

void CBotCoop::init()
{
	CBot::init();
}

// initialize structures and classes used be the bot
// i.e. create 'new' classes
void CBotCoop::setup()
{
	CBot::setup();
}



bool CBotCoop::startGame()
{
	return true;
}

void CBotCoop::spawnInit()
{
	CBot::spawnInit();

	m_CurrentUtil = BOT_UTIL_MAX;
	// reset objects
	m_pNearbyWeapon = NULL;
	m_pBattery = NULL;
	m_pHealthKit = NULL;
	m_pAmmoKit = NULL;
	m_pCurrentWeapon = NULL;
	m_pCharger = NULL;
	m_pNPCAlyx = NULL;
	m_pNPCBarney = NULL;
	m_fUseButtonTime = 0.0f;
	m_fUseCrateTime = 0.0f;
	m_fScavengeTime = (engine->Time() + RandomFloat(5.0f + 60.0f));
	m_iHealthPack = CClassInterface::getSynPlrHealthPack(m_pEdict);
}

/*
* called when a bot dies
*/
void CBotCoop::died(edict_t* pKiller, const char* pszWeapon)
{
	spawnInit();

	if (m_pSquad != NULL)
	{
		// died
		CBotSquads::removeSquadMember(m_pSquad, m_pEdict);
	}

	m_vLastDiedOrigin = getOrigin();

}

/*
* called when a bot kills something
*/
void CBotCoop::killed(edict_t* pVictim, char* weapon)
{
	if (pVictim == m_pLastEnemy)
		m_pLastEnemy = NULL;

	if( m_pButtons->holdingButton(IN_ATTACK) )
	{
		m_pButtons->letGo(IN_ATTACK);
	}

	if (m_pButtons->holdingButton(IN_ATTACK2))
	{
		m_pButtons->letGo(IN_ATTACK2);
	}

	m_pButtons->tap(IN_RELOAD);
}

void CBotCoop :: modThink ()
{
	// find enemies and health stations / objectives etc
	m_fIdealMoveSpeed = CClassInterface::getMaxSpeed(m_pEdict);
	m_iHealthPack = CClassInterface::getSynPlrHealthPack(m_pEdict);

	CBotWeapon* pWeapon = getCurrentWeapon();
	int iCurrentWpt = m_pNavigator->getCurrentWaypointID();
	CWaypoint* pWpt = CWaypoints::getWaypoint(iCurrentWpt);

	if (CClassInterface::onLadder(m_pEdict) != NULL || (pWpt != NULL && (pWpt->getFlags() & CWaypointTypes::W_FL_LADDER)))
	{
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		setLookAtTask(LOOK_WAYPOINT);
		m_pButtons->holdButton(IN_FORWARD, 0, 1, 0);
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}

	if (getHealthPercent() < 0.50f && !hasSomeConditions(CONDITION_NEED_HEALTH))
	{
		updateCondition(CONDITION_NEED_HEALTH);
		updateCondition(CONDITION_CHANGED);
	}
	else if(getHealthPercent() > 0.51f)
	{
		removeCondition(CONDITION_NEED_HEALTH);
	}

	if (pWeapon && (pWeapon->getClip1(this) == 0) && (pWeapon->getAmmo(this) > 0))
	{
		m_pButtons->tap(IN_RELOAD);
	}

	if (m_pButtons->holdingButton(IN_ATTACK2) && m_pEnemy == NULL)
	{
		m_pButtons->letGo(IN_ATTACK2);
	}
}

void CBotCoop::getTasks(unsigned int iIgnore)
{
	static CBotUtilities utils;
	static CBotUtility* next;
	static CBotWeapon* gravgun;
	static CBotWeapon* crossbow;
	static CWeapon* pWeapon;
	static bool bCheckCurrent;

	if (!hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty())
		return;

	removeCondition(CONDITION_CHANGED);

	bCheckCurrent = true; // important for checking current schedule

	// low on health? Pick some up if there's any near by
	if (hasSomeConditions(CONDITION_NEED_HEALTH))
	{
		ADD_UTILITY(BOT_UTIL_HL2DM_USE_HEALTH_CHARGER, (m_pHealthCharger.get() != NULL) && (CClassInterface::getAnimCycle(m_pHealthCharger) < 1.0f) && (getHealthPercent() < 1.0f), (1.0f - getHealthPercent()));
		ADD_UTILITY(BOT_UTIL_FIND_NEAREST_HEALTH, (m_pHealthKit.get() != NULL) && (getHealthPercent() < 1.0f), 1.0f - getHealthPercent());
		ADD_UTILITY(BOT_UTIL_GETHEALTHKIT, (m_pHealthKit.get() == NULL && m_pHealthCharger.get() == NULL), 1.0f - getHealthPercent());
	}

	if (m_pNearbyTeamMate.get() != NULL)
	{
		int iTeamMateHP = CClassInterface::getPlayerHealth(m_pNearbyTeamMate.get());
		float flTMHP = static_cast<float>(iTeamMateHP) / 100; // max health fixed at 100 for now.
		ADD_UTILITY(BOT_UTIL_MEDIC_HEAL, ((flTMHP < 0.7f) && (m_iHealthPack >= 10)), 1.0f - flTMHP);
	}

	// low on armor?
	ADD_UTILITY(BOT_UTIL_HL2DM_FIND_ARMOR, (m_pBattery.get() != NULL) && (getArmorPercent() < 1.0f), (1.0f - getArmorPercent()) * 0.75f);
	ADD_UTILITY(BOT_UTIL_HL2DM_USE_CHARGER, (m_pCharger.get() != NULL) && (CClassInterface::getAnimCycle(m_pCharger) < 1.0f) && (getArmorPercent() < 1.0f), (1.0f - getArmorPercent()) * 0.75f);

	ADD_UTILITY(BOT_UTIL_HL2DM_USE_CRATE, (m_pAmmoCrate.get() != NULL) && (m_fUseCrateTime < engine->Time()), 1.0f);
	// low on ammo? ammo nearby?
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_AMMO, (m_pAmmoKit.get() != NULL) && (getAmmo(0) < 5), 0.01f * (100 - getAmmo(0)));
	ADD_UTILITY(BOT_UTIL_GETAMMOKIT, ShouldScavengeItems(), 0.01f * (100 - getAmmo(0)));

	// always able to roam around
	ADD_UTILITY(BOT_UTIL_ROAM, true, 0.01f);

	// I have an enemy 
	ADD_UTILITY(BOT_UTIL_FIND_LAST_ENEMY, wantToFollowEnemy() && !m_bLookedForEnemyLast && m_pLastEnemy && CBotGlobals::entityIsValid(m_pLastEnemy) && CBotGlobals::entityIsAlive(m_pLastEnemy), getHealthPercent() * (getArmorPercent() + 0.1));

	if (!hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && hasSomeConditions(CONDITION_SEE_LAST_ENEMY_POS) && m_pLastEnemy && m_fLastSeeEnemy && ((m_fLastSeeEnemy + 10.0) > engine->Time()) && m_pWeapons->hasWeapon(SYN_WEAPON_FRAG))
	{
		float fDistance = distanceFrom(m_vLastSeeEnemyBlastWaypoint);

		if ((fDistance > BLAST_RADIUS) && (fDistance < 1500))
		{
			CWeapon* pWeapon = CWeapons::getWeapon(SYN_WEAPON_FRAG);
			CBotWeapon* pBotWeapon = m_pWeapons->getWeapon(pWeapon);

			ADD_UTILITY(BOT_UTIL_THROW_GRENADE, pBotWeapon && (pBotWeapon->getAmmo(this) > 0), 1.0f - (getHealthPercent() * 0.2));
		}
	}

	if (m_pNearbyWeapon.get())
	{
		pWeapon = CWeapons::getWeapon(m_pNearbyWeapon.get()->GetClassName());

		if (pWeapon && !m_pWeapons->hasWeapon(pWeapon->getID()))
		{
			ADD_UTILITY(BOT_UTIL_PICKUP_WEAPON, true, 0.6f + pWeapon->getPreference() * 0.1f);
		}
	}

	if ((m_pNPCAlyx.get() != NULL || m_pNPCBarney.get() != NULL) && m_pEnemy == NULL)
	{
		CDataInterface datainterface;
		CBaseEntity* pNPCEntity;
		edict_t* pNPCEdict = NULL;

		if (m_pNPCAlyx.get() != NULL)
			pNPCEdict = m_pNPCAlyx;
		else
			pNPCEdict = m_pNPCBarney;

		pNPCEntity = pNPCEdict->GetUnknown()->GetBaseEntity();

		ADD_UTILITY(BOT_UTIL_SYN_GOTO_NPC, true, 1.0f - datainterface.GetEntityHealthPercent(pNPCEntity));
	}


	utils.execute();

	while ((next = utils.nextBest()) != NULL)
	{
		if (!m_pSchedules->isEmpty() && bCheckCurrent)
		{
			if (m_CurrentUtil != next->getId())
				m_pSchedules->freeMemory();
			else
				break;
		}

		bCheckCurrent = false;

		if (executeAction(next->getId()))
		{
			m_CurrentUtil = next->getId();

			if (m_fUtilTimes[next->getId()] < engine->Time())
				m_fUtilTimes[next->getId()] = engine->Time() + randomFloat(0.1f, 2.0f); // saves problems with consistent failing

			if (CClients::clientsDebugging(BOT_DEBUG_UTIL))
			{
				CClients::clientDebugMsg(BOT_DEBUG_UTIL, g_szUtils[next->getId()], this);
			}
			break;
		}
	}

	utils.freeMemory();
}

bool CBotCoop::executeAction(eBotAction iAction)
{
	switch (iAction)
	{
	case BOT_UTIL_HL2DM_USE_CRATE:
		// check if it is worth it first
	{
		const char* szModel;
		char type;
		CBotWeapon* pWeapon = NULL;

		/*
		possible models
		0000000000111111111122222222223333
		0123456789012345678901234567890123
		models/items/ammocrate_ar2.mdl
		models/items/ammocrate_grenade.mdl
		models/items/ammocrate_rockets.mdl
		models/items/ammocrate_smg1.mdl
		*/

		szModel = m_pAmmoCrate.get()->GetIServerEntity()->GetModelName().ToCStr();
		type = szModel[23];

		if (type == 'a') // ar2
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_AR2));
			if (pWeapon == NULL)
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_MG1));
			}
		}
		else if (type == 'g') // grenade
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_FRAG));
		}
		else if (type == 'r') // rocket
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_RPG));
		}
		else if (type == 's') // smg
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_SMG1));
			if (pWeapon == NULL)
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_MP5K));
			}
		}
		else if (type == '3')
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_357));
			if (pWeapon == NULL)
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_DEAGLE));
			}
		}
		else if (type == 'b')
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_SHOTGUN));
		}
		else if (type == 'c')
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_CROSSBOW));
		}
		else if (type == 'p')
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_PISTOL));
		}

		if (pWeapon && (pWeapon->getAmmo(this) < 1))
		{
			CBotSchedule* pSched = new CBotSchedule();

			pSched->addTask(new CFindPathTask(m_pAmmoCrate.get()));
			pSched->addTask(new CBotHL2DMUseButton(m_pAmmoCrate.get()));

			m_pSchedules->add(pSched);

			m_fUtilTimes[iAction] = engine->Time() + randomFloat(5.0f, 10.0f);
			return true;
		}
	}
	return false;
	case BOT_UTIL_PICKUP_WEAPON:
	{
		m_pSchedules->add(new CBotPickupSched(m_pNearbyWeapon.get()));
		return true;
	}
	case BOT_UTIL_FIND_NEAREST_HEALTH:
	{
		m_pSchedules->add(new CBotPickupSched(m_pHealthKit.get()));
		return true;
	}
	case BOT_UTIL_HL2DM_FIND_ARMOR:
	{
		m_pSchedules->add(new CBotPickupSched(m_pBattery.get()));
		return true;
	}
	case BOT_UTIL_FIND_NEAREST_AMMO:
	{
		m_pSchedules->add(new CBotPickupSched(m_pAmmoKit.get()));
		m_fUtilTimes[iAction] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
	}
	case BOT_UTIL_HL2DM_USE_HEALTH_CHARGER:
	{
		CBotSchedule* pSched = new CBotSchedule();

		pSched->addTask(new CFindPathTask(m_pHealthCharger.get()));
		pSched->addTask(new CBotHL2DMUseCharger(m_pHealthCharger.get(), CHARGER_HEALTH));

		m_pSchedules->add(pSched);

		m_fUtilTimes[BOT_UTIL_HL2DM_USE_HEALTH_CHARGER] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
	}
	case BOT_UTIL_HL2DM_USE_CHARGER:
	{
		CBotSchedule* pSched = new CBotSchedule();

		pSched->addTask(new CFindPathTask(m_pCharger.get()));
		pSched->addTask(new CBotHL2DMUseCharger(m_pCharger.get(), CHARGER_ARMOR));

		m_pSchedules->add(pSched);

		m_fUtilTimes[BOT_UTIL_HL2DM_USE_CHARGER] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
	}
	case BOT_UTIL_GETHEALTHKIT:
	{
		CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_HEALTH);

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
			return true;
		}
	}
	case BOT_UTIL_GETAMMOKIT:
	{
		CWaypoint* pWaypoint;

		if (RandomInt(1, 2) == 2)
		{
			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_AMMO_CRATE);
			if(pWaypoint == NULL)
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_AMMO);
		}
		else
			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_AMMO);

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
			return true;
		}

	}
	case BOT_UTIL_MEDIC_HEAL:
	{
		if (m_pNearbyTeamMate.get() != NULL)
		{
			m_pSchedules->add(new CSynBotHealTeamMate(this, m_pNearbyTeamMate.get()));
		}
	}
	case BOT_UTIL_FIND_LAST_ENEMY: // todo: update this code to support NPCs
	{
		Vector vVelocity = Vector(0, 0, 0);
		CClient* pClient = CClients::get(m_pLastEnemy);
		CBotSchedule* pSchedule = new CBotSchedule();

		CFindPathTask* pFindPath = new CFindPathTask(m_vLastSeeEnemy);

		pFindPath->setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);

		if (!CClassInterface::getVelocity(m_pLastEnemy, &vVelocity))
		{
			if (pClient)
				vVelocity = pClient->getVelocity();
		}

		pSchedule->addTask(pFindPath);
		pSchedule->addTask(new CFindLastEnemy(m_vLastSeeEnemy, vVelocity));

		//////////////
		pFindPath->setNoInterruptions();

		m_pSchedules->add(pSchedule);

		m_bLookedForEnemyLast = true;

		return true;
	}
	case BOT_UTIL_THROW_GRENADE:
	{
		// find hide waypoint
		CWaypoint* pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::GetCoverWaypoint(getOrigin(), m_vLastSeeEnemy, NULL));

		if (pWaypoint)
		{
			CBotSchedule* pSched = new CBotSchedule();
			pSched->addTask(new CThrowGrenadeTask(m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_FRAG)), getAmmo(CWeapons::getWeapon(SYN_WEAPON_FRAG)->getAmmoIndex1()), m_vLastSeeEnemyBlastWaypoint)); // first - throw
			pSched->addTask(new CFindPathTask(pWaypoint->getOrigin())); // 2nd -- hide
			m_pSchedules->add(pSched);
			return true;
		}

	}
	case BOT_UTIL_SNIPE:
	{
		// roam
		CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER);

		if (pWaypoint)
		{
			CBotSchedule* snipe = new CBotSchedule();
			CBotTask* findpath = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
			CBotTask* snipetask;

			// use DOD task
			snipetask = new CBotHL2DMSnipe(m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_CROSSBOW)), pWaypoint->getOrigin(), pWaypoint->getAimYaw(), false, 0);

			findpath->setCompleteInterrupt(CONDITION_PUSH);
			snipetask->setCompleteInterrupt(CONDITION_PUSH);

			snipe->setID(SCHED_DEFENDPOINT);
			snipe->addTask(findpath);
			snipe->addTask(snipetask);

			m_pSchedules->add(snipe);

			return true;
		}

		break;
	}
	case BOT_UTIL_ROAM:
	{
		// roam
		CWaypoint* pWaypoint = NULL;
		CWaypoint* pRoute = NULL;
		CBotSchedule* pSched = new CBotSchedule();

		pSched->setID(SCHED_GOTO_ORIGIN);

		int iRandom = RandomInt(1, 3);

		switch (iRandom)
		{
		case 1:
		{
			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_COOP_GOAL);
			CClients::clientDebugMsg(this, BOT_DEBUG_UTIL, "BOT_UTIL_ROAM Random Case: Goal");
		}
		case 2:
		{
			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_COOP_MAPEND);
			if (pWaypoint == NULL) // use GOAL if no MAPEND exist
			{
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_COOP_GOAL);
			}
			CClients::clientDebugMsg(this, BOT_DEBUG_UTIL, "BOT_UTIL_ROAM Random Case: Map End");
		}
		case 3: // any waypoint
		{
			pWaypoint = CWaypoints::randomWaypointGoal(-1);
			CClients::clientDebugMsg(this, BOT_DEBUG_UTIL, "BOT_UTIL_ROAM Random Case: Any");
		}
		}

		

		if (pWaypoint)
		{
			pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), 0, 0);
			if ((m_fUseRouteTime < engine->Time()))
			{
				
				if (pRoute)
				{
					int iRoute = CWaypoints::getWaypointIndex(pRoute);
					pSched->addTask(new CFindPathTask(iRoute, LOOK_WAYPOINT));
					pSched->addTask(new CMoveToTask(pRoute->getOrigin()));
					m_pSchedules->add(pSched);
					m_fUseRouteTime = engine->Time() + 30.0f;
				}
			}

			if (pRoute == NULL)
			{
				int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
				pSched->addTask(new CFindPathTask(iWaypoint, LOOK_WAYPOINT));
				pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
				m_pSchedules->add(pSched);
			}

			return true;
		}

		break;
	}
	case BOT_UTIL_SYN_GOTO_NPC:
	{
		CWaypoint* pWaypoint = NULL;
		Vector vNPC = NULL;

		if (m_pNPCAlyx != NULL)
		{
			vNPC = CBotGlobals::entityOrigin(m_pNPCAlyx);
			pWaypoint = CWaypoints::getWaypoint(CWaypoints::nearestWaypointGoal(-1, vNPC, 512.0f));

			if (pWaypoint)
			{
				m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
				return true;
			}
		}

		if (m_pNPCBarney != NULL)
		{
			vNPC = CBotGlobals::entityOrigin(m_pNPCBarney);
			pWaypoint = CWaypoints::getWaypoint(CWaypoints::nearestWaypointGoal(-1, vNPC, 512.0f));

			if (pWaypoint)
			{
				m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
				return true;
			}
		}
		break;
	}
}

return false;
}

bool CBotCoop::canAvoid(edict_t* pEntity)
{
	float distance;
	Vector vAvoidOrigin;
	const char* classname;

	extern ConVar bot_avoid_radius;

	if (!CBotGlobals::entityIsValid(pEntity))
		return false;
	if (m_pEdict == pEntity) // can't avoid self!!!
		return false;
	if (m_pLookEdict == pEntity)
		return false;
	if (m_pLastEnemy == pEntity)
		return false;

	classname = pEntity->GetClassName();

	if (strcmp(classname, "item_ammo_crate") == 0) // don't avoid crates.
		return false;

	vAvoidOrigin = CBotGlobals::entityOrigin(pEntity);

	distance = distanceFrom(vAvoidOrigin);

	if ((distance > 1) && (distance < bot_avoid_radius.GetFloat()) && (fabs(getOrigin().z - vAvoidOrigin.z) < 32))
	{
		SolidType_t solid = pEntity->GetCollideable()->GetSolid();

		if ((solid == SOLID_BBOX) || (solid == SOLID_VPHYSICS))
		{
			return isEnemy(pEntity, false);
		}
	}

	return false;
}

bool CBotCoop::isEnemy(edict_t* pEdict, bool bCheckWeapons)
{
	extern ConVar rcbot_notarget;
	const char* szclassname;
	static int iGlobalState = CSynergyMod::GetMapGlobal();

	if (iGlobalState == SYN_MAPGLOBAL_PRECRIMINAL)
		return false;

	if (ENTINDEX(pEdict) == 0)
		return false;

	// if no target on - listen sever player is a non target
	if (rcbot_notarget.GetBool())
		return false;

	// not myself
	if (pEdict == m_pEdict)
		return false;

	// no shooting players
	if (ENTINDEX(pEdict) <= CBotGlobals::maxClients())
	{
		return false;
	}

	szclassname = pEdict->GetClassName();

	if (strncmp(szclassname, "npc_", 4) == 0)
	{
		if ((strcmp(szclassname, "npc_antlion") == 0) && iGlobalState == SYN_MAPGLOBAL_ANTLIONSALLY)
			return false;

		if (((strcmp(szclassname, "npc_combinegunship") == 0) || (strcmp(szclassname, "npc_helicopter") == 0) || (strcmp(szclassname, "npc_strider") == 0))
			&& m_pWeapons->hasWeapon(SYN_WEAPON_RPG))
		{
			return true; // ignore gunships, helicopters and striders if I don't have an RPG.
		}

		if (strcmp(szclassname, "npc_metropolice") == 0 || strcmp(szclassname, "npc_combine_s") == 0 || strcmp(szclassname, "npc_manhack") == 0 ||
			strcmp(szclassname, "npc_zombie") == 0 || strcmp(szclassname, "npc_fastzombie") == 0 || strcmp(szclassname, "npc_poisonzombie") == 0 || strcmp(szclassname, "npc_zombine") == 0 ||
			strcmp(szclassname, "npc_antlionguard") == 0 || strcmp(szclassname, "npc_antlion") == 0 || strcmp(szclassname, "npc_headcrab") == 0 || strcmp(szclassname, "npc_headcrab_fast") == 0 ||
			strcmp(szclassname, "npc_headcrab_black") == 0 || strcmp(szclassname, "npc_hunter") == 0 || strcmp(szclassname, "npc_fastzombie_torso") == 0 || strcmp(szclassname, "npc_zombie_torso") == 0 ||
			strcmp(szclassname, "npc_barnacle") == 0)
		{
			return true;
		}
	}

	if (strcmp(szclassname, "item_item_crate") == 0)
		return true;

	return false;
}

void CBotCoop::voiceCommand(const int cmd)
{
	char vcmd[32];

	sprintf(vcmd, "menuselect %d", cmd);

	helpers->ClientCommand(m_pEdict, "voice_menu 0");
	helpers->ClientCommand(m_pEdict, vcmd);
}

void CBotCoop::hearVoiceCommand(edict_t* pPlayer, byte cmd)
{
	CBotGlobals::botMessage(NULL, 0, "CBotCoop::hearVoiceCommand");
}

bool CBotCoop::setVisible(edict_t* pEntity, bool bVisible)
{
	static float fDist;
	const char* szClassname;
	szClassname = pEntity->GetClassName();
	CClients::clientDebugMsg(this, BOT_DEBUG_EDICTS, "CBotCoop::setVisible | %s", szClassname);

	bool bValid = CBot::setVisible(pEntity, bVisible);

	fDist = distanceFrom(pEntity);

	// if no draw effect it is invisible
	if (bValid && bVisible && !(CClassInterface::getEffects(pEntity) & EF_NODRAW))
	{
		//szClassname = pEntity->GetClassName();

		if ((strncmp(szClassname, "item_ammo", 9) == 0) &&
			(!m_pAmmoKit.get() || (fDist < distanceFrom(m_pAmmoKit.get()))))
		{
			m_pAmmoKit = pEntity;
			if (strcmp(szClassname, "item_ammo_crate") == 0)
				m_pAmmoKit = NULL;
		}
		else if ((strncmp(szClassname, "item_health", 11) == 0) &&
			(!m_pHealthKit.get() || (fDist < distanceFrom(m_pHealthKit.get()))))
		{
			m_pHealthKit = pEntity;
		}
		else if ((strcmp(szClassname, "item_battery") == 0) &&
			(!m_pBattery.get() || (fDist < distanceFrom(m_pBattery.get()))))
		{
			m_pBattery = pEntity;
		}
		else if ((strcmp(szClassname, "npc_alyx") == 0) &&
			(!m_pNPCAlyx.get() || (fDist < distanceFrom(m_pNPCAlyx.get()))))
		{
			m_pNPCAlyx = pEntity;
		}
		else if ((strcmp(szClassname, "npc_barney") == 0) &&
			(!m_pNPCBarney.get() || (fDist < distanceFrom(m_pNPCBarney.get()))))
		{
			m_pNPCBarney = pEntity;
		}
		else if (((strcmp(szClassname, "func_breakable") == 0) || (strncmp(szClassname, "prop_physics", 12) == 0)) && (CClassInterface::getPlayerHealth(pEntity) > 0) &&
			(!m_NearestBreakable.get() || (fDist < distanceFrom(m_NearestBreakable.get()))))
		{
			m_NearestBreakable = pEntity;
		}
		else if ((pEntity != m_pNearestButton) && (strcmp(szClassname, "func_button") == 0))
		{
			if (!m_pNearestButton.get() || (fDist < distanceFrom(m_pNearestButton.get())))
				m_pNearestButton = pEntity;
		}
		else if ((pEntity != m_pAmmoCrate) && (strcmp(szClassname, "item_ammo_crate") == 0))
		{
			if (!m_pAmmoCrate.get() || (fDist < distanceFrom(m_pAmmoCrate.get())))
				m_pAmmoCrate = pEntity;
		}
		else if ((strncmp(szClassname, "item_suitcharger", 16) == 0) &&
			(!m_pCharger.get() || (fDist < distanceFrom(m_pCharger.get()))))
		{
			if (m_pCharger.get())
			{
				// less juice than the current one I see
				if (CClassInterface::getAnimCycle(m_pCharger) < CClassInterface::getAnimCycle(pEntity))
				{
					return bValid;
				}
			}

			if (m_pPlayerInfo->GetArmorValue() < 50)
				updateCondition(CONDITION_CHANGED);

			m_pCharger = pEntity;
		}
		else if ((strncmp(szClassname, "item_healthcharger", 18) == 0) &&
			(!m_pHealthCharger.get() || (fDist < distanceFrom(m_pHealthCharger.get()))))
		{
			if (m_pHealthCharger.get())
			{
				// less juice than the current one I see - forget it
				if (CClassInterface::getAnimCycle(m_pHealthCharger) < CClassInterface::getAnimCycle(pEntity))
				{
					return bValid;
				}
			}

			if (getHealthPercent() < 1.0f)
				updateCondition(CONDITION_CHANGED); // update tasks

			m_pHealthCharger = pEntity;
		}
		else if ((strncmp(szClassname, "weapon_", 7) == 0) &&
			(!m_pNearbyWeapon.get() || (fDist < distanceFrom(m_pNearbyWeapon.get()))))
		{


			m_pNearbyWeapon = pEntity;
		}
		else if ((strcmp(szClassname, "player") == 0) && pEntity != m_pEdict &&
			(!m_pNearbyTeamMate.get() || (fDist < distanceFrom(m_pNearbyTeamMate.get()))))
		{
			m_pNearbyTeamMate = pEntity;
		}
	}
	else
	{
	if (m_pAmmoKit == pEntity)
		m_pAmmoKit = NULL;
	else if (m_pAmmoCrate == pEntity)
		m_pAmmoCrate = NULL;
	else if (m_pHealthKit == pEntity)
		m_pHealthKit = NULL;
	else if (m_pBattery == pEntity)
		m_pBattery = NULL;
	else if (m_pCharger == pEntity)
		m_pCharger = NULL;
	else if (m_pHealthCharger == pEntity)
		m_pHealthCharger = NULL;
	else if (m_NearestBreakable == pEntity)
		m_NearestBreakable = NULL;
	else if (m_pNearbyWeapon == pEntity)
		m_pNearbyWeapon = NULL;
	else if (m_pNearestButton == pEntity)
		m_pNearestButton = NULL;
	else if (m_pNPCAlyx == pEntity)
		m_pNPCAlyx = NULL;
	else if (m_pNPCBarney == pEntity)
		m_pNPCBarney = NULL;
	}

	return bValid;
}

void CBotCoop::touchedWpt(CWaypoint* pWaypoint, int iNextWaypoint, int iPrevWaypoint)
{
	resetTouchDistance(48.0f);

	// if the waypoint contains both ladder & use flag and the bot is not on a ladder, press use.
	if (pWaypoint->getFlags() & CWaypointTypes::W_FL_LADDER)
	{
		if (CClassInterface::onLadder(m_pEdict) == NULL && (pWaypoint->getFlags() & CWaypointTypes::W_FL_USE_BUTTON))
		{
			use();
			debugMsg(BOT_DEBUG_THINK, "Ladder Waypoint: Pressing use");
		}
	}

	if (pWaypoint->getFlags() & CWaypointTypes::W_FL_USE_BUTTON)
	{
		CClients::clientDebugMsg(this, BOT_DEBUG_THINK, "(%s) Use Waypoint!", m_szBotName);
		edict_t* pDoor = CClassInterface::FindNearbyEntityByClassname(getOrigin(), "prop_door_rotating", 256.0f);

		if (pDoor)
		{
			CSynergyMod synmod;
			CDataInterface data;
			CBaseEntity* pEntity = pDoor->GetUnknown()->GetBaseEntity();
			int doorstate = synmod.GetPropDoorState(pEntity);
			CClients::clientDebugMsg(this, BOT_DEBUG_THINK, "Use Waypoint: Received door state: %i", doorstate);

			bool bLocked = data.IsEntityLocked(pEntity);

			if (!bLocked) // door is not locked
			{
				if (doorstate == 0) // found closed door near use waypoint
				{
					CBotSchedule* pSched = new CBotSchedule();

					pSched->addTask(new CMoveToTask(pDoor));
					pSched->addTask(new CBotHL2DMUseButton(pDoor));

					m_pSchedules->addFront(pSched);
					debugMsg(BOT_DEBUG_THINK, "USE Waypoint: Found prop_door_rotating");
				}
				else
					debugMsg(BOT_DEBUG_THINK, "Use Waypoint: Door is already open");
			}
		}

		edict_t* pButton = CClassInterface::FindNearbyEntityByClassname(getOrigin(), "func_button", 256.0f);

		if (pButton && pDoor == NULL)
		{
			CDataInterface data;
			CBaseEntity* pEntity = pButton->GetUnknown()->GetBaseEntity();
			bool bLocked = data.IsEntityLocked(pEntity);
			if (!bLocked)
			{
				CBotSchedule* pSched = new CBotSchedule();

				pSched->addTask(new CMoveToTask(pButton));
				pSched->addTask(new CBotHL2DMUseButton(pButton));
				pSched->addTask(new CBotDefendTask(getOrigin(), 1.5f)); // hack, make bot wait

				m_pSchedules->addFront(pSched);
				debugMsg(BOT_DEBUG_THINK, "USE Waypoint: Found func_button");
			}
		}
	}

	m_fWaypointStuckTime = engine->Time() + randomFloat(7.5f, 12.5f);

	if (pWaypoint->getFlags() & CWaypointTypes::W_FL_JUMP)
		jump();
	if (pWaypoint->getFlags() & CWaypointTypes::W_FL_CROUCH)
		duck();
	if (pWaypoint->getFlags() & CWaypointTypes::W_FL_SPRINT)
		updateCondition(CONDITION_RUN);

	updateDanger(m_pNavigator->getBelief(CWaypoints::getWaypointIndex(pWaypoint)));
}

// returns true if offset has been applied when not before
bool CBotCoop::walkingTowardsWaypoint(CWaypoint* pWaypoint, bool* bOffsetApplied, Vector& vOffset)
{
	if (pWaypoint->getFlags() & CWaypointTypes::W_FL_NO_VEHICLES)
	{
		CSynergyMod synmod;
		if (synmod.IsPlayerInVehicle(m_pEdict))
		{
			use();
		}
	}

	if (pWaypoint->hasFlag(CWaypointTypes::W_FL_CROUCH))
	{
		duck(true);
	}

	if (pWaypoint->hasFlag(CWaypointTypes::W_FL_LIFT))
	{
		updateCondition(CONDITION_LIFT);
	}
	else
	{
		removeCondition(CONDITION_LIFT);
	}

	if (!*bOffsetApplied)
	{
		float fRadius = pWaypoint->getRadius();

		if (fRadius > 0)
			vOffset = Vector(randomFloat(-fRadius, fRadius), randomFloat(-fRadius, fRadius), 0);
		else
			vOffset = Vector(0, 0, 0);

		*bOffsetApplied = true;

		return true;
	}

	return false;
}

// Called when working out route
bool CBotCoop::canGotoWaypoint(Vector vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev)
{
	return CBot::canGotoWaypoint(vPrevWaypoint, pWaypoint, pPrev);
}

bool CBotCoop::handleAttack(CBotWeapon* pWeapon, edict_t* pEnemy)
{
	const char* enemyclassname = pEnemy->GetClassName();
	if (pWeapon)
	{
		clearFailedWeaponSelect();

		if (pWeapon->isMelee())
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));

		if (pWeapon->mustHoldAttack())
			primaryAttack(true);
		else
			primaryAttack();
	}
	else
		primaryAttack();

	if (strcmp(enemyclassname, "item_item_crate") == 0)
		setMoveTo(CBotGlobals::entityOrigin(pEnemy));

	return true;
}

void CBotCoop::handleWeapons()
{
	//
	// Handle attacking at this point
	//
	if (m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD) &&
		hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot() &&
		isVisible(m_pEnemy) && isEnemy(m_pEnemy))
	{
		CBotWeapon* pWeapon;
		CBotWeapon* pRPG = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_RPG));

		edict_t* pEnemy = m_pEnemy.get();
		const char* enemyclassname = pEnemy->GetClassName();
		
		if ((strcmp(enemyclassname, "npc_combinegunship") == 0) || (strcmp(enemyclassname, "npc_helicopter") == 0) || (strcmp(enemyclassname, "npc_strider") == 0))
		{
			if (pRPG && pRPG->hasWeapon() && !pRPG->outOfAmmo(this))
			{
				pWeapon = pRPG;
				if ((pWeapon != NULL) && (pWeapon != getCurrentWeapon()))
					selectWeapon(pWeapon->getWeaponIndex());
			}	
		}
		else
			pWeapon = getBestWeapon(m_pEnemy);

		if (strcmp(enemyclassname, "item_item_crate") == 0)
		{
			pWeapon = getBestWeapon(m_pEnemy, true, true, true, false); // use melee to break crates
			if ((pWeapon != NULL) && (pWeapon != getCurrentWeapon()))
				selectWeapon(pWeapon->getWeaponIndex());
		}


		if (m_bWantToChangeWeapon && (pWeapon != NULL) && (pWeapon != getCurrentWeapon()) && pWeapon->getWeaponIndex())
		{
			//selectWeaponSlot(pWeapon->getWeaponInfo()->getSlot());
			selectWeapon(pWeapon->getWeaponIndex());
		}

		setLookAtTask(LOOK_ENEMY);

		if (!handleAttack(pWeapon, m_pEnemy))
		{
			m_pEnemy = NULL;
			m_pOldEnemy = NULL;
			wantToShoot(false);
		}
	}
}

bool CBotCoop::checkStuck()
{
	bool bStuck = CBot::checkStuck();
	return bStuck;
}

bool CBotCoop::ShouldScavengeItems(float fNextDelay)
{
	if (engine->Time() > m_fScavengeTime)
	{
		if (fNextDelay > 0.0f)
		{
			m_fScavengeTime = (engine->Time() + fNextDelay);
		}
		else
			m_fScavengeTime = (engine->Time() + RandomFloat(45.0f, 160.0f));

		return true;
	}
	else
		return false;
}

void CBotCoop::HealPlayer()
{
	helpers->ClientCommand(m_pEdict, "drophealth");
}