#include "skin_steal.h"

#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/client/prediction/gameworld.h>

void CSkinSteal::OnInit()
{
	m_LastStealTime = 0;
	m_LastHookedPlayer = -1;
	m_WasHooked = false;
	m_WasFiringHammer = false;
	m_LastStolenFrom = -1;
	m_LastProcessedEventTick = -1;
	m_LastHammerHitTick = -1;
	m_HammerStealTriggeredThisTick = false;
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
		// Reset tick-based flag at the start of each frame
		m_HammerStealTriggeredThisTick = false;
		
		// Get current game tick
		int CurrentTick = GameClient()->m_PredictedWorld.GameTick();
		
		// Method 1: Use predicted hammer events (more reliable, event-driven)
		CheckPredictedHammerEvents();
		
		// Method 2: Use snap data as fallback (check attack tick change)
		// Only if event-based detection didn't trigger this tick
		if(!m_HammerStealTriggeredThisTick)
		{
			const CNetObj_Character &PrevChar = GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Prev;
			bool IsFiringHammer = (Char.m_Weapon == WEAPON_HAMMER) && (Char.m_AttackTick != PrevChar.m_AttackTick);
			
			if(IsFiringHammer && !m_WasFiringHammer)
			{
				// Hammer just hit something - use predicted world for more accurate detection
				StealFromHammerHitPredicted();
			}
			
			m_WasFiringHammer = IsFiringHammer;
		}
	}
}

void CSkinSteal::StealFromHammerHit()
{
	// Get local character
	const CNetObj_Character &Local = GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Cur;
	if(!GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Active)
		return;
	
	// Calculate hammer hit position
	vec2 LocalPos = vec2(Local.m_X, Local.m_Y);
	float Angle = Local.m_Angle * (pi / 180.0f) / 256.0f;
	vec2 Direction = vec2(cosf(Angle), sinf(Angle));
	vec2 HammerPos = LocalPos + Direction * 28.0f; // Hammer hit position
	
	// Find all players in hammer range and select the best target
	int BestTarget = -1;
	float BestScore = -1.0f;
	
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
		if(Dist > 40.0f)
			continue;
		
		// Skip if we just stole from this player via hook
		if(i == m_LastStolenFrom)
		{
			m_LastStolenFrom = -1;
			continue;
		}
		
		// Calculate score based on distance and direction
		vec2 ToTarget = normalize(TargetPos - LocalPos);
		float Alignment = dot(Direction, ToTarget);
		
		// Prefer targets in front (Alignment > 0) and closer
		if(Alignment > 0.0f)
		{
			float Score = Alignment * (1.0f - Dist / 40.0f);
			if(Score > BestScore)
			{
				BestScore = Score;
				BestTarget = i;
			}
		}
	}
	
	// If found a valid target in front, steal from it
	if(BestTarget >= 0 && BestScore > 0.0f)
	{
		StealSkin(BestTarget);
	}
}

void CSkinSteal::CheckPredictedHammerEvents()
{
	// Check for predicted hammer hit events in the predicted world
	CGameWorld *pPredictedWorld = &GameClient()->m_PredictedWorld;
	if(!pPredictedWorld)
		return;
	
	int LocalId = GameClient()->m_Snap.m_LocalClientId;
	int CurrentTick = pPredictedWorld->GameTick();
	
	// If we already processed an event for this tick, skip
	if(m_HammerStealTriggeredThisTick)
		return;
	
	// Iterate through predicted events
	for(const auto &Event : pPredictedWorld->m_PredictedEvents)
	{
		// Only process hammer hit events for local player
		if(Event.m_EventId != NETEVENTTYPE_HAMMERHIT)
			continue;
		
		if(Event.m_Id != LocalId)
			continue;
		
		// Skip already processed events
		if(Event.m_Tick <= m_LastProcessedEventTick)
			continue;
		
		// Update last processed tick
		if(Event.m_Tick > m_LastProcessedEventTick)
			m_LastProcessedEventTick = Event.m_Tick;
		
		// Mark that we've triggered for this tick
		m_HammerStealTriggeredThisTick = true;
		m_LastHammerHitTick = CurrentTick;
		
		// Find target at hammer hit position using predicted world
		vec2 HammerHitPos = Event.m_Pos;
		int BestTarget = -1;
		float BestDist = 1000.0f;
		
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(i == LocalId)
				continue;
			
			CCharacter *pTarget = pPredictedWorld->GetCharacterById(i);
			if(!pTarget)
				continue;
			
			float Dist = length(pTarget->Core()->m_Pos - HammerHitPos);
			if(Dist < BestDist && Dist < 60.0f) // Within hammer hit range
			{
				BestDist = Dist;
				BestTarget = i;
			}
		}
		
		if(BestTarget >= 0)
		{
			StealSkin(BestTarget);
		}
		
		// Only process one event per tick to avoid duplicates
		break;
	}
}

