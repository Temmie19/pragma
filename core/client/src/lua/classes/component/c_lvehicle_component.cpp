#include "stdafx_client.h"
#include "pragma/entities/components/c_vehicle_component.hpp"
#include "pragma/lua/classes/ldef_entity.h"
#include "luasystem.h"
#include "pragma/lua/classes/components/c_lentity_components.hpp"
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/lua/classes/lproperty.hpp>
#include <pragma/lua/lua_entity_component.hpp>
#include <pragma/lua/lentity_components_base_types.hpp>

void Lua::register_cl_vehicle_component(lua_State *l,luabind::module_ &module)
{
	auto def = luabind::class_<CVehicleHandle,BaseEntityComponentHandle>("VehicleComponent");
	Lua::register_base_vehicle_component_methods<luabind::class_<CVehicleHandle,BaseEntityComponentHandle>,CVehicleHandle>(l,def);
	module[def];
}
