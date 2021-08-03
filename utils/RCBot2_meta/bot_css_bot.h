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
#ifndef __CSS_RCBOT_H__
#define __CSS_RCBOT_H__

// bot for CS Source
class CCSSBot : public CBot
{
public:
	bool isCSS() override { return true; }
    void init(bool bVarInit=false) override;
    void spawnInit() override;
	void died(edict_t *pKiller, const char *pszWeapon) override;
	void setup() override;
	void selectTeam();
	void selectModel();
	bool startGame() override;
	bool isAlive() override;
	bool isEnemy(edict_t *pEdict,bool bCheckWeapons = true) override;
    void handleWeapons() override;
    bool handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy) override;
	void modThink() override;
	unsigned int maxEntityIndex() override { return gpGlobals->maxEntities; }
	void getTasks (unsigned int iIgnore=0) override;
	virtual bool executeAction(eBotAction iAction);
	virtual void buy(const char *item);
	virtual void executeBuy();
	virtual void say(const char *message);
	virtual void sayteam(const char *message);
	virtual void primaryattackCS(bool hold = false);
private:
	edict_t *m_pCurrentWeapon; // The bot current weapon
	bool m_bDidBuy; // Did the bot buy on this round?
	float m_fNextAttackTime; // Control timer for bot primary attack
};

#endif