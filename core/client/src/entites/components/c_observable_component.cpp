#include "stdafx_client.h"
#include "pragma/entities/components/c_observable_component.hpp"
#include "pragma/entities/components/c_player_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"

using namespace pragma;

extern DLLCLIENT CGame *c_game;
luabind::object CObservableComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CObservableComponentHandleWrapper>(l);}
void CObservableComponent::ReceiveData(NetPacket &packet)
{
	constexpr auto numTypes = umath::to_integral(CameraType::Count);
	for(auto i=0u;i<numTypes;++i)
	{
		auto &data = GetCameraData(static_cast<CameraType>(i));
		*data.enabled = packet->Read<bool>();
		*data.localOrigin = packet->Read<Vector3>();
		*data.offset = packet->Read<Vector3>();
		data.rotateWithObservee = packet->Read<bool>();
		auto hasLimits = packet->Read<bool>();
		if(hasLimits)
		{
			auto minLimits = packet->Read<EulerAngles>();
			auto maxLimits = packet->Read<EulerAngles>();
			data.angleLimits = {minLimits,maxLimits};
		}
	}
}

Bool CObservableComponent::ReceiveNetEvent(pragma::NetEventId eventId,NetPacket &packet)
{
	if(eventId == m_netSetObserverOrigin)
	{
		auto camType = packet->Read<CameraType>();
		auto origin = packet->Read<Vector3>();
		SetLocalCameraOrigin(camType,origin);
	}
	else if(eventId == m_netSetObserverOffset)
	{
		auto camType = packet->Read<CameraType>();
		auto offset = packet->Read<Vector3>();
		SetLocalCameraOffset(camType,offset);
	}
	else
		return CBaseNetComponent::ReceiveNetEvent(eventId,packet);
	return true;
}

void CObservableComponent::SetLocalCameraOrigin(CameraType type,const Vector3 &offset)
{
	BaseObservableComponent::SetLocalCameraOrigin(type,offset);
	auto *pl = c_game->GetLocalPlayer();
	if(pl == nullptr)
		return;
	auto *pObsTgt = pl->GetObserverTarget();
	if(pObsTgt == nullptr || &pObsTgt->GetEntity() != &GetEntity())
		return;
	pl->UpdateObserverOffset();
}
