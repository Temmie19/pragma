#ifndef __C_ENV_SOUND_DSP_EQUALIZER_H__
#define __C_ENV_SOUND_DSP_EQUALIZER_H__
#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/entities/environment/audio/c_env_sound_dsp.h"
#include "pragma/entities/environment/audio/env_sound_dsp_equalizer.h"

namespace pragma
{
	class DLLCLIENT CSoundDspEqualizerComponent final
		: public CBaseSoundDspComponent,
		public BaseEnvSoundDspEqualizer
	{
	public:
		CSoundDspEqualizerComponent(BaseEntity &ent) : CBaseSoundDspComponent(ent) {}
		virtual void ReceiveData(NetPacket &packet) override;
		virtual void OnEntitySpawn() override;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
	};
};

class DLLCLIENT CEnvSoundDspEqualizer
	: public CBaseEntity
{
public:
	virtual void Initialize() override;
};

#endif