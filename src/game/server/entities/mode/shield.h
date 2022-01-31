/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_SHIELD_H
#define GAME_SERVER_ENTITIES_SHIELD_H

#include <engine/shared/config.h>
#include <game/server/entities/flag.h>

class CShield : public CEntity
{
public:
	CShield(CGameWorld *pGameWorld, int Owner, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	vec2 m_Pos;

private:
	int m_Tick;
	int m_Owner;
	int m_Team;

	CCharacter * m_Ch;
};

#endif
