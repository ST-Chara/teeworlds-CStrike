/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "grenade.h"


CGrenade::CGrenade(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_TmpPos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;

	GameWorld()->InsertEntity(this);
}

void CGrenade::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CGrenade::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;
	
	Curvature = 2.0f;
	Speed = 1000.0f;

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CGrenade::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);

	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	m_LifeSpan--;

	if(TargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		
		GameServer()->CreateSound(CurPos, m_SoundImpact);

		if(m_Explosive)
			GameServer()->CreateExplosion(CurPos, m_Owner, WEAPON_GRENADE, false);

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			
			CCharacter *pVictim = GameServer()->GetPlayerChar(i);

			if(i == m_Owner || !pVictim)
				continue;

			if(pVictim->GetPlayer()->GetTeam() != OwnerChar->GetPlayer()->GetTeam())
			{
				if(distance(pVictim->m_Pos, CurPos) < 100)
				{
					pVictim->IncreaseHealth(-1*g_Config.m_SvHealthGrenade, m_Owner);
					pVictim->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, WEAPON_GRENADE);
				}
			}
		}
			
		GameServer()->CreateDamageInd2(CurPos, 0, 10);

		GameServer()->m_World.DestroyEntity(this);
		
	}
}

void CGrenade::TickPaused()
{
	++m_StartTick;
}

void CGrenade::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = WEAPON_GRENADE;
}

void CGrenade::Snap(int SnappingClient)
{

	if(NetworkClipped(SnappingClient))
		return;

	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
	

}
