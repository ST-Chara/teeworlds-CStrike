/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>

#include "character.h"
#include "laser.h"
#include "projectile.h"

#include <game/server/entities/mode/gun.h>//edited spawnprotect
#include <game/server/entities/mode/gun2.h>
#include <game/server/entities/mode/grenade.h>
#include <game/server/entities/mode/shield.h>
//#include <game/server/entities/mode/wizardhelper.h>//edited wizardhelper
//#include <game/server/entities/mode/fire.h>//edited hammerfire
//#include <game/server/entities/mode/teleport.h>//edited teleport
#include <game/server/entities/mode/mine.h>//edited mine
//#include <game/server/entities/mode/jetpack.h>//edited jetpack
//#include <game/server/entities/mode/create.h>//edited create
//#include <game/server/entities/mode/rocket.h>//edited rocket
//#include <game/server/entities/mode/srocket.h>//edited srocket
//#include <game/server/entities/mode/bomb.h>//edited bomb

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;
	m_Shield = false;

}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_ActiveWeapon = WEAPON_GUN;
	m_SubActiveWeapon = 0;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;
	m_Veled_Ticks = 0;

	m_pPlayer = pPlayer;
	m_Pos = Pos;

	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision(), &m_ChrTuning);
	m_Core.m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	if(m_ChrTuning != *(GameServer()->Tuning()))
	{
		m_ChrTuning = *(GameServer()->Tuning());
		GameServer()->SendTuningParams(m_pPlayer->GetCID());
	}

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;

	GameServer()->m_pController->OnCharacterSpawn(this);

	m_FrozenBy = -1;
	m_MoltenBy = -1;
	m_MoltenAt = -1;

	m_HammeredBy = -1;

	m_BloodTicks = 0;
	m_LagTicks = 0;
	m_Veled_Ticks = 0;

	for(int i=0; i < NUM_WEAPONS; i++)
		m_LastSubWeapons[i] = 0;

	return true;
}

void CCharacter::Destroy()
{
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;

	m_LastSubWeapons[m_ActiveWeapon] = m_SubActiveWeapon;
	m_SubActiveWeapon = m_LastSubWeapons[W];

	m_LastWeapon = m_ActiveWeapon;
	//m_SubActiveWeapon = 0;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);

	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS)
		m_ActiveWeapon = 0;
}

bool CCharacter::IsGrounded()
{
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}


