#include "stdafx_client.h"
#include "pragma/entities/components/c_vehicle_component.hpp"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/entities/baseentity_luaobject.h"
#include "pragma/entities/components/c_player_component.hpp"
#include "pragma/entities/environment/c_env_camera.h"
#include "pragma/entities/c_viewmodel.h"
#include "pragma/entities/c_viewbody.h"
#include "pragma/lua/c_lentity_handles.hpp"
#include "pragma/entities/components/c_render_component.hpp"
#include "pragma/entities/components/c_observable_component.hpp"
#include <pragma/input/inkeys.h>

using namespace pragma;

extern DLLCLIENT CGame *c_game;

std::vector<CVehicleComponent*> CVehicleComponent::s_vehicles;
const std::vector<CVehicleComponent*> &CVehicleComponent::GetAll() {return s_vehicles;}
unsigned int CVehicleComponent::GetVehicleCount() {return CUInt32(s_vehicles.size());}

CVehicleComponent::CVehicleComponent(BaseEntity &ent)
	: BaseVehicleComponent(ent)
{
	s_vehicles.push_back(this);
}

CVehicleComponent::~CVehicleComponent()
{
	if(m_hCbSteeringWheel.IsValid())
		m_hCbSteeringWheel.Remove();
	ClearDriver();
	for(int i=0;i<s_vehicles.size();i++)
	{
		if(s_vehicles[i] == this)
		{
			s_vehicles.erase(s_vehicles.begin() +i);
			break;
		}
	}
}

luabind::object CVehicleComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CVehicleHandle>(l);}

void CVehicleComponent::ReadWheelInfo(NetPacket &packet)
{
	auto conPoint = packet->Read<Vector3>();
	auto wheelDir = packet->Read<Vector3>();
	auto axle = packet->Read<Vector3>();
	auto suspensionRest = packet->Read<Float>();
	auto radius = packet->Read<Float>();
	auto bFrontWheel = packet->Read<Bool>();
	auto translation = packet->Read<Vector3>();
	auto rotation = packet->Read<Quat>();
	UChar wheelId = 0;
	//if(AddWheel(conPoint,axle,bFrontWheel,&wheelId,translation,rotation) == false)
	//	return;
#ifdef ENABLE_DEPRECATED_PHYSICS
	auto *info = GetWheelInfo(wheelId);
	if(info == nullptr)
		return;
	info->m_wheelDirectionCS = btVector3(wheelDir.x,wheelDir.y,wheelDir.z);
	info->m_suspensionRestLength1 = suspensionRest;
	info->m_wheelsRadius = radius;
#endif
}

void CVehicleComponent::ReceiveData(NetPacket &packet)
{
	auto *entSteeringWheel = nwm::read_entity(packet);
	m_steeringWheel = entSteeringWheel ? entSteeringWheel->GetHandle() : EntityHandle{};
	InitializeSteeringWheel();
}
void CVehicleComponent::ClearDriver()
{
	auto *entDriver = GetDriver();
	if(entDriver != nullptr && entDriver->IsPlayer())
	{
		auto plComponent = entDriver->GetPlayerComponent();
		if(plComponent->IsLocalPlayer())
		{
			c_game->EnableRenderMode(RenderMode::View);
			auto *vb = c_game->GetViewBody();
			if(vb != nullptr)
			{
				auto pRenderComponent = static_cast<CBaseEntity&>(vb->GetEntity()).GetRenderComponent();
				if(pRenderComponent.valid())
					pRenderComponent->SetRenderMode(RenderMode::View);
			}

			plComponent->SetObserverTarget(nullptr);
			plComponent->SetObserverMode(OBSERVERMODE::FIRSTPERSON);
		}
	}
	BaseVehicleComponent::ClearDriver();
}
void CVehicleComponent::SetDriver(BaseEntity *ent)
{
	if(ent == GetDriver())
		return;
	ClearDriver();
	BaseVehicleComponent::SetDriver(ent);
	if(!ent->IsPlayer() || !ent->GetPlayerComponent()->IsLocalPlayer())
		return;
	c_game->DisableRenderMode(RenderMode::View);
	auto *vb = c_game->GetViewBody();
	if(vb != nullptr)
	{
		auto pRenderComponent = static_cast<CBaseEntity&>(vb->GetEntity()).GetRenderComponent();
		if(pRenderComponent.valid())
			pRenderComponent->SetRenderMode(RenderMode::None);
	}
	auto plComponent = ent->GetPlayerComponent();
	plComponent->SetObserverMode(OBSERVERMODE::THIRDPERSON);
	auto pObsComponent = GetEntity().GetComponent<pragma::CObservableComponent>();
	plComponent->SetObserverTarget(pObsComponent.get());
}
void CVehicleComponent::Initialize()
{
	BaseVehicleComponent::Initialize();
}
Bool CVehicleComponent::ReceiveNetEvent(pragma::NetEventId eventId,NetPacket &packet)
{
	if(eventId == m_netEvSteeringWheelModel)
	{
		auto ent = nwm::read_entity(packet);
		m_steeringWheel = ent ? ent->GetHandle() : EntityHandle{};
		InitializeSteeringWheel();
	}
	else
		return false;
	return true;
}
