/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_SHADER_TEXTURED_ALPHA_TRANSITION_HPP__
#define __C_SHADER_TEXTURED_ALPHA_TRANSITION_HPP__

#include "pragma/rendering/shaders/world/c_shader_textured.hpp"

namespace pragma
{
	class DLLCLIENT ShaderTexturedAlphaTransition
		: public ShaderTextured3DBase
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_ALPHA;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_ALPHA;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_MATERIAL;
		enum class MaterialBinding : uint32_t
		{
			DiffuseMap2 = umath::to_integral(ShaderTextured3DBase::MaterialBinding::Count),
			DiffuseMap3
		};

#pragma pack(push,1)
		struct PushConstants
		{
			int32_t alphaCount;
		};
#pragma pack(pop)

		ShaderTexturedAlphaTransition(prosper::IPrContext &context,const std::string &identifier);
		virtual bool Draw(CModelSubMesh &mesh) override;
	protected:
		virtual void InitializeGfxPipelinePushConstantRanges(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
		virtual prosper::DescriptorSetInfo &GetMaterialDescriptorSetInfo() const override;
		virtual std::shared_ptr<prosper::IDescriptorSetGroup> InitializeMaterialDescriptorSet(CMaterial &mat) override;
	};
};

#endif