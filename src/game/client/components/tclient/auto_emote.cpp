#include "auto_emote.h"

#include <engine/shared/config.h>
#include <game/client/gameclient.h>

void CAutoEmote::OnInit()
{
	m_LastEmoteTime = 0;
	m_WasChatOpen = false;
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

	int EmoteType = g_Config.m_TcAutoEmoteType;
	
	// Random mode
	if(EmoteType == 5)
	{
		static const int s_aAllEmotes[] = {EMOTE_NORMAL, EMOTE_HAPPY, EMOTE_PAIN, EMOTE_SURPRISE, EMOTE_ANGRY, EMOTE_BLINK};
		EmoteType = s_aAllEmotes[(time_get() / time_freq()) % 6];
	}

	SendEmote(EmoteType);
}

void CAutoEmote::SendEmote(int EmoteType)
{
	const char *pCmd = GetEmoteCommand(EmoteType);
	if(pCmd)
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "/emote %s 9999", pCmd);
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