void CCharacter::HandleNinja()
{
	if(m_ActiveWeapon != WEAPON_NINJA)
		return;

	if ((Server()->Tick() - m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		TakeNinja();
		return;
	}

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir*m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, m_pPlayer->GetCID(), WEAPON_NINJA);
			}
		}

		return;
	}

	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_aWeapons[WEAPON_NINJA].m_Got)
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon+1)%NUM_WEAPONS;
			if(m_aWeapons[WantedWeapon].m_Got)
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_aWeapons[WantedWeapon].m_Got)
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_aWeapons[WantedWeapon].m_Got)
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0)
		return;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_ActiveWeapon == WEAPON_RIFLE)
		FullAuto = true;


	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && m_aWeapons[m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if (!g_Config.m_SvNinja && m_ActiveWeapon == WEAPON_NINJA)
		WillFire = false;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_aWeapons[m_ActiveWeapon].m_Ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
		return;
	}

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;

	switch(m_ActiveWeapon)
	{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if ((pTarget == this) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				

				bool MeltHit = pTarget->GetPlayer()->GetTeam() == GetPlayer()->GetTeam() && pTarget->GetFreezeTicks() > 0;

				vec2 Force = (vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f);
				if (!MeltHit)
				{
					Force.x *= g_Config.m_SvHammerScaleX*0.01f;
					Force.y *= g_Config.m_SvHammerScaleY*0.01f;
				}
				else
				{
					Force.x *= g_Config.m_SvMeltHammerScaleX*0.01f;
					Force.y *= g_Config.m_SvMeltHammerScaleY*0.01f;
				}

				pTarget->TakeDamage(Force, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage, m_pPlayer->GetCID(), m_ActiveWeapon);
				Hits++;

				pTarget->m_HammeredBy = GetPlayer()->GetCID();

				if (MeltHit)
				{
					pTarget->Freeze(pTarget->GetFreezeTicks() - g_Config.m_SvHammerMelt * Server()->TickSpeed());
					if (pTarget->GetFreezeTicks() <= 0)
					{
						pTarget->m_MoltenBy = m_pPlayer->GetCID();
						pTarget->m_MoltenAt = -1; // we don't want the unfreezability to take effect when being molten by hammer
					}
				}
					
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;

			if(m_pPlayer->m_Upgrades.m_MineHit && m_pPlayer->m_Ammo_Upgrades.m_MineHit != 0 && m_SubActiveWeapon == m_pPlayer->m_Hammer[1])
			{
				new CMine(&GameServer()->m_World, m_Pos, Direction, m_pPlayer->GetCID() , MINE_HIT);//edited mine
				m_pPlayer->m_Ammo_Upgrades.m_MineHit--;
				if(!m_pPlayer->m_Ammo_Upgrades.m_MineHit) 
				{
					for(int i=0;i<8;i++)
					{
						if(i==1)
							continue;
						if(m_pPlayer->m_Hammer[1] < m_pPlayer->m_Hammer[i])
							m_pPlayer->m_Hammer[i]--;
					}
					m_pPlayer->m_Hammer[1] = -1;
					m_pPlayer->m_Hammer_ST_NUM--;
					m_pPlayer->m_Upgrades.m_MineHit = false;
					m_SubActiveWeapon = 0;
				}
			}

			if(m_pPlayer->m_Upgrades.m_MineFreeze && m_pPlayer->m_Ammo_Upgrades.m_MineFreeze != 0 && m_SubActiveWeapon == m_pPlayer->m_Hammer[2])
			{
				new CMine(&GameServer()->m_World, m_Pos, Direction, m_pPlayer->GetCID() , MINE_FREEZE);//edited mine
				m_pPlayer->m_Ammo_Upgrades.m_MineFreeze--;
				if(!m_pPlayer->m_Ammo_Upgrades.m_MineFreeze) 
				{
					for(int i=0;i<8;i++)
					{
						if(i==2)
							continue;
						if(m_pPlayer->m_Hammer[2] < m_pPlayer->m_Hammer[i])
							m_pPlayer->m_Hammer[i]--;
					}
					m_pPlayer->m_Hammer[2] = -1;
					m_pPlayer->m_Hammer_ST_NUM--;
					m_pPlayer->m_Upgrades.m_MineFreeze = false;
					m_SubActiveWeapon = 0;
				}
			}

			if(m_pPlayer->m_Upgrades.m_MineKill && m_pPlayer->m_Ammo_Upgrades.m_MineKill != 0 && m_SubActiveWeapon == m_pPlayer->m_Hammer[3])
			{
				new CMine(&GameServer()->m_World, m_Pos, Direction, m_pPlayer->GetCID() , MINE_KILL);//edited mine
				m_pPlayer->m_Ammo_Upgrades.m_MineKill--;
				if(!m_pPlayer->m_Ammo_Upgrades.m_MineKill) 
				{
					for(int i=0;i<8;i++)
					{
						if(i==3)
							continue;
						if(m_pPlayer->m_Hammer[3] < m_pPlayer->m_Hammer[i])
							m_pPlayer->m_Hammer[i]--;
					}
					m_pPlayer->m_Hammer[3] = -1;
					m_pPlayer->m_Hammer_ST_NUM--;
					m_pPlayer->m_Upgrades.m_MineKill = false;
					m_SubActiveWeapon = 0;
				}
			}

			if(m_pPlayer->m_Upgrades.m_Flash && m_pPlayer->m_Ammo_Upgrades.m_Flash != 0 && m_SubActiveWeapon == m_pPlayer->m_Hammer[4])
			{
				new CGun2(GameWorld(), POWERUP_ARMOR,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*2),
				1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);
				

				m_pPlayer->m_Ammo_Upgrades.m_Flash--;
				if(!m_pPlayer->m_Ammo_Upgrades.m_Flash) 
				{
					for(int i=0;i<8;i++)
					{
						if(i==4)
							continue;
						if(m_pPlayer->m_Hammer[4] < m_pPlayer->m_Hammer[i])
							m_pPlayer->m_Hammer[i]--;
					}
					m_pPlayer->m_Hammer[4] = -1;
					m_pPlayer->m_Hammer_ST_NUM--;
					m_pPlayer->m_Upgrades.m_Flash = false;
					m_SubActiveWeapon = 0;
				}
			}

			if(m_pPlayer->m_Upgrades.m_Smoke && m_pPlayer->m_Ammo_Upgrades.m_Smoke != 0 && m_SubActiveWeapon == m_pPlayer->m_Hammer[5])
			{
				new CGun2(GameWorld(), POWERUP_HEALTH,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*2),
				1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);
				

				m_pPlayer->m_Ammo_Upgrades.m_Smoke--;
				if(!m_pPlayer->m_Ammo_Upgrades.m_Smoke) 
				{
					for(int i=0;i<8;i++)
					{
						if(i==5)
							continue;
						if(m_pPlayer->m_Hammer[5] < m_pPlayer->m_Hammer[i])
							m_pPlayer->m_Hammer[i]--;
					}

					m_pPlayer->m_Hammer[5] = -1;
					m_pPlayer->m_Hammer_ST_NUM--;
					m_pPlayer->m_Upgrades.m_Smoke = false;
					m_SubActiveWeapon = 0;
				}
			}

			if(m_pPlayer->m_Upgrades.m_Grenade && m_SubActiveWeapon == m_pPlayer->m_Hammer[6])
			{

				new CGrenade(GameWorld(),
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
					1, true, 0, SOUND_GRENADE_EXPLODE);

				

				m_pPlayer->m_Ammo_Upgrades.m_Grenade--;
				if(!m_pPlayer->m_Ammo_Upgrades.m_Grenade) 
				{
					for(int i=0;i<8;i++)
					{
						if(i==6)
							continue;
						if(m_pPlayer->m_Hammer[6] < m_pPlayer->m_Hammer[i])
							m_pPlayer->m_Hammer[i]--;
					}
					m_pPlayer->m_Hammer[6] = -1;
					m_pPlayer->m_Hammer_ST_NUM--;
					m_pPlayer->m_Upgrades.m_Grenade = false;
					m_SubActiveWeapon = 0;
				}

			}

			if(!m_Shield && m_pPlayer->m_Upgrades.m_Shield && m_SubActiveWeapon == m_pPlayer->m_Hammer[7])
			{

				new CShield(GameWorld(), m_pPlayer->GetCID(), ProjStartPos);

				m_pPlayer->m_Ammo_Upgrades.m_Shield--;
				if(!m_pPlayer->m_Ammo_Upgrades.m_Shield) 
				{
					for(int i=0;i<8;i++)
					{
						if(i==7)
							continue;
						if(m_pPlayer->m_Hammer[7] < m_pPlayer->m_Hammer[i])
							m_pPlayer->m_Hammer[i]--;
					}
					m_pPlayer->m_Hammer[7] = -1;
					m_pPlayer->m_Hammer_ST_NUM--;
					m_pPlayer->m_Upgrades.m_Shield = false;
					m_SubActiveWeapon = 0;
				}

			}

		} break;

		case WEAPON_GUN:
		{

			if(m_pPlayer->m_Upgrades.m_Glock && m_pPlayer->m_Ammo_Upgrades.m_Glock != 0 && m_SubActiveWeapon == m_pPlayer->m_Gun[0])
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
					1, 0, 0, -1, WEAPON_GUN);

				m_pPlayer->m_Ammo_Upgrades.m_Glock--;
				GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);

				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}

			if(m_pPlayer->m_Upgrades.m_USP && m_pPlayer->m_Ammo_Upgrades.m_USP != 0 && m_SubActiveWeapon == m_pPlayer->m_Gun[2])
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
					1, 0, 0, -1, WEAPON_GUN);
			
				m_pPlayer->m_Ammo_Upgrades.m_USP--;
				GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);

				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}

			if(m_pPlayer->m_Upgrades.m_Desert && m_pPlayer->m_Ammo_Upgrades.m_Desert != 0 && m_SubActiveWeapon == m_pPlayer->m_Gun[1])
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
					1, 0, 0, -1, WEAPON_SHOTGUN);

				m_pPlayer->m_Ammo_Upgrades.m_Desert--;
				GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);

				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}
			// pack the Projectile and send it to the client Directly

			
			
		} break;

		case WEAPON_SHOTGUN:
		{
			if(m_pPlayer->m_Upgrades.m_FAMAS && m_pPlayer->m_Ammo_Upgrades.m_FAMAS != 0 && m_SubActiveWeapon == m_pPlayer->m_Shotgun[0])
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread*2+1);

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
						1, 0, 0, -1, WEAPON_SHOTGUN);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, 0,m_pPlayer->GetCID());

				m_pPlayer->m_Ammo_Upgrades.m_FAMAS--;
				GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);

			}
		} break;

		case WEAPON_GRENADE:
		{
			if(m_pPlayer->m_Upgrades.m_RPG && m_pPlayer->m_Ammo_Upgrades.m_RPG != 0 && m_SubActiveWeapon == m_pPlayer->m_Grenade[0])
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
					1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(1);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

				m_pPlayer->m_Ammo_Upgrades.m_RPG--;
				GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
			}
		} break;

		case WEAPON_RIFLE:
		{
			//new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID());
			if(m_pPlayer->m_Upgrades.m_AWP && m_pPlayer->m_Ammo_Upgrades.m_AWP != 0 && m_SubActiveWeapon == m_pPlayer->m_Rifle[0])
			{
				new CGun(GameWorld(), WEAPON_GUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
					1, false, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

				m_pPlayer->m_Ammo_Upgrades.m_AWP--;
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
			}

			if(m_pPlayer->m_Upgrades.m_AK47 && m_pPlayer->m_Ammo_Upgrades.m_AK47 != 0 && m_SubActiveWeapon == m_pPlayer->m_Rifle[1])
			{
				new CGun(GameWorld(), WEAPON_GUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
					1, false, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

				m_pPlayer->m_Ammo_Upgrades.m_AK47--;
				//GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
				GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
				GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
			}

			if(m_pPlayer->m_Upgrades.m_MP4a && m_pPlayer->m_Ammo_Upgrades.m_MP4a != 0 && m_SubActiveWeapon == m_pPlayer->m_Rifle[2])
			{
				new CGun(GameWorld(), WEAPON_GUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
					1, false, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

				m_pPlayer->m_Ammo_Upgrades.m_MP4a--;
				//GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
				GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
			}


		}
		break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			m_NumObjectsHit = 0;

			m_Ninja.m_ActivationDir = Direction;
			m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

			GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);
		} break;

		case WEAPON_KNIFE:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			/*if (!(g_Config.m_SvSilentXXL && m_FastReload))
				GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()))*/;

			/*if (m_Hit&DISABLE_HIT_HAMMER) break;*/

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				//if ((pTarget == this) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
				if((pTarget == this))
					continue;

				if(pTarget->GetPlayer()->GetTeam() == m_pPlayer->GetTeam())
					continue;

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);
					
				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
					m_pPlayer->GetCID(), m_ActiveWeapon);

				pTarget->SetEmote(EMOTE_PAIN, Server()->Tick() + Server()->TickSpeed());
				GameServer()->CreateDeath(pTarget->m_Pos, pTarget->GetPlayer()->GetCID());
				
				GameServer()->CreateSound(m_Pos, SOUND_NINJA_HIT);
				GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);

				
				if(pTarget->GetHealth() > g_Config.m_SvHealthKnife)
					pTarget->IncreaseHealth(-1*g_Config.m_SvHealthKnife, m_pPlayer->GetCID());
				else
				{
					pTarget->GetPlayer()->KilledBy(m_pPlayer->GetCID(), WEAPON_NINJA);
					m_pPlayer->m_Money += g_Config.m_SvKillKnife;

					GameServer()->m_pController->AddTeamScore(m_pPlayer->GetTeam(), g_Config.m_SvSacrTeamscore);
					m_pPlayer->m_Score += g_Config.m_SvSacrScore;
				}

				
				      
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;
			else
			{
			    m_ReloadTimer = 150.0f * Server()->TickSpeed() / 1000;
				GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);
			}

		}
		break;

	}

	m_AttackTick = Server()->Tick();

	if (!g_Config.m_SvUnlimitedAmmo && m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
		m_aWeapons[m_ActiveWeapon].m_Ammo--;

	if(!m_ReloadTimer)
	{
		m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 1000;

		if(m_ActiveWeapon == WEAPON_RIFLE && m_pPlayer->m_Upgrades.m_AK47 && m_SubActiveWeapon == m_pPlayer->m_Rifle[1] ||
			m_ActiveWeapon == WEAPON_RIFLE && m_pPlayer->m_Upgrades.m_MP4a && m_SubActiveWeapon == m_pPlayer->m_Rifle[2])
			m_ReloadTimer = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Firedelay * Server()->TickSpeed() / 1000;
	}
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();

	HandleAmmo();
	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	// ammo regen
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Ammoregentime;
	if(AmmoRegenTime)
	{
		// If equipped and not active, regen ammo?
		if (m_ReloadTimer <= 0)
		{
			if (m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if ((Server()->Tick() - m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				//m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_aWeapons[m_ActiveWeapon].m_Ammo + 1, 10);
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}

	return;
}

void CCharacter::HandleAmmo()
{
	if(m_ActiveWeapon == WEAPON_GUN && m_pPlayer->m_Upgrades.m_Glock && m_SubActiveWeapon == m_pPlayer->m_Gun[0])
	{
		m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_pPlayer->m_Ammo_Upgrades.m_Glock , 10);
	}
	if(m_ActiveWeapon == WEAPON_GUN && m_pPlayer->m_Upgrades.m_USP && m_SubActiveWeapon == m_pPlayer->m_Gun[2])
	{
		m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_pPlayer->m_Ammo_Upgrades.m_USP , 10);
	}
	if(m_ActiveWeapon == WEAPON_GUN && m_pPlayer->m_Upgrades.m_Desert && m_SubActiveWeapon == m_pPlayer->m_Gun[1])
	{
		m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_pPlayer->m_Ammo_Upgrades.m_Desert , 10);
	}

	if(m_ActiveWeapon == WEAPON_SHOTGUN && m_pPlayer->m_Upgrades.m_FAMAS && m_SubActiveWeapon == m_pPlayer->m_Shotgun[0])
	{
		m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_pPlayer->m_Ammo_Upgrades.m_FAMAS , 10);
	}

	if(m_ActiveWeapon == WEAPON_GRENADE && m_pPlayer->m_Upgrades.m_RPG && m_SubActiveWeapon == m_pPlayer->m_Grenade[0])
	{
		m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_pPlayer->m_Ammo_Upgrades.m_RPG , 10);
	}

	if(m_ActiveWeapon == WEAPON_RIFLE && m_pPlayer->m_Upgrades.m_AWP && m_SubActiveWeapon == m_pPlayer->m_Rifle[0])
	{
		m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_pPlayer->m_Ammo_Upgrades.m_AWP , 10);
	}
	if(m_ActiveWeapon == WEAPON_RIFLE && m_pPlayer->m_Upgrades.m_AK47 && m_SubActiveWeapon == m_pPlayer->m_Rifle[1])
	{
		m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_pPlayer->m_Ammo_Upgrades.m_AK47 , 10);
	}
	if(m_ActiveWeapon == WEAPON_RIFLE && m_pPlayer->m_Upgrades.m_MP4a && m_SubActiveWeapon == m_pPlayer->m_Rifle[2])
	{
		m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_pPlayer->m_Ammo_Upgrades.m_MP4a , 10);
	}

}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	if(m_aWeapons[Weapon].m_Ammo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo || !m_aWeapons[Weapon].m_Got)
	{
		m_aWeapons[Weapon].m_Got = true;
		m_aWeapons[Weapon].m_Ammo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
		return true;
	}
	return false;
}

