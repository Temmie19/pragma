/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer */

#include "stdafx_server.h"
#include "pragma/entities/s_npc_dragonworm.h"
#include "pragma/entities/s_entityfactories.h"
#include <pragma/entities/components/base_model_component.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(npc_dragonworm,NPCDragonWorm);

void SDragonWormComponent::Initialize()
{
	BaseEntityComponent::Initialize();
}

void SDragonWormComponent::OnEntitySpawn()
{
	BaseEntityComponent::OnEntitySpawn();
	auto mdlComponent = GetEntity().GetModelComponent();
	if(mdlComponent.valid())
		mdlComponent->SetModel("creatures/dragonworm.wmd");
}
