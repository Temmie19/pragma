#include "stdafx_server.h"
#include "pragma/entities/s_baseentity.h"
#include "pragma/entities/s_entityfactories.h"
#include "pragma/audio/s_alsound.h"
#include "pragma/audio/s_alsoundscript.h"
#include <pragma/lua/luacallback.h>
#include <pragma/networking/nwm_util.h>
#include "luasystem.h"
#include "pragma/entities/parentinfo.h"
#include <pragma/lua/luafunction_call.h>
#include "pragma/entities/player.h"
#include "pragma/networking/recipient_filter.hpp"
#include "pragma/model/s_modelmanager.h"
#include "pragma/entities/components/s_entity_component.hpp"
#include "pragma/entities/components/s_player_component.hpp"
#include "pragma/entities/components/s_model_component.hpp"
#include "pragma/entities/components/s_animated_component.hpp"
#include "pragma/entities/components/s_weapon_component.hpp"
#include "pragma/entities/components/s_vehicle_component.hpp"
#include "pragma/entities/components/s_ai_component.hpp"
#include "pragma/entities/components/s_character_component.hpp"
#include "pragma/entities/components/s_physics_component.hpp"
#include "pragma/entities/components/s_time_scale_component.hpp"
#include "pragma/entities/components/s_name_component.hpp"
#include <servermanager/sv_nwm_recipientfilter.h>
#include <pragma/networking/nwm_util.h>
#include <pragma/networking/enums.hpp>
#include <sharedutils/scope_guard.h>
#include <pragma/lua/libraries/ltimer.h>
#include <pragma/physics/raytraces.h>
#include <pragma/physics/collisionmasks.h>
#include <pragma/util/bulletinfo.h>
#include <pragma/audio/alsound_type.h>
#include <pragma/entities/components/base_entity_component.hpp>
#include <pragma/entities/components/base_actor_component.hpp>
#include <pragma/entities/components/base_player_component.hpp>
#include <pragma/entities/components/base_character_component.hpp>
#include <pragma/entities/entity_component_system_t.hpp>

extern EntityClassMap<SBaseEntity> *g_ServerEntityFactories;
extern ServerEntityNetworkMap *g_SvEntityNetworkMap;

extern DLLENGINE Engine *engine;
extern ServerState *server;
extern SGame *s_game;

#pragma optimize("",off)
SBaseEntity::SBaseEntity()
	: BaseEntity(),
	m_bShared(false),
	m_bSynchronized(true)
{}

void SBaseEntity::DoSpawn()
{
	BaseEntity::DoSpawn();
	Game *game = server->GetGameState();
	game->SpawnEntity(this);
}

BaseEntity *SBaseEntity::GetClientsideEntity() const
{
	if(IsShared() == false)
		return nullptr;
	auto *clState = engine->GetClientState();
	if(clState == nullptr)
		return nullptr;
	auto *game = clState->GetGameState();
	if(game == nullptr)
		return nullptr;
	return game->GetEntity(GetIndex());
}

Bool SBaseEntity::IsSynchronized() const {return (IsShared() && m_bSynchronized) ? true : false;}
void SBaseEntity::SetSynchronized(Bool b) {m_bSynchronized = b;}

void SBaseEntity::Initialize()
{
	BaseEntity::Initialize();

	SGame *game = server->GetGameState();
	lua_State *lua = game->GetLuaState();
	InitializeLuaObject(lua);
	g_ServerEntityFactories->GetClassName(typeid(*this),&m_class);
	unsigned int ID = g_SvEntityNetworkMap->GetFactoryID(typeid(*this));
	if(ID == 0)
		return;
	m_bShared = true;
}
bool SBaseEntity::IsShared() const {return m_bShared;}
void SBaseEntity::SetShared(bool b) {m_bShared = b;}
Bool SBaseEntity::IsNetworked() {return (IsShared() && IsSpawned()) ? true : false;}

bool SBaseEntity::IsServersideOnly() const {return IsShared() == false;}
bool SBaseEntity::IsNetworkLocal() const {return IsServersideOnly();}

void SBaseEntity::SendData(NetPacket &packet,pragma::networking::ClientRecipientFilter &rp)
{
	packet->Write<uint64_t>(GetUniqueIndex());
	packet->Write<uint32_t>(GetSpawnFlags());

	auto &componentManager = s_game->GetEntityComponentManager();
	auto &components = GetComponents();
	auto offset = packet->GetOffset();
	auto numComponents = umath::min(components.size(),static_cast<size_t>(std::numeric_limits<uint8_t>::max()));
	packet->Write<uint8_t>(numComponents);
	for(auto &pComponent : components)
	{
		if(pComponent->ShouldTransmitNetData() == false)
		{
			--numComponents;
			continue;
		}
		auto *pNetComponent = dynamic_cast<pragma::SBaseNetComponent*>(pComponent.get());
		if(pNetComponent == nullptr)
		{
			throw std::logic_error("Component must be derived from SBaseNetComponent if net data is enabled!");
			continue;
		}
		auto id = pComponent->GetComponentId();
		packet->Write<pragma::ComponentId>(id);
		auto szComponent = 0u;
		auto offset = packet->GetOffset();
		packet->Write<uint8_t>(static_cast<uint8_t>(0u));
		pNetComponent->SendData(packet,rp);
		szComponent = packet->GetOffset() -(offset +sizeof(uint8_t));
		if(szComponent > std::numeric_limits<uint8_t>::max())
			throw std::runtime_error("Component size mustn't exceed " +std::to_string(std::numeric_limits<uint8_t>::max()) +" bytes!");
		packet->Write<uint8_t>(szComponent,&offset);
	}
	packet->Write<uint8_t>(numComponents,&offset);
}

