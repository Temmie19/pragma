/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/game/c_game.h"
//#include "shader.h" // prosper TODO
#include <pragma/console/convars.h>
//#include "shader_screen.h" // prosper TODO
#include "pragma/rendering/shaders/world/c_shader_wireframe.hpp"
#include "pragma/rendering/shaders/world/c_shader_textured_alpha_transition.hpp"
#include "pragma/rendering/shaders/debug/c_shader_debug.hpp"
#include "pragma/rendering/shaders/debug/c_shader_debug_text.hpp"
#include "pragma/rendering/shaders/world/c_shader_flat.hpp"
#include "pragma/rendering/shaders/world/c_shader_prepass.hpp"
#include "pragma/rendering/shaders/c_shader_depth_to_rgb.h"
#include "pragma/rendering/shaders/particles/c_shader_particle.hpp"
#include "pragma/rendering/shaders/particles/c_shader_particle_model.hpp"
#include "pragma/rendering/shaders/particles/c_shader_particle_polyboard.hpp"
#include "pragma/rendering/shaders/particles/c_shader_particle_blob.hpp"
#include "pragma/rendering/shaders/c_shader_forwardp_light_indexing.hpp"
#include "pragma/rendering/shaders/c_shader_forwardp_light_culling.hpp"
#include "pragma/rendering/shaders/c_shader_shadow.hpp"
#include "pragma/rendering/shaders/image/c_shader_calc_image_color.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_pp_hdr.hpp"
#include "pragma/rendering/shaders/image/c_shader_clear_color.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_pp_fog.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_pp_water.hpp"
#include "pragma/rendering/shaders/world/c_shader_skybox.hpp"
#include "pragma/rendering/shaders/world/c_shader_loading.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_ssao.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_ssao_blur.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_glow.hpp"
#include "pragma/rendering/shaders/world/water/c_shader_water.hpp"
#include "pragma/rendering/shaders/world/water/c_shader_water_splash.hpp"
#include "pragma/rendering/shaders/world/water/c_shader_water_surface_integrate.hpp"
#include "pragma/rendering/shaders/world/water/c_shader_water_surface_sum_edges.hpp"
#include "pragma/rendering/shaders/world/water/c_shader_water_surface_solve_edges.hpp"
#include "pragma/rendering/shaders/world/c_shader_light_cone.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_pp_fxaa.hpp"
#include "pragma/rendering/shaders/c_shader_equirectangular_to_cubemap.hpp"
#include "pragma/rendering/shaders/c_shader_cubemap_to_equirectangular.hpp"
#include "pragma/rendering/shaders/world/raytracing/c_shader_raytracing.hpp"
#include "pragma/rendering/shaders/world/c_shader_pbr.hpp"
#include "pragma/rendering/shaders/world/c_shader_eye.hpp"
#include "pragma/rendering/shaders/world/c_shader_unlit.hpp"
#include "pragma/rendering/shaders/c_shader_convolute_cubemap_lighting.hpp"
#include "pragma/rendering/shaders/c_shader_compute_irradiance_map_roughness.hpp"
#include "pragma/rendering/shaders/c_shader_brdf_convolution.hpp"
#include "pragma/rendering/shaders/util/c_shader_specular_to_roughness.hpp"
#include "pragma/rendering/shaders/util/c_shader_extract_diffuse_ambient_occlusion.hpp"
#include "pragma/rendering/shaders/util/c_shader_compose_rma.hpp"
#include "pragma/rendering/shaders/util/c_shader_specular_glossiness_to_metalness_roughness.hpp"
#include <pragma/console/convars.h>
#include "pragma/console/c_cvar.h"
#include "pragma/rendering/world_environment.hpp"
#include <buffers/prosper_buffer.hpp>



extern DLLCENGINE CEngine *c_engine;
extern DLLCLIENT CGame *c_game;

