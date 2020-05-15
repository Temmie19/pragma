/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/rendering/shaders/world/c_shader_prepass.hpp"
#include "pragma/rendering/shaders/world/c_shader_textured.hpp"
#include "pragma/model/c_vertex_buffer_data.hpp"
#include "pragma/model/c_modelmesh.h"
#include <shader/prosper_pipeline_create_info.hpp>
#include <pragma/model/vertex.h>
#include <prosper_command_buffer.hpp>
#include <prosper_util.hpp>

using namespace pragma;

ShaderPrepassBase::Pipeline ShaderPrepassBase::GetPipelineIndex(prosper::SampleCountFlags sampleCount)
{
	return (sampleCount == prosper::SampleCountFlags::e1Bit) ? Pipeline::Regular : Pipeline::MultiSample;
}

decltype(ShaderPrepassBase::VERTEX_BINDING_BONE_WEIGHT) ShaderPrepassBase::VERTEX_BINDING_BONE_WEIGHT = {prosper::VertexInputRate::Vertex};
decltype(ShaderPrepassBase::VERTEX_ATTRIBUTE_BONE_WEIGHT_ID) ShaderPrepassBase::VERTEX_ATTRIBUTE_BONE_WEIGHT_ID = {ShaderEntity::VERTEX_ATTRIBUTE_BONE_WEIGHT_ID,VERTEX_BINDING_BONE_WEIGHT};
decltype(ShaderPrepassBase::VERTEX_ATTRIBUTE_BONE_WEIGHT) ShaderPrepassBase::VERTEX_ATTRIBUTE_BONE_WEIGHT = {ShaderEntity::VERTEX_ATTRIBUTE_BONE_WEIGHT,VERTEX_BINDING_BONE_WEIGHT};

decltype(ShaderPrepassBase::VERTEX_BINDING_BONE_WEIGHT_EXT) ShaderPrepassBase::VERTEX_BINDING_BONE_WEIGHT_EXT = {prosper::VertexInputRate::Vertex};
decltype(ShaderPrepassBase::VERTEX_ATTRIBUTE_BONE_WEIGHT_EXT_ID) ShaderPrepassBase::VERTEX_ATTRIBUTE_BONE_WEIGHT_EXT_ID = {ShaderEntity::VERTEX_ATTRIBUTE_BONE_WEIGHT_EXT_ID,VERTEX_BINDING_BONE_WEIGHT_EXT};
decltype(ShaderPrepassBase::VERTEX_ATTRIBUTE_BONE_WEIGHT_EXT) ShaderPrepassBase::VERTEX_ATTRIBUTE_BONE_WEIGHT_EXT = {ShaderEntity::VERTEX_ATTRIBUTE_BONE_WEIGHT_EXT,VERTEX_BINDING_BONE_WEIGHT_EXT};

decltype(ShaderPrepassBase::VERTEX_BINDING_VERTEX) ShaderPrepassBase::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex,sizeof(VertexBufferData)};
decltype(ShaderPrepassBase::VERTEX_ATTRIBUTE_POSITION) ShaderPrepassBase::VERTEX_ATTRIBUTE_POSITION = {ShaderEntity::VERTEX_ATTRIBUTE_POSITION,VERTEX_BINDING_VERTEX};

decltype(ShaderPrepassBase::DESCRIPTOR_SET_INSTANCE) ShaderPrepassBase::DESCRIPTOR_SET_INSTANCE = {&ShaderEntity::DESCRIPTOR_SET_INSTANCE};
decltype(ShaderPrepassBase::DESCRIPTOR_SET_CAMERA) ShaderPrepassBase::DESCRIPTOR_SET_CAMERA = {&ShaderScene::DESCRIPTOR_SET_CAMERA};

static prosper::util::RenderPassCreateInfo::AttachmentInfo get_depth_render_pass_attachment_info(prosper::SampleCountFlags sampleCount)
{
	return prosper::util::RenderPassCreateInfo::AttachmentInfo {
		ShaderTextured3DBase::RENDER_PASS_DEPTH_FORMAT,prosper::ImageLayout::DepthStencilAttachmentOptimal,prosper::AttachmentLoadOp::Clear,
		prosper::AttachmentStoreOp::Store,sampleCount,prosper::ImageLayout::DepthStencilAttachmentOptimal
	};
}

ShaderPrepassBase::ShaderPrepassBase(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader)
	: ShaderEntity(context,identifier,vsShader,fsShader)
{
	SetPipelineCount(umath::to_integral(Pipeline::Count));
}
ShaderPrepassBase::ShaderPrepassBase(prosper::IPrContext &context,const std::string &identifier)
	: ShaderEntity(context,identifier,"world/prepass/vs_prepass_depth","")
{
	SetPipelineCount(umath::to_integral(Pipeline::Count));
}

