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
#ifndef __BOT_COOP_H__
#define __BOT_COOP_H__

#include "bot_utility.h"

typedef enum
{
	SYN_VC_HELP = 0,
	SYN_VC_INCOMING = 1,
	SYN_VC_ENEMY = 2,
	SYN_VC_FOLLOW = 3,
	SYN_VC_LETSGO = 4,
	SYN_VC_LEAD = 5,
	SYN_VC_INVALID = 6,
}eSYNVoiceCMD;

class CBotCoop : public CBot
{
public:
	virtual bool isSYN() { return true; }
	void spawnInit();
	void died(edict_t* pKiller, const char* pszWeapon);
	void killed(edict_t* pVictim, char* weapon);
	virtual void modThink ();

	void getTasks(unsigned int iIgnore);

	bool executeAction(eBotAction iAction);

	virtual bool canAvoid(edict_t* pEntity);
	virtual bool isEnemy ( edict_t *pEdict, bool bCheckWeapons = true  );

	void voiceCommand(int cmd) override;
	virtual void hearVoiceCommand(edict_t* pPlayer, byte cmd);

	void init();

	void setup();

	virtual bool startGame ();
	virtual bool checkStuck();
	bool setVisible(edict_t* pEntity, bool bVisible);

	void touchedWpt(CWaypoint* pWaypoint, int iNextWaypoint, int iPrevWaypoint);

	bool walkingTowardsWaypoint(CWaypoint* pWaypoint, bool* bOffsetApplied, Vector& vOffset);

	bool canGotoWaypoint(Vector vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev);

	virtual bool handleAttack(CBotWeapon* pWeapon, edict_t* pEnemy);

	virtual void handleWeapons();

	virtual unsigned int maxEntityIndex() { return gpGlobals->maxEntities; }

	float getArmorPercent() { return (0.01f * m_pPlayerInfo->GetArmorValue()); }
	void HealPlayer();
	bool ShouldScavengeItems(float fNextDelay = -1.0f);
private:
	MyEHandle m_NearestBreakable;
	MyEHandle m_pHealthCharger;
	MyEHandle m_pHealthKit;
	MyEHandle m_pAmmoKit; // nearest healthkit
	MyEHandle m_pBattery; // nearest battery
	MyEHandle m_pCharger; // nearest charger
	MyEHandle m_pNearbyWeapon;
	MyEHandle m_pNearestButton;
	MyEHandle m_pAmmoCrate;
	MyEHandle m_pNPCAlyx; // allow bots to track alyx
	MyEHandle m_pNPCBarney; // allow bots to track barney
	MyEHandle m_pNearbyTeamMate;
	edict_t* m_pCurrentWeapon;

	CBaseHandle* m_Weapons;

	float m_fUseButtonTime;
	float m_fUseCrateTime;
	float m_fScavengeTime; // scavenge for items (ammo, weapons, armor);

	int m_iHealthPack;
};

#endif
