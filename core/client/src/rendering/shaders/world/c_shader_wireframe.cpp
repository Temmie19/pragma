/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/rendering/shaders/world/c_shader_wireframe.hpp"

using namespace pragma;

ShaderWireframe::ShaderWireframe(prosper::IPrContext &context,const std::string &identifier)
	: ShaderTextured3DBase(context,identifier,"world/vs_wireframe","world/fs_wireframe")
{
	// SetBaseShader<ShaderTextured3DBase>();
}

void ShaderWireframe::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderTextured3DBase::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	prosper::util::set_graphics_pipeline_polygon_mode(pipelineInfo,Anvil::PolygonMode::LINE);
	prosper::util::set_graphics_pipeline_line_width(pipelineInfo,1.f);
}