bool ShaderPrepassBase::BeginDraw(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &cmdBuffer,Pipeline pipelineIdx)
{
	Set3DSky(false);
	return ShaderEntity::BeginDraw(cmdBuffer,umath::to_integral(pipelineIdx)) &&
		cmdBuffer->RecordSetDepthBias();
}

bool ShaderPrepassBase::BindClipPlane(const Vector4 &clipPlane)
{
	return RecordPushConstants(clipPlane);
}

bool ShaderPrepassBase::BindDrawOrigin(const Vector4 &drawOrigin)
{
	return RecordPushConstants(drawOrigin,offsetof(PushConstants,drawOrigin));
}

void ShaderPrepassBase::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderPrepassBase>({{get_depth_render_pass_attachment_info(GetSampleCount(pipelineIdx))}},outRenderPass,pipelineIdx);
}

void ShaderPrepassBase::Set3DSky(bool is3dSky) {umath::set_flag(m_stateFlags,Flags::RenderAs3DSky,is3dSky);}

bool ShaderPrepassBase::Draw(CModelSubMesh &mesh)
{
	auto flags = Flags::None;
	if(mesh.GetExtendedVertexWeights().empty() == false)
		flags |= Flags::UseExtendedVertexWeights;
	if(umath::is_flag_set(m_stateFlags,Flags::RenderAs3DSky))
		flags |= Flags::RenderAs3DSky;
	return RecordPushConstants(flags,offsetof(PushConstants,flags)) && ShaderEntity::Draw(mesh);
}

void ShaderPrepassBase::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderEntity::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	if(pipelineIdx == umath::to_integral(Pipeline::Reflection))
		prosper::util::set_graphics_pipeline_cull_mode_flags(pipelineInfo,prosper::CullModeFlags::FrontBit);

	pipelineInfo.ToggleDepthWrites(true);
	pipelineInfo.ToggleDepthTest(true,prosper::CompareOp::Less);

	pipelineInfo.ToggleDepthBias(true,0.f,0.f,0.f);
	pipelineInfo.ToggleDynamicState(true,prosper::DynamicState::DepthBias); // Required for decals

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_BONE_WEIGHT_ID);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_BONE_WEIGHT);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_BONE_WEIGHT_EXT_ID);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_BONE_WEIGHT_EXT);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);

	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::VertexBit);

	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_INSTANCE);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_CAMERA);
}

uint32_t ShaderPrepassBase::GetCameraDescriptorSetIndex() const {return DESCRIPTOR_SET_CAMERA.setIndex;}
uint32_t ShaderPrepassBase::GetInstanceDescriptorSetIndex() const {return DESCRIPTOR_SET_INSTANCE.setIndex;}
void ShaderPrepassBase::GetVertexAnimationPushConstantInfo(uint32_t &offset) const
{
	offset = offsetof(PushConstants,vertexAnimInfo);
}

//////////////////

decltype(ShaderPrepass::VERTEX_ATTRIBUTE_NORMAL) ShaderPrepass::VERTEX_ATTRIBUTE_NORMAL = {ShaderTextured3DBase::VERTEX_ATTRIBUTE_NORMAL,VERTEX_BINDING_VERTEX};

decltype(ShaderPrepass::RENDER_PASS_NORMAL_FORMAT) ShaderPrepass::RENDER_PASS_NORMAL_FORMAT = prosper::Format::R16G16B16A16_SFloat;
ShaderPrepass::ShaderPrepass(prosper::IPrContext &context,const std::string &identifier)
	: ShaderPrepassBase(context,identifier,"world/prepass/vs_prepass","world/prepass/fs_prepass")
{
	// SetBaseShader<ShaderTextured3DBase>();
}

void ShaderPrepass::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	auto sampleCount = GetSampleCount(pipelineIdx);
	prosper::util::RenderPassCreateInfo rpInfo {{{
		RENDER_PASS_NORMAL_FORMAT,prosper::ImageLayout::ColorAttachmentOptimal,prosper::AttachmentLoadOp::DontCare,
		prosper::AttachmentStoreOp::Store,sampleCount,prosper::ImageLayout::ColorAttachmentOptimal
	},get_depth_render_pass_attachment_info(sampleCount)}};
	CreateCachedRenderPass<ShaderPrepass>({rpInfo},outRenderPass,pipelineIdx);
}

void ShaderPrepass::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderPrepassBase::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_NORMAL);
}
