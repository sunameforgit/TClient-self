#include "skin_steal.h"

#include <engine/shared/config.h>
#include <game/client/gameclient.h>

void CSkinSteal::OnInit()
{
	m_LastStealTime = 0;
}

void CSkinSteal::OnHammerHit(int TargetId)
{
	if(!g_Config.m_TcHammerStealSkin)
		return;
	
	StealSkin(TargetId);
}

void CSkinSteal::OnHookAttach(int TargetId)
{
	if(!g_Config.m_TcHookStealSkin)
		return;
	
	StealSkin(TargetId);
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