void CSkinSteal::StealFromHammerHitPredicted()
{
	// Skip if we already triggered this tick
	if(m_HammerStealTriggeredThisTick)
		return;
	
	// Use predicted world for more accurate hammer hit detection
	CGameWorld *pPredictedWorld = &GameClient()->m_PredictedWorld;
	if(!pPredictedWorld)
	{
		// Fallback to snap data if predicted world not available
		StealFromHammerHit();
		return;
	}
	
	int LocalId = GameClient()->m_Snap.m_LocalClientId;
	int CurrentTick = pPredictedWorld->GameTick();
	
	CCharacter *pLocalChar = pPredictedWorld->GetCharacterById(LocalId);
	if(!pLocalChar)
	{
		// Fallback to snap data
		StealFromHammerHit();
		return;
	}
	
	// Mark that we've triggered for this tick
	m_HammerStealTriggeredThisTick = true;
	m_LastHammerHitTick = CurrentTick;
	
	// Get local character data from predicted world
	vec2 LocalPos = pLocalChar->Core()->m_Pos;
	float Angle = pLocalChar->Core()->m_Angle / 256.0f * (pi / 180.0f);
	vec2 Direction = vec2(cosf(Angle), sinf(Angle));
	vec2 HammerPos = LocalPos + Direction * 28.0f;
	
	// Find all players in hammer range using predicted world
	int BestTarget = -1;
	float BestScore = -1.0f;
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == LocalId)
			continue;
		
		CCharacter *pTarget = pPredictedWorld->GetCharacterById(i);
		if(!pTarget)
			continue;
		
		vec2 TargetPos = pTarget->Core()->m_Pos;
		float Dist = length(HammerPos - TargetPos);
		
		// Check if in hammer range
		if(Dist > 50.0f) // Slightly larger range for predicted world
			continue;
		
		// Skip if we just stole from this player via hook
		if(i == m_LastStolenFrom)
		{
			m_LastStolenFrom = -1;
			continue;
		}
		
		// Calculate score based on distance and direction
		vec2 ToTarget = normalize(TargetPos - LocalPos);
		float Alignment = dot(Direction, ToTarget);
		
		// Prefer targets in front (Alignment > 0) and closer
		if(Alignment > 0.0f)
		{
			float Score = Alignment * (1.0f - Dist / 50.0f);
			if(Score > BestScore)
			{
				BestScore = Score;
				BestTarget = i;
			}
		}
	}
	
	// If found a valid target in front, steal from it
	if(BestTarget >= 0 && BestScore > 0.0f)
	{
		StealSkin(BestTarget);
	}
}

void CSkinSteal::StealSkin(int TargetId)
{
	// Check cooldown (10ms for very fast response)
	int64_t CurrentTime = time();
	if((CurrentTime - m_LastStealTime) * 1000 / time_freq() < 10)
		return;
	
	// Validate target
	if(TargetId < 0 || TargetId >= MAX_CLIENTS)
		return;
	
	// Use snap data for skin info
	if(!GameClient()->m_Snap.m_aCharacters[TargetId].m_Active)
		return;
	
	// Get skin info
	const CGameClient::CClientData &Target = GameClient()->m_aClients[TargetId];
	if(!Target.m_Active)
		return;
	
	// Copy skin
	m_LastStealTime = CurrentTime;
	
	// Use the existing skin profiles system
	char aBuf[512];
	
	// Check if target uses custom colors
	bool UseCustomColor = Target.m_UseCustomColor;
	
	// For original skin (no custom color), reset colors first
	if(!UseCustomColor)
	{
		Console()->ExecuteLine("player_use_custom_color 0", -1);
		Console()->ExecuteLine("player_color_body 0", -1);
		Console()->ExecuteLine("player_color_feet 0", -1);
	}
	else
	{
		Console()->ExecuteLine("player_use_custom_color 1", -1);
	}
	
	// Set skin name
	str_format(aBuf, sizeof(aBuf), "player_skin %s", Target.m_aSkinName);
	Console()->ExecuteLine(aBuf, -1);
	
	// Only set colors if target uses custom colors
	if(UseCustomColor)
	{
		str_format(aBuf, sizeof(aBuf), "player_color_body %d", Target.m_ColorBody);
		Console()->ExecuteLine(aBuf, -1);
		
		str_format(aBuf, sizeof(aBuf), "player_color_feet %d", Target.m_ColorFeet);
		Console()->ExecuteLine(aBuf, -1);
	}
}
