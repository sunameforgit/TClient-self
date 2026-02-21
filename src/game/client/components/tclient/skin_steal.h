#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H

#include <game/client/component.h>

class CSkinSteal : public CComponent
{
public:
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	virtual void OnRender() override;

private:
	int64_t m_LastStealTime;
	int m_LastHookedPlayer;
	bool m_WasHooked;
	int m_LastAttackTick;
	
	void StealSkin(int TargetId);
	void CheckHammerHit();
	void CheckHookAttach();
};

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H
