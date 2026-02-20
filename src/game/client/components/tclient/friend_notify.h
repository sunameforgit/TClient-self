#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_FRIEND_NOTIFY_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_FRIEND_NOTIFY_H

#include <game/client/component.h>
#include <unordered_set>
#include <string>

class CFriendNotify : public CComponent
{
public:
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	virtual void OnRender() override;

private:
	std::unordered_set<std::string> m_LastOnlineFriends;
	int64_t m_LastCheckTime;
	static constexpr int64_t CHECK_INTERVAL = 10; // seconds
	
	void CheckFriendsOnline();
	void NotifyFriendOnline(const char *pName);
};

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_FRIEND_NOTIFY_H
