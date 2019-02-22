#include "stdafx_client.h"
#include "pragma/entities/environment/effects/c_env_explosion.h"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/entities/environment/c_env_quake.h"
#include "pragma/entities/components/c_sound_emitter_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/audio/alsound_type.h>
#include <pragma/entities/components/base_transform_component.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(env_explosion,CEnvExplosion);

extern ClientState *client;
extern CGame *c_game;

void CExplosionComponent::Initialize()
{
	BaseEnvExplosionComponent::Initialize();
	pragma::CParticleSystemComponent::Precache("explosion");
	client->LoadSoundScripts("fx.txt");
}

void CExplosionComponent::Explode()
{
#pragma message("TODO: Underwater effects +sounds")
#pragma message("TODO: Apply damage to all ents within range -> Serverside")
#pragma message("TODO: Leave a scorch mark!")
	auto &ent = GetEntity();
	auto *particle = pragma::CParticleSystemComponent::Create("explosion");
	if(particle != NULL)
	{
		auto pTrComponent = ent.GetTransformComponent();
		auto pTrComponentPt = particle->GetEntity().GetTransformComponent();
		if(pTrComponent.valid() && pTrComponentPt.valid())
			pTrComponentPt->SetPosition(pTrComponent->GetPosition());
		particle->SetRemoveOnComplete(true);
		if(particle != NULL)
			particle->Start();
	}
	auto pSoundEmitterComponent = ent.GetComponent<pragma::CSoundEmitterComponent>();
	if(pSoundEmitterComponent.valid())
		pSoundEmitterComponent->EmitSound("fx.explosion",ALSoundType::Effect,1.f);
	auto radius = 500.f;
	auto *entQuake = c_game->CreateEntity<CEnvQuake>();
	if(entQuake != nullptr)
	{
		auto *pQuakeComponent = static_cast<pragma::CQuakeComponent*>(entQuake->FindComponent("quake").get());
		if(pQuakeComponent != nullptr)
		{
			pQuakeComponent->SetFrequency(50.f);
			pQuakeComponent->SetAmplitude(50.f);
			pQuakeComponent->SetRadius(radius);
		}
		auto pAttComponent = entQuake->AddComponent<CAttachableComponent>();
		if(pAttComponent.valid())
		{
			AttachmentInfo attInfo {};
			attInfo.flags |= FAttachmentMode::SnapToOrigin | FAttachmentMode::PositionOnly;
			pAttComponent->AttachToEntity(&ent,attInfo);
		}
		entQuake->SetKeyValue("spawnflags",std::to_string(SF_QUAKE_IN_AIR | SF_QUAKE_REMOVE_ON_COMPLETE));
		entQuake->Spawn();
		auto pIoComponent = entQuake->GetComponent<CIOComponent>();
		if(pIoComponent.valid())
			pIoComponent->Input("StartShake");
	}
}
luabind::object CExplosionComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CExplosionComponentHandleWrapper>(l);}

////////////////

void CEnvExplosion::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CExplosionComponent>();
}
