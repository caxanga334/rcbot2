#ifndef _BOT_SYNERGY_H_
#define _BOT_SYNERGY_H_

class CBotSynergy : public CBot
{
    bool isSYN () override { return true; }
    void modThink () override { return; }
    unsigned int maxEntityIndex() override { return gpGlobals->maxEntities; }
    bool isEnemy ( edict_t *pEdict, bool bCheckWeapons = true ) override;
};

#endif