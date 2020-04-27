/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer */

#ifndef __BASE_RENDER_COMPONENT_HPP__
#define __BASE_RENDER_COMPONENT_HPP__

#include "pragma/entities/components/base_entity_component.hpp"

namespace pragma
{
	enum class FRenderFlags : uint32_t;
	class DLLNETWORK BaseRenderComponent
		: public BaseEntityComponent
	{
	public:
		using BaseEntityComponent::BaseEntityComponent;
		virtual void Initialize() override;
		virtual void SetUnlit(bool b);
		virtual void SetCastShadows(bool b);
		bool IsUnlit() const;
		bool GetCastShadows() const;

		virtual void Save(DataStream &ds) override;
		virtual void Load(DataStream &ds,uint32_t version) override;
	protected:
		FRenderFlags m_renderFlags = static_cast<FRenderFlags>(0u);
	};
};

#endif