bool CCharacter::TakeWeapon(int Weapon)
{
	int NumWeps = 0;
	for(int i = 0; i < NUM_WEAPONS; i++)
		if (m_aWeapons[i].m_Got)
			NumWeps++;

	if (Weapon < 0 || Weapon >= NUM_WEAPONS || NumWeps <= 1 || !m_aWeapons[Weapon].m_Got)
		return false;

	m_aWeapons[Weapon].m_Got = false;

	if (m_ActiveWeapon == Weapon)
	{
		int NewWeap = 0;
		if (m_LastWeapon != -1 && m_LastWeapon != Weapon && m_aWeapons[m_LastWeapon].m_Got)
			NewWeap = m_LastWeapon;
		else
			for(; NewWeap < NUM_WEAPONS && !m_aWeapons[NewWeap].m_Got; NewWeap++);

		SetWeapon(NewWeap);
	}

	if (m_LastWeapon != -1 && !m_aWeapons[m_LastWeapon].m_Got)
		m_LastWeapon = m_ActiveWeapon;

	if (m_QueuedWeapon != -1 && !m_aWeapons[m_QueuedWeapon].m_Got)
		m_QueuedWeapon = -1;
	
	return true;
}

void CCharacter::GiveNinja(bool Silent)
{
	m_Ninja.m_ActivationTick = Server()->Tick();
	m_aWeapons[WEAPON_NINJA].m_Got = true;
	m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
	if (m_ActiveWeapon != WEAPON_NINJA)
		m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;
	m_SubActiveWeapon = 0;

	if (!Silent)
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);
}

