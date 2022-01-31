/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include "player.h"


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();
	m_ChatScore = 0;
	m_Money = 0;
	m_NoBroadcastTimer = 0;
	m_TeamChangeTick = Server()->Tick();

	Clear();
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
}

int CPlayer::TotalHP() const
{
    //return 10 + m_Upgrades.m_Health + GameServer()->m_pController->GetPlayerExtraHP();
	return 10;
}

bool CPlayer::GotAll() const
{
    /*

    if(!m_Upgrades.m_Mine)
        return false;

    if(!m_Upgrades.m_InfAmmo)
        return false;
	*/
    return true;
}

void CPlayer::Tick()
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	if (m_ChatScore > 0)
		m_ChatScore--;

	Server()->SetClientScore(m_ClientID, m_Score);

	// do latency stuff
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(!GameServer()->m_World.m_Paused)
	{
		if(!m_pCharacter && m_Team == TEAM_SPECTATORS && m_SpectatorID == SPEC_FREEVIEW)
			m_ViewPos -= vec2(clamp(m_ViewPos.x-m_LatestActivity.m_TargetX, -500.0f, 500.0f), clamp(m_ViewPos.y-m_LatestActivity.m_TargetY, -400.0f, 400.0f));

		if(!m_pCharacter && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
			m_Spawning = true;

		if(m_pCharacter)
		{
			if(m_pCharacter->IsAlive())
			{
				m_ViewPos = m_pCharacter->m_Pos;
			}
			else
			{
				delete m_pCharacter;
				m_pCharacter = 0;
			}
		}
		else if(m_Spawning && m_RespawnTick <= Server()->Tick())
			TryRespawn();
	}
	else
	{
		++m_RespawnTick;
		++m_DieTick;
		++m_ScoreStartTick;
		++m_LastActionTick;
		++m_TeamChangeTick;
 	}

	 if(m_NoBroadcastTimer)
        m_NoBroadcastTimer --;

	if(!GameServer()->m_pController->m_PreparTimer && !m_NoBroadcastTimer && Server()->ClientIngame(m_ClientID)&&
		GetCharacter() && m_Team != TEAM_SPECTATORS)
    {
		if(!m_pCharacter->GetFreezeTicks()&&(Server()->Tick()%20 == 0))
		{
			//&&(Server()->Tick()%25 == 0)

			char aBuf[128];
			char aType[64];
			char aSubType[64];

			if(GetCharacter())
			{
				str_format(aType, sizeof(aType), "\nType : %s", NameWeapons(GetCharacter()->GetActiveWeapon()));
				if(m_TmpAmmo!=-1)
					str_format(aSubType, sizeof(aSubType), "\nSubType : %s %d", NameSubType(GetCharacter()->GetSubActiveWeapon()), m_TmpAmmo);
				else
					str_format(aSubType, sizeof(aSubType), "\nSubType : %s", NameSubType(GetCharacter()->GetSubActiveWeapon()));
			}

			str_format(aBuf, sizeof(aBuf), "Money : %d $%s%s\n                                  ¤", m_Money, aType, aSubType);

			GameServer()->SendBroadcast(aBuf, m_ClientID);
		}
    }

	if( GetCharacter() && !GameServer()->m_pController->m_PreparTimer && !m_NoBroadcastTimer && Server()->ClientIngame(m_ClientID))
	{
		if(GetCharacter()->GetFreezeTicks())
			GameServer()->SendBroadcast("", m_ClientID);
	}

	/*if(!GameServer()->m_pController->m_PreparTimer && !m_NoBroadcastTimer && m_Team == TEAM_SPECTATORS && Server()->ClientIngame(m_ClientID))
		GameServer()->SendBroadcast("", m_ClientID);*/

}

char* CPlayer::NameWeapons(int Type) const
{
	switch(Type)
	{
		case 0:
			return "WEAPON_HAMMER";
			break;
		case 1:
			return "WEAPON_GUN";
			break;
		case 2:
			return "WEAPON_SHOTGUN";
			break;
		case 3:
			return "WEAPON_GRENADE";
			break;
		case 4:
			return "WEAPON_RIFLE";
			break;
		case 5:
			return "WEAPON_NINJA";
			break;
		case 6:
			return "WEAPON_NINJA";
			break;
	}
	return "";
}

char* CPlayer::NameSubType(int SubType)
{
	int Type = GetCharacter()->GetActiveWeapon();
	switch(Type)
	{
	case 0:
		{
			if(SubType == m_Hammer[0])
			{
				m_TmpAmmo =  -1;
				return "Hammer";
			}
			if(SubType == m_Hammer[1])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_MineHit;
				return "Minehit";
			}
			if(SubType == m_Hammer[2])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_MineFreeze;
				return "MineFreeze";
			}
			if(SubType == m_Hammer[3])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_MineKill;
				return "MineKill";
			}
			if(SubType == m_Hammer[4])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_Flash;
				return "Flash";
			}
			if(SubType == m_Hammer[5])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_Smoke;
				return "Smoke";
			}
			if(SubType == m_Hammer[6])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_Grenade;
				return "Grenade";
			}
			if(SubType == m_Hammer[7])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_Shield;
				return "Shield";
			}
		}
		break;
	case 1:
		{
			if(SubType == m_Gun[0])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_Glock;
				return "Glock";
			}
			if(SubType == m_Gun[1])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_Desert;
				return "Desert";
			}
			if(SubType == m_Gun[2])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_USP;
				return "USP";
			}
		}
		break;
	case 2:
		{
			if(SubType == m_Shotgun[0])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_FAMAS;
				return "FAMAS";
			}
		}
		break;
	case 3:
		{
			if(SubType == m_Grenade[0])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_RPG;
				return "RPG";
			}
		}
		break;
	case 4:
		{
			if(SubType == m_Rifle[0])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_AWP;
				return "AWP";
			}
			if(SubType == m_Rifle[1])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_AK47;
				return "AK47";
			}
			if(SubType == m_Rifle[2])
			{
				m_TmpAmmo =  m_Ammo_Upgrades.m_MP4a;
				return "MP4a";
			}
		}
		break;
	case 5:
		{
			m_TmpAmmo = -1;

			if(SubType == m_Ninja[0])
				return "Ninja";
		}
		break;
	case 6:
		{
			m_TmpAmmo = -1;
			if(SubType == m_Knife[0])
				return "Knife";
		}
		break;
	}

	m_TmpAmmo = -1;

	return "";
}

