/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/rendering/shaders/util/c_shader_compose_rma.hpp"
#include <prosper_util.hpp>
#include <prosper_context.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <prosper_command_buffer.hpp>
#include <util_image_buffer.hpp>
#include <util_texture_info.hpp>
#include <image/prosper_render_target.hpp>
#include <image/prosper_texture.hpp>
#include <image/prosper_image.hpp>
#include <image/prosper_sampler.hpp>
#include <sharedutils/util_file.h>
#include <cmaterialmanager.h>

extern DLLCLIENT CGame *c_game;
extern DLLCLIENT ClientState *client;

decltype(pragma::ShaderComposeRMA::DESCRIPTOR_SET_TEXTURE) pragma::ShaderComposeRMA::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding { // Roughness Map
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		},
		prosper::DescriptorSetInfo::Binding { // Metalness Map
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		},
		prosper::DescriptorSetInfo::Binding { // Ambient occlusion Map
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		}
	}
};
pragma::ShaderComposeRMA::ShaderComposeRMA(prosper::IPrContext &context,const std::string &identifier)
	: ShaderBaseImageProcessing{context,identifier,"util/fs_compose_rma.gls"}
{}

std::shared_ptr<prosper::IImage> pragma::ShaderComposeRMA::ComposeRMA(
	prosper::IPrContext &context,prosper::Texture *roughnessMap,prosper::Texture *metalnessMap,prosper::Texture *aoMap,
	Flags flags
)
{
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
	imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
	imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit | prosper::ImageUsageFlags::SampledBit;

	auto fGetWhiteTex = [&context]() -> prosper::Texture* {
		TextureManager::LoadInfo loadInfo {};
		loadInfo.flags = TextureLoadFlags::LoadInstantly;
		std::shared_ptr<void> tex = nullptr;
		if(static_cast<CMaterialManager&>(client->GetMaterialManager()).GetTextureManager().Load(context,"white",loadInfo,&tex) == false)
			return nullptr;
		return std::static_pointer_cast<Texture>(tex)->GetVkTexture().get();
	};

	if(roughnessMap == nullptr)
		roughnessMap = fGetWhiteTex();
	if(metalnessMap == nullptr)
		metalnessMap = fGetWhiteTex();
	if(aoMap == nullptr)
		aoMap = fGetWhiteTex();
	if(roughnessMap == nullptr || metalnessMap == nullptr || aoMap == nullptr)
		return nullptr;

	auto &imgRoughness = roughnessMap->GetImage();
	auto &imgMetalness = metalnessMap->GetImage();
	auto &imgAo = aoMap->GetImage();

	auto extents = imgRoughness.GetExtents();
	extents.width = std::max({extents.width,imgMetalness.GetExtents().width,imgAo.GetExtents().width});
	extents.height = std::max({extents.height,imgMetalness.GetExtents().height,imgAo.GetExtents().height});

	imgCreateInfo.width = extents.width;
	imgCreateInfo.height = extents.height;

	auto imgRMA = context.CreateImage(imgCreateInfo);

	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	auto texRMA = context.CreateTexture({},*imgRMA,imgViewCreateInfo);
	auto rt = context.CreateRenderTarget({texRMA},GetRenderPass());

	auto dsg = CreateDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE.setIndex);
	auto &ds = *dsg->GetDescriptorSet();
	ds.SetBindingTexture(*roughnessMap,umath::to_integral(TextureBinding::RoughnessMap));
	ds.SetBindingTexture(*metalnessMap,umath::to_integral(TextureBinding::MetalnessMap));
	ds.SetBindingTexture(*aoMap,umath::to_integral(TextureBinding::AmbientOcclusionMap));

	auto &setupCmd = context.GetSetupCommandBuffer();
	if(setupCmd->RecordBeginRenderPass(*rt))
	{
		if(BeginDraw(setupCmd))
		{
			PushConstants pushConstants {};
			pushConstants.flags = flags;
			if(RecordPushConstants(pushConstants))
				Draw(ds);
			EndDraw();
		}
		setupCmd->RecordEndRenderPass();
	}
	context.FlushSetupCommandBuffer();

	return texRMA->GetImage().shared_from_this();
}
bool pragma::ShaderComposeRMA::InsertAmbientOcclusion(prosper::IPrContext &context,const std::string &rmaInputPath,prosper::IImage &aoImg,const std::string *optRmaOutputPath)
{
	auto &texManager = static_cast<CMaterialManager&>(client->GetMaterialManager()).GetTextureManager();
	std::shared_ptr<void> rmaTexInfo;
	TextureManager::LoadInfo loadInfo {};
	loadInfo.flags = TextureLoadFlags::LoadInstantly;
	if(texManager.Load(context,rmaInputPath,loadInfo,&rmaTexInfo) == false || rmaTexInfo == nullptr || std::static_pointer_cast<Texture>(rmaTexInfo)->HasValidVkTexture() == false)
		return false;

	prosper::util::TextureCreateInfo texCreateInfo {};
	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	prosper::util::SamplerCreateInfo samplerCreateInfo {};
	auto aoTex = context.CreateTexture(texCreateInfo,aoImg,imgViewCreateInfo,samplerCreateInfo);

	auto texRMA = std::static_pointer_cast<Texture>(rmaTexInfo)->GetVkTexture();
	auto newRMA = ComposeRMA(context,texRMA.get(),texRMA.get(),aoTex.get());

	uimg::TextureInfo imgWriteInfo {};
	imgWriteInfo.alphaMode = uimg::TextureInfo::AlphaMode::None;
	imgWriteInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
	imgWriteInfo.flags = uimg::TextureInfo::Flags::GenerateMipmaps;
	imgWriteInfo.inputFormat = uimg::TextureInfo::InputFormat::R8G8B8A8_UInt;
	imgWriteInfo.outputFormat = uimg::TextureInfo::OutputFormat::ColorMap;

	auto rmaOutputPath = optRmaOutputPath ? *optRmaOutputPath : rmaInputPath;

	std::string materialsRootDir = "materials/";
	auto matName = materialsRootDir +rmaOutputPath;
	ufile::remove_extension_from_filename(matName);

	Con::cout<<"Writing RMA texture file '"<<rmaOutputPath<<"'..."<<Con::endl;
	// TODO: RMA should overwrite the existing one
	return c_game->SaveImage(*newRMA,"addons/converted/" +matName,imgWriteInfo);
}
bool pragma::ShaderComposeRMA::InsertAmbientOcclusion(prosper::IPrContext &context,const std::string &rmaInputPath,uimg::ImageBuffer &imgBuffer,const std::string *optRmaOutputPath)
{
	auto aoImg = context.CreateImage(imgBuffer);
	return InsertAmbientOcclusion(context,rmaInputPath,*aoImg,optRmaOutputPath);
}

void pragma::ShaderComposeRMA::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::FragmentBit);
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
}

void pragma::ShaderComposeRMA::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<pragma::ShaderComposeRMA>(
		std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
			{prosper::Format::R8G8B8A8_UNorm} // RMA
	},outRenderPass,pipelineIdx);
}
