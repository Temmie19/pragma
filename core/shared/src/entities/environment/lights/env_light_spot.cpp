#include "stdafx_shared.h"
#include "pragma/entities/environment/lights/env_light_spot.h"
#include <sharedutils/util.h>
#include "pragma/util/util_handled.hpp"
#include "pragma/entities/baseentity_events.hpp"
#include <algorithm>

using namespace pragma;

BaseEnvLightSpotComponent::BaseEnvLightSpotComponent(BaseEntity &ent)
	: BaseEntityComponent(ent),m_angInnerCutoff(util::FloatProperty::Create(0.f)),
	m_angOuterCutoff(util::FloatProperty::Create(0.f))
{}
void BaseEnvLightSpotComponent::Initialize()
{
	BaseEntityComponent::Initialize();

	BindEvent(BaseEntity::EVENT_HANDLE_KEY_VALUE,[this](std::reference_wrapper<pragma::ComponentEvent> evData) -> util::EventReply {
		auto &kvData = static_cast<CEKeyValueData&>(evData.get());
		if(ustring::compare(kvData.key,"outercutoff",false))
			*m_angOuterCutoff = util::to_float(kvData.value);
		else if(ustring::compare(kvData.key,"innercutoff",false))
			*m_angInnerCutoff = util::to_float(kvData.value);
		else
			return util::EventReply::Unhandled;
		return util::EventReply::Handled;
	});

	auto &ent = GetEntity();
	ent.AddComponent("light");
	ent.AddComponent("radius");
}

void BaseEnvLightSpotComponent::SetOuterCutoffAngle(float ang) {*m_angOuterCutoff = ang;}
float BaseEnvLightSpotComponent::GetOuterCutoffAngle() {return *m_angOuterCutoff;}
void BaseEnvLightSpotComponent::SetInnerCutoffAngle(float ang) {*m_angInnerCutoff = ang;}
float BaseEnvLightSpotComponent::GetInnerCutoffAngle() {return *m_angInnerCutoff;}