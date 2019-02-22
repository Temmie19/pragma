#include "stdafx_client.h"
#include "pragma/entities/environment/audio/c_env_sound_probe.hpp"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/util/util_handled.hpp>
#include <pragma/entities/baseentity_events.hpp>
#include <pragma/entities/components/base_transform_component.hpp>
#include <pragma/entities/components/base_physics_component.hpp>

using namespace pragma;

extern DLLCLIENT ClientState *client;

LINK_ENTITY_TO_CLASS(env_sound_probe,CEnvSoundProbe);

decltype(CEnvSoundProbeComponent::s_probes) CEnvSoundProbeComponent::s_probes = {};
decltype(CEnvSoundProbeComponent::s_probeCallback) CEnvSoundProbeComponent::s_probeCallback = {};
void CEnvSoundProbeComponent::ClearProbes() {s_probes = {};}
const std::vector<CEnvSoundProbeComponent::ProbeInfo> &CEnvSoundProbeComponent::GetProbes() {return s_probes;}

void CEnvSoundProbeComponent::Initialize()
{
	BaseEntityComponent::Initialize();

	BindEvent(BaseEntity::EVENT_HANDLE_KEY_VALUE,[this](std::reference_wrapper<pragma::ComponentEvent> evData) -> util::EventReply {
		auto &kvData = static_cast<CEKeyValueData&>(evData.get());
		if(ustring::compare(kvData.key,"spacing",false))
			m_spacing = util::to_float(kvData.value);
		else if(ustring::compare(kvData.key,"height_above_floor",false))
			m_heightAboveFloor = util::to_float(kvData.value);
		else if(ustring::compare(kvData.key,"radius",false))
			m_radius = util::to_float(kvData.value);
		else
			return util::EventReply::Unhandled;
		return util::EventReply::Handled;
	});
}
void CEnvSoundProbeComponent::OnEntitySpawn()
{
	BaseEntityComponent::OnEntitySpawn();
	if(s_probes.size() == s_probes.capacity())
		s_probes.reserve(s_probes.size() +50);

	auto &ent = GetEntity();
	auto pTrComponent = ent.GetTransformComponent();
	if(pTrComponent.expired())
		return;
	auto &origin = pTrComponent->GetPosition();
	auto mdlComponent = ent.GetModelComponent();
	if(mdlComponent.expired() || mdlComponent->GetModel() == nullptr)
	{
		// Use radius
		s_probes.push_back({origin,origin,Placement::Centroid,m_radius,0.f});
		return;
	}
	auto pPhysComponent = ent.GetPhysicsComponent();
	Vector3 min {};
	Vector3 max {};
	if(pPhysComponent.valid())
		pPhysComponent->GetCollisionBounds(&min,&max);
	s_probes.push_back({origin +min,origin +max,Placement::UniformFloor,m_spacing,m_heightAboveFloor});
	if(s_probeCallback.IsValid() == false)
	{
		s_probeCallback = client->AddCallback("EndGame",FunctionCallback<void,CGame*>::Create([](CGame *game) {
			CEnvSoundProbeComponent::ClearProbes();
			s_probeCallback.Remove();
		}));
	}
	ent.RemoveSafely();
}
luabind::object CEnvSoundProbeComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CEnvSoundProbeComponentHandleWrapper>(l);}

////////////

void CEnvSoundProbe::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CEnvSoundProbeComponent>();
}
