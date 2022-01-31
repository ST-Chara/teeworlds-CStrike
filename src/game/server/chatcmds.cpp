/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Copyright © 2013 Neox.                                                                                                */
/* If you are missing that file, acquire a complete release at https://www.teeworlds.com/forum/viewtopic.php?pid=106934  */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
//#include "entities/protector.h"
#include <stdio.h>
#define CFG(A) g_Config.m_Sv ## A

void CGameContext::ExecuteCommand(int ClientID, const char* pCmd)
{
    CPlayer* pPlayer = m_apPlayers[ClientID];
    pCmd ++; // Skip the '/'

	int Col = 0;
	if(GetPlayerChar(ClientID))
		Col = Collision()->GetCollisionAt(GetPlayerChar(ClientID)->m_Pos.x, GetPlayerChar(ClientID)->m_Pos.y);

    if(!str_comp_nocase(pCmd, "cmdlist"))
    {
        SendChatTarget(ClientID, "⇠⇠⇠⇠⇠⇠CMDLIST⇢⇢⇢⇢⇢⇢");
        SendChatTarget(ClientID, "/info - Informations about the mod");
        SendChatTarget(ClientID, "/help - Help about the mod");
        SendChatTarget(ClientID, "/shop - Buy some upgrades");
    }
    else if(!str_comp_nocase(pCmd, "info"))
    {
        SendChatTarget(ClientID, "⇠⇠⇠⇠⇠⇠INFO⇢⇢⇢⇢⇢⇢");
        SendChatTarget(ClientID, "Mod Counter Strike based on OpenFng made by Kako");
		SendChatTarget(ClientID, "Thanks to GojE GrEEN ! for his efforts to make this mod");
        //SendChatTarget(ClientID, "You can download this mod from the official Teeworlds forum.");
    }
    else if(!str_comp_nocase(pCmd, "help"))
    {
        SendChatTarget(ClientID, "⇠⇠⇠⇠⇠⇠HELP⇢⇢⇢⇢⇢⇢");
        SendChatTarget(ClientID, "You need to kill others, get points, ");
		SendChatTarget(ClientID, "and with these points you can buy upgrades.");
        SendChatTarget(ClientID, "This help you fight against others easier.");
        SendChatTarget(ClientID, "Your kills are your points. ");
        SendChatTarget(ClientID, "If you wanna look at the shop, just type \"/shop\"");
        SendChatTarget(ClientID, "Have fun.");
    }
    else if(!str_comp_nocase(pCmd, "shop"))
    {
		if(!GetPlayerChar(ClientID))
        {
			SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }
		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "⇠⇠⇠⇠⇠⇠SHOP⇢⇢⇢⇢⇢⇢");
			SendChatTarget(ClientID, "Terrorist items");
			SendChatTarget(ClientID, "/minehit");
			SendChatTarget(ClientID, "/minefreeze");
			SendChatTarget(ClientID, "/minekill");
			SendChatTarget(ClientID, "/flash");
			SendChatTarget(ClientID, "/smoke");
			SendChatTarget(ClientID, "/grenade");
			SendChatTarget(ClientID, "/shield");
			SendChatTarget(ClientID, "/glock");
			SendChatTarget(ClientID, "/desert");
			SendChatTarget(ClientID, "/rpg");
			SendChatTarget(ClientID, "/awp");
			SendChatTarget(ClientID, "/ak47");
		}
		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "⇠⇠⇠⇠⇠⇠SHOP⇢⇢⇢⇢⇢⇢");
			SendChatTarget(ClientID, "Counter Terrorist items");
			SendChatTarget(ClientID, "/minehit_c");
			SendChatTarget(ClientID, "/minefreeze_c");
			SendChatTarget(ClientID, "/minekill_c");
			SendChatTarget(ClientID, "/flash_c");
			SendChatTarget(ClientID, "/smoke_c");
			SendChatTarget(ClientID, "/grenade_c");
			SendChatTarget(ClientID, "/shield_c");
			SendChatTarget(ClientID, "/usp_c");
			SendChatTarget(ClientID, "/desert_c");
			SendChatTarget(ClientID, "/famas_c");
			SendChatTarget(ClientID, "/awp_c");
			SendChatTarget(ClientID, "/m4a_c");
		}
    }
	else if(!str_comp_nocase(pCmd, "st"))
	{
		if(!GetPlayerChar(ClientID))
        {
            //SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(GetPlayerChar(ClientID)->GetFreezeTicks())
			return;

		int stt,st = GetPlayerChar(ClientID)->GetSubActiveWeapon();
		stt = st;

		switch(GetPlayerChar(ClientID)->GetActiveWeapon())
		{
			case 0:
				st = (st+1) % pPlayer->m_Hammer_ST_NUM;
				break;
			case 1:
				st = (st+1) % pPlayer->m_Gun_ST_NUM;
				break;
			case 2:
				st = (st+1) % pPlayer->m_Shotgun_ST_NUM;
				break;
			case 3:
				st = (st+1) % pPlayer->m_Grenade_ST_NUM;
				break;
			case 4:
				st = (st+1) % pPlayer->m_Rifle_ST_NUM;
				break;
			case 5:
				st = (st+1) % pPlayer->m_Ninja_ST_NUM;
				break;
		
		}
		
		GetPlayerChar(ClientID)->SetSubActiveWeapon(st);
		if(stt != st) CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_WEAPON_SPAWN, (1<<pPlayer->GetCID()));
	}
	else if(!m_pController->m_BuyTimer || !m_pController->m_RoundCount)
	{
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		SendChatTarget(ClientID, "Shop is Closed Or This Command Not exist!");
        return;
	}
	else if(Col != TILE_BUYRED && pPlayer->GetTeam() == 0)
	{
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		SendChatTarget(ClientID, "You Must Be in Terorrist Shop Zone Position!");
		return;
	}
	else if(Col != TILE_BUYBLUE && pPlayer->GetTeam() == 1)
	{
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		SendChatTarget(ClientID, "You Must Be in Counter Terorrist Shop Zone Position!");
		return;
	}
	else if(!str_comp_nocase(pCmd, "minehit"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

        if(pPlayer->m_Money >= CFG(MoneyOneMineHit) && pPlayer->m_Ammo_Upgrades.m_MineHit < CFG(ManyAddMineHit))
        {
           
			if(!pPlayer->m_Upgrades.m_MineHit)
			{
				pPlayer->m_Hammer[1] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_MineHit = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneMineHit);
			int Need = CFG(ManyAddMineHit) - pPlayer->m_Ammo_Upgrades.m_MineHit;
			pPlayer->m_Ammo_Upgrades.m_MineHit += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneMineHit):Many*CFG(MoneyOneMineHit);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You now have the mine hit");
        }
        else if(pPlayer->m_Upgrades.m_MineHit && pPlayer->m_Ammo_Upgrades.m_MineHit == CFG(ManyAddMineHit))
            SendChatTarget(ClientID, "You already have the mine hit");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "minefreeze"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneMineFreeze) && pPlayer->m_Ammo_Upgrades.m_MineFreeze < CFG(ManyAddMineFreeze))
        {
			if(!pPlayer->m_Upgrades.m_MineFreeze)
			{
				pPlayer->m_Hammer[2] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_MineFreeze = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneMineFreeze);
			int Need = CFG(ManyAddMineFreeze) - pPlayer->m_Ammo_Upgrades.m_MineFreeze;
			pPlayer->m_Ammo_Upgrades.m_MineFreeze += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneMineFreeze):Many*CFG(MoneyOneMineFreeze);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the mine freeze");
        }
        else if(pPlayer->m_Upgrades.m_MineFreeze && pPlayer->m_Ammo_Upgrades.m_MineFreeze == CFG(ManyAddMineFreeze))
            SendChatTarget(ClientID, "You already have the mine freeze");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "minekill"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneMineKill) && pPlayer->m_Ammo_Upgrades.m_MineKill < CFG(ManyAddMineKill))
        {
			if(!pPlayer->m_Upgrades.m_MineKill)
			{
				pPlayer->m_Hammer[3] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_MineKill = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneMineKill);
			int Need = CFG(ManyAddMineKill) - pPlayer->m_Ammo_Upgrades.m_MineKill;
			pPlayer->m_Ammo_Upgrades.m_MineKill += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneMineKill):Many*CFG(MoneyOneMineKill);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the mine kill");
        }
        else if(pPlayer->m_Upgrades.m_MineKill && pPlayer->m_Ammo_Upgrades.m_MineKill == CFG(ManyAddMineKill))
            SendChatTarget(ClientID, "You already have the mine kill");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "flash"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneFlash) && pPlayer->m_Ammo_Upgrades.m_Flash < CFG(ManyAddFlash))
        {
			if(!pPlayer->m_Upgrades.m_Flash)
			{
				pPlayer->m_Hammer[4] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_Flash = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneFlash);
			int Need = CFG(ManyAddFlash) - pPlayer->m_Ammo_Upgrades.m_Flash;
			pPlayer->m_Ammo_Upgrades.m_Flash += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneFlash):Many*CFG(MoneyOneFlash);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the flash");
        }
        else if(pPlayer->m_Upgrades.m_Flash && pPlayer->m_Ammo_Upgrades.m_Flash == CFG(ManyAddFlash))
            SendChatTarget(ClientID, "You already have the flash");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "smoke"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneSmoke) && pPlayer->m_Ammo_Upgrades.m_Smoke < CFG(ManyAddSmoke))
        {
			if(!pPlayer->m_Upgrades.m_Smoke)
			{
				pPlayer->m_Hammer[5] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_Smoke = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneSmoke);
			int Need = CFG(ManyAddSmoke) - pPlayer->m_Ammo_Upgrades.m_Smoke;
			pPlayer->m_Ammo_Upgrades.m_Smoke += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneSmoke):Many*CFG(MoneyOneSmoke);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the smoke");
        }
        else if(pPlayer->m_Upgrades.m_Smoke && pPlayer->m_Ammo_Upgrades.m_Smoke == CFG(ManyAddSmoke))
            SendChatTarget(ClientID, "You already have the smoke");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "grenade"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneGrenade) && pPlayer->m_Ammo_Upgrades.m_Grenade < CFG(ManyAddGrenade))
        {
			if(!pPlayer->m_Upgrades.m_Grenade)
			{
				pPlayer->m_Hammer[6] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_Grenade = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneGrenade);
			int Need = CFG(ManyAddGrenade) - pPlayer->m_Ammo_Upgrades.m_Grenade;
			pPlayer->m_Ammo_Upgrades.m_Grenade += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneGrenade):Many*CFG(MoneyOneGrenade);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the grenade");
        }
        else if(pPlayer->m_Upgrades.m_Grenade && pPlayer->m_Ammo_Upgrades.m_Grenade == CFG(ManyAddGrenade))
            SendChatTarget(ClientID, "You already have the grenade");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "shield"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneShield) && pPlayer->m_Ammo_Upgrades.m_Shield < CFG(ManyAddShield))
        {
			if(!pPlayer->m_Upgrades.m_Shield)
			{
				pPlayer->m_Hammer[7] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_Shield = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneShield);
			int Need = CFG(ManyAddShield) - pPlayer->m_Ammo_Upgrades.m_Shield;
			pPlayer->m_Ammo_Upgrades.m_Shield += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneShield):Many*CFG(MoneyOneShield);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the shield");
        }
        else if(pPlayer->m_Upgrades.m_Shield && pPlayer->m_Ammo_Upgrades.m_Shield == CFG(ManyAddShield))
            SendChatTarget(ClientID, "You already have the shield");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "glock"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyGlock) && !pPlayer->m_Upgrades.m_Glock)
		{
			pPlayer->m_Gun[0] = pPlayer->m_Gun_ST_NUM;
			pPlayer->m_Gun_ST_NUM++;
            pPlayer->m_Upgrades.m_Glock = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_GUN, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_GUN);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the glock");

			pPlayer->m_Ammo_Upgrades.m_Glock = CFG(ManyAddGlock);
			pPlayer->m_Money -= CFG(BuyGlock);
		}
		else if(pPlayer->m_Upgrades.m_Glock && pPlayer->m_Ammo_Upgrades.m_Glock < CFG(ManyAddGlock) && pPlayer->m_Money >= CFG(MoneyOneAmmoGlock))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoGlock);
			int Need = CFG(ManyAddGlock) - pPlayer->m_Ammo_Upgrades.m_Glock;
			pPlayer->m_Ammo_Upgrades.m_Glock += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoGlock):Many*CFG(MoneyOneAmmoGlock);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the glock");
        }
        else if(pPlayer->m_Upgrades.m_Glock && pPlayer->m_Ammo_Upgrades.m_Glock == CFG(ManyAddGlock))
            SendChatTarget(ClientID, "You already have the glock");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "desert"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyDesert) && !pPlayer->m_Upgrades.m_Desert)
		{
			pPlayer->m_Gun[1] = pPlayer->m_Gun_ST_NUM;
			pPlayer->m_Gun_ST_NUM++;
            pPlayer->m_Upgrades.m_Desert = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_GUN, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_GUN);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the desert");

			pPlayer->m_Ammo_Upgrades.m_Desert = CFG(ManyAddDesert);
			pPlayer->m_Money -= CFG(BuyDesert);
        }
		else if(pPlayer->m_Upgrades.m_Desert && pPlayer->m_Ammo_Upgrades.m_Desert < CFG(ManyAddDesert) && pPlayer->m_Money >= CFG(MoneyOneAmmoDesert))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoDesert);
			int Need = CFG(ManyAddDesert) - pPlayer->m_Ammo_Upgrades.m_Desert;
			pPlayer->m_Ammo_Upgrades.m_Desert += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoDesert):Many*CFG(MoneyOneAmmoDesert);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the desert");
        }
        else if(pPlayer->m_Upgrades.m_Desert && pPlayer->m_Ammo_Upgrades.m_Desert == CFG(ManyAddDesert))
            SendChatTarget(ClientID, "You already have the desert");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "rpg"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyRPG) && !pPlayer->m_Upgrades.m_RPG)
		{
			pPlayer->m_Grenade[0] = pPlayer->m_Grenade_ST_NUM;
			pPlayer->m_Grenade_ST_NUM++;
            pPlayer->m_Upgrades.m_RPG = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_GRENADE, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_GRENADE);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_GRENADE);
            SendChatTarget(ClientID, "You now have the RPG");

			pPlayer->m_Ammo_Upgrades.m_RPG = CFG(ManyAddRPG);
			pPlayer->m_Money -= CFG(BuyRPG);
        }
		else if(pPlayer->m_Upgrades.m_RPG && pPlayer->m_Ammo_Upgrades.m_RPG < CFG(ManyAddRPG) && pPlayer->m_Money >= CFG(MoneyOneAmmoRPG))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoRPG);
			int Need = CFG(ManyAddRPG) - pPlayer->m_Ammo_Upgrades.m_RPG;
			pPlayer->m_Ammo_Upgrades.m_RPG += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoRPG):Many*CFG(MoneyOneAmmoRPG);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the RPG");
        }
        else if(pPlayer->m_Upgrades.m_RPG && pPlayer->m_Ammo_Upgrades.m_RPG == CFG(ManyAddRPG))
            SendChatTarget(ClientID, "You already have the RPG");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "awp"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyAWP) && !pPlayer->m_Upgrades.m_AWP)
		{
			pPlayer->m_Rifle[0] = pPlayer->m_Rifle_ST_NUM;
			pPlayer->m_Rifle_ST_NUM++;
            pPlayer->m_Upgrades.m_AWP = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_RIFLE, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_RIFLE);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_GRENADE);
            SendChatTarget(ClientID, "You now have the AWP");

			pPlayer->m_Ammo_Upgrades.m_AWP = CFG(ManyAddAWP);
			pPlayer->m_Money -= CFG(BuyAWP);
        }
		else if(pPlayer->m_Upgrades.m_AWP && pPlayer->m_Ammo_Upgrades.m_AWP < CFG(ManyAddAWP) && pPlayer->m_Money >= CFG(MoneyOneAmmoAWP))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoAWP);
			int Need = CFG(ManyAddAWP) - pPlayer->m_Ammo_Upgrades.m_AWP;
			pPlayer->m_Ammo_Upgrades.m_AWP += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoAWP):Many*CFG(MoneyOneAmmoAWP);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the AWP");
        }
        else if(pPlayer->m_Upgrades.m_AWP && pPlayer->m_Ammo_Upgrades.m_AWP == CFG(ManyAddAWP))
            SendChatTarget(ClientID, "You already have the AWP");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "ak47"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 1)
		{
			SendChatTarget(ClientID, "You can just buy Counter Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyAK47) && !pPlayer->m_Upgrades.m_AK47)
		{
			pPlayer->m_Rifle[1] = pPlayer->m_Rifle_ST_NUM;
			pPlayer->m_Rifle_ST_NUM++;
            pPlayer->m_Upgrades.m_AK47 = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_RIFLE, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_RIFLE);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_GRENADE);
            SendChatTarget(ClientID, "You now have the AK47");

			pPlayer->m_Ammo_Upgrades.m_AK47 = CFG(ManyAddAK47);
			pPlayer->m_Money -= CFG(BuyAK47);
        }
		else if(pPlayer->m_Upgrades.m_AK47 && pPlayer->m_Ammo_Upgrades.m_AK47 < CFG(ManyAddAK47) && pPlayer->m_Money >= CFG(MoneyOneAmmoAK47))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoAK47);
			int Need = CFG(ManyAddAK47) - pPlayer->m_Ammo_Upgrades.m_AK47;
			pPlayer->m_Ammo_Upgrades.m_AK47 += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoAK47):Many*CFG(MoneyOneAmmoAK47);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the AK47");
        }
        else if(pPlayer->m_Upgrades.m_AK47 && pPlayer->m_Ammo_Upgrades.m_AK47 == CFG(ManyAddAK47))
            SendChatTarget(ClientID, "You already have the AK47");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "minehit_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		 if(pPlayer->m_Money >= CFG(MoneyOneMineHit) && pPlayer->m_Ammo_Upgrades.m_MineHit < CFG(ManyAddMineHit))
        {
           
			if(!pPlayer->m_Upgrades.m_MineHit)
			{
				pPlayer->m_Hammer[1] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_MineHit = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneMineHit);
			int Need = CFG(ManyAddMineHit) - pPlayer->m_Ammo_Upgrades.m_MineHit;
			pPlayer->m_Ammo_Upgrades.m_MineHit += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneMineHit):Many*CFG(MoneyOneMineHit);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You now have the mine hit");
        }
        else if(pPlayer->m_Upgrades.m_MineHit && pPlayer->m_Ammo_Upgrades.m_MineHit == CFG(ManyAddMineHit))
            SendChatTarget(ClientID, "You already have the mine hit");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "minefreeze_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneMineFreeze) && pPlayer->m_Ammo_Upgrades.m_MineFreeze < CFG(ManyAddMineFreeze))
        {
			if(!pPlayer->m_Upgrades.m_MineFreeze)
			{
				pPlayer->m_Hammer[2] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_MineFreeze = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneMineFreeze);
			int Need = CFG(ManyAddMineFreeze) - pPlayer->m_Ammo_Upgrades.m_MineFreeze;
			pPlayer->m_Ammo_Upgrades.m_MineFreeze += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneMineFreeze):Many*CFG(MoneyOneMineFreeze);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the mine freeze");
        }
        else if(pPlayer->m_Upgrades.m_MineFreeze && pPlayer->m_Ammo_Upgrades.m_MineFreeze == CFG(ManyAddMineFreeze))
            SendChatTarget(ClientID, "You already have the mine freeze");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "minekill_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneMineKill) && pPlayer->m_Ammo_Upgrades.m_MineKill < CFG(ManyAddMineKill))
        {
			if(!pPlayer->m_Upgrades.m_MineKill)
			{
				pPlayer->m_Hammer[3] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_MineKill = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneMineKill);
			int Need = CFG(ManyAddMineKill) - pPlayer->m_Ammo_Upgrades.m_MineKill;
			pPlayer->m_Ammo_Upgrades.m_MineKill += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneMineKill):Many*CFG(MoneyOneMineKill);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the mine kill");
        }
        else if(pPlayer->m_Upgrades.m_MineKill && pPlayer->m_Ammo_Upgrades.m_MineKill == CFG(ManyAddMineKill))
            SendChatTarget(ClientID, "You already have the mine kill");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "flash_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneFlash) && pPlayer->m_Ammo_Upgrades.m_Flash < CFG(ManyAddFlash))
        {
			if(!pPlayer->m_Upgrades.m_Flash)
			{
				pPlayer->m_Hammer[4] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_Flash = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneFlash);
			int Need = CFG(ManyAddFlash) - pPlayer->m_Ammo_Upgrades.m_Flash;
			pPlayer->m_Ammo_Upgrades.m_Flash += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneFlash):Many*CFG(MoneyOneFlash);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the flash");
        }
        else if(pPlayer->m_Upgrades.m_Flash && pPlayer->m_Ammo_Upgrades.m_Flash == CFG(ManyAddFlash))
            SendChatTarget(ClientID, "You already have the flash");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "smoke_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneSmoke) && pPlayer->m_Ammo_Upgrades.m_Smoke < CFG(ManyAddSmoke))
        {
			if(!pPlayer->m_Upgrades.m_Smoke)
			{
				pPlayer->m_Hammer[5] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_Smoke = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneSmoke);
			int Need = CFG(ManyAddSmoke) - pPlayer->m_Ammo_Upgrades.m_Smoke;
			pPlayer->m_Ammo_Upgrades.m_Smoke += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneSmoke):Many*CFG(MoneyOneSmoke);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the smoke");
        }
        else if(pPlayer->m_Upgrades.m_Smoke && pPlayer->m_Ammo_Upgrades.m_Smoke == CFG(ManyAddSmoke))
            SendChatTarget(ClientID, "You already have the smoke");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "grenade_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneGrenade) && pPlayer->m_Ammo_Upgrades.m_Grenade < CFG(ManyAddGrenade))
        {
			if(!pPlayer->m_Upgrades.m_Grenade)
			{
				pPlayer->m_Hammer[6] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_Grenade = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneGrenade);
			int Need = CFG(ManyAddGrenade) - pPlayer->m_Ammo_Upgrades.m_Grenade;
			pPlayer->m_Ammo_Upgrades.m_Grenade += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneGrenade):Many*CFG(MoneyOneGrenade);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the grenade");
        }
        else if(pPlayer->m_Upgrades.m_Grenade && pPlayer->m_Ammo_Upgrades.m_Grenade == CFG(ManyAddGrenade))
            SendChatTarget(ClientID, "You already have the grenade");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "shield_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(MoneyOneShield) && pPlayer->m_Ammo_Upgrades.m_Shield < CFG(ManyAddShield))
        {
			if(!pPlayer->m_Upgrades.m_Shield)
			{
				pPlayer->m_Hammer[7] = pPlayer->m_Hammer_ST_NUM;
				pPlayer->m_Hammer_ST_NUM++;
				pPlayer->m_Upgrades.m_Shield = true;
			}

			int Many = pPlayer->m_Money/CFG(MoneyOneShield);
			int Need = CFG(ManyAddShield) - pPlayer->m_Ammo_Upgrades.m_Shield;
			pPlayer->m_Ammo_Upgrades.m_Shield += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneShield):Many*CFG(MoneyOneShield);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the shield");
        }
        else if(pPlayer->m_Upgrades.m_Shield && pPlayer->m_Ammo_Upgrades.m_Shield == CFG(ManyAddShield))
            SendChatTarget(ClientID, "You already have the shield");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "usp_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyUSP) && !pPlayer->m_Upgrades.m_USP)
		{
			pPlayer->m_Gun[2] = pPlayer->m_Gun_ST_NUM;
			pPlayer->m_Gun_ST_NUM++;
            pPlayer->m_Upgrades.m_USP = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_GUN, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_GUN);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the USP");

			pPlayer->m_Ammo_Upgrades.m_USP = CFG(ManyAddUSP);
			pPlayer->m_Money -= CFG(BuyUSP);
        }
		else if(pPlayer->m_Upgrades.m_USP && pPlayer->m_Ammo_Upgrades.m_USP < CFG(ManyAddUSP) && pPlayer->m_Money >= CFG(MoneyOneAmmoUSP))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoUSP);
			int Need = CFG(ManyAddUSP) - pPlayer->m_Ammo_Upgrades.m_USP;
			pPlayer->m_Ammo_Upgrades.m_USP += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoUSP):Many*CFG(MoneyOneAmmoUSP);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the USP");
        }
        else if(pPlayer->m_Upgrades.m_USP && pPlayer->m_Ammo_Upgrades.m_USP == CFG(ManyAddUSP))
            SendChatTarget(ClientID, "You already have the USP");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "desert_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyDesert) && !pPlayer->m_Upgrades.m_Desert)
		{
			pPlayer->m_Gun[1] = pPlayer->m_Gun_ST_NUM;
			pPlayer->m_Gun_ST_NUM++;
            pPlayer->m_Upgrades.m_Desert = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_GUN, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_GUN);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
            SendChatTarget(ClientID, "You now have the desert");

			pPlayer->m_Ammo_Upgrades.m_Desert = CFG(ManyAddDesert);
			pPlayer->m_Money -= CFG(BuyDesert);
        }
		else if(pPlayer->m_Upgrades.m_Desert && pPlayer->m_Ammo_Upgrades.m_Desert < CFG(ManyAddDesert) && pPlayer->m_Money >= CFG(MoneyOneAmmoDesert))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoDesert);
			int Need = CFG(ManyAddDesert) - pPlayer->m_Ammo_Upgrades.m_Desert;
			pPlayer->m_Ammo_Upgrades.m_Desert += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoDesert):Many*CFG(MoneyOneAmmoDesert);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the desert");
        }
        else if(pPlayer->m_Upgrades.m_Desert && pPlayer->m_Ammo_Upgrades.m_Desert == CFG(ManyAddDesert))
            SendChatTarget(ClientID, "You already have the desert");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "famas_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyFAMAS) && !pPlayer->m_Upgrades.m_FAMAS)
		{
			pPlayer->m_Shotgun[0] = pPlayer->m_Shotgun_ST_NUM;
			pPlayer->m_Shotgun_ST_NUM++;
            pPlayer->m_Upgrades.m_FAMAS = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_SHOTGUN, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_SHOTGUN);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_SHOTGUN);
            SendChatTarget(ClientID, "You now have the FAMAS");

			pPlayer->m_Ammo_Upgrades.m_FAMAS = CFG(ManyAddFAMAS);
			pPlayer->m_Money -= CFG(BuyFAMAS);
        }
		else if(pPlayer->m_Upgrades.m_FAMAS && pPlayer->m_Ammo_Upgrades.m_FAMAS < CFG(ManyAddFAMAS) && pPlayer->m_Money >= CFG(MoneyOneAmmoFAMAS))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoFAMAS);
			int Need = CFG(ManyAddFAMAS) - pPlayer->m_Ammo_Upgrades.m_FAMAS;
			pPlayer->m_Ammo_Upgrades.m_FAMAS += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoFAMAS):Many*CFG(MoneyOneAmmoFAMAS);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the FAMAS");
        }
        else if(pPlayer->m_Upgrades.m_FAMAS && pPlayer->m_Ammo_Upgrades.m_FAMAS == CFG(ManyAddFAMAS))
            SendChatTarget(ClientID, "You already have the FAMAS");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "awp_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyAWP) && !pPlayer->m_Upgrades.m_AWP)
		{
			pPlayer->m_Rifle[0] = pPlayer->m_Rifle_ST_NUM;
			pPlayer->m_Rifle_ST_NUM++;
            pPlayer->m_Upgrades.m_AWP = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_RIFLE, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_RIFLE);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_GRENADE);
            SendChatTarget(ClientID, "You now have the AWP");

			pPlayer->m_Ammo_Upgrades.m_AWP = CFG(ManyAddAWP);
			pPlayer->m_Money -= CFG(BuyAWP);
        }
		else if(pPlayer->m_Upgrades.m_AWP && pPlayer->m_Ammo_Upgrades.m_AWP < CFG(ManyAddAWP) && pPlayer->m_Money >= CFG(MoneyOneAmmoAWP))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoAWP);
			int Need = CFG(ManyAddAWP) - pPlayer->m_Ammo_Upgrades.m_AWP;
			pPlayer->m_Ammo_Upgrades.m_AWP += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoAWP):Many*CFG(MoneyOneAmmoAWP);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the AWP");
        }
        else if(pPlayer->m_Upgrades.m_AWP && pPlayer->m_Ammo_Upgrades.m_AWP == CFG(ManyAddAWP))
            SendChatTarget(ClientID, "You already have the AWP");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	else if(!str_comp_nocase(pCmd, "m4a_c"))
    {
		if(!GetPlayerChar(ClientID))
        {
            SendChatTarget(ClientID, "Wait to respawn to do this");
            return;
        }

		if(pPlayer->GetTeam() == 0)
		{
			SendChatTarget(ClientID, "You can just buy Terrorist items");
			return;
		}

		if(pPlayer->m_Money >= CFG(BuyMP4a) && !pPlayer->m_Upgrades.m_MP4a)
		{
			pPlayer->m_Rifle[2] = pPlayer->m_Rifle_ST_NUM;
			pPlayer->m_Rifle_ST_NUM++;
            pPlayer->m_Upgrades.m_MP4a = true;
			GetPlayerChar(ClientID)->GiveWeapon(WEAPON_RIFLE, 10);
			GetPlayerChar(ClientID)->SetWeapon(WEAPON_RIFLE);
			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_SHOTGUN);
            SendChatTarget(ClientID, "You now have the MP4a");

			pPlayer->m_Ammo_Upgrades.m_MP4a = CFG(ManyAddMP4a);
			pPlayer->m_Money -= CFG(BuyMP4a);
        }
		else if(pPlayer->m_Upgrades.m_MP4a && pPlayer->m_Ammo_Upgrades.m_MP4a < CFG(ManyAddMP4a) && pPlayer->m_Money >= CFG(MoneyOneAmmoMP4a))
        {
			int Many = pPlayer->m_Money/CFG(MoneyOneAmmoMP4a);
			int Need = CFG(ManyAddMP4a) - pPlayer->m_Ammo_Upgrades.m_MP4a;
			pPlayer->m_Ammo_Upgrades.m_MP4a += (Many-Need) > 0 ? Need:Many;
			pPlayer->m_Money -= (Many-Need) > 0 ? Need*CFG(MoneyOneAmmoMP4a):Many*CFG(MoneyOneAmmoMP4a);

			CreateSound(GetPlayerChar(ClientID)->GetPosition(), SOUND_PICKUP_ARMOR);
			SendChatTarget(ClientID, "You buy ammo for the MP4a");
        }
        else if(pPlayer->m_Upgrades.m_MP4a && pPlayer->m_Ammo_Upgrades.m_MP4a == CFG(ManyAddMP4a))
            SendChatTarget(ClientID, "You already have the MP4a");
        else
            SendChatTarget(ClientID, "You dont have enough money to buy this");
    }
	



	//bool m_MineHit;		1
	//bool m_MineFreeze;	2
	//bool m_MineKill;		3
	//bool m_Flash;			4 
	//bool m_Smoke;			5
	//bool m_Grenade;		6

	//// gun sub weapons
	//bool m_Glock;			0
	//bool m_Desert;		1
	//bool m_USP;			2

	//// shotgun sub weapons
	//bool m_FAMAS;			0

	//// grenade sub weapons
	//bool m_RPG;			0

	//// rifle sub weapons
	//bool m_AWP;			0
	//bool m_AK47;			1
	//bool m_MP4a;			2
    else
    {
        char aBuf[512];
        str_format(aBuf, sizeof(aBuf), "Typed : /%s", pCmd);
        SendChatTarget(ClientID, "This command doesn't exist, try \"/cmdlist\".");
        SendChatTarget(ClientID, aBuf);
    }
}
