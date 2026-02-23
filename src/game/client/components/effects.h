/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_EFFECTS_H
#define GAME_CLIENT_COMPONENTS_EFFECTS_H

#include <base/vmath.h>

#include <game/client/component.h>

#include <deque>

class CEffects : public CComponent
{
private:
	bool m_Add5hz;
	int64_t m_LastUpdate5hz = 0;

	bool m_Add50hz;
	int64_t m_LastUpdate50hz = 0;

	bool m_Add100hz;
	int64_t m_LastUpdate100hz = 0;

	int64_t m_SkidSoundTimer = 0;

	// Deduplication system for effects
	struct CEffectRecord
	{
		int m_Type;
		vec2 m_Pos;
		int64_t m_Time;
	};
	std::deque<CEffectRecord> m_RecentEffects;
	static constexpr int64_t EFFECT_DEDUP_TIME = 100 * 1000; // 100ms in microseconds
	static constexpr size_t MAX_RECORDS = 32;

	bool IsEffectRecentlyPlayed(int Type, vec2 Pos);
	void RecordEffect(int Type, vec2 Pos);

public:
	CEffects();
	int Sizeof() const override { return sizeof(*this); }

	void OnRender() override;

	void BulletTrail(vec2 Pos, float Alpha, float TimePassed);
	void SmokeTrail(vec2 Pos, vec2 Vel, float Alpha, float TimePassed);
	void SkidTrail(vec2 Pos, vec2 Vel, int Direction, float Alpha, float Volume);
	void Explosion(vec2 Pos, float Alpha);
	void HammerHit(vec2 Pos, float Alpha, float Volume);
	void AirJump(vec2 Pos, float Alpha, float Volume);
	void DamageIndicator(vec2 Pos, vec2 Dir, float Alpha);
	void PlayerSpawn(vec2 Pos, float Alpha, float Volume);
	void PlayerDeath(vec2 Pos, int ClientId, float Alpha);
	void PowerupShine(vec2 Pos, vec2 Size, float Alpha);
	void FreezingFlakes(vec2 Pos, vec2 Size, float Alpha);
	void SparkleTrail(vec2 Pos, float Alpha);
	void Confetti(vec2 Pos, float Alpha);

	void Update();
};
#endif
