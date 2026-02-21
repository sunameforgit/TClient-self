#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_AUTO_EMOTE_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_AUTO_EMOTE_H

#include <game/client/component.h>

class CAutoEmote : public CComponent
{
public:
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	virtual void OnRender() override;

private:
	int64_t m_LastEmoteTime;
	bool m_WasChatOpen;
	bool m_IsNormalState; // true = normal, false = selected emote
	
	void SendEmote(int EmoteType);
	const char* GetEmoteCommand(int EmoteType);
	int GetCurrentEmoteType();
};

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_AUTO_EMOTE_H
