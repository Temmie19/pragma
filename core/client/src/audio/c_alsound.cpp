/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/clientstate/clientstate.h"
#include "pragma/audio/c_alsound.h"
#include "pragma/c_engine.h"
#include <pragma/networking/nwm_util.h>
#include "pragma/console/c_cvar.h"
#include "pragma/networking/c_nwm_util.h"
#include "luasystem.h"
#include <pragma/audio/alsound_type.h>
#include <pragma/entities/components/base_transform_component.hpp>

extern DLLCENGINE CEngine *c_engine;
extern DLLCLIENT ClientState *client;
DLLCLIENT void NET_cl_snd_precache(NetPacket packet)
{
	std::string snd = packet->ReadString();
	auto mode = packet->Read<uint8_t>();
	client->PrecacheSound(snd,static_cast<ALChannel>(mode));
}

DLLCLIENT void NET_cl_snd_create(NetPacket packet)
{
	std::string snd = packet->ReadString();
	auto type = packet->Read<ALSoundType>();
	unsigned int idx = packet->Read<unsigned int>();
	auto createFlags = packet->Read<ALCreateFlags>();
	auto as = client->CreateSound(snd,ALSoundType::Generic,createFlags);
	if(as == nullptr)
		return;
	client->IndexSound(as,idx);

	auto fullUpdate = packet->Read<bool>();
	if(fullUpdate == false)
		return;
	auto state = packet->Read<ALState>();

	as->SetOffset(packet->Read<float>());
	as->SetPitch(packet->Read<float>());
	as->SetLooping(packet->Read<bool>());
	as->SetGain(packet->Read<float>());
	as->SetPosition(packet->Read<Vector3>());
	as->SetVelocity(packet->Read<Vector3>());
	as->SetDirection(packet->Read<Vector3>());
	as->SetRelative(packet->Read<bool>());
	as->SetReferenceDistance(packet->Read<float>());
	as->SetRolloffFactor(packet->Read<float>());
	as->SetRoomRolloffFactor(packet->Read<float>());
	as->SetMaxDistance(packet->Read<float>());
	as->SetMinGain(packet->Read<float>());
	as->SetMaxGain(packet->Read<float>());
	as->SetInnerConeAngle(packet->Read<float>());
	as->SetOuterConeAngle(packet->Read<float>());
	as->SetOuterConeGain(packet->Read<float>());
	as->SetOuterConeGainHF(packet->Read<float>());
	as->SetFlags(packet->Read<uint32_t>());

	auto start = packet->Read<float>();
	auto end = packet->Read<float>();
	as->SetRange(start,end);

	as->SetFadeInDuration(packet->Read<float>());
	as->SetFadeOutDuration(packet->Read<float>());
	as->SetPriority(packet->Read<uint32_t>());

	auto at = packet->Read<Vector3>();
	auto up = packet->Read<Vector3>();
	as->SetOrientation(at,up);

	as->SetDopplerFactor(packet->Read<float>());
	as->SetLeftStereoAngle(packet->Read<float>());
	as->SetRightStereoAngle(packet->Read<float>());

	as->SetAirAbsorptionFactor(packet->Read<float>());

	auto directHF = packet->Read<bool>();
	auto send = packet->Read<bool>();
	auto sendHF = packet->Read<bool>();
	as->SetGainAuto(directHF,send,sendHF);

	auto gain = packet->Read<float>();
	auto gainHF = packet->Read<float>();
	auto gainLF = packet->Read<float>();
	as->SetDirectFilter({gain,gainHF,gainLF});

	std::weak_ptr<ALSound> wpSnd = as;
	nwm::read_unique_entity(packet,[wpSnd](BaseEntity *ent) {
		if(ent == nullptr || wpSnd.expired())
			return;
		wpSnd.lock()->SetSource(ent);
	});

	switch(state)
	{
	case ALState::Paused:
		as->Pause();
		break;
	case ALState::Playing:
		as->Play();
		break;
	case ALState::Stopped:
		as->Stop();
		break;
	case ALState::Initial:
		break;
	}
}

