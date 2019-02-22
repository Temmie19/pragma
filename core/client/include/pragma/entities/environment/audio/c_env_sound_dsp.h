#ifndef __C_ENV_SOUND_DSP_H__
#define __C_ENV_SOUND_DSP_H__

#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/entities/components/c_entity_component.hpp"
#include <pragma/entities/environment/audio/env_sound_dsp.h>
#include <alsound_source.hpp>

namespace pragma
{
	class DLLCLIENT CBaseSoundDspComponent
		: public BaseEnvSoundDspComponent,
		public CBaseNetComponent
	{
	public:
		virtual ~CBaseSoundDspComponent() override;
		virtual void Initialize() override;
		virtual void ReceiveData(NetPacket &packet) override;
		virtual util::EventReply HandleEvent(ComponentEventId eventId,ComponentEvent &evData) override;
		virtual Bool ReceiveNetEvent(UInt32 eventId,NetPacket &p) override;
		virtual bool ShouldTransmitNetData() const override {return true;}
		virtual void OnEntitySpawn() override;
	protected:
		CBaseSoundDspComponent(BaseEntity &ent) : BaseEnvSoundDspComponent(ent) {}

		bool m_bAffectRelative = false;
		bool m_bApplyGlobal = false;
		bool m_bAllWorldSounds = false;
		bool m_bAllSounds = false;
		std::shared_ptr<al::Effect> m_dsp = nullptr;
		std::vector<std::pair<al::SoundSourceHandle,uint32_t>> m_affectedSounds;
		ALSoundType m_types = ALSoundType::All &~ALSoundType::GUI;

		ALSoundType GetTargetSoundTypes() const;
		std::vector<std::pair<al::SoundSourceHandle,uint32_t>>::iterator FindSoundSource(al::SoundSource &src);
		void UpdateSoundSource(al::SoundSource &src,float gain);
		void DetachSoundSource(al::SoundSource &src);
		void DetachAllSoundSources();
	};

	class DLLCLIENT CSoundDspComponent final
		: public CBaseSoundDspComponent
	{
	public:
		CSoundDspComponent(BaseEntity &ent) : CBaseSoundDspComponent(ent) {}
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
	};
};

class DLLCLIENT CEnvSoundDsp
	: public CBaseEntity
{
public:
	virtual void Initialize() override;
};

/*
#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/entities/environment/audio/env_sound_dsp.h"
#include <alsound_source.hpp>
#include <memory>

class DLLCLIENT CEnvSoundDsp
	: public CBaseEntity,
	public BaseEnvSoundDsp
{
protected:
	static std::vector<CEnvSoundDsp*> s_soundDsps;
	std::shared_ptr<al::Effect> m_dsp = nullptr;
	bool m_bGloballyActive = false;
	uint8_t m_bAffectGlobal;
	std::vector<al::SoundSourceHandle> m_affected;
	void AttachSound(std::shared_ptr<ALSound> &ptrSnd);
	void DetachSound(int idx);
	bool IsActive() const;
	void ThinkRelativeToPlayer();
	void ThinkRelativeToSound();
public:
	CEnvSoundDsp();
	virtual ~CEnvSoundDsp() override;
	virtual void ReceiveData(NetPacket &packet) override;
	virtual void Spawn() override;
	virtual void Think(double tDelta) override;
	void SetDSPGlobal(bool b);
	void SetGloballyActive(bool b);
};
*/
#endif
