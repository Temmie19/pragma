#include "stdafx_client.h"
#include "pragma/entities/environment/effects/c_env_fire.h"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/entities/components/basetoggle.h>
#include <pragma/entities/components/base_transform_component.hpp>
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(env_fire,CEnvFire);

CFireComponent::~CFireComponent()
{
	DestroyParticle();
}
void CFireComponent::Initialize()
{
	BaseEnvFireComponent::Initialize();
	pragma::CParticleSystemComponent::Precache("fire.wpt");
}
void CFireComponent::ReceiveData(NetPacket &packet)
{
	m_fireType = packet->ReadString();
}
util::EventReply CFireComponent::HandleEvent(ComponentEventId eventId,ComponentEvent &evData)
{
	if(BaseEnvFireComponent::HandleEvent(eventId,evData) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(eventId == BaseToggleComponent::EVENT_ON_TURN_ON)
		InitializeParticle();
	else if(eventId == BaseToggleComponent::EVENT_ON_TURN_OFF)
		DestroyParticle();
	return util::EventReply::Unhandled;
}
void CFireComponent::InitializeParticle()
{
	auto &ent = GetEntity();
	auto *pToggleComponent = static_cast<pragma::BaseToggleComponent*>(ent.FindComponent("toggle").get());
	if((pToggleComponent != nullptr && pToggleComponent->IsTurnedOn() == false) || m_hParticle.valid() == true)
		return;
	auto *pt = pragma::CParticleSystemComponent::Create(m_fireType);
	if(pt == nullptr)
		return;
	pt->SetContinuous(true);
	pt->Start();
	m_hParticle = pt->GetHandle<pragma::CParticleSystemComponent>();
}

void CFireComponent::DestroyParticle()
{
	if(m_hParticle.valid())
	{
		m_hParticle->Die();
		m_hParticle->SetRemoveOnComplete(true);
	}
}
luabind::object CFireComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CFireComponentHandleWrapper>(l);}

//////////////

void CEnvFire::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CFireComponent>();
}