void CCharacter::TakeNinja()
{
	if (m_ActiveWeapon != WEAPON_NINJA)
		return;

	m_aWeapons[WEAPON_NINJA].m_Got = false;
	m_ActiveWeapon = m_LastWeapon;
	m_SubActiveWeapon = 0;
	if(m_ActiveWeapon == WEAPON_NINJA)
		m_ActiveWeapon = WEAPON_HAMMER;
	//SetWeapon(m_ActiveWeapon); //has no effect
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

int CCharacter::GetFreezeTicks()
{
	return m_Core.m_Frozen;
}

void CCharacter::Freeze(int Ticks, int By)
{
	if (Ticks < 0)
		Ticks = 0;
	if (By != -1 && Ticks > 0)
		m_FrozenBy = By;
	m_Core.m_Frozen = Ticks;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::Tick()
{
	if(m_pPlayer->m_ForceBalanced)
	{
		char Buf[128];
		str_format(Buf, sizeof(Buf), "You were moved to %s due to team balancing", GameServer()->m_pController->GetTeamName(m_pPlayer->GetTeam()));
		GameServer()->SendBroadcast(Buf, m_pPlayer->GetCID());

		m_pPlayer->m_ForceBalanced = false;
	}

	// increase health
	if(Server()->Tick()%(Server()->TickSpeed())==0)
		IncreaseHealth(1, -1);

	// prepartimer start round
	if(GameServer()->m_pController->m_PreparTimer && GameServer()->m_pController->m_IsEnd)
	{
		m_Input.m_Direction = 0;
		//m_Input.m_Jump = 0;
		m_Input.m_Hook = 0;
		//return;
	}

	m_Core.m_Input = m_Input;
	m_Core.Tick(true);

	int Col = GameServer()->Collision()->GetCollisionAt(m_Pos.x, m_Pos.y);
	if((Col <= 7 && Col&CCollision::COLFLAG_DEATH) || GameLayerClipped(m_Pos)) //seriously.
	{
		// handle death-tiles and leaving gamelayer
		m_Core.m_Frozen = 0; //we just unfreeze so it never counts as a sacrifice
		Die(m_pPlayer->GetCID(), WEAPON_WORLD,true);
	}

	if (m_Core.m_Frozen)
	{
		if (m_ActiveWeapon != WEAPON_NINJA)
			GiveNinja(true);
		else if (m_Ninja.m_ActivationTick + 5 * Server()->TickSpeed() < Server()->Tick())
			m_Ninja.m_ActivationTick = Server()->Tick(); // this should fix the end-of-ninja missprediction bug

		//openfng handles this in mod gamectl
		//if ((m_Core.m_Frozen+1) % Server()->TickSpeed() == 0)
		//	GameServer()->CreateDamageInd(m_Pos, 0, (m_Core.m_Frozen+1) / Server()->TickSpeed());

		m_MoltenBy = -1;
		m_MoltenAt = -1;
	}
	else
	{
		if (m_ActiveWeapon == WEAPON_NINJA)
		{
			TakeNinja();
			m_MoltenAt = Server()->Tick();
		}
		m_FrozenBy = -1;
	}

	if (m_BloodTicks > 0)
	{
		if (m_BloodTicks % g_Config.m_SvBloodInterval == 0)
			GameServer()->CreateDeath(m_Core.m_Pos, m_pPlayer->GetCID());
		--m_BloodTicks;
	}

	// handle lag ticks
	if(m_LagTicks)
	{
		if(!m_Core.m_Frozen)
		{
			//m_Core.m_Vel = vec2(.5,.5);
			m_ChrTuning.Set("ground_control_accel", -2.0f);
			m_ChrTuning.Set("air_control_accel", -1.5f);
			GameServer()->SendTuningParams(m_pPlayer->GetCID());
			m_LagTicks--;
		}
		else
		{
			m_ChrTuning.Set("ground_control_accel", 2.0f);
			m_ChrTuning.Set("air_control_accel", 1.5f);
			GameServer()->SendTuningParams(m_pPlayer->GetCID());
			m_LagTicks = 0;
		}

		if(m_LagTicks==0)
		{
			m_ChrTuning.Set("ground_control_accel", 2.0f);
			m_ChrTuning.Set("air_control_accel", 1.5f);
			GameServer()->SendTuningParams(m_pPlayer->GetCID());
		}
	}

	if(m_Veled_Ticks)
	{
		if(!m_Core.m_Frozen)
		{
			//m_Core.m_Vel = vec2(.5,.5);
			m_ChrTuning.Set("air_control_speed", 3.0f);
			m_ChrTuning.Set("ground_control_speed", 3.0f);
			m_ChrTuning.Set("hook_drag_speed", 3.0f);
			GameServer()->SendTuningParams(m_pPlayer->GetCID());
			m_Veled_Ticks--;
		}
		else
		{
			m_ChrTuning.Set("air_control_speed", 250.0f / 50.0f);
			m_ChrTuning.Set("ground_control_speed", 10.0f);
			m_ChrTuning.Set("hook_drag_speed", 15.0f);
			GameServer()->SendTuningParams(m_pPlayer->GetCID());
			m_Veled_Ticks = 0;
		}

		if(m_Veled_Ticks==0)
		{
			m_ChrTuning.Set("air_control_speed", 250.0f / 50.0f);
			m_ChrTuning.Set("ground_control_speed", 10.0f);
			m_ChrTuning.Set("hook_drag_speed", 15.0f);
			GameServer()->SendTuningParams(m_pPlayer->GetCID());
		}
	}

	// handle Weapons
	HandleWeapons();

	// Previnput
	m_PrevInput = m_Input;

	return;
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision());
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	m_Core.Move();
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Mask);

	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Mask);


	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	IServer::CClientInfo CltInfo;
	Server()->GetClientInfo(m_pPlayer->GetCID(), &CltInfo);

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0
				|| (m_Core.m_Frozen > 0 && !(Server()->Tick()&1)))
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

