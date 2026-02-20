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
