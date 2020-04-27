/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_ENV_SHADOW_HPP__
#define __C_ENV_SHADOW_HPP__

#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/entities/components/c_entity_component.hpp"
#include "pragma/entities/game/c_game_shadow_manager.hpp"
#include <pragma/entities/components/base_entity_component.hpp>
#include <pragma/indexmap.h>

namespace prosper {class Framebuffer; class RenderPass; class RenderTarget; class Texture;};
namespace pragma
{
	class DLLCLIENT CShadowComponent final
		: public BaseEntityComponent
	{
	public:
		enum class Type : uint8_t
		{
			Generic = 1,
			Cube
		};
		static prosper::IDescriptorSet *GetDescriptorSet();

		CShadowComponent(BaseEntity &ent);
		virtual void Initialize() override;
		virtual void OnRemove() override;

		void ReloadDepthTextures();
		Type GetType() const;
		uint32_t GetLayerCount() const;
		void SetType(Type type);
		bool ShouldUpdateLayer(uint32_t layerId) const;
		void SetTextureReloadCallback(const std::function<void(void)> &f);

		prosper::RenderTarget *GetDepthRenderTarget() const;
		prosper::Texture *GetDepthTexture() const;
		prosper::IRenderPass *GetRenderPass() const;
		prosper::IFramebuffer *GetFramebuffer(uint32_t layerId=0u);

		int64_t GetLastFrameRendered() const;
		void SetLastFrameRendered(int64_t frameId);

		bool IsDirty() const;
		void SetDirty(bool dirty);

		std::weak_ptr<CShadowManagerComponent::RenderTarget> RequestRenderTarget();
		void FreeRenderTarget();
		bool HasRenderTarget() const;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
	protected:
		void DestroyTextures();
		int64_t m_lastFrameRendered = -1;
		bool m_bDirty = true;
		Type m_type = CShadowComponent::Type::Generic;
		CShadowManagerComponent::RtHandle m_hRt = {};
		std::function<void(void)> m_onTexturesReloaded = nullptr;
		void InitializeDepthTextures(uint32_t size);
	};
};

#endif
