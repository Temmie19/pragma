/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer */

#include "stdafx_server.h"
#include "pragma/entities/filter/s_filter_entity_class.h"
#include "pragma/entities/s_entityfactories.h"
#include "pragma/lua/s_lentity_handles.hpp"
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(filter_entity_class,FilterEntityClass);

extern SGame *s_game;

luabind::object SFilterClassComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<SFilterClassComponentHandleWrapper>(l);}

void FilterEntityClass::Initialize()
{
	SBaseEntity::Initialize();
	AddComponent<SFilterClassComponent>();
}
