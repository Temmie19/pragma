/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_SHADER_PP_HDR_HPP__
#define __C_SHADER_PP_HDR_HPP__

#include "pragma/rendering/shaders/post_processing/c_shader_pp_base.hpp"

namespace pragma
{
	namespace rendering
	{
		enum class ToneMapping : uint32_t;
	};
	class DLLCLIENT ShaderPPHDR
		: public ShaderPPBase
	{
	public:
		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;
		static prosper::Format RENDER_PASS_FORMAT;

		enum class TextureBinding : uint32_t
		{
			Texture = 0u,
			Bloom,
			Glow
		};
#pragma pack(push,1)
		struct PushConstants
		{
			float exposure;
			float bloomScale;
			float glowScale;
			rendering::ToneMapping toneMapping;
		};
#pragma pack(pop)

		ShaderPPHDR(prosper::Context &context,const std::string &identifier);
		bool Draw(prosper::IDescriptorSet &descSetTexture,float exposure,float bloomScale,float glowScale);
	protected:
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
	};
};

#endif
