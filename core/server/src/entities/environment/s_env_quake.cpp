/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer */

#include "stdafx_server.h"
#include "pragma/entities/environment/s_env_quake.h"
#include "pragma/entities/s_entityfactories.h"
#include <sharedutils/util_string.h>
#include "pragma/lua/s_lentity_handles.hpp"
#include <sharedutils/netpacket.hpp>
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(env_quake,EnvQuake);

void SQuakeComponent::Initialize()
{
	BaseEnvQuakeComponent::Initialize();
}

void SQuakeComponent::SendData(NetPacket &packet,networking::ClientRecipientFilter &rp)
{
	packet->Write<UInt32>(m_quakeFlags);
	packet->Write<Float>(GetFrequency());
	packet->Write<Float>(GetAmplitude());
	packet->Write<Float>(GetRadius());
	packet->Write<Float>(GetDuration());
	packet->Write<Float>(GetFadeInDuration());
	packet->Write<Float>(GetFadeOutDuration());
}

luabind::object SQuakeComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<SQuakeComponentHandleWrapper>(l);}

///////////////

extern ServerState *server;

void EnvQuake::Initialize()
{
	SBaseEntity::Initialize();
	AddComponent<SQuakeComponent>();
}
