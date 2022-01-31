/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "gun2.h"
#include <game/server/entities/laser.h>
#include <engine/shared/config.h>


CGun2::CGun2(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Type = Type;
	m_Pos = Pos;
	m_TmpPos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;

	m_Bounces = 0;

	GameWorld()->InsertEntity(this);

	
}

void CGun2::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CGun2::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case POWERUP_HEALTH:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case POWERUP_ARMOR:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		/*case POWERUP_ARMOR:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;*/
	}


	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CGun2::Tick()
{
	
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	m_TmpPos = CurPos;
	// City
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, &m_BouncePos);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	/*if(TargetChr)
	{
		if(TargetChr->GetPlayer()->m_Insta)
			return;
	}*/

	m_LifeSpan--;

	bool Destroy = false;
	/*const int MAX_PLASMA = 4*16;

	CPlasma *apEnts[MAX_PLASMA];
	int Num = GameServer()->m_World.FindEntities(CurPos, 1024, (CEntity**)apEnts,
					MAX_PLASMA, CGameWorld::ENTTYPE_PLASMA);

	for(int i = 0; i < Num; i++)
	{
		CPlasma *pTarget = apEnts[i];

		if(pTarget)
		{
			if(distance(pTarget->m_Pos, CurPos) < 16)
			{
				pTarget->Reset();
				Destroy = true;
			}
		}
	}

	
*/
	//// City
	//if(OwnerChar && TargetChr)
	//{
	//	if(OwnerChar->GetPlayer()->m_AccData.m_GunFreeze && m_Type == WEAPON_GUN && !TargetChr->m_God && !TargetChr->Protected())
	//		TargetChr->Freeze(OwnerChar->GetPlayer()->m_AccData.m_GunFreeze * Server()->TickSpeed());
	//}

	if(TargetChr && OwnerChar && m_Type == POWERUP_HEALTH && OwnerChar->GetPlayer()->GetTeam() != TargetChr->GetPlayer()->GetTeam())
	{
		GameServer()->CreateDeath(TargetChr->m_Pos, TargetChr->GetPlayer()->GetCID());
		TargetChr->SetVel(Server()->TickSpeed()*g_Config.m_SvSmokeLage);
	}

	if(TargetChr && OwnerChar && Server()->Tick()%5==0  && OwnerChar->GetPlayer()->GetTeam() != TargetChr->GetPlayer()->GetTeam())
	{
		TargetChr->IncreaseHealth(-1*g_Config.m_SvHealthSmoke, m_Owner);
	}
	
	int Bounces;
	//if(OwnerChar)
	{
		Bounces = 4;

		if(m_Type == POWERUP_ARMOR)
			Bounces = 2;

		if(Collide&& m_Bounces <= Bounces)
		{
			vec2 TempPos = m_BouncePos;
			vec2 TempDir = m_Direction*4.f ;

			GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
			m_Pos = TempPos;
			m_Direction = normalize(TempDir);
			m_StartTick = Server()->Tick();
			m_Bounces++;
		}
	}
	

	if(TargetChr || Collide  || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if(m_LifeSpan >= 0)
			GameServer()->CreateSound(CurPos, m_SoundImpact);

		/*if(m_Explosive)
		{
			if(m_Type == POWERUP_HEALTH)
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false);
			else
			{
				GameServer()->CreatePlayerSpawn(CurPos);
				}
		}*/

		
		if(m_LifeSpan < 0 || m_Bounces > Bounces || !OwnerChar)
		{
			if(m_Type == POWERUP_HEALTH)
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false);

			if(m_Type == POWERUP_ARMOR)
			{
				for(int i=0; i<=10;i++)
					new CLaser(GameWorld(), PrevPos, GetDir(i*36/(2*pi)), 300, m_Owner);
					

				GameServer()->CreatePlayerSpawn(CurPos);
			}
			
			
			GameServer()->m_World.DestroyEntity(this);
		}
		
	}

	

}

void CGun2::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CGun2::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	

	CCharacter *pOwner = GameServer()->GetPlayerChar(m_Owner);
	
	CNetObj_Pickup * pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;
	pP->m_X = (int)m_TmpPos.x;
	pP->m_Y = (int)m_TmpPos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = 0;
	
	/*CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);*/
	
		
}