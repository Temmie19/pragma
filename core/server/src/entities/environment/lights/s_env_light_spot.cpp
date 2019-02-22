#include "stdafx_server.h"
#include "pragma/entities/environment/lights/s_env_light_spot.h"
#include "pragma/entities/s_entityfactories.h"
#include "pragma/entities/baseentity_luaobject.h"
#include <pragma/networking/nwm_util.h>
#include "pragma/lua/s_lentity_handles.hpp"

using namespace pragma;

LINK_ENTITY_TO_CLASS(env_light_spot,EnvLightSpot);

void SLightSpotComponent::SendData(NetPacket &packet,nwm::RecipientFilter &rp)
{
	packet->Write<float>(*m_angOuterCutoff);
	packet->Write<float>(*m_angInnerCutoff);
}

void SLightSpotComponent::SetOuterCutoffAngle(float ang)
{
	BaseEnvLightSpotComponent::SetOuterCutoffAngle(ang);
	auto &ent = GetEntity();
	if(!ent.IsSpawned())
		return;
	NetPacket p;
	nwm::write_entity(p,&ent);
	p->Write<float>(ang);
	server->BroadcastTCP("env_light_spot_outercutoff_angle",p);
}

void SLightSpotComponent::SetInnerCutoffAngle(float ang)
{
	BaseEnvLightSpotComponent::SetInnerCutoffAngle(ang);
	auto &ent = GetEntity();
	if(!ent.IsSpawned())
		return;
	NetPacket p;
	nwm::write_entity(p,&ent);
	p->Write<float>(ang);
	server->BroadcastTCP("env_light_spot_innercutoff_angle",p);
}

luabind::object SLightSpotComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<SLightSpotComponentHandleWrapper>(l);}

///////////

void EnvLightSpot::Initialize()
{
	SBaseEntity::Initialize();
	AddComponent<SLightComponent>();
	AddComponent<SLightSpotComponent>();
}
