#include "stdafx_client.h"
#include "pragma/entities/components/c_point_at_target_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"
#include "pragma/networking/c_nwm_util.h"

using namespace pragma;

void CPointAtTargetComponent::ReceiveData(NetPacket &packet)
{
	auto hEnt = GetHandle();
	nwm::read_unique_entity(packet,[this,hEnt](BaseEntity *ent) {
		if(hEnt.expired())
			return;
		SetPointAtTarget(ent);
	});
}
luabind::object CPointAtTargetComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CPointAtTargetComponentHandleWrapper>(l);}

Bool CPointAtTargetComponent::ReceiveNetEvent(pragma::NetEventId eventId,NetPacket &packet)
{
	if(eventId == m_netEvSetPointAtTarget)
	{
		auto *ent = nwm::read_entity(packet);
		SetPointAtTarget(ent);
	}
	else
		return CBaseNetComponent::ReceiveNetEvent(eventId,packet);
	return true;
}