void SBaseEntity::SendSnapshotData(NetPacket&,pragma::BasePlayerComponent&) {}

pragma::NetEventId SBaseEntity::RegisterNetEvent(const std::string &name) const
{
	return static_cast<SGame*>(GetNetworkState()->GetGameState())->RegisterNetEvent(name);
}

void SBaseEntity::Remove()
{
	BaseEntity::Remove();
	Game *game = server->GetGameState();
	game->RemoveEntity(this);
}

NetworkState *SBaseEntity::GetNetworkState() const {return server;}

void SBaseEntity::EraseFunction(int function)
{
	Game *game = server->GetGameState();
	lua_removereference(game->GetLuaState(),function);
}

void SBaseEntity::SendNetEvent(pragma::NetEventId eventId,NetPacket &packet,pragma::networking::Protocol protocol,const pragma::networking::ClientRecipientFilter &rf)
{
	if(!IsShared() || !IsSpawned())
		return;
	nwm::write_entity(packet,this);
	packet->Write<UInt32>(eventId);
	server->SendPacket("ent_event",packet,protocol,rf);
}
void SBaseEntity::SendNetEvent(pragma::NetEventId eventId,NetPacket &packet,pragma::networking::Protocol protocol)
{
	if(!IsShared() || !IsSpawned())
		return;
	SendNetEvent(eventId,packet,protocol,pragma::networking::ClientRecipientFilter{});
}
void SBaseEntity::SendNetEvent(pragma::NetEventId eventId,NetPacket &packet)
{
	if(!IsShared() || !IsSpawned())
		return;
	SendNetEvent(eventId,packet,pragma::networking::Protocol::FastUnreliable);
}
void SBaseEntity::SendNetEvent(pragma::NetEventId eventId,pragma::networking::Protocol protocol)
{
	if(!IsShared() || !IsSpawned())
		return;
	SendNetEvent(eventId,NetPacket{},protocol);
}
Bool SBaseEntity::ReceiveNetEvent(pragma::BasePlayerComponent &pl,pragma::NetEventId eventId,NetPacket &packet)
{
	for(auto &pComponent : GetComponents())
	{
		auto *pNetComponent = dynamic_cast<pragma::SBaseNetComponent*>(pComponent.get());
		if(pNetComponent == nullptr)
			continue;
		if(pNetComponent->ReceiveNetEvent(pl,eventId,packet))
			return true;
	}
	Con::csv<<"WARNING: Unhandled net event '"<<eventId<<"' for entity "<<GetClass()<<Con::endl;
	return false;
}

util::WeakHandle<pragma::BaseModelComponent> SBaseEntity::GetModelComponent() const
{
	auto pComponent = GetComponent<pragma::SModelComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BaseModelComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BaseModelComponent>{};
}
util::WeakHandle<pragma::BaseAnimatedComponent> SBaseEntity::GetAnimatedComponent() const
{
	auto pComponent = GetComponent<pragma::SAnimatedComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BaseAnimatedComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BaseAnimatedComponent>{};
}
util::WeakHandle<pragma::BaseWeaponComponent> SBaseEntity::GetWeaponComponent() const
{
	auto pComponent = GetComponent<pragma::SWeaponComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BaseWeaponComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BaseWeaponComponent>{};
}
util::WeakHandle<pragma::BaseVehicleComponent> SBaseEntity::GetVehicleComponent() const
{
	auto pComponent = GetComponent<pragma::SVehicleComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BaseVehicleComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BaseVehicleComponent>{};
}
util::WeakHandle<pragma::BaseAIComponent> SBaseEntity::GetAIComponent() const
{
	auto pComponent = GetComponent<pragma::SAIComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BaseAIComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BaseAIComponent>{};
}
util::WeakHandle<pragma::BaseCharacterComponent> SBaseEntity::GetCharacterComponent() const
{
	auto pComponent = GetComponent<pragma::SCharacterComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BaseCharacterComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BaseCharacterComponent>{};
}
util::WeakHandle<pragma::BasePlayerComponent> SBaseEntity::GetPlayerComponent() const
{
	auto pComponent = GetComponent<pragma::SPlayerComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BasePlayerComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BasePlayerComponent>{};
}
util::WeakHandle<pragma::BasePhysicsComponent> SBaseEntity::GetPhysicsComponent() const
{
	auto pComponent = GetComponent<pragma::SPhysicsComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BasePhysicsComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BasePhysicsComponent>{};
}
util::WeakHandle<pragma::BaseTimeScaleComponent> SBaseEntity::GetTimeScaleComponent() const
{
	auto pComponent = GetComponent<pragma::STimeScaleComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BaseTimeScaleComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BaseTimeScaleComponent>{};
}
util::WeakHandle<pragma::BaseNameComponent> SBaseEntity::GetNameComponent() const
{
	auto pComponent = GetComponent<pragma::SNameComponent>();
	return pComponent.valid() ? std::static_pointer_cast<pragma::BaseNameComponent>(pComponent->shared_from_this()) : util::WeakHandle<pragma::BaseNameComponent>{};
}
bool SBaseEntity::IsCharacter() const {return HasComponent<pragma::SCharacterComponent>();}
bool SBaseEntity::IsPlayer() const {return HasComponent<pragma::SPlayerComponent>();}
bool SBaseEntity::IsWeapon() const {return HasComponent<pragma::SWeaponComponent>();}
bool SBaseEntity::IsVehicle() const {return HasComponent<pragma::SVehicleComponent>();}
bool SBaseEntity::IsNPC() const {return HasComponent<pragma::SAIComponent>();}
#pragma optimize("",on)
