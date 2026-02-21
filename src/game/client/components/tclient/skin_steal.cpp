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
	m_WasFiringHammer = false;
}

void CSkinSteal::OnRender()
{
	if(g_Config.m_TcHammerStealSkin)
		CheckHammerHit();
	
	if(g_Config.m_TcHookStealSkin)
		CheckHookAttach();
}

void CSkinSteal::CheckHammerHit()
{
	// Get local character
	CCharacter *pLocalChar = GameClient()->m_PredictedWorld.GetCharacterById(GameClient()->m_Snap.m_LocalClientId);
	if(!pLocalChar)
		return;
	
	// Check if currently firing hammer using the controls input data
	int Dummy = g_Config.m_ClDummy;
	bool IsFiringHammer = pLocalChar->GetActiveWeapon() == WEAPON_HAMMER && 
	                      GameClient()->m_Controls.m_aInputData[Dummy].m_Fire;
	
	// Detect hammer fire start
	if(IsFiringHammer && !m_WasFiringHammer)
	{
		// Hammer was just fired, check for hits
		// Get direction from angle (m_Angle is in units, 1 unit = 1/256 of a full rotation)
		float Angle = pLocalChar->Core()->m_Angle * (pi / 180.0f) / 256.0f;
		vec2 Direction = vec2(cosf(Angle), sinf(Angle));
		vec2 ProjStartPos = pLocalChar->Core()->m_Pos + Direction * pLocalChar->GetProximityRadius() * 0.75f;
		
		CEntity *apEnts[MAX_CLIENTS];
		int Num = GameClient()->m_PredictedWorld.FindEntities(ProjStartPos, pLocalChar->GetProximityRadius() * 0.5f, apEnts,
			MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		
		for(int i = 0; i < Num; ++i)
		{
			auto *pTarget = static_cast<CCharacter *>(apEnts[i]);
			int TargetId = pTarget->GetCid();
			
			// Skip self and invalid targets
			if(TargetId == GameClient()->m_Snap.m_LocalClientId || TargetId < 0)
				continue;
			
			// Check if can collide (not in same team, etc.)
			if(!pLocalChar->CanCollide(TargetId))
				continue;
			
			// Steal skin from this target
			StealSkin(TargetId);
		}
	}
	
	m_WasFiringHammer = IsFiringHammer;
}

void CSkinSteal::CheckHookAttach()
{
	// Get local character
	CCharacter *pLocalChar = GameClient()->m_PredictedWorld.GetCharacterById(GameClient()->m_Snap.m_LocalClientId);
	if(!pLocalChar)
		return;
	
	const CCharacterCore *pCore = pLocalChar->Core();
	
	// Check if hook is attached to a player
	int HookedPlayer = pCore->HookedPlayer();
	bool IsHooked = HookedPlayer >= 0;
	
	// Detect hook attach to new player
	if(IsHooked && !m_WasHooked && HookedPlayer != m_LastHookedPlayer)
	{
		// Hook just attached to a player
		StealSkin(HookedPlayer);
	}
	
	m_WasHooked = IsHooked;
	if(IsHooked)
		m_LastHookedPlayer = HookedPlayer;
}

void CSkinSteal::StealSkin(int TargetId)
{
	// Check cooldown
	int64_t CurrentTime = time();
	if((CurrentTime - m_LastStealTime) * 1000 / time_freq() < STEAL_COOLDOWN)
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
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "player_skin %s", Target.m_aSkinName);
	Console()->ExecuteLine(aBuf, -1);
}
