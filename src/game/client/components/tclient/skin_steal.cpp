#include "skin_steal.h"

#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/client/prediction/gameworld.h>

void CSkinSteal::OnInit()
{
	m_LastStealTime = 0;
	m_LastHookedPlayer = -1;
	m_WasHooked = false;
	m_WasFiring = false;
}

void CSkinSteal::OnRender()
{
	if(!g_Config.m_TcHammerStealSkin && !g_Config.m_TcHookStealSkin)
		return;
	
	// Get local character from snap (more reliable than predicted world)
	const CNetObj_Character &Char = GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Cur;
	const CNetObj_Character &PrevChar = GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Prev;
	
	if(!GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Active)
		return;
	
	// Check hammer hit
	if(g_Config.m_TcHammerStealSkin)
	{
		// Detect hammer fire by checking if attack tick changed and weapon is hammer
		bool IsFiring = (Char.m_Weapon == WEAPON_HAMMER) && 
		                (Char.m_AttackTick != PrevChar.m_AttackTick);
		
		if(IsFiring && !m_WasFiring)
		{
			// Hammer just hit something, find target
			StealFromNearestPlayer();
		}
		
		m_WasFiring = IsFiring;
	}
	
	// Check hook attach
	if(g_Config.m_TcHookStealSkin)
	{
		// Check if hook is attached to a player
		int HookedPlayer = -1;
		if(Char.m_HookState == HOOK_GRABBED)
		{
			// Find which player we're hooked to
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(i == GameClient()->m_Snap.m_LocalClientId)
					continue;
				
				if(!GameClient()->m_Snap.m_aCharacters[i].m_Active)
					continue;
				
				const CNetObj_Character &Target = GameClient()->m_Snap.m_aCharacters[i].m_Cur;
				
				// Check if hook is attached to this player
				if(length(vec2(Char.m_HookX, Char.m_HookY) - vec2(Target.m_X, Target.m_Y)) < 28.0f)
				{
					HookedPlayer = i;
					break;
				}
			}
		}
		
		bool IsHooked = HookedPlayer >= 0;
		
		// Detect hook attach to new player
		if(IsHooked && !m_WasHooked && HookedPlayer != m_LastHookedPlayer)
		{
			StealSkin(HookedPlayer);
		}
		
		m_WasHooked = IsHooked;
		if(IsHooked)
			m_LastHookedPlayer = HookedPlayer;
	}
}

void CSkinSteal::StealFromNearestPlayer()
{
	// Find nearest player in hammer range
	const CNetObj_Character &Local = GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Cur;
	if(!GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Active)
		return;
	
	vec2 LocalPos = vec2(Local.m_X, Local.m_Y);
	float Angle = Local.m_Angle * (pi / 180.0f) / 256.0f;
	vec2 Direction = vec2(cosf(Angle), sinf(Angle));
	vec2 HammerPos = LocalPos + Direction * 28.0f; // Hammer hit position
	
	int BestTarget = -1;
	float BestDist = 1000.0f;
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == GameClient()->m_Snap.m_LocalClientId)
			continue;
		
		if(!GameClient()->m_Snap.m_aCharacters[i].m_Active)
			continue;
		
		const CNetObj_Character &Target = GameClient()->m_Snap.m_aCharacters[i].m_Cur;
		
		vec2 TargetPos = vec2(Target.m_X, Target.m_Y);
		float Dist = length(HammerPos - TargetPos);
		
		// Check if in hammer range (approximately 28 units)
		if(Dist < 40.0f && Dist < BestDist)
		{
			BestDist = Dist;
			BestTarget = i;
		}
	}
	
	if(BestTarget >= 0)
	{
		StealSkin(BestTarget);
	}
}

void CSkinSteal::StealSkin(int TargetId)
{
	// Check cooldown (100ms to prevent spam but allow quick response)
	int64_t CurrentTime = time();
	if((CurrentTime - m_LastStealTime) * 1000 / time_freq() < 100)
		return;
	
	// Validate target
	if(TargetId < 0 || TargetId >= MAX_CLIENTS)
		return;
	
	const CGameClient::CClientData &Target = GameClient()->m_aClients[TargetId];
	if(!Target.m_Active || TargetId == GameClient()->m_Snap.m_LocalClientId)
		return;
	
	// Copy skin
	m_LastStealTime = CurrentTime;
	
	// Use the existing skin profiles system
	char aBuf[512];
	
	// Enable custom colors
	Console()->ExecuteLine("player_use_custom_color 1", -1);
	
	// Set skin name
	str_format(aBuf, sizeof(aBuf), "player_skin %s", Target.m_aSkinName);
	Console()->ExecuteLine(aBuf, -1);
	
	// Set body color
	str_format(aBuf, sizeof(aBuf), "player_color_body %d", Target.m_ColorBody);
	Console()->ExecuteLine(aBuf, -1);
	
	// Set feet color
	str_format(aBuf, sizeof(aBuf), "player_color_feet %d", Target.m_ColorFeet);
	Console()->ExecuteLine(aBuf, -1);
}
