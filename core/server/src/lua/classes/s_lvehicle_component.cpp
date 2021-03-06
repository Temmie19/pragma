/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer */

#include "stdafx_server.h"
#include "pragma/lua/classes/ldef_entity.h"
#include "luasystem.h"
#include "pragma/entities/components/s_vehicle_component.hpp"
#include "pragma/lua/s_lentity_handles.hpp"
#include "pragma/lua/s_lentity_components.hpp"
#include <pragma/lua/lentity_components_base_types.hpp>
#include <pragma/lua/lua_entity_component.hpp>

namespace Lua
{
	namespace Vehicle
	{
		namespace Server
		{

		};
	};
};
void Lua::register_sv_vehicle_component(lua_State *l,luabind::module_ &module)
{
	auto def = luabind::class_<SVehicleHandle,BaseEntityComponentHandle>("VehicleComponent");
	Lua::register_base_vehicle_component_methods<luabind::class_<SVehicleHandle,BaseEntityComponentHandle>,SVehicleHandle>(l,def);
	module[def];
}
