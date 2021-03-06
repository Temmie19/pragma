/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/rendering/renderers/base_renderer.hpp"

using namespace pragma::rendering;

BaseRenderer::BaseRenderer(Scene &scene)
	: m_scene{scene}
{}
bool BaseRenderer::operator==(const BaseRenderer &other) const {return &other == this;}
bool BaseRenderer::operator!=(const BaseRenderer &other) const {return !operator==(other);}
bool BaseRenderer::RenderScene(std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd,FRender renderFlags)
{
	BeginRendering(drawCmd);
	return true;
}
void BaseRenderer::BeginRendering(std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
{
	GetScene().UpdateBuffers(drawCmd);
}
void BaseRenderer::Resize(uint32_t width, uint32_t height) {}
void BaseRenderer::UpdateRenderSettings(pragma::RenderSettings &renderSettings) {}
void BaseRenderer::UpdateCameraData(pragma::CameraData &cameraData) {}
bool BaseRenderer::IsRasterizationRenderer() const {return false;}
bool BaseRenderer::IsRayTracingRenderer() const {return false;}
prosper::Texture *BaseRenderer::GetPresentationTexture() {return GetSceneTexture();}
const prosper::Texture *BaseRenderer::GetSceneTexture() const {return const_cast<BaseRenderer*>(this)->GetSceneTexture();}
const prosper::Texture *BaseRenderer::GetPresentationTexture() const {return const_cast<BaseRenderer*>(this)->GetPresentationTexture();}
const prosper::Texture *BaseRenderer::GetHDRPresentationTexture() const {return const_cast<BaseRenderer*>(this)->GetHDRPresentationTexture();}

Scene &BaseRenderer::GetScene() const {return m_scene;}
