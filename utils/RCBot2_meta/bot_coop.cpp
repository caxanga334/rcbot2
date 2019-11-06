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

void CSynergyMod::initMod()
{
	CBotGlobals::botMessage(NULL, 0, "Synergy Init.");
	// load weapons
	CWeapons::loadWeapons((m_szWeaponListName == NULL) ? "SYNERGY" : m_szWeaponListName, SYNERGYWeaps);
}

void CSynergyMod::mapInit()
{
	//
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
	m_fUseButtonTime = 0.0f;
	m_fUseCrateTime = 0.0f;
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

	CBotWeapon* pWeapon = getCurrentWeapon();

	if (CClassInterface::onLadder(m_pEdict) != NULL)
	{
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		setLookAtTask(LOOK_WAYPOINT);
		m_pButtons->holdButton(IN_FORWARD, 0, 1, 0);
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}

	if (getHealthPercent() < 0.35f && !hasSomeConditions(CONDITION_NEED_HEALTH))
	{
		updateCondition(CONDITION_NEED_HEALTH);
		updateCondition(CONDITION_CHANGED);
	}
	else if(getHealthPercent() > 0.36f)
	{
		removeCondition(CONDITION_NEED_HEALTH);
	}

	if (pWeapon && (pWeapon->getClip1(this) == 0) && (pWeapon->getAmmo(this) > 0))
	{
		m_pButtons->tap(IN_RELOAD);
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
	ADD_UTILITY(BOT_UTIL_HL2DM_USE_HEALTH_CHARGER, (m_pHealthCharger.get() != NULL) && (CClassInterface::getAnimCycle(m_pHealthCharger) < 1.0f) && (getHealthPercent() < 1.0f), (1.0f - getHealthPercent()));
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_HEALTH, (m_pHealthKit.get() != NULL) && (getHealthPercent() < 1.0f), 1.0f - getHealthPercent());

	// low on armor?
	ADD_UTILITY(BOT_UTIL_HL2DM_FIND_ARMOR, (m_pBattery.get() != NULL) && (getArmorPercent() < 1.0f), (1.0f - getArmorPercent()) * 0.75f);
	ADD_UTILITY(BOT_UTIL_HL2DM_USE_CHARGER, (m_pCharger.get() != NULL) && (CClassInterface::getAnimCycle(m_pCharger) < 1.0f) && (getArmorPercent() < 1.0f), (1.0f - getArmorPercent()) * 0.75f);

	ADD_UTILITY(BOT_UTIL_HL2DM_USE_CRATE, (m_pAmmoCrate.get() != NULL) && (m_fUseCrateTime < engine->Time()), 1.0f);
	// low on ammo? ammo nearby?
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_AMMO, (m_pAmmoKit.get() != NULL) && (getAmmo(0) < 5), 0.01f * (100 - getAmmo(0)));

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

		if (pWeapon && (pWeapon->getAmmo(this) < 1))
		{
			CBotSchedule* pSched = new CBotSchedule();

			pSched->addTask(new CFindPathTask(m_pAmmoCrate));
			pSched->addTask(new CBotHL2DMUseButton(m_pAmmoCrate));

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

		pSched->addTask(new CFindPathTask(m_pHealthCharger));
		pSched->addTask(new CBotHL2DMUseCharger(m_pHealthCharger, CHARGER_HEALTH));

		m_pSchedules->add(pSched);

		m_fUtilTimes[BOT_UTIL_HL2DM_USE_HEALTH_CHARGER] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
	}
	case BOT_UTIL_HL2DM_USE_CHARGER:
	{
		CBotSchedule* pSched = new CBotSchedule();

		pSched->addTask(new CFindPathTask(m_pCharger));
		pSched->addTask(new CBotHL2DMUseCharger(m_pCharger, CHARGER_ARMOR));

		m_pSchedules->add(pSched);

		m_fUtilTimes[BOT_UTIL_HL2DM_USE_CHARGER] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
	}
	case BOT_UTIL_FIND_LAST_ENEMY:
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
		CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_COOP_GOAL);

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
			return true;
		}

		break;
	}
}

	return false;
}

bool CBotCoop::isEnemy(edict_t* pEdict, bool bCheckWeapons)
{
	extern ConVar rcbot_notarget;
	const char* szclassname;

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

	// todo: filter NPCs
	if (strncmp(szclassname, "npc_", 4) == 0)
	{
		if (strcmp(szclassname, "npc_metropolice") == 0 || strcmp(szclassname, "npc_combine_s") == 0 || strcmp(szclassname, "npc_manhack") == 0 ||
			strcmp(szclassname, "npc_zombie") == 0 || strcmp(szclassname, "npc_fastzombie") == 0 || strcmp(szclassname, "npc_poisonzombie") == 0 || strcmp(szclassname, "npc_zombine") == 0 ||
			strcmp(szclassname, "npc_antlionguard") == 0 || strcmp(szclassname, "npc_antlion") == 0 || strcmp(szclassname, "npc_headcrab") == 0 || strcmp(szclassname, "npc_headcrab_fast") == 0 ||
			strcmp(szclassname, "npc_headcrab_black") == 0 || strcmp(szclassname, "npc_hunter") == 0 || strcmp(szclassname, "npc_fastzombie_torso") == 0 || strcmp(szclassname, "npc_zombie_torso") == 0 ||
			strcmp(szclassname, "npc_barnacle") == 0)
		{
			return true;
		}
	}
	return false;
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
	}

	return bValid;
}

void CBotCoop::touchedWpt(CWaypoint* pWaypoint, int iNextWaypoint, int iPrevWaypoint)
{
	if (CWaypoints::validWaypointIndex(iPrevWaypoint) && CWaypoints::validWaypointIndex(iNextWaypoint))
	{
		CWaypoint* pPrev = CWaypoints::getWaypoint(iPrevWaypoint);
		CWaypoint* pNext = CWaypoints::getWaypoint(iNextWaypoint);
		// bot touched the first ladder waypoint, 
		if ( (pPrev->getFlags() & CWaypointTypes::W_FL_LADDER) == 0 && (pWaypoint->getFlags() & CWaypointTypes::W_FL_LADDER))
		{
			m_pButtons->tap(IN_USE);
			CClients::clientDebugMsg(this, BOT_DEBUG_NAV, "Bot touched first ladder waypoint");
		}
		// bot touched the last ladder waypoint
		if ((pNext->getFlags() & CWaypointTypes::W_FL_LADDER) == 0 && (pWaypoint->getFlags() & CWaypointTypes::W_FL_LADDER))
		{
			m_pButtons->tap(IN_USE);
			CClients::clientDebugMsg(this, BOT_DEBUG_NAV, "Bot touched last ladder waypoint");
		}
	}

	CBot::touchedWpt(pWaypoint, iNextWaypoint, iPrevWaypoint);
}

// returns true if offset has been applied when not before
bool CBotCoop::walkingTowardsWaypoint(CWaypoint* pWaypoint, bool* bOffsetApplied, Vector& vOffset)
{
	return CBot::walkingTowardsWaypoint(pWaypoint, bOffsetApplied, vOffset);
}

// Called when working out route
bool CBotCoop::canGotoWaypoint(Vector vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev)
{
	return CBot::canGotoWaypoint(vPrevWaypoint, pWaypoint, pPrev);
}