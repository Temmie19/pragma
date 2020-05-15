/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/rendering/shaders/world/c_shader_light_cone.hpp"
#include "pragma/rendering/renderers/rasterization_renderer.hpp"
#include "pragma/model/c_modelmesh.h"
#include "pragma/model/vk_mesh.h"
#include "pragma/entities/environment/lights/c_env_light_spot_vol.h"
#include "pragma/entities/environment/lights/c_env_light.h"
#include <shader/prosper_pipeline_create_info.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_util.hpp>
#include <datasystem_color.h>
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

extern DLLCENGINE CEngine *c_engine;

decltype(ShaderLightCone::DESCRIPTOR_SET_DEPTH_MAP) ShaderLightCone::DESCRIPTOR_SET_DEPTH_MAP = {
	{
		prosper::DescriptorSetInfo::Binding { // Depth Map
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		}
	}
};
ShaderLightCone::ShaderLightCone(prosper::IPrContext &context,const std::string &identifier)
	: ShaderTextured3DBase(context,identifier,"effects/vs_light_cone","effects/fs_light_cone")
{
	// SetBaseShader<ShaderTextured3DBase>();
	umath::set_flag(m_stateFlags,StateFlags::ShouldUseLightMap,false);
}

bool ShaderLightCone::BindSceneCamera(const pragma::rendering::RasterizationRenderer &renderer,bool bView)
{
	if(ShaderTextured3DBase::BindSceneCamera(renderer,bView) == false)
		return false;
	auto *descSetDepth = renderer.GetDepthDescriptorSet();
	if(descSetDepth == nullptr)
		return false;
	return RecordBindDescriptorSet(*descSetDepth,DESCRIPTOR_SET_DEPTH_MAP.setIndex);
}

bool ShaderLightCone::BindEntity(CBaseEntity &ent)
{
	if(ShaderTextured3DBase::BindEntity(ent) == false)
		return false;
	auto pSpotVolComponent = ent.GetComponent<CLightSpotVolComponent>();
	auto lightIndex = -1;
	if(pSpotVolComponent.valid())
	{
		auto *entSpotlight = pSpotVolComponent->GetSpotlightTarget();
		if(entSpotlight != nullptr)
		{
			auto pLightComponent = entSpotlight->GetComponent<CLightComponent>();
			if(pLightComponent.valid())
			{
				auto &renderBuffer = pLightComponent->GetRenderBuffer();
				if(renderBuffer != nullptr)
					lightIndex = renderBuffer->GetBaseIndex();
			}
		}
	}
	m_boundLightIndex = lightIndex;
	return true;
}

std::shared_ptr<prosper::IDescriptorSetGroup> ShaderLightCone::InitializeMaterialDescriptorSet(CMaterial &mat)
{
	auto descSetGroup = GetContext().CreateDescriptorSetGroup(DESCRIPTOR_SET_MATERIAL);
	mat.SetDescriptorSetGroup(*this,descSetGroup);
	return descSetGroup;
}

bool ShaderLightCone::Draw(CModelSubMesh &mesh)
{
	return RecordPushConstants( // Light cone shader doesn't use lightmaps, so we hijack the lightmapFlags push constant for our own purposes
		static_cast<uint32_t>(m_boundLightIndex),
		sizeof(ShaderTextured3DBase::PushConstants) +offsetof(PushConstants,boundLightIndex)
	) && ShaderTextured3DBase::Draw(mesh);
}

bool ShaderLightCone::BindMaterialParameters(CMaterial &mat)
{
	if(ShaderTextured3DBase::BindMaterialParameters(mat) == false)
		return false;
	auto &data = mat.GetDataBlock();
	auto coneLength = 100.f;
	if(data != nullptr)
		coneLength = data->GetFloat("cone_height");
	return RecordPushConstants(
		PushConstants{coneLength},
		sizeof(ShaderTextured3DBase::PushConstants) +offsetof(PushConstants,coneLength)
	);
}

void ShaderLightCone::InitializeGfxPipelinePushConstantRanges(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	AttachPushConstantRange(pipelineInfo,0u,sizeof(ShaderTextured3DBase::PushConstants) +sizeof(PushConstants),prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
}
void ShaderLightCone::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderTextured3DBase::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_DEPTH_MAP);
	prosper::util::set_graphics_pipeline_cull_mode_flags(pipelineInfo,prosper::CullModeFlags::None);
	pipelineInfo.ToggleDepthWrites(false);
}
