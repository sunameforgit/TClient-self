#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H

#include <game/client/component.h>

class CCharacter;

class CSkinSteal : public CComponent
{
public:
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	virtual void OnRender() override;
	
	// Called from chat command
	void StealSkin(int TargetId);

private:
	int64_t m_LastStealTime;
	int m_LastHookedPlayer;
	bool m_WasHooked;
	bool m_WasFiringHammer;
	int m_LastStolenFrom;
	
	// For predicted hammer hit detection
	int m_LastProcessedEventTick;
	int m_LastHammerHitTick;
	bool m_HammerStealTriggeredThisTick;
	
	void StealFromHammerHit();
	void StealFromHammerHitPredicted();
	void CheckPredictedHammerEvents();
};

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_SKIN_STEAL_H
