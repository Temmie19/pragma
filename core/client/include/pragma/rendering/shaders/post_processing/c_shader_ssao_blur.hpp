#ifndef __C_SHADER_SSAO_BLUR_HPP__
#define __C_SHADER_SSAO_BLUR_HPP__

#include "pragma/clientdefinitions.h"
#include <shader/prosper_shader_base_image_processing.hpp>

namespace pragma
{
	class DLLCLIENT ShaderSSAOBlur
		: public prosper::ShaderBaseImageProcessing
	{
	public:
		ShaderSSAOBlur(prosper::Context &context,const std::string &identifier);
	protected:
		virtual void InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx) override;
	};
};

// prosper TODO
#if 0
#include "shadersystem.h"
#include "shader_screen.h"

namespace Shader
{
	class DLLCLIENT SSAOBlur
		: public Screen
	{
	public:
		SSAOBlur();
	protected:
		virtual void InitializeAttachments(std::vector<vk::PipelineColorBlendAttachmentState> &attachments) override;
		virtual void InitializeRenderPasses() override;
	};
};
#endif
#endif
