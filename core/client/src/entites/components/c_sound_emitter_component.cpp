#include "stdafx_client.h"
#include "pragma/entities/components/c_sound_emitter_component.hpp"
#include "pragma/entities/components/c_flex_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/audio/alsound_type.h>
#include <pragma/audio/alsoundscript.h>
#include <pragma/entities/components/base_transform_component.hpp>

using namespace pragma;

extern DLLCLIENT ClientState *client;

luabind::object CSoundEmitterComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CSoundEmitterComponentHandleWrapper>(l);}
bool CSoundEmitterComponent::ShouldRemoveSound(ALSound &snd) const {return (BaseSoundEmitterComponent::ShouldRemoveSound(snd) && snd.GetIndex() == 0) ? true : false;}

void CSoundEmitterComponent::AddSound(std::shared_ptr<ALSound> snd) {InitializeSound(snd);}

void CSoundEmitterComponent::PrecacheSounds()
{
	BaseSoundEmitterComponent::PrecacheSounds();
	client->PrecacheSound("fx.fire_small",ALChannel::Mono);
	CParticleSystemComponent::Precache("fire");
}

void CSoundEmitterComponent::ReceiveData(NetPacket &packet)
{
	auto numSounds = packet->Read<uint8_t>();
	for(auto i=decltype(numSounds){0};i<numSounds;++i)
	{
		auto sndIdx = packet->Read<uint32_t>();
		auto snd = client->GetSoundByIndex(sndIdx);
		if(snd == nullptr)
			continue;
		AddSound(snd);
	}
}

std::shared_ptr<ALSound> CSoundEmitterComponent::CreateSound(std::string sndname,ALSoundType type)
{
	std::shared_ptr<ALSound> snd = client->CreateSound(sndname,type,ALCreateFlags::Mono);
	if(snd == NULL)
		return snd;
	InitializeSound(snd);
	return snd;
}

std::shared_ptr<ALSound> CSoundEmitterComponent::EmitSound(std::string sndname,ALSoundType type,float gain,float pitch)
{
	std::shared_ptr<ALSound> snd = CreateSound(sndname,type);
	if(snd == NULL)
		return snd;
	auto pTrComponent = GetEntity().GetTransformComponent();
	ALSound *al = snd.get();
	al->SetGain(gain);
	al->SetPitch(pitch);
	al->SetPosition(pTrComponent.valid() ? pTrComponent->GetPosition() : Vector3{});
	//al->SetVelocity(*GetVelocity());
	// TODO: Orientation
	al->Play();
	return snd;
}
void CSoundEmitterComponent::MaintainSounds()
{
	BaseSoundEmitterComponent::MaintainSounds();
	auto pFlexComponent = GetEntity().GetComponent<pragma::CFlexComponent>();
	for(auto &snd : m_sounds)
	{
		if(snd->IsPlaying() == false || (snd->GetType() &ALSoundType::Voice) == ALSoundType::Generic)
			continue;
		if(snd->IsSoundScript() == false)
		{
			if(pFlexComponent.valid())
				pFlexComponent->UpdateSoundPhonemes(static_cast<CALSound&>(*snd));
			continue;
		}
		auto *sndScript = dynamic_cast<ALSoundScript*>(snd.get());
		if(sndScript == nullptr)
			continue;
		auto numSounds = sndScript->GetSoundCount();
		for(auto i=decltype(numSounds){0};i<numSounds;++i)
		{
			auto *snd = sndScript->GetSound(i);
			if(snd == nullptr)
				continue;
			if(pFlexComponent.valid())
				pFlexComponent->UpdateSoundPhonemes(static_cast<CALSound&>(*snd));
		}
	}
}
