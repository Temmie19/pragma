/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_SHADER_SHADOW_HPP__
#define __C_SHADER_SHADOW_HPP__

#include "pragma/rendering/shaders/world/c_shader_scene.hpp"

namespace pragma
{
	class CLightPointComponent;
	class CLightSpotComponent;
	class CLightComponent;
	class DLLCLIENT ShaderShadow
		: public ShaderEntity
	{
	public:
		static prosper::Format RENDER_PASS_DEPTH_FORMAT;

		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_BONE_WEIGHT;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_BONE_WEIGHT_ID;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_BONE_WEIGHT;

		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_INSTANCE;

		enum class Flags : uint32_t
		{
			None = 0u,
			UseExtendedVertexWeights = 1u
		};

#pragma pack(push,1)
		struct PushConstants
		{
			Mat4 depthMVP;
			Vector4 lightPos; // 4th component stores the distance
			Flags flags;
		};
#pragma pack(pop)

		ShaderShadow(prosper::IPrContext &context,const std::string &identifier);
		ShaderShadow(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader);

		bool BindLight(CLightComponent &light);
		bool BindEntity(CBaseEntity &ent,const Mat4 &depthMVP);
		bool BindMaterial(CMaterial &mat); // TODO: Transparent only
		virtual bool Draw(CModelSubMesh &mesh) override;
	protected:
		bool BindDepthMatrix(const Mat4 &depthMVP);
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
	private:
		virtual uint32_t GetRenderSettingsDescriptorSetIndex() const override;
		virtual uint32_t GetCameraDescriptorSetIndex() const override;
		virtual uint32_t GetLightDescriptorSetIndex() const override;
		virtual uint32_t GetInstanceDescriptorSetIndex() const override;
		virtual void GetVertexAnimationPushConstantInfo(uint32_t &offset) const override;
		virtual bool BindEntity(CBaseEntity &ent) override {return false;}
	};

	class DLLCLIENT ShaderShadowTransparent
		: public ShaderShadow
	{
	public:
		//bool BindMaterial(CMaterial &mat);
	};

	//////////////////

	class DLLCLIENT ShaderShadowSpot
		: public ShaderShadow
	{
	public:
		ShaderShadowSpot(prosper::IPrContext &context,const std::string &identifier);
	};

	//////////////////

	class DLLCLIENT ShaderShadowCSM
		: public ShaderShadow
	{
	public:
		ShaderShadowCSM(prosper::IPrContext &context,const std::string &identifier);
	protected:
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
	};

	//////////////////

	class DLLCLIENT ShaderShadowCSMTransparent
		: public ShaderShadowCSM
	{
	public:
		//bool BindMaterial(CMaterial &mat);
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(pragma::ShaderShadow::Flags)

#endif