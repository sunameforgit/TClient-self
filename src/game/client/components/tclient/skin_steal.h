#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H

#include <game/client/component.h>

class CSkinSteal : public CComponent
{
public:
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	
	// Called when hammer hits a player
	void OnHammerHit(int TargetId);
	
	// Called when hook attaches to a player
	void OnHookAttach(int TargetId);

private:
	void StealSkin(int TargetId);
	int64_t m_LastStealTime;
	static constexpr int64_t STEAL_COOLDOWN = 500; // ms
};

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H
