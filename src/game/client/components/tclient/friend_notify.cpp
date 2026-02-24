#include "friend_notify.h"

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <game/client/gameclient.h>

void CFriendNotify::OnInit()
{
	m_LastCheckTime = 0;
}

void CFriendNotify::OnRender()
{
	// Auto greet friends after joining (check every 60 frames ~ 1 second at 60fps)
	if(!m_AutoGreetDone && g_Config.m_TcAutoGreetFriends && m_JoinTime > 0)
	{
		if(++m_GreetCheckCounter >= 60)
		{
			m_GreetCheckCounter = 0;
			// Wait 3 seconds after joining to ensure we have player data
			if(time() - m_JoinTime > time_freq() * 3)
			{
				AutoGreetFriends();
				m_AutoGreetDone = true;
			}
		}
	}

	if(!g_Config.m_TcFriendOnlineNotify)
		return;

	int64_t CurrentTime = time();
	if(CurrentTime - m_LastCheckTime < time_freq() * CHECK_INTERVAL)
		return;

	m_LastCheckTime = CurrentTime;
	CheckFriendsOnline();
}

void CFriendNotify::CheckFriendsOnline()
{
	std::unordered_set<std::string> CurrentOnlineFriends;

	// Check all servers in browser
	for(int i = 0; i < ServerBrowser()->NumServers(); i++)
	{
		const CServerInfo *pInfo = ServerBrowser()->Get(i);
		if(!pInfo || pInfo->m_FriendState == IFriends::FRIEND_NO)
			continue;

		for(int j = 0; j < pInfo->m_NumClients; j++)
		{
			const CServerInfo::CClient &Client = pInfo->m_aClients[j];
			if(Client.m_FriendState != IFriends::FRIEND_NO)
			{
				CurrentOnlineFriends.insert(std::string(Client.m_aName));
			}
		}
	}

	// Check for newly online friends
	for(const auto &FriendName : CurrentOnlineFriends)
	{
		if(m_LastOnlineFriends.find(FriendName) == m_LastOnlineFriends.end())
		{
			NotifyFriendOnline(FriendName.c_str());
		}
	}

	m_LastOnlineFriends = CurrentOnlineFriends;
}

void CFriendNotify::NotifyFriendOnline(const char *pName)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "[TClient] Friend '%s' is now online!", pName);
	GameClient()->m_Chat.AddLine(-2, 0, aBuf); // -2 = CLIENT_MSG (green)
}

void CFriendNotify::OnStateChange(int OldState, int NewState)
{
	// Reset auto greet when connecting
	if(NewState == IClient::STATE_ONLINE)
	{
		m_AutoGreetDone = false;
		m_JoinTime = time();
		m_GreetCheckCounter = 0;
	}
}

void CFriendNotify::AutoGreetFriends()
{
	if(!g_Config.m_TcAutoGreetFriends || g_Config.m_TcAutoGreetMessage[0] == '\0')
		return;

	int LocalId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalId < 0)
		return;

	// Find friends on current server
	std::vector<int> vFriendIds;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == LocalId)
			continue;

		const CGameClient::CClientData &Client = GameClient()->m_aClients[i];
		if(!Client.m_Active)
			continue;

		// Check if this player is a friend
		if(Client.m_Friend)
		{
			vFriendIds.push_back(i);
		}
	}

	// Send greet message to each friend via whisper
	for(int FriendId : vFriendIds)
	{
		const char *pFriendName = GameClient()->m_aClients[FriendId].m_aName;
		
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "/w %s %s", pFriendName, g_Config.m_TcAutoGreetMessage);
		
		// Send the message
		GameClient()->m_Chat.SendChatQueued(aBuf);
	}
}
