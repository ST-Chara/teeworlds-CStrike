/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/projectile.h>
//#include "spawnprotect.h"
#include "shield.h"

CShield::CShield(CGameWorld *pGameWorld, int Owner, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Owner = Owner;
	m_Pos = Pos;
	m_Ch = GameServer()->GetPlayerChar(m_Owner);
	m_Tick = g_Config.m_SvShieldTime*Server()->TickSpeed();

	if(m_Ch)
	{
		m_Ch->m_Shield = true;
		m_Ch->IncreaseArmor(g_Config.m_SvShieldTime);
	}

	GameWorld()->InsertEntity(this);
}

void CShield::Reset()
{
	
	if(m_Ch)
		m_Ch->m_Shield = false;
		
	GameServer()->m_World.DestroyEntity(this);
}

void CShield::Tick()
{	
	m_Tick--;

	if(m_Ch && m_Tick && m_Ch->m_Shield)
	{
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			GameServer()->CreateDeath(m_Ch->m_Pos, m_Owner);
			m_Ch->IncreaseArmor(-1);
		}
	}
	else
		Reset();
	
}