DLLCLIENT void NET_cl_snd_ev(NetPacket packet)
{
	unsigned char ev = packet->Read<unsigned char>();
	unsigned int idx = packet->Read<unsigned int>();
	std::shared_ptr<ALSound> as = client->GetSoundByIndex(idx);
	if(as == NULL)
		return;
	CALSound *cas = static_cast<CALSound*>(as.get());
	switch(static_cast<ALSound::NetEvent>(ev))
	{
	case ALSound::NetEvent::Play:
		cas->Play();
		break;
	case ALSound::NetEvent::Stop:
		cas->Stop();
		break;
	case ALSound::NetEvent::Pause:
		cas->Pause();
		break;
	case ALSound::NetEvent::Rewind:
		cas->Rewind();
		break;
	case ALSound::NetEvent::SetOffset:
		{
			float offset = packet->Read<float>();
			cas->SetOffset(offset);
			break;
		}
	case ALSound::NetEvent::SetPitch:
		{
			float pitch = packet->Read<float>();
			cas->SetPitch(pitch);
			break;
		}
	case ALSound::NetEvent::SetLooping:
		{
			bool loop = packet->Read<bool>();
			cas->SetLooping(loop);
			break;
		}
	case ALSound::NetEvent::SetGain:
		{
			float gain = packet->Read<float>();
			cas->SetGain(gain);
			break;
		}
	case ALSound::NetEvent::SetPos:
		{
			Vector3 pos = nwm::read_vector(packet);
			cas->SetPosition(pos);
			break;
		}
	case ALSound::NetEvent::SetVelocity:
		{
			Vector3 vel = nwm::read_vector(packet);
			cas->SetVelocity(vel);
			break;
		}
	case ALSound::NetEvent::SetDirection:
		{
			Vector3 dir = nwm::read_vector(packet);
			cas->SetDirection(dir);
			break;
		}
	case ALSound::NetEvent::SetRelative:
		{
			bool relative = packet->Read<bool>();
			cas->SetRelative(relative);
			break;
		}
	case ALSound::NetEvent::SetReferenceDistance:
		{
			float distRef = packet->Read<float>();
			cas->SetReferenceDistance(distRef);
			break;
		}
	case ALSound::NetEvent::SetRolloffFactor:
		{
			float rolloff = packet->Read<float>();
			cas->SetRolloffFactor(rolloff);
			break;
		}
	case ALSound::NetEvent::SetRoomRolloffFactor:
		{
			auto roomRolloff = packet->Read<float>();
			cas->SetRoomRolloffFactor(roomRolloff);
			break;
		}
	case ALSound::NetEvent::SetMaxDistance:
		{
			float dist = packet->Read<float>();
			cas->SetMaxDistance(dist);
			break;
		}
	case ALSound::NetEvent::SetMinGain:
		{
			float gain = packet->Read<float>();
			cas->SetMinGain(gain);
			break;
		}
	case ALSound::NetEvent::SetMaxGain:
		{
			float gain = packet->Read<float>();
			cas->SetMaxGain(gain);
			break;
		}
	case ALSound::NetEvent::SetConeInnerAngle:
		{
			float coneInnerAngle = packet->Read<float>();
			cas->SetInnerConeAngle(coneInnerAngle);
			break;
		}
	case ALSound::NetEvent::SetConeOuterAngle:
		{
			float coneOuterAngle = packet->Read<float>();
			cas->SetOuterConeAngle(coneOuterAngle);
			break;
		}
	case ALSound::NetEvent::SetConeOuterGain:
		{
			float coneOuterGain = packet->Read<float>();
			cas->SetOuterConeGain(coneOuterGain);
			break;
		}
	case ALSound::NetEvent::SetConeOuterGainHF:
		{
			float coneOuterGainHF = packet->Read<float>();
			cas->SetOuterConeGainHF(coneOuterGainHF);
			break;
		}
	case ALSound::NetEvent::SetFlags:
		{
			unsigned int flags = packet->Read<unsigned int>();
			cas->SetFlags(flags);
			break;
		}
	case ALSound::NetEvent::SetType:
		{
			auto type = packet->Read<ALSoundType>();
			cas->SetType(type);
			break;
		}
	case ALSound::NetEvent::SetSource:
		{
			auto *ent = nwm::read_entity(packet);
			cas->SetSource(ent);
			break;
		}
	case ALSound::NetEvent::SetRange:
		{
			auto start = packet->Read<float>();
			auto end = packet->Read<float>();
			cas->SetRange(start,end);
			break;
		}
	case ALSound::NetEvent::ClearRange:
		{
			cas->ClearRange();
			break;
		}
	case ALSound::NetEvent::SetFadeInDuration:
		{
			auto t = packet->Read<float>();
			cas->SetFadeInDuration(t);
			break;
		}
	case ALSound::NetEvent::SetFadeOutDuration:
		{
			auto t = packet->Read<float>();
			cas->SetFadeOutDuration(t);
			break;
		}
	case ALSound::NetEvent::FadeIn:
		{
			auto t = packet->Read<float>();
			cas->FadeIn(t);
			break;
		}
	case ALSound::NetEvent::FadeOut:
		{
			auto t = packet->Read<float>();
			cas->FadeOut(t);
			break;
		}
	case ALSound::NetEvent::SetIndex:
		{
			auto idx = packet->Read<uint32_t>();
			CALSound::SetIndex(cas,idx);
			break;
		}
	case ALSound::NetEvent::SetPriority:
		{
			auto priority = packet->Read<uint32_t>();
			cas->SetPriority(priority);
			break;
		}
	case ALSound::NetEvent::SetOrientation:
		{
			auto at = packet->Read<Vector3>();
			auto up = packet->Read<Vector3>();
			cas->SetOrientation(at,up);
			break;
		}
	case ALSound::NetEvent::SetDopplerFactor:
		{
			auto factor = packet->Read<float>();
			cas->SetDopplerFactor(factor);
			break;
		}
	case ALSound::NetEvent::SetLeftStereoAngle:
		{
			auto ang = packet->Read<float>();
			cas->SetLeftStereoAngle(ang);
			break;
		}
	case ALSound::NetEvent::SetRightStereoAngle:
		{
			auto ang = packet->Read<float>();
			cas->SetRightStereoAngle(ang);
			break;
		}
	case ALSound::NetEvent::SetAirAbsorptionFactor:
		{
			auto factor = packet->Read<float>();
			cas->SetAirAbsorptionFactor(factor);
			break;
		}
	case ALSound::NetEvent::SetGainAuto:
		{
			auto directHF = packet->Read<float>();
			auto send = packet->Read<float>();
			auto sendHF = packet->Read<float>();
			cas->SetGainAuto(directHF,send,sendHF);
			break;
		}
	case ALSound::NetEvent::SetDirectFilter:
		{
			auto gain = packet->Read<float>();
			auto gainHF = packet->Read<float>();
			auto gainLF = packet->Read<float>();
			cas->SetDirectFilter({gain,gainHF,gainLF});
			break;
		}
	case ALSound::NetEvent::AddEffect:
		{
			auto effectName = packet->ReadString();
			auto gain = packet->Read<float>();
			auto gainHF = packet->Read<float>();
			auto gainLF = packet->Read<float>();
			cas->AddEffect(effectName,{gain,gainHF,gainLF});
			break;
		}
	case ALSound::NetEvent::RemoveEffect:
		{
			auto effectName = packet->ReadString();
			cas->RemoveEffect(effectName);
			break;
		}
	case ALSound::NetEvent::SetEffectParameters:
		{
			auto effectName = packet->ReadString();
			auto gain = packet->Read<float>();
			auto gainHF = packet->Read<float>();
			auto gainLF = packet->Read<float>();
			cas->SetEffectParameters(effectName,{gain,gainHF,gainLF});
			break;
		}
	case ALSound::NetEvent::SetEntityMapIndex:
		{
			auto idx = packet->Read<uint32_t>();
			//cas->SetIdentifier("world_sound" +std::to_string(idx)); // Has to correspond to identifier in c_game_audio.cpp
			break;
		}
	default:
		{
			Con::cwar<<"WARNING: Unhandled sound net event "<<ev<<"!"<<Con::endl;
			break;
		}
	}
}

decltype(CALSound::s_svIndexedSounds) CALSound::s_svIndexedSounds = {};
ALSound *CALSound::FindByServerIndex(uint32_t idx)
{
	auto it = s_svIndexedSounds.find(idx);
	if(it == s_svIndexedSounds.end())
		return nullptr;
	return it->second.lock().get();
}

CALSound::CALSound(NetworkState *nw,al::SoundSystem &system,al::SoundBuffer &buffer,al::InternalSource *source)
	: ALSound(nw),al::SoundSource(system,buffer,source)
{
	UpdateVolume();
	RegisterCallback<void,std::reference_wrapper<float>>("UpdateGain");
}
CALSound::CALSound(NetworkState *nw,al::SoundSystem &system,al::Decoder &decoder,al::InternalSource *source)
	: ALSound(nw),al::SoundSource(system,decoder,source)
{
	UpdateVolume();
	RegisterCallback<void,std::reference_wrapper<float>>("UpdateGain");
}

CALSound::~CALSound()
{
	auto it = s_svIndexedSounds.find(GetIndex());
	if(it != s_svIndexedSounds.end())
		s_svIndexedSounds.erase(it);
	for(auto it=s_svIndexedSounds.begin();it!=s_svIndexedSounds.end();)
	{
		if(it->second.expired() == true)
			it = s_svIndexedSounds.erase(it);
		else
			++it;
	}
}

