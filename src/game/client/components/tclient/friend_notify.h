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
	virtual void OnStateChange(int OldState, int NewState) override;

private:
	std::unordered_set<std::string> m_LastOnlineFriends;
	int64_t m_LastCheckTime;
	static constexpr int64_t CHECK_INTERVAL = 10; // seconds
	
	// Auto greet friends on join
	bool m_AutoGreetDone = false;
	int64_t m_JoinTime = 0;
	int m_GreetCheckCounter = 0;
	
	void CheckFriendsOnline();
	void NotifyFriendOnline(const char *pName);
	void AutoGreetFriends();
};

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_FRIEND_NOTIFY_H