void CPlayer::Clear()
{
    /*if(m_IsDeadByMonster)
        SetTeam(TEAM_RED, false);

    m_IsDeadByMonster = false;
    m_IsRainbow = false;
    m_DeathHealth = 10;

    m_Upgrades.m_Health = 0;*/

	m_IsDead = TEAM_SPECTATORS;//important
	//m_Money = 0;

	m_Hammer_ST_NUM = 1;
	m_Gun_ST_NUM = 0;
	m_Shotgun_ST_NUM = 0;
	m_Grenade_ST_NUM = 0;
	m_Rifle_ST_NUM = 0;
	m_Ninja_ST_NUM = 0;
	m_Knife_ST_NUM = 1;

	for(int i=0;i<8;i++)
	{
		m_Hammer[i]=-1;
		m_Gun[i]=-1;
		m_Shotgun[i]=-1;
		m_Grenade[i]=-1;
		m_Rifle[i]=-1;
		m_Ninja[i]=-1;
		m_Knife[i]=-1;
	}

	m_Hammer[0]= 0;
	m_Knife[0]= 0;

	m_Upgrades.m_MineHit = false;
	m_Upgrades.m_MineFreeze = false;
	m_Upgrades.m_MineKill = false;
	m_Upgrades.m_Flash = false;
	m_Upgrades.m_Smoke = false;
	m_Upgrades.m_Grenade = false;
	m_Upgrades.m_Shield = false;
	m_Upgrades.m_Glock = false;
	m_Upgrades.m_Desert = false;
	m_Upgrades.m_USP = false;
	m_Upgrades.m_FAMAS = false; 
	m_Upgrades.m_RPG = false; 
	m_Upgrades.m_AWP = false;
	m_Upgrades.m_AK47 = false;
	m_Upgrades.m_MP4a = false;
	m_Upgrades.m_Health = 0;


	m_Ammo_Upgrades.m_MineHit = 0;
	m_Ammo_Upgrades.m_MineFreeze = 0;
	m_Ammo_Upgrades.m_MineKill = 0;
	m_Ammo_Upgrades.m_Flash = 0;
	m_Ammo_Upgrades.m_Smoke = 0;
	m_Ammo_Upgrades.m_Grenade = 0;
	m_Ammo_Upgrades.m_Shield = 0;
	m_Ammo_Upgrades.m_Glock = 0;
	m_Ammo_Upgrades.m_Desert = 0;
	m_Ammo_Upgrades.m_USP = 0;
	m_Ammo_Upgrades.m_FAMAS = 0; 
	m_Ammo_Upgrades.m_RPG = 0; 
	m_Ammo_Upgrades.m_AWP = 0;
	m_Ammo_Upgrades.m_AK47 = 0;
	m_Ammo_Upgrades.m_MP4a = 0;

	m_TmpAmmo = -1;
}

void CPlayer::PostTick()
{
	if(!Server()->ClientIngame(m_ClientID))
        return;

	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS && m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[m_SpectatorID])
		m_ViewPos = GameServer()->m_apPlayers[m_SpectatorID]->m_ViewPos;
}

void CPlayer::Snap(int SnappingClient)
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, m_ClientID, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(m_ClientID));

	char aBuf[32];
	str_format(aBuf,sizeof(aBuf),"%d $",m_Money);
	if(Server()->Tick() % 100 < 50)
		StrToInts(&pClientInfo->m_Clan0, 3, aBuf);
	else
		StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));

	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, m_ClientID, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = m_ClientID;
	pPlayerInfo->m_Score = m_Score;
	pPlayerInfo->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;

	if(m_ClientID == SnappingClient && m_Team == TEAM_SPECTATORS)
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(m_ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING)
			return;

		// reset input
		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
 		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		m_pCharacter->Die(m_ClientID, Weapon,true);
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::Respawn()
{
	if(m_Team != TEAM_SPECTATORS)
		m_Spawning = true;
}

void CPlayer::SetTeam(int Team, bool DoChatMsg)
{
	// clamp the team
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team)
		return;

	char aBuf[512];
	if(DoChatMsg)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(m_ClientID), GameServer()->m_pController->GetTeamName(Team));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		if(Team == TEAM_SPECTATORS)
			GameServer()->SendBroadcast("", m_ClientID);;
	}

	KillCharacter();

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

void CPlayer::KilledBy(int Killer, int Weapon)
{
	m_IsDead = m_Team;
	SetTeam(TEAM_SPECTATORS,false);

	CNetMsg_Sv_KillMsg Msg;
	Msg.m_Killer = Killer;
	Msg.m_Victim = m_ClientID;
	Msg.m_Weapon = Weapon;
	Msg.m_ModeSpecial = 0;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	if(!GameServer()->m_pController->CanSpawn(m_Team, &SpawnPos))
		return;

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(SpawnPos);
}
