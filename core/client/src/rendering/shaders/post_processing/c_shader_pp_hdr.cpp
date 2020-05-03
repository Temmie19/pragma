/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/rendering/shaders/post_processing/c_shader_pp_hdr.hpp"
#include "pragma/rendering/c_settings.hpp"
#include "pragma/console/c_cvar.h"
#include <pragma/console/convars.h>
#include <prosper_util.hpp>
#include <shader/prosper_shader_copy_image.hpp>

using namespace pragma;

decltype(ShaderPPHDR::DESCRIPTOR_SET_TEXTURE) ShaderPPHDR::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding { // Texture
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		},
		prosper::DescriptorSetInfo::Binding { // Bloom
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		},
		prosper::DescriptorSetInfo::Binding { // Glow
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		}
	}
};
decltype(ShaderPPHDR::RENDER_PASS_FORMAT) ShaderPPHDR::RENDER_PASS_FORMAT = prosper::Format::R8G8B8A8_UNorm;
ShaderPPHDR::ShaderPPHDR(prosper::IPrContext &context,const std::string &identifier)
	: ShaderPPBase(context,identifier,"screen/fs_pp_hdr")
{
	SetBaseShader<prosper::ShaderCopyImage>();
}

void ShaderPPHDR::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderPPHDR>({{
		{
			RENDER_PASS_FORMAT,prosper::ImageLayout::ColorAttachmentOptimal,prosper::AttachmentLoadOp::DontCare,
			prosper::AttachmentStoreOp::Store,prosper::SampleCountFlags::e1Bit,prosper::ImageLayout::TransferSrcOptimal
		}
	}},outRenderPass,pipelineIdx);
}

void ShaderPPHDR::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::FragmentBit);
}

static auto cvToneMapping = GetClientConVar("cl_render_tone_mapping");
bool ShaderPPHDR::Draw(prosper::IDescriptorSet &descSetTexture,float exposure,float bloomScale,float glowScale)
{
	auto toneMapping = rendering::ToneMapping::Reinhard;
	auto toneMappingCvarVal = cvToneMapping->GetInt();
	switch(toneMappingCvarVal)
	{
	case -1:
		break;
	default:
		toneMapping = static_cast<rendering::ToneMapping>(toneMappingCvarVal);
		break;
	}
	return RecordPushConstants(PushConstants{exposure,bloomScale,glowScale,toneMapping}) &&
		ShaderPPBase::Draw(descSetTexture) == true;
}
