#include "stdafx_client.h"
#include "pragma/entities/prop/c_prop_dynamic.hpp"
#include "pragma/entities/c_entityfactories.h"
#include <pragma/physics/physobj.h>
#include "pragma/physics/movetypes.h"
#include "pragma/rendering/scene/camera.h"
#include "pragma/entities/components/c_physics_component.hpp"
#include "pragma/entities/components/c_render_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(prop_dynamic,CPropDynamic);

void CPropDynamicComponent::Initialize()
{
	BasePropDynamicComponent::Initialize();
}

void CPropDynamicComponent::OnEntityComponentAdded(BaseEntityComponent &component)
{
	BasePropDynamicComponent::OnEntityComponentAdded(component);
	if(typeid(component) == typeid(pragma::CPhysicsComponent))
	{
		auto *pPhysComponent = static_cast<pragma::CPhysicsComponent*>(&component);
		pPhysComponent->SetMoveType(MOVETYPE::NONE);
		// m_propPhysType = PHYSICSTYPE::DYNAMIC;
	}
	else if(typeid(component) == typeid(pragma::CRenderComponent))
	{
		auto *pRenderComponent = static_cast<pragma::CRenderComponent*>(&component);
		pRenderComponent->SetCastShadows(true);
	}
}
luabind::object CPropDynamicComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CPropDynamicComponentHandleWrapper>(l);}

//////////////////

void CPropDynamic::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CPropDynamicComponent>();
}
