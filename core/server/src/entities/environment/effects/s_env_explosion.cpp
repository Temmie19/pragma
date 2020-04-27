/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer */

#include "stdafx_server.h"
#include "pragma/entities/environment/effects/s_env_explosion.h"
#include "pragma/entities/s_entityfactories.h"
#include <pragma/game/damageinfo.h>
#include <sharedutils/util_string.h>
#include <pragma/networking/nwm_util.h>
#include <pragma/networking/enums.hpp>
#include "pragma/lua/s_lentity_handles.hpp"
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(env_explosion,EnvExplosion);

extern ServerState *server;

void SExplosionComponent::Explode()
{
	auto &ent = static_cast<SBaseEntity&>(GetEntity());
	if(ent.IsShared())
	{
		NetPacket p;
		nwm::write_entity(p,&ent);
		server->SendPacket("envexplosion_explode",p,pragma::networking::Protocol::SlowReliable);
	}
	BaseEnvExplosionComponent::Explode();
}

luabind::object SExplosionComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<SExplosionComponentHandleWrapper>(l);}

void EnvExplosion::Initialize()
{
	SBaseEntity::Initialize();
	AddComponent<SExplosionComponent>();
}