void CCharacter::TickPaused()
{
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart > -1)
		++m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount, int By)
{
	if(!GetFreezeTicks() && !m_Shield || GetFreezeTicks() && Amount > 0)
		m_Health = clamp(m_Health+Amount, 0, 10);

	if(!m_Health && By != -1 && !GetFreezeTicks())
	{
		Freeze(GameServer()->Tuning()->m_LaserDamage * Server()->TickSpeed(), By);
	}

	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	/*if(m_Armor >= 10)
		return false;*/
	m_Armor = clamp(m_Armor+Amount, 0, 10);
	return true;
}

void CCharacter::Die(int Killer, int Weapon, bool NoKillMsg)
{
	// we got to wait 0.5 secs before respawning
	m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;

	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
		Killer, Server()->ClientName(Killer),
		m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// send the kill message, except for when are sacrificed (openfng)
	// because mod gamectrl will create it in that case
	if (!NoKillMsg)
	{
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = Killer;
		Msg.m_Victim = m_pPlayer->GetCID();
		Msg.m_Weapon = Weapon;
		Msg.m_ModeSpecial = ModeSpecial;

		//if (GetFreezeTicks() <= 0 || WasFrozenBy() < 0 || 
		//                      !(GameServer()->IsClientReady(WasFrozenBy()) && GameServer()->IsClientPlayer(WasFrozenBy())))
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick()-3*Server()->TickSpeed();//lol

	m_Alive = false;
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	m_Core.m_Vel += Force;

	if(!g_Config.m_SvDamage || (GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From) && !g_Config.m_SvTeamdamage))
		return false;

	// m_pPlayer only inflicts half damage on self
	if(From == m_pPlayer->GetCID())
		Dmg = max(1, Dmg/2);

	m_DamageTaken++;

	// create healthmod indicator
	if(Server()->Tick() < m_DamageTakenTick+25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
	}
	else
	{
		m_DamageTaken = 0;
		GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
	}

	if(Dmg)
	{
		if(m_Armor)
		{
			if(Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if(Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
	}

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
	}

	// check for death
	if(m_Health <= 0)
	{
		Die(From, Weapon);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChr->m_EmoteType = EMOTE_HAPPY;
				pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
			}
		}

		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Bleed(int Ticks)
{
	m_BloodTicks = Ticks;
}

void CCharacter::Snap(int SnappingClient)
{
	CNetObj_Character Measure; // used only for measuring the offset between vanilla and extended core
	if(NetworkClipped(SnappingClient))
		return;

	IServer::CClientInfo CltInfo;
	Server()->GetClientInfo(SnappingClient, &CltInfo);

	// measure distance between start and and first vanilla field
	size_t Offset = (char*)(&Measure.m_Tick) - (char*)(&Measure);

	// vanilla size for vanilla clients, extended for custom client
	size_t Sz = sizeof (CNetObj_Character) - (CltInfo.m_CustClt?0:Offset);

	// create a snap item of the size the client expects
	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), Sz));
	if(!pCharacter)
		return;

	// for vanilla clients, make pCharacter point before the start our snap item start, so that the vanilla core
	// aligns with the snap item. we may not access the extended fields then, since they are out of bounds (to the left)
	if (!CltInfo.m_CustClt)
		pCharacter = (CNetObj_Character*)(((char*)pCharacter)-Offset); // moar cookies.

	// write down the m_Core
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter, !CltInfo.m_CustClt);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter, !CltInfo.m_CustClt);
	}

	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = m_ActiveWeapon;
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;
		if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
			pCharacter->m_AmmoCount = m_aWeapons[m_ActiveWeapon].m_Ammo;
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;

}
