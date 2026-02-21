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
	m_LastFireTick = 0;
	m_LastStolenFrom = -1;
}

void CSkinSteal::OnRender()
{
	if(!g_Config.m_TcHammerStealSkin && !g_Config.m_TcHookStealSkin)
		return;
	
	// Get local character from snap
	const CNetObj_Character &Char = GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Cur;
	
	if(!GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Active)
		return;
	
	// Check hook attach first (higher priority)
	bool HookTriggered = false;
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
			HookTriggered = true;
			m_LastStolenFrom = HookedPlayer;
		}
		
		m_WasHooked = IsHooked;
		if(IsHooked)
			m_LastHookedPlayer = HookedPlayer;
	}
	
	// Check hammer hit (skip if hook just triggered to avoid double steal)
	if(g_Config.m_TcHammerStealSkin && !HookTriggered)
	{
		// Get predicted character for hammer hit detection
		CCharacter *pLocalChar = GameClient()->m_PredictedWorld.GetCharacterById(GameClient()->m_Snap.m_LocalClientId);
		if(pLocalChar && pLocalChar->GetActiveWeapon() == WEAPON_HAMMER)
		{
			int CurrentAttackTick = pLocalChar->GetAttackTick();
			if(CurrentAttackTick > m_LastFireTick)
			{
				// Hammer just fired, use predicted world to find hit target
				StealFromPredictedHammerHit(pLocalChar);
				m_LastFireTick = CurrentAttackTick;
			}
		}
	}
}

void CSkinSteal::StealFromPredictedHammerHit(CCharacter *pLocalChar)
{
	if(!pLocalChar)
		return;
	
	// Use the same logic as the game's FireWeapon function
	float ProximityRadius = pLocalChar->GetProximityRadius();
	float Angle = pLocalChar->Core()->m_Angle * (pi / 180.0f) / 256.0f;
	vec2 Direction = vec2(cosf(Angle), sinf(Angle));
	vec2 ProjStartPos = pLocalChar->Core()->m_Pos + Direction * (ProximityRadius * 0.75f);
	
	CEntity *apEnts[MAX_CLIENTS];
	int Num = GameClient()->m_PredictedWorld.FindEntities(ProjStartPos, ProximityRadius * 0.5f, apEnts,
		MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
	
	// Find the best target (same logic as game - first valid hit)
	for(int i = 0; i < Num; ++i)
	{
		auto *pTarget = static_cast<CCharacter *>(apEnts[i]);
		int TargetId = pTarget->GetCid();
		
		// Skip self and invalid targets
		if(TargetId == GameClient()->m_Snap.m_LocalClientId || TargetId < 0)
			continue;
		
		// Check if can collide
		if(!pLocalChar->CanCollide(TargetId))
			continue;
		
		// Skip if we just stole from this player via hook (avoid double steal)
		if(TargetId == m_LastStolenFrom)
		{
			m_LastStolenFrom = -1;  // Reset after skipping
			continue;
		}
		
		// Found a valid target - steal skin
		StealSkin(TargetId);
		return;  // Only steal from first hit target (like game logic)
	}
}

void CSkinSteal::StealSkin(int TargetId)
{
	// Check cooldown (50ms for faster response)
	int64_t CurrentTime = time();
	if((CurrentTime - m_LastStealTime) * 1000 / time_freq() < 50)
		return;
	
	// Validate target
	if(TargetId < 0 || TargetId >= MAX_CLIENTS)
		return;
	
	// Use snap data for skin info (more reliable than m_aClients)
	if(!GameClient()->m_Snap.m_aCharacters[TargetId].m_Active)
		return;
	
	// Get skin info from the snap data
	const CGameClient::CClientData &Target = GameClient()->m_aClients[TargetId];
	if(!Target.m_Active)
		return;
	
	// Copy skin
	m_LastStealTime = CurrentTime;
	
	// Use the existing skin profiles system
	char aBuf[512];
	
	// Check if target uses custom colors
	bool UseCustomColor = Target.m_UseCustomColor;
	
	// For original skin (no custom color), reset colors first to avoid seeing previous colors
	if(!UseCustomColor)
	{
		// Reset to default colors first
		Console()->ExecuteLine("player_use_custom_color 0", -1);
		Console()->ExecuteLine("player_color_body 0", -1);
		Console()->ExecuteLine("player_color_feet 0", -1);
	}
	else
	{
		// Enable custom colors
		Console()->ExecuteLine("player_use_custom_color 1", -1);
	}
	
	// Set skin name
	str_format(aBuf, sizeof(aBuf), "player_skin %s", Target.m_aSkinName);
	Console()->ExecuteLine(aBuf, -1);
	
	// Only set colors if target uses custom colors
	if(UseCustomColor)
	{
		// Set body color
		str_format(aBuf, sizeof(aBuf), "player_color_body %d", Target.m_ColorBody);
		Console()->ExecuteLine(aBuf, -1);
		
		// Set feet color
		str_format(aBuf, sizeof(aBuf), "player_color_feet %d", Target.m_ColorFeet);
		Console()->ExecuteLine(aBuf, -1);
	}
}
