#include "stdafx_client.h"
#include "pragma/entities/func/c_func_physics.h"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/rendering/c_rendermode.h"
#include "pragma/entities/components/c_render_component.hpp"
#include "pragma/physics/movetypes.h"
#include <pragma/physics/physobj.h>
#include <pragma/networking/nwm_util.h>
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(func_physics,CFuncPhysics);

void CFuncPhysicsComponent::Initialize()
{
	BaseFuncPhysicsComponent::Initialize();
	auto pRenderComponent = static_cast<CBaseEntity&>(GetEntity()).GetRenderComponent();
	if(pRenderComponent.valid())
		pRenderComponent->SetRenderMode(RenderMode::World);
}

void CFuncPhysicsComponent::ReceiveData(NetPacket &packet)
{
	m_kvMass = packet->Read<float>();
	m_kvSurfaceMaterial = packet->ReadString();
}

void CFuncPhysicsComponent::OnEntitySpawn()
{
	BaseFuncPhysicsComponent::OnEntitySpawn();
}
luabind::object CFuncPhysicsComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CFuncPhysicsComponentHandleWrapper>(l);}

/////////////

void CFuncPhysics::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CFuncPhysicsComponent>();
}
