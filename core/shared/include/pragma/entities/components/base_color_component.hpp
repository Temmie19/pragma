#ifndef __BASE_COLOR_COMPONENT_HPP__
#define __BASE_COLOR_COMPONENT_HPP__

#include "pragma/entities/components/base_entity_component.hpp"
#include <sharedutils/property/util_property_color.hpp>

namespace pragma
{
	struct DLLNETWORK CEOnColorChanged
		: public ComponentEvent
	{
		CEOnColorChanged(const Color &oldColor,const Color &color);
		virtual void PushArguments(lua_State *l) override;
		const Color &oldColor;
		const Color &color;
	};
	class DLLNETWORK BaseColorComponent
		: public BaseEntityComponent
	{
	public:
		static pragma::ComponentEventId EVENT_ON_COLOR_CHANGED;
		static void RegisterEvents(pragma::EntityComponentManager &componentManager);
		virtual void Initialize() override;
		const Color &GetColor() const;
		const util::PColorProperty &GetColorProperty() const;

		virtual ~BaseColorComponent() override;
		void SetColor(const Color &color);
		void SetColor(const Vector4 &color);
	protected:
		BaseColorComponent(BaseEntity &ent);
		util::PColorProperty m_color;
		CallbackHandle m_cbOnColorChanged = {};
		pragma::NetEventId m_netEvSetColor = pragma::INVALID_NET_EVENT;
	};
};

#endif
