/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Copyright � 2013 Neox.                                                                                                */
/* If you are missing that file, acquire a complete release at https://www.teeworlds.com/forum/viewtopic.php?pid=106934  */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "game/server/gamecontroller.h"
#include "mine.h"

CMine::CMine(CGameWorld *pGameWorld, vec2 Pos, vec2 Dir, int Owner , int Type)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Dir = Dir;
	m_LifeTime = Server()->TickSpeed() * 30;
	m_NeedToGive = true;
	m_Type = Type;

	GameWorld()->InsertEntity(this);
	m_TeamMask = 1;
}

void CMine::HitCharacter()
{
    //GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_HAMMER, false);
	vec2 ProjStartPos = m_Pos;
	CCharacter *apEnts[MAX_CLIENTS];
	int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts,
				MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

	for (int i = 0; i < Num; ++i)
	{
		CCharacter *pTarget = apEnts[i];

		if(!pTarget)
			continue;

		if ((pTarget->GetPlayer()->GetCID() == m_Owner) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
			continue;

		if(pTarget->GetPlayer()->GetTeam() == GameServer()->m_apPlayers[m_Owner]->GetTeam())
			continue;

		if(m_Type == MINE_HIT)
		{
			if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
				GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
			else
				GameServer()->CreateHammerHit(ProjStartPos);

			vec2 Dir;
			if (length(pTarget->m_Pos - m_Pos) > 0.0f)
				Dir = normalize(pTarget->m_Pos - m_Pos);
			else
				Dir = vec2(0.f, -1.f);

			pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage, m_Owner, 1);
			pTarget->IncreaseHealth(-1*g_Config.m_SvHealthMineHit, m_Owner);

		}

		if(m_Type == MINE_FREEZE)
		{
			pTarget->IncreaseHealth(-1*g_Config.m_SvHealthMineFreeze, m_Owner);
			pTarget->Freeze(GameServer()->Tuning()->m_LaserDamage * Server()->TickSpeed(), m_Owner);
			Reset();
		}

		if(m_Type == MINE_KILL)
		{
			GameServer()->CreateSound(pTarget->m_Pos, SOUND_PLAYER_PAIN_LONG, -1);
			pTarget->GetPlayer()->KilledBy( m_Owner, WEAPON_HAMMER);
			GameServer()->m_apPlayers[m_Owner]->m_Money += g_Config.m_SvKillMine;
			Reset();
		}
		
	}

}

void CMine::Reset()
{
    GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_HAMMER, false);
    GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
    /*if(GameServer()->GetPlayerChar(m_Owner))
       GameServer()->m_apPlayers[m_Owner]->m_MineNum = clamp(GameServer()->m_apPlayers[m_Owner]->m_MineNum + 1, 0, 20);*/
	GameWorld()->DestroyEntity(this);
}

void CMine::Tick()
{
    if(!GameServer()->GetPlayerChar(m_Owner))
    {
        Reset();
        return;
    }

	if(m_LifeTime)
    {
        m_LifeTime --;
        HitCharacter();
        if(!m_LifeTime)
            Reset();
    }
}

void CMine::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_VelX = 1;
	pObj->m_VelY = 1;
	pObj->m_Type = WEAPON_RIFLE;
}
