#include "auto_emote.h"

#include <engine/shared/config.h>
#include <game/client/gameclient.h>

void CAutoEmote::OnInit()
{
	m_LastEmoteTime = time();
	m_WasChatOpen = false;
	m_IsNormalState = true; // Start with normal
}

void CAutoEmote::OnRender()
{
	if(!g_Config.m_TcAutoEmoteToggle)
		return;

	// Don't send emotes if chat is open
	if(GameClient()->m_Chat.IsActive())
		return;

	int64_t CurrentTime = time();
	int64_t Interval = time_freq() * g_Config.m_TcAutoEmoteInterval / 1000;

	if(CurrentTime - m_LastEmoteTime < Interval)
		return;

	m_LastEmoteTime = CurrentTime;

	// Toggle between normal and selected emote
	m_IsNormalState = !m_IsNormalState;

	int EmoteType = GetCurrentEmoteType();
	SendEmote(EmoteType);
}

int CAutoEmote::GetCurrentEmoteType()
{
	int SelectedType = g_Config.m_TcAutoEmoteType;
	
	// Random mode (5 = Random)
	if(SelectedType == 5)
	{
		if(m_IsNormalState)
		{
			return EMOTE_NORMAL;
		}
		else
		{
			// Random emote from all available emotes (excluding normal)
			static const int s_aRandomEmotes[] = {EMOTE_HAPPY, EMOTE_PAIN, EMOTE_SURPRISE, EMOTE_ANGRY, EMOTE_BLINK};
			return s_aRandomEmotes[(time_get() / time_freq()) % 5];
		}
	}
	
	// Normal mode: toggle between NORMAL and selected emote
	if(m_IsNormalState)
	{
		return EMOTE_NORMAL;
	}
	else
	{
		// Map config value to emote type
		switch(SelectedType)
		{
			case 0: return EMOTE_HAPPY;
			case 1: return EMOTE_PAIN;
			case 2: return EMOTE_SURPRISE;
			case 3: return EMOTE_ANGRY;
			case 4: return EMOTE_BLINK;
			default: return EMOTE_HAPPY;
		}
	}
}

void CAutoEmote::SendEmote(int EmoteType)
{
	const char *pCmd = GetEmoteCommand(EmoteType);
	if(pCmd)
	{
		// Use chat command to send emote (like in emoticon.cpp)
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "/emote %s %d", pCmd, g_Config.m_ClEyeDuration);
		GameClient()->m_Chat.SendChatQueued(aBuf);
	}
}

const char* CAutoEmote::GetEmoteCommand(int EmoteType)
{
	switch(EmoteType)
	{
		case EMOTE_NORMAL: return "normal";
		case EMOTE_HAPPY: return "happy";
		case EMOTE_PAIN: return "pain";
		case EMOTE_SURPRISE: return "surprise";
		case EMOTE_ANGRY: return "angry";
		case EMOTE_BLINK: return "blink";
		default: return "normal";
	}
}
