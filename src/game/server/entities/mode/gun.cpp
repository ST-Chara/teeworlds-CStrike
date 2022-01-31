/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "gun.h"

CGun::CGun(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;

	GameWorld()->InsertEntity(this);
}

void CGun::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CGun::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;

		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CGun::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	m_LifeSpan--;

	if(TargetChr && OwnerChar && TargetChr->GetPlayer()->GetTeam() != OwnerChar->GetPlayer()->GetTeam())
	{
		TargetChr->IncreaseHealth(-1*g_Config.m_SvHealthAK47, m_Owner);
	}

	if(TargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
			GameServer()->CreateSound(CurPos, m_SoundImpact);

		if(m_Explosive && !GameServer()->m_pController->m_PreparTimer && !GameServer()->m_pController->m_IsEnd)
			GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false);

		if(TargetChr && OwnerChar && TargetChr->GetPlayer()->GetTeam() != OwnerChar->GetPlayer()->GetTeam())
		{
			GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);
			TargetChr->SetEmote(EMOTE_PAIN, Server()->Tick() + Server()->TickSpeed());
			TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, WEAPON_GRENADE);

		}

		GameServer()->m_World.DestroyEntity(this);
	}
}

void CGun::TickPaused()
{
	++m_StartTick;
}

//void CGun::FillInfo(CNetObj_Laser *pObj)
//{
//	pObj->m_X = (int)m_Pos.x;
//	pObj->m_Y = (int)m_Pos.y;
//	pObj->m_FromX = (int)m_Pos.x;
//	pObj->m_FromY = (int)m_Pos.y;
//	pObj->m_StartTick = m_StartTick;
//}

void CGun::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Laser *pProj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(pProj)
		//FillInfo(pProj);
	{
		pProj->m_X = (int)GetPos((Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed()).x;
		pProj->m_Y = (int)GetPos((Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed()).y;
		pProj->m_FromX = (int)m_Pos.x;
		pProj->m_FromY = (int)m_Pos.y;
		pProj->m_StartTick = m_StartTick;
	}
}
