/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/entities/environment/audio/c_env_sound_dsp_equalizer.h"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/c_engine.h"
#include "pragma/entities/components/c_player_component.hpp"
#include <pragma/networking/nwm_util.h>
#include "pragma/lua/c_lentity_handles.hpp"
#include <alsoundsystem.hpp>
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

extern DLLCENGINE CEngine *c_engine;

LINK_ENTITY_TO_CLASS(env_sound_dsp_equalizer,CEnvSoundDspEqualizer);

void CSoundDspEqualizerComponent::ReceiveData(NetPacket &packet)
{
	m_lowGain = packet->Read<float>();
	m_lowCutoff = packet->Read<float>();
	m_mid1Gain = packet->Read<float>();
	m_mid1Center = packet->Read<float>();
	m_mid1Width = packet->Read<float>();
	m_mid2Gain = packet->Read<float>();
	m_mid2Center = packet->Read<float>();
	m_mid2Width = packet->Read<float>();
	m_highGain = packet->Read<float>();
	m_highCutoff = packet->Read<float>();
}

void CSoundDspEqualizerComponent::OnEntitySpawn()
{
	//BaseEnvSoundDspEqualizer::OnEntitySpawn(); // Not calling BaseEnvSoundDspEqualizer::OnEntitySpawn() to skip the dsp effect lookup
	auto *soundSys = c_engine->GetSoundSystem();
	if(soundSys == nullptr)
		return;
	al::EfxEqualizer props {};
	props.flLowGain = m_lowGain;
	props.flLowCutoff = m_lowCutoff;
	props.flMid1Gain = m_mid1Gain;
	props.flMid1Center = m_mid1Center;
	props.flMid1Width = m_mid1Width;
	props.flMid2Gain = m_mid2Gain;
	props.flMid2Center = m_mid2Center;
	props.flMid2Width = m_mid2Width;
	props.flHighGain = m_highGain;
	props.flHighCutoff = m_highCutoff;
	m_dsp = soundSys->CreateEffect(props);
}
luabind::object CSoundDspEqualizerComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CSoundDspEqualizerComponentHandleWrapper>(l);}

//////////////////

void CEnvSoundDspEqualizer::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CSoundDspEqualizerComponent>();
}
