/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/rendering/shaders/world/c_shader_textured_base.hpp"

using namespace pragma;

std::shared_ptr<prosper::IDescriptorSetGroup> pragma::ShaderTexturedBase::InitializeMaterialDescriptorSet(CMaterial &mat,bool bReload)
{
	auto &shader = *dynamic_cast<prosper::Shader*>(this);
	auto descSetGroup = mat.GetDescriptorSetGroup(shader);
	if(descSetGroup != nullptr)
	{
		if(bReload == false)
			return descSetGroup;
		mat.SetDescriptorSetGroup(shader,nullptr);
	}
	return InitializeMaterialDescriptorSet(mat);
}

std::shared_ptr<prosper::IDescriptorSetGroup> pragma::ShaderTexturedBase::InitializeMaterialDescriptorSet(CMaterial &mat) {return nullptr;}