REGISTER_CONVAR_CALLBACK_CL(cl_render_shader_quality,[](NetworkState*,ConVar*,int,int val) {
	if(c_game == nullptr)
		return;
	c_game->GetWorldEnvironment().SetShaderQuality(val);
});

static void CVAR_CALLBACK_render_unlit(NetworkState*,ConVar*,bool,bool val)
{
	if(c_game == nullptr)
		return;
	c_game->GetWorldEnvironment().SetUnlit(val);
}
REGISTER_CONVAR_CALLBACK_CL(render_unlit,CVAR_CALLBACK_render_unlit);

static void CVAR_CALLBACK_cl_render_shadow_resolution(NetworkState*,ConVar*,int,int val)
{
	if(c_game == nullptr)
		return;
	c_game->GetWorldEnvironment().SetShadowResolution(val);
}
REGISTER_CONVAR_CALLBACK_CL(cl_render_shadow_resolution,CVAR_CALLBACK_cl_render_shadow_resolution);

void CGame::InitShaders()
{
	Con::cout<<"Loading shaders..."<<Con::endl;
	
	auto &shaderManager = c_engine->GetShaderManager();
	pragma::ShaderScene::SetRenderPassSampleCount(static_cast<prosper::SampleCountFlags>(GetMSAASampleCount()));

	shaderManager.RegisterShader("clear_color",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderClearColor(context,identifier);});

	shaderManager.RegisterShader("prepass",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderPrepass(context,identifier);});
	shaderManager.RegisterShader("prepass_depth",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderPrepassDepth(context,identifier);});

	shaderManager.RegisterShader("forwardp_light_indexing",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderForwardPLightIndexing(context,identifier);});
	shaderManager.RegisterShader("forwardp_light_culling",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderForwardPLightCulling(context,identifier);});

	shaderManager.RegisterShader("raytracing",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderRayTracing(context,identifier);});
	shaderManager.RegisterShader("pbr",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderPBR(context,identifier);});
	shaderManager.RegisterShader("pbr_blend",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderPBRBlend(context,identifier);});
	shaderManager.RegisterShader("eye",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderEye(context,identifier);});

	shaderManager.RegisterShader("flat",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderFlat(context,identifier);});
	shaderManager.RegisterShader("unlit",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderUnlit(context,identifier);});
	shaderManager.RegisterShader("wireframe",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderWireframe(context,identifier);});
	shaderManager.RegisterShader("texturedalphatransition",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderTexturedAlphaTransition(context,identifier);});
	shaderManager.RegisterShader("skybox",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderSkybox(context,identifier);});
	shaderManager.RegisterShader("skybox_equirect",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderSkyboxEquirect(context,identifier);});
	shaderManager.RegisterShader("loading",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderLoading(context,identifier);});
	shaderManager.RegisterShader("light_cone",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderLightCone(context,identifier);});
	shaderManager.RegisterShader("water",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderWater(context,identifier);});

	m_gameShaders.at(umath::to_integral(GameShader::Debug)) = shaderManager.RegisterShader("debug",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderDebug(context,identifier);});
	m_gameShaders.at(umath::to_integral(GameShader::DebugTexture)) = shaderManager.RegisterShader("debug_texture",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderDebugTexture(context,identifier);});
	m_gameShaders.at(umath::to_integral(GameShader::DebugVertex)) = shaderManager.RegisterShader("debug_vertex",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderDebugVertexColor(context,identifier);});
	shaderManager.RegisterShader("debug_depth_to_rgb",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderDepthToRGB(context,identifier);});
	shaderManager.RegisterShader("debug_cube_depth_to_rgb",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderCubeDepthToRGB(context,identifier);});
	shaderManager.RegisterShader("debug_csm_depth_to_rgb",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderCSMDepthToRGB(context,identifier);});

	shaderManager.RegisterShader("particle",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderParticle(context,identifier);});
	shaderManager.RegisterShader("particle_rotational",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderParticleRotational(context,identifier);});
	//shaderManager.RegisterShader("particlemodel",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderParticleModel(context,identifier);});
	shaderManager.RegisterShader("particlepolyboard",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderParticlePolyboard(context,identifier);});

	m_gameShaders.at(umath::to_integral(GameShader::Shadow)) = shaderManager.RegisterShader("shadow",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderShadow(context,identifier);});
	m_gameShaders.at(umath::to_integral(GameShader::ShadowCSM)) = shaderManager.RegisterShader("shadowcsm",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderShadowCSM(context,identifier);});
	m_gameShaders.at(umath::to_integral(GameShader::ShadowSpot)) = shaderManager.RegisterShader("shadow_spot",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderShadowSpot(context,identifier);});
	// TODO: Transparent shaders
	//shaderManager.RegisterShader("hdr",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderHDR(context,identifier);});
	
	shaderManager.RegisterShader("equirectangular_to_cubemap",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderEquirectangularToCubemap(context,identifier);});
	shaderManager.RegisterShader("cubemap_to_equirectangular",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderCubemapToEquirectangular(context,identifier);});
	shaderManager.RegisterShader("convolute_cubemap_lighting",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderConvoluteCubemapLighting(context,identifier);});
	shaderManager.RegisterShader("compute_irradiance_map_roughness",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderComputeIrradianceMapRoughness(context,identifier);});
	shaderManager.RegisterShader("brdf_convolution",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderBRDFConvolution(context,identifier);});
	shaderManager.RegisterShader("specular_to_roughness",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderSpecularToRoughness(context,identifier);});
	shaderManager.RegisterShader("extract_diffuse_ambient_occlusion",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderExtractDiffuseAmbientOcclusion(context,identifier);});
	shaderManager.RegisterShader("compose_rma",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderComposeRMA(context,identifier);});
	shaderManager.RegisterShader("specular_glossiness_to_metalness_roughness",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderSpecularGlossinessToMetalnessRoughness(context,identifier);});

	shaderManager.RegisterShader("calcimagecolor",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderCalcImageColor(context,identifier);});
	shaderManager.RegisterShader("watersplash",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderWaterSplash(context,identifier);});
	shaderManager.RegisterShader("watersurface",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderWaterSurface(context,identifier);});
	shaderManager.RegisterShader("watersurfaceintegrate",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderWaterSurfaceIntegrate(context,identifier);});
	shaderManager.RegisterShader("watersurfacesumedges",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderWaterSurfaceSumEdges(context,identifier);});
	shaderManager.RegisterShader("watersurfacesolveedges",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderWaterSurfaceSolveEdges(context,identifier);});

	shaderManager.RegisterShader("ssao",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderSSAO(context,identifier);});
	shaderManager.RegisterShader("ssao_blur",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderSSAOBlur(context,identifier);});
	shaderManager.RegisterShader("glow",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderGlow(context,identifier);});
	m_gameShaders.at(umath::to_integral(GameShader::PPTonemapping)) = shaderManager.RegisterShader("pp_hdr",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderPPHDR(context,identifier);});
	m_gameShaders.at(umath::to_integral(GameShader::PPFog)) = shaderManager.RegisterShader("pp_fog",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderPPFog(context,identifier);});
	shaderManager.RegisterShader("pp_water",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderPPWater(context,identifier);});
	m_gameShaders.at(umath::to_integral(GameShader::PPFXAA)) = shaderManager.RegisterShader("pp_fxaa",[](prosper::IPrContext &context,const std::string &identifier) {return new pragma::ShaderPPFXAA(context,identifier);});
}

void CGame::UpdateShaderTimeData()
{
	c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_globalRenderSettingsBufferData->timeBuffer,0ull,pragma::ShaderTextured3DBase::TimeData{
		static_cast<float>(CurTime()),static_cast<float>(DeltaTime()),
		static_cast<float>(RealTime()),static_cast<float>(DeltaRealTime())
	});
}