void CALSound::Terminate()
{
	if(m_bTerminated == true)
		return;
	Stop();
	m_bTerminated = true;
}

void CALSound::SetPitchModifier(float mod)
{
	m_modPitch = mod;
	UpdatePitch();
}
float CALSound::GetPitchModifier() const {return m_modPitch;}
void CALSound::SetVolumeModifier(float mod)
{
	m_modVol = mod;
	UpdateVolume();
}
float CALSound::GetVolumeModifier() const {return m_modVol;}

static auto cvAlwaysPlay = GetClientConVar("cl_audio_always_play");
void CALSound::UpdateVolume()
{
	// TODO: CALSound::GetEffectiveGain
	if(m_bTerminated == true)
		return;
	auto gain = m_gain;
	if(c_engine->IsWindowFocused() == false && cvAlwaysPlay->GetBool() == false)
		gain = 0.f;
	else
	{
		gain *= GetVolumeModifier();
		auto &volumes = client->GetSoundVolumes();
		auto minGain = 1.f;
		for(auto it=volumes.begin();it!=volumes.end();++it)
		{
			if((umath::to_integral(m_type) &umath::to_integral(it->first)) != 0)
			{
				if(it->second < minGain)
					minGain = it->second;
			}
		}
		gain *= minGain;
		gain *= client->GetMasterSoundVolume();
	}
	CallCallbacks<void,std::reference_wrapper<float>>("UpdateGain",std::ref<float>(gain));
	al::SoundSource::SetGain(gain);
}

float CALSound::GetMaxAudibleDistance() const {return al::SoundSource::GetMaxAudibleDistance();}

void CALSound::UpdatePitch()
{
	if(m_bTerminated == true)
		return;
	float pitch = GetPitch();
	pitch *= GetPitchModifier();
	al::SoundSource::SetPitch(pitch);
}

unsigned int CALSound::GetIndex() const {return m_index;}

void CALSound::SetIndex(ALSound *snd,uint32_t idx)
{
	auto it = s_svIndexedSounds.find(idx);
	if(it != s_svIndexedSounds.end())
		s_svIndexedSounds.erase(it);

	snd->SetIndex(idx);
	if(idx == 0)
		return;
	s_svIndexedSounds.insert(decltype(s_svIndexedSounds)::value_type(idx,snd->shared_from_this()));
}

void CALSound::Update()
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::Update();
	auto old = GetState();
	UpdateState();
	if(IsStopped() == true)
	{
		CancelFade();
		CheckStateChange(old);
	}
	else if(m_fade != nullptr)
	{
		auto t = client->RealTime() -m_fade->start;
		if(t >= m_fade->duration)
			CancelFade();
		else
		{
			auto gain = (t /m_fade->duration) *m_fade->gain;
			if(m_fade->fadein)
				SetGain(gain);
			else
				SetGain(m_fade->gain -gain);
		}
	}
}

void CALSound::PostUpdate()
{
	if(m_bTerminated == true)
		return;
	ALSound::PostUpdate();
}

ALState CALSound::GetState() const
{
	if(IsPlaying())
		return ALState::Playing;
	if(IsStopped())
		return ALState::Stopped;
	if(IsPaused())
		return ALState::Paused;
	// TODO
	return ALState::Initial;//static_cast<ALState>(m_state);
}

void CALSound::FadeIn(float time)
{
	float gain = GetGain();
	SetGain(0);
	CancelFade();
	if(!IsPlaying())
		Play();
	m_fade = std::unique_ptr<SoundFade>(new SoundFade(true,client->RealTime(),time,gain));
}

void CALSound::FadeOut(float time)
{
	if(!IsPlaying())
		return;
	float gain = GetGain();
	CancelFade();
	m_fade = std::unique_ptr<SoundFade>(new SoundFade(false,client->RealTime(),time,gain));
}

void CALSound::UpdateState()
{
	if(m_bTerminated == true)
		return;
}

void CALSound::Play()
{
	if(m_bTerminated == true)
		return;
	CancelFade();
	auto old = GetState();

	auto bPaused = (GetState() == ALState::Paused) ? true : false;
	if(bPaused == false)
		InitRange();
	if(bPaused == false)
	{
		try
		{
			al::SoundSource::Play();
		}
		catch(const std::runtime_error &err)
		{
			Con::cwar<<"WARNING: Unable to play sound "<<GetIndex()<<": "<<err.what()<<Con::endl;
			return;
		}
	}
	else
		al::SoundSource::Resume();
	UpdateState();
	CheckStateChange(old);
	if(m_tFadeIn > 0.f)
		FadeIn(m_tFadeIn);
}

void CALSound::Stop()
{
	if(m_bTerminated == true)
		return;
	CancelFade();
	auto old = GetState();
	al::SoundSource::Stop();
	UpdateState();
	CheckStateChange(old);
}

void CALSound::Pause()
{
	if(m_bTerminated == true)
		return;
	CancelFade();
	auto old = GetState();
	al::SoundSource::Pause();
	UpdateState();
	CheckStateChange(old);
}

void CALSound::Rewind()
{
	if(m_bTerminated == true)
		return;
	CancelFade();
	auto old = GetState();
	SetOffset(0.f);
	UpdateState();
	CheckStateChange(old);
}

void CALSound::SetOffset(float offset)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetOffset(offset);
}

float CALSound::GetOffset() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetOffset();
}

void CALSound::SetPitch(float pitch) {m_pitch = pitch; UpdatePitch();}

float CALSound::GetPitch() const {return m_pitch;}

void CALSound::SetLooping(bool loop)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetLooping(loop);
}

bool CALSound::IsIdle() const
{
	if(GetIndex() > 0)
		return false; // This is a server-side sound, keep it around until server-side representation is removed
	return al::SoundSource::IsIdle();
}
bool CALSound::IsLooping() const
{
	if(m_bTerminated == true)
		return false;
	return al::SoundSource::IsLooping();
}
bool CALSound::IsPlaying() const
{
	if(m_bTerminated == true)
		return false;
	return al::SoundSource::IsPlaying();
}
bool CALSound::IsPaused() const
{
	if(m_bTerminated == true)
		return false;
	return al::SoundSource::IsPaused();
}
bool CALSound::IsStopped() const
{
	if(m_bTerminated == true)
		return false;
	return al::SoundSource::IsStopped();
}
void CALSound::SetGain(float gain) {m_gain = gain; UpdateVolume();}
float CALSound::GetGain() const
{
	if(m_bTerminated == true)
		return 0.f;
	return glm::clamp(al::SoundSource::GetGain(),GetMinGain(),GetMaxGain());
}
void CALSound::SetPosition(const Vector3 &pos)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetPosition(pos);
}
Vector3 CALSound::GetPosition() const
{
	if(m_bTerminated == true)
		return Vector3(0.f,0.f,0.f);
	if(m_hSourceEntity.IsValid())
	{
		auto pTrComponent = m_hSourceEntity.get()->GetTransformComponent();
		if(pTrComponent.valid())
			return pTrComponent->GetPosition();
	}
	return al::SoundSource::GetPosition();
}
void CALSound::SetVelocity(const Vector3 &vel)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetVelocity(vel);
}
Vector3 CALSound::GetVelocity() const
{
	if(m_bTerminated == true)
		return Vector3(0.f,0.f,0.f);
	return al::SoundSource::GetVelocity();
}
void CALSound::SetDirection(const Vector3 &dir)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetDirection(dir);
}
Vector3 CALSound::GetDirection() const
{
	if(m_bTerminated == true)
		return Vector3(0.f,0.f,0.f);
	return al::SoundSource::GetDirection();
}
void CALSound::SetRelative(bool b)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetRelative(b);
}
bool CALSound::IsRelative() const
{
	if(m_bTerminated == true)
		return false;
	return al::SoundSource::IsRelative();
}
void CALSound::SetTimeOffset(float sec)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetTimeOffset(sec);
}
float CALSound::GetTimeOffset() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetTimeOffset();
}
float CALSound::GetDuration() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetDuration();
}
float CALSound::GetReferenceDistance() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetReferenceDistance();
}
void CALSound::SetReferenceDistance(float dist)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetReferenceDistance(dist);
}
void CALSound::SetRoomRolloffFactor(float roomFactor)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetRoomRolloffFactor(roomFactor);
}
float CALSound::GetRolloffFactor() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetRolloffFactor();
}
float CALSound::GetRoomRolloffFactor() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetRoomRolloffFactor();
}
void CALSound::SetRolloffFactor(float rolloff)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetRolloffFactors(rolloff);
}
float CALSound::GetMaxDistance() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetMaxDistance();
}
void CALSound::SetMaxDistance(float dist)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetMaxDistance(dist);
}
float CALSound::GetMinGain() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetMinGain();
}
void CALSound::SetMinGain(float gain)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetMinGain(gain);
}
float CALSound::GetMaxGain() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetMaxGain();
}
void CALSound::SetMaxGain(float gain)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetMaxGain(gain);
}
float CALSound::GetInnerConeAngle() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetInnerConeAngle();
}
void CALSound::SetInnerConeAngle(float ang)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetInnerConeAngle(ang);
}
float CALSound::GetOuterConeAngle() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetOuterConeAngle();
}
void CALSound::SetOuterConeAngle(float ang)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetOuterConeAngle(ang);
}
float CALSound::GetOuterConeGain() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetOuterConeGain();
}
float CALSound::GetOuterConeGainHF() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetOuterConeGainHF();
}
void CALSound::SetOuterConeGain(float gain)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetOuterConeGain(gain);
}
void CALSound::SetOuterConeGainHF(float gain)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetOuterConeGainHF(gain);
}

