/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>

#include <game/gamecore.h>

enum
{
	WEAPON_GAME = -3, // team switching etc
	WEAPON_SELF = -2, // console kill command
	WEAPON_WORLD = -1, // death tiles etc
};

enum
{
	MINE_HIT = 0,
	MINE_FREEZE,
	MINE_KILL
};

class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()

public:
	//character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld);

	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	bool IsGrounded();
	bool m_Shield;

	void SetWeapon(int W);
	int GetActiveWeapon() const{ return m_ActiveWeapon;}
	int GetSubActiveWeapon() const{return m_SubActiveWeapon;}
	void SetSubActiveWeapon(int Sub) {m_SubActiveWeapon = Sub;}
	void HandleWeaponSwitch();
	void DoWeaponSwitch();

	void HandleWeapons();
	void HandleNinja();
	void HandleAmmo();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	void Die(int Killer, int Weapon, bool NoKillMsg = false);
	bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);

	void Bleed(int Ticks);

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);
	bool Remove();

	bool IncreaseHealth(int Amount, int By);
	bool IncreaseArmor(int Amount);

	bool GiveWeapon(int Weapon, int Ammo);
	bool TakeWeapon(int Weapon); //cannot take the last weapon btw

	void GiveNinja(bool Silent = false);
	void TakeNinja();

	void SetEmote(int Emote, int Tick);
	void SetLagTicks(int Tick) {m_LagTicks = Tick;};

	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }

	void Freeze(int Ticks, int By = -1);
	int GetFreezeTicks();
	int WasFrozenBy() { return m_FrozenBy; };
	int WasMoltenBy() { return m_MoltenBy; };
	int GetMeltTick() { return m_MoltenAt; }
	int GetHookedPlayer() { return m_Core.m_HookedPlayer; }
	vec2 GetPosition() { return m_Core.m_Pos;}
	void SetVel(int Tick) { m_Veled_Ticks = Tick;}
	int Get_Vel_Lag() { return m_Veled_Ticks+m_LagTicks;}
	int GetHealth() { return m_Health;}
	int GetHookTick() { return m_Core.m_HookTick; }//starts from 0 on every new hooking
	int LastHammeredBy() { return m_HammeredBy; }
	void ClearLastHammeredBy() { m_HammeredBy = -1; } 

	CTuningParams m_ChrTuning;

private:
	// player controlling this character
	class CPlayer *m_pPlayer;

	bool m_Alive;

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;

	struct WeaponStat
	{
		int m_AmmoRegenStart;
		int m_Ammo;
		int m_Ammocost;
		bool m_Got;

	} m_aWeapons[NUM_WEAPONS];

	int m_ActiveWeapon;
	int m_SubActiveWeapon;

	int m_LastSubWeapons[NUM_WEAPONS];

	int m_LastWeapon;
	int m_QueuedWeapon;

	int m_ReloadTimer;
	int m_AttackTick;

	int m_DamageTaken;

	int m_EmoteType;
	int m_EmoteStop;

	// last tick that the player took any action ie some input
	int m_LastAction;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;

	int m_DamageTakenTick;

	int m_Health;
	int m_Armor;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CCharacterCore m_Core;

	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core

	int m_FrozenBy;
	int m_MoltenBy;
	int m_MoltenAt;
	int m_LagTicks;

	int m_HammeredBy;
	int m_Veled_Ticks;

	int m_BloodTicks;

};

#endif
