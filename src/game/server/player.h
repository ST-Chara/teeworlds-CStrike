/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.h"
#include "gamecontext.h"

// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

public:
	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Init(int CID);

	void TryRespawn();
	void Respawn();
	void SetTeam(int Team, bool DoChatMsg=true);
	int GetTeam() const { return m_Team; };
	int GetCID() const { return m_ClientID; };
	void KilledBy(int Killer, int Weapon);

	void Tick();
	void PostTick();
	void Snap(int SnappingClient);

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void KillCharacter(int Weapon = WEAPON_GAME);
	CCharacter *GetCharacter();

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;
	bool m_IsReady;

	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int m_RespawnTick;
	int m_DieTick;
	int m_Score;
	int m_ScoreStartTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	int m_TeamChangeTick;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	int m_ChatScore;
	
	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;


	struct
	{
		// hammer sub weapons
        bool m_MineHit;
		bool m_MineFreeze;
		bool m_MineKill;
		bool m_Flash;
		bool m_Smoke;
        bool m_Grenade;
		bool m_Shield;

		// gun sub weapons
		bool m_Glock;
		bool m_Desert;
		bool m_USP;

		// shotgun sub weapons
		bool m_FAMAS; 

		// grenade sub weapons
		bool m_RPG; 

		// rifle sub weapons
		bool m_AWP;
		bool m_AK47;
		bool m_MP4a;

        int m_Health;
	} m_Upgrades;

	struct
	{
		// hammer sub weapons
        int m_MineHit;
		int m_MineFreeze;
		int m_MineKill;
		int m_Flash;
		int m_Smoke;
        int m_Grenade;
		int m_Shield;

		// gun sub weapons
		int m_Glock;
		int m_Desert;
		int m_USP;

		// shotgun sub weapons
		int m_FAMAS; 

		// grenade sub weapons
		int m_RPG; 

		// rifle sub weapons
		int m_AWP;
		int m_AK47;
		int m_MP4a;

	} m_Ammo_Upgrades;

	int m_Hammer[8];
	int m_Gun[8];
	int m_Shotgun[8];
	int m_Grenade[8];
	int m_Rifle[8];
	int m_Ninja[8];
	int m_Knife[8];

	int m_Hammer_ST_NUM;
	int m_Gun_ST_NUM;
	int m_Shotgun_ST_NUM;
	int m_Grenade_ST_NUM;
	int m_Rifle_ST_NUM;
	int m_Ninja_ST_NUM;
	int m_Knife_ST_NUM;

	//int m_MineNum;

	int m_IsDead;//edited
	int m_Money;
	int m_NoBroadcastTimer;

	void Clear();
    bool GotAll() const;
    int TotalHP() const;
	char* NameWeapons(int) const;
	char* NameSubType(int);

private:
	CCharacter *m_pCharacter;
	CGameContext *m_pGameServer;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	//
	bool m_Spawning;
	int m_ClientID;
	int m_Team;
	int m_TmpAmmo;
};

#endif