uint32_t CALSound::GetPriority()
{
	if(m_bTerminated == true)
		return 0;
	return al::SoundSource::GetPriority();
}
void CALSound::SetPriority(uint32_t priority)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetPriority(priority);
}
void CALSound::SetOrientation(const Vector3 &at,const Vector3 &up)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetOrientation(at,up);
}
std::pair<Vector3,Vector3> CALSound::GetOrientation() const
{
	if(m_bTerminated == true)
		return {};
	return al::SoundSource::GetOrientation();
}
void CALSound::SetDopplerFactor(float factor)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetDopplerFactor(factor);
}
float CALSound::GetDopplerFactor() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetDopplerFactor();
}
void CALSound::SetLeftStereoAngle(float ang)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetLeftStereoAngle(ang);
}
float CALSound::GetLeftStereoAngle() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetLeftStereoAngle();
}
void CALSound::SetRightStereoAngle(float ang)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetRightStereoAngle(ang);
}
float CALSound::GetRightStereoAngle() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetRightStereoAngle();
}
void CALSound::SetAirAbsorptionFactor(float factor)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetAirAbsorptionFactor(factor);
}
float CALSound::GetAirAbsorptionFactor() const
{
	if(m_bTerminated == true)
		return 0.f;
	return al::SoundSource::GetAirAbsorptionFactor();
}
void CALSound::SetGainAuto(bool directHF,bool send,bool sendHF)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetGainAuto(directHF,send,sendHF);
}
std::tuple<bool,bool,bool> CALSound::GetGainAuto() const
{
	if(m_bTerminated == true)
		return {false,false,false};
	return al::SoundSource::GetGainAuto();
}
void CALSound::SetDirectFilter(const EffectParams &params)
{
	if(m_bTerminated == true)
		return;
	al::SoundSource::SetDirectFilter(reinterpret_cast<const al::Effect::Params&>(params));
}
const ALSound::EffectParams &CALSound::GetDirectFilter() const
{
	if(m_bTerminated == true)
	{
		static ALSound::EffectParams params {};
		return params;
	}
	return reinterpret_cast<const ALSound::EffectParams&>(al::SoundSource::GetDirectFilter());
}
bool CALSound::AddEffect(const std::string &effectName,const EffectParams &params)
{
	auto effect = c_engine->GetAuxEffect(effectName);
	if(effect == nullptr)
		return false;
	return al::SoundSource::AddEffect(*effect,reinterpret_cast<const al::Effect::Params&>(params));
}
void CALSound::RemoveEffect(const std::string &effectName)
{
	auto effect = c_engine->GetAuxEffect(effectName);
	if(effect == nullptr)
		return;
	al::SoundSource::RemoveEffect(*effect);
}
void CALSound::SetEffectParameters(const std::string &effectName,const EffectParams &params)
{
	auto effect = c_engine->GetAuxEffect(effectName);
	if(effect == nullptr)
		return;
	al::SoundSource::SetEffectParameters(*effect,reinterpret_cast<const al::Effect::Params&>(params));
}


void CALSound::SetType(ALSoundType type)
{
	if(m_bTerminated == true)
		return;
	ALSound::SetType(type);
	UpdateVolume();
}

REGISTER_CONVAR_CALLBACK_CL(cl_audio_always_play,[](NetworkState*,ConVar*,bool,bool) {
	client->UpdateSoundVolume();
})
