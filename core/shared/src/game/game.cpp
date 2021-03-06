/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_shared.h"
#include <pragma/game/game.h>
#include "pragma/networkstate/networkstate.h"
#include <pragma/console/convars.h>
#include "luasystem.h"
#include "pragma/physics/physobj.h"
#include "pragma/entities/baseentity.h"
#include "pragma/model/brush/brushmesh.h"
#include "pragma/level/mapgeometry.h"
#include <pragma/engine.h>
#include "pragma/ai/navsystem.h"
#include "pragma/physics/environment.hpp"
#include "pragma/physics/contact.hpp"
#include "pragma/physics/constraint.hpp"
#include "pragma/lua/libraries/ltimer.h"
#include "pragma/game/gamemode/gamemodemanager.h"
#include <pragma/console/convars.h>
#include "pragma/console/engine_cvar.h"
#include "pragma/game/game_callback.h"
#include "pragma/lua/luafunction_call.h"
#include "pragma/addonsystem/addonsystem.h"
#include "pragma/lua/lua_script_watcher.h"
#include "pragma/util/resource_watcher.h"
#include "pragma/game/game_resources.hpp"
#include "pragma/input/inkeys.h"
#include "pragma/entities/trigger/base_trigger_touch.hpp"
#include "pragma/entities/components/base_player_component.hpp"
#include "pragma/entities/entity_component_manager.hpp"
#include "pragma/entities/prop/prop_base.h"
#include "pragma/entities/components/base_physics_component.hpp"
#include "pragma/entities/components/base_ai_component.hpp"
#include "pragma/entities/baseworld.h"
#include "pragma/entities/entity_component_system_t.hpp"
#include "pragma/level/level_info.hpp"
#include "pragma/entities/components/logic_component.hpp"
#include "pragma/lua/sh_lua_component.hpp"
#include "pragma/lua/class_manager.hpp"
#include "pragma/util/util_bsp_tree.hpp"
#include "pragma/entities/entity_iterator.hpp"
#include "pragma/asset_types/world.hpp"
#include "pragma/model/model.h"
#include "pragma/model/modelmanager.h"
#include "pragma/physics/collisionmesh.h"
#include <sharedutils/util_library.hpp>
#include <luainterface.hpp>


DLLNETWORK void BuildDisplacementTriangles(std::vector<Vector3> &sideVerts,unsigned int start,
	Vector3 &nu,Vector3 &nv,float sw,float sh,float ou,float ov,float su,float sv,
	unsigned char power,std::vector<std::vector<Vector3>> &normals,std::vector<std::vector<Vector3>> &offsets,std::vector<std::vector<float>> &distances,unsigned char numAlpha,std::vector<std::vector<Vector2>> &alphas,
	std::vector<Vector3> &outVertices,std::vector<Vector3> &outNormals,std::vector<Vector2> &outUvs,std::vector<unsigned int> &outTriangles,std::vector<Vector2> *outAlphas)
{
	int rows = umath::pow(2,CInt32(power)) +1;
	unsigned int numVerts = rows *rows;
	outNormals.resize(numVerts);
	outUvs.resize(numVerts);
	if(numAlpha > 0)
		outAlphas->resize(numVerts);
	Vector3 sortedSideVerts[4] = {{},{},{},{}};
	char j = 0;
	for(auto i=start;i<CUInt32(sideVerts.size());i++)
	{
		sortedSideVerts[j] = sideVerts[i];
		j++;
	}
	for(unsigned int i=0;i<start;i++)
	{
		sortedSideVerts[j] = sideVerts[i];
		j++;
	}
	Vector3 &x1 = sortedSideVerts[0];
	Vector3 &x2 = sortedSideVerts[1];
	Vector3 &y1 = sortedSideVerts[3];
	Vector3 &y2 = sortedSideVerts[2];
	Vector3 xOffset1 = (x2 -x1) /float(rows -1);
	Vector3 xOffset2 = (y2 -y1) /float(rows -1);
	Vector3 yOffset = (y1 -x1) /float(rows -1); // CHECKME
	outVertices.resize(numVerts);
	Vector3 cur = sortedSideVerts[0];
	for(int col=0;col<rows;col++)
	{
		std::vector<Vector3> &cNormals = normals[col];
		std::vector<Vector2> *cAlphas = (numAlpha > 0) ? &alphas[col] : NULL;
		std::vector<float> &cDistances = distances[col];
		std::vector<Vector3> &cOffsets = offsets[col];

		Vector3 rowPos = cur;
		float offsetScale = col /float(rows -1);
		for(int row=0;row<rows;row++)
		{
			unsigned int idx = col *rows +row;
			//outNormals[idx] = -cNormals[row]; // This is the offset normal, not the actual face normal!
			if(numAlpha > 0)
				(*outAlphas)[idx] = (*cAlphas)[row];

			Vector3 vA = rowPos +cNormals[row] *cDistances[row] +cOffsets[row];
			Vector2 uv;
			uv.x = (glm::dot(rowPos,nu) *sw) /su +ou *sw;
			uv.y = (glm::dot(rowPos,nv) *sh) /sv +ov *sh;
			outUvs[idx] = uv;
			outVertices[idx] = vA;

			Vector3 xOffset = (1.f -offsetScale) *xOffset1 +offsetScale *xOffset2;
			rowPos += xOffset;
		}
		cur = sortedSideVerts[0] +(yOffset *float(col +1));
		//if(col < rows -1)
		//	cur += yOffset +normals[col +1][0] *distances[col +1][0]; // +cNormals[0] *cDistances[0]; TODO!! -> Next column!
	}
	outTriangles.resize((rows -1) *(rows -1) *6);
	unsigned int idx = 0;
	std::vector<Vector3> faceNormals;
	faceNormals.reserve(umath::pow(rows -1,2) *2);
	for(int col=0;col<rows -1;col++)
	{
		for(int row=0;row<rows -1;row++)
		{
			int a = col *rows +row;
			int b = a +1;
			int c = (col +1) *rows +row;
			outTriangles[idx] = a;
			outTriangles[idx +1] = b;
			outTriangles[idx +2] = c;

			auto na = -uvec::cross(outVertices[c] -outVertices[a],outVertices[b] -outVertices[a]);
			uvec::normalize(&na);
			faceNormals.push_back(na);

			idx += 3;
			int d = (col +1) *rows +row +1;
			outTriangles[idx] = b;
			outTriangles[idx +1] = d;
			outTriangles[idx +2] = c;

			auto nb = -uvec::cross(outVertices[c] -outVertices[b],outVertices[d] -outVertices[b]);
			uvec::normalize(&nb);
			faceNormals.push_back(nb);

			idx += 3;
		}
	}
	// Calculate Vertex Normals
	auto up = Vector3(0.f,1.f,0.f);
	for(auto col=0;col<rows;col++)
	{
		for(auto row=0;row<rows;row++)
		{
			auto vertId = col *rows +row;
			auto a = (col > 0 && row > 0) ?
				((col -1) *(rows -1) +(row -1))
				: -1;
			auto b = (row > 0 && col < (rows -1)) ?
				(col *(rows -1) +(row -1))
				: -1;
			auto c = (col > 0) ?
				((col -1) *(rows -1) +row)
				: -1;
			auto d = (col < (rows -1)) ?
				(col *(rows -1) +row)
				: -1;

			auto &na1 = (a >= 0) ? faceNormals[a *2 +1] : up;
			auto &na2 = na1;
			auto &nb1 = (b >= 0) ? faceNormals[b *2] : up;
			auto &nb2 = (b >= 0) ? faceNormals[b *2 +1] : up;
			auto &nc1 = (c >= 0) ? faceNormals[c *2] : up;
			auto &nc2 = (c >= 0) ? faceNormals[c *2 +1] : up;
			auto &nd1 = (c >= 0) ? faceNormals[d *2] : up;
			auto &nd2 = nd1;

			auto &n = outNormals[vertId] = (na1 +na2 +nb1 +nb2 +nc1 +nc2 +nd1 +nd2) /8.f;
			uvec::normalize(&n);
		}
	}
	//
}

DLLNETWORK void ToTriangles(const std::vector<Vector3> &vertices,std::vector<uint16_t> &outTriangles)
{
	size_t pivot = 0;
	//Vector3 &va = (*vertices)[pivot];
	auto numVerts = vertices.size();
	if(numVerts == 0)
		return;
	auto numVals = (numVerts -2) *3;
	outTriangles.resize(numVals);
	auto idx = 0;
	for(auto i=pivot +2;i<numVerts;i++)
	{
		outTriangles[idx] = static_cast<uint16_t>(pivot);
		outTriangles[idx +1] = static_cast<uint16_t>(i -1);
		outTriangles[idx +2] = static_cast<uint16_t>(i);
		idx += 3;
	}
}

DLLNETWORK void Lua::VarDump(lua_State *lua,int n)
{
	auto t = GetType(lua,n);
	switch(t)
	{
		case Lua::Type::None:
			Con::cout<<"none";
			break;
		case Lua::Type::Nil:
			Con::cout<<"nil";
			break;
		case Lua::Type::Bool:
			Con::cout<<"bool ("<<(ToBool(lua,n) ? "true" : "false")<<")";
			break;
		case Lua::Type::LightUserData:
			Con::cout<<"lightuserdata";
			break;
		case Lua::Type::Number:
			Con::cout<<"number ("<<ToNumber(lua,n)<<")";
			break;
		case Lua::Type::String:
			Con::cout<<"string ("<<ToString(lua,n)<<")";
			break;
		case Lua::Type::Table:
			Con::cout<<"table";
			break;
		case Lua::Type::Function:
			Con::cout<<"function";
			break;
		case Lua::Type::UserData:
			Con::cout<<"userdata";
			break;
		case Lua::Type::Thread:
			Con::cout<<"thread";
			break;
		default:
			Con::cout<<"other ("<<GetTypeName(lua,n)<<")";
	}
}

DLLNETWORK void Lua::StackDump(lua_State *lua)
{
    int top = GetStackTop(lua);
	Con::cout<<"------------ LUA STACKDUMP ------------"<<Con::endl;
	Con::cout<<"Values in stack: "<<top<<Con::endl;
	for(int i=1;i<=top;i++)
	{
		Con::cout<<"\t"<<i<<": ";
		VarDump(lua,i);
		Con::cout<<Con::endl;
	}
	Con::cout<<"---------------------------------------"<<Con::endl;
}

DLLNETWORK void Lua::TableDump(lua_State *lua,int n)
{
	if(n < 0)
		n = Lua::GetStackTop(lua) +n +1;
	Con::cout<<"------------ LUA TABLEDUMP ------------"<<Con::endl;
	if(n <= 0)
	{
		Con::cout<<"INVALID STACK INDEX ("<<n<<")"<<Con::endl;
		return;
	}
	if(!Lua::IsTable(lua,n))
	{
		Con::cout<<"VALUE "<<n<<" ON STACK IS A ";
		VarDump(lua,n);
		Con::cout<<", NOT A TABLE!"<<Con::endl;
		return;
	}
	Lua::PushNil(lua);
	while(Lua::GetNextPair(lua,n) != 0)
	{
		Con::cout<<"\t";
		VarDump(lua,-2);
		Con::cout<<" = ";
		VarDump(lua,-1);
		Con::cout<<Con::endl;
		Lua::Pop(lua,1); // We need the key at the top for the next iteration
	}
	Con::cout<<"---------------------------------------"<<Con::endl;
}

////////////////

extern DLLENGINE Engine *engine;
Game::Game(NetworkState *state)
{
	m_stateNetwork = state;
	m_mapInfo.name = "";
	m_mapInfo.md5 = "";
	m_luaNetMessageIndex.push_back("invalid");
	m_luaEnts = std::make_unique<LuaEntityManager>();
	m_ammoTypes = std::make_unique<AmmoTypeManager>();

	RegisterCallback<void>("Tick");
	RegisterCallback<void>("Think");

	RegisterCallback<void,lua_State*>("OnLuaReleased");
	RegisterCallback<void,pragma::BasePlayerComponent*>("OnPlayerReady");
	RegisterCallback<void,pragma::BasePlayerComponent*,pragma::networking::DropReason>("OnPlayerDropped");
	RegisterCallback<void,pragma::BasePlayerComponent*>("OnPlayerJoined");
	RegisterCallback<void,BaseEntity*>("OnEntityCreated");
	RegisterCallback<void>("PrePhysicsSimulate");
	RegisterCallback<void>("PostPhysicsSimulate");

	RegisterCallback<void,BaseEntity*,uint16_t,uint16_t>("OnEntityHealthChanged");
	RegisterCallback<void,BaseEntity*,std::reference_wrapper<DamageInfo>>("OnEntityTakeDamage");
	RegisterCallback<void,BaseEntity*,std::reference_wrapper<DamageInfo>,uint16_t,uint16_t>("OnEntityTakenDamage");
	RegisterCallback<void,pragma::BaseAIComponent*,DamageInfo*>("OnNPCDeath");

	RegisterCallback<void,pragma::BasePlayerComponent*,DamageInfo*>("OnPlayerDeath");
	RegisterCallback<void,pragma::BasePlayerComponent*>("OnPlayerSpawned");

	RegisterCallbackWithOptionalReturn<bool,pragma::BasePlayerComponent*,Action,bool>("OnActionInput");

	RegisterCallback<void,lua_State*>("OnLuaInitialized");
	RegisterCallback<void,BaseEntity*>("OnEntitySpawned");
	RegisterCallback<void,Game*>("OnGameInitialized");
	RegisterCallback<void>("OnMapLoaded");
	RegisterCallback<void>("OnPreLoadMap");
	RegisterCallback<void>("OnGameReady");
	RegisterCallback<void,ALSound*>("OnSoundCreated");
	RegisterCallback<void,std::reference_wrapper<std::shared_ptr<Model>>>("OnModelLoaded");

	RegisterCallback<void>("EndGame");

	LoadSoundScripts("game_sounds_generic.txt");
	LoadSoundScripts("fx_physics_impact.txt");
}

Game::~Game() {}

void Game::OnRemove()
{
	pragma::BaseAIComponent::ReleaseNavThread();
	CallCallbacks<void>("OnLuaReleased",GetLuaState());
	m_luaCallbacks.clear();
	m_luaGameMode = nullptr;
	m_luaEnts = nullptr;
	m_componentManager = nullptr;
	ClearTimers(); // Timers have to be removed before the lua state is closed
	m_cvarCallbacks.clear();

	// We have to clear all of the collision mesh shapes, because they may
	// contain luabind objects, which have to be destroyed before we destroy the
	// lua state.
	// TODO: Implement this properly!
	for(auto &pair : GetNetworkState()->GetCachedModels())
	{
		for(auto &colMesh : pair.second->GetCollisionMeshes())
			colMesh->ClearShape();
	}

	auto *state = m_lua->GetState();
	auto identifier = m_lua->GetIdentifier();
	GetNetworkState()->ClearConsoleCommandOverrides();
	GetNetworkState()->TerminateLuaModules(state);
	pragma::BaseLuaBaseEntityComponent::ClearMembers(state);
	m_surfaceMaterialManager = nullptr; // Has to be destroyed before physics environment!
	m_physEnvironment = nullptr; // Physics environment has to be destroyed before the Lua state! (To make sure Lua-handles are destroyed)
	m_luaClassManager = nullptr;
	m_lua = nullptr;
	GetNetworkState()->DeregisterLuaModules(state,identifier); // Has to be called AFTER Lua instance has been released!
	if(m_cbProfilingHandle.IsValid())
		m_cbProfilingHandle.Remove();
}

void Game::OnMapLoaded() {}

void Game::InitializeLuaScriptWatcher()
{
	m_scriptWatcher = std::make_unique<LuaDirectoryWatcherManager>(this);
}

luabind::object *Game::GetGameModeLuaObject() {return m_luaGameMode.get();}
bool Game::IsGameInitialized() const {return (m_flags &GameFlags::GameInitialized) != GameFlags::None;}
bool Game::IsMapLoaded() const {return (m_flags &GameFlags::MapLoaded) != GameFlags::None;}

void Game::OnPlayerReady(pragma::BasePlayerComponent &pl)
{
	CallCallbacks<void,pragma::BasePlayerComponent*>("OnPlayerReady",&pl);
	CallLuaCallbacks<void,luabind::object>("OnPlayerReady",pl.GetLuaObject());
}

void Game::OnPlayerDropped(pragma::BasePlayerComponent &pl,pragma::networking::DropReason reason)
{
	CallCallbacks<void,pragma::BasePlayerComponent*,pragma::networking::DropReason>("OnPlayerDropped",&pl,reason);
	CallLuaCallbacks<void,luabind::object,std::underlying_type_t<decltype(reason)>>("OnPlayerDropped",pl.GetLuaObject(),umath::to_integral(reason));
}

void Game::OnPlayerJoined(pragma::BasePlayerComponent &pl)
{
	CallCallbacks<void,pragma::BasePlayerComponent*>("OnPlayerJoined",&pl);
	CallLuaCallbacks<void,luabind::object>("OnPlayerJoined",pl.GetLuaObject());
}

unsigned char Game::GetPlayerCount() {return m_numPlayers;}

LuaEntityManager &Game::GetLuaEntityManager() {return *m_luaEnts.get();}

const GameModeInfo *Game::GetGameMode() const {return const_cast<Game*>(this)->GetGameMode();}
GameModeInfo *Game::GetGameMode() {return m_gameMode;}
void Game::SetGameMode(const std::string &gameMode)
{
	auto *info = GameModeManager::GetGameModeInfo(gameMode);
	m_gameMode = info;
}

void Game::SetupEntity(BaseEntity*)
{}

bool Game::IsMultiPlayer() const {return engine->IsMultiPlayer();}
bool Game::IsSinglePlayer() const {return engine->IsSinglePlayer();}

const pragma::physics::IEnvironment *Game::GetPhysicsEnvironment() const {return const_cast<Game*>(this)->GetPhysicsEnvironment();}
pragma::physics::IEnvironment *Game::GetPhysicsEnvironment() {return m_physEnvironment.get();}

void Game::OnEntityCreated(BaseEntity *ent)
{
	CallCallbacks<void,BaseEntity*>("OnEntityCreated",ent);
	auto *o = ent->GetLuaObject();
	CallLuaCallbacks<void,luabind::object>("OnEntityCreated",*o);
}

Vector3 &Game::GetGravity() {return m_gravity;}
void Game::SetGravity(Vector3 &gravity) {m_gravity = gravity;}

std::vector<std::string> *Game::GetLuaNetMessageIndices() {return &m_luaNetMessageIndex;}

LuaDirectoryWatcherManager &Game::GetLuaScriptWatcher() {return *m_scriptWatcher;}
ResourceWatcherManager &Game::GetResourceWatcher() {return GetNetworkState()->GetResourceWatcher();}

const std::shared_ptr<pragma::nav::Mesh> &Game::GetNavMesh() const {return const_cast<Game*>(this)->GetNavMesh();}
std::shared_ptr<pragma::nav::Mesh> &Game::GetNavMesh() {return m_navMesh;}

std::shared_ptr<pragma::nav::Mesh> Game::LoadNavMesh(const std::string &fname) {return pragma::nav::Mesh::Load(*this,fname);}

bool Game::LoadNavMesh(bool bReload)
{
	if(m_navMesh != nullptr)
	{
		if(bReload == false)
			return true;
		m_navMesh = nullptr;
	}
	std::string path = "maps\\";
	path += GetMapName();
	path += ".wnav";
	Con::cout<<"Loading navigation mesh..."<<Con::endl;

	m_navMesh = LoadNavMesh(path);
	if(m_navMesh == nullptr)
		Con::cwar<<"WARNING: Unable to load navigation mesh!"<<Con::endl;
	pragma::BaseAIComponent::ReloadNavThread(*this);
	return m_navMesh != nullptr;
}

void Game::Initialize()
{
	m_componentManager = InitializeEntityComponentManager();
	InitializeEntityComponents(*m_componentManager);
	InitializeLuaScriptWatcher();
	m_scriptWatcher->MountDirectory("lua");
	auto &addons = AddonSystem::GetMountedAddons();
	for(auto &info : addons)
		m_scriptWatcher->MountDirectory(info.GetAbsolutePath() +"/lua",true);

	LoadSoundScripts("fx.txt");
}
void Game::SetUp() {}

class PhysEventCallback
	: public pragma::physics::IEventCallback
{
public:
	// Called if contact report is enabled for a collision object and it
	// collided with another actor. This is NOT called for triggers!
	virtual void OnContact(const pragma::physics::ContactInfo &contactInfo) override
	{
		if(contactInfo.collisionObj0.IsExpired() || contactInfo.collisionObj1.IsExpired())
			return;
		auto *physObj0 = contactInfo.collisionObj0->GetPhysObj();
		auto *entC0 = physObj0 ? physObj0->GetOwner() : nullptr;
		auto *touchC0 = entC0 ? static_cast<pragma::BaseTouchComponent*>(entC0->GetEntity().FindComponent("touch").get()) : nullptr;
		if(touchC0 == nullptr)
			return;
		touchC0->Contact(contactInfo);
	}

	// Called whenever a constraint is broken
	virtual void OnConstraintBroken(pragma::physics::IConstraint &constraint) override
	{
		auto *ent = constraint.GetEntity();
		if(ent == nullptr)
			return;
		// TODO: Check constraint component
	}

	// Called when an actor has started touching another for the first time.
	virtual void OnStartTouch(pragma::physics::ICollisionObject &o0,pragma::physics::ICollisionObject &o1) override
	{
		// TODO: TocuhComponent:SetTouchEnabled -> Rely to physicxs
		// TODO: Make sure EndTouch is called when actor is removed and still touching!
		auto *physObj0 = o0.GetPhysObj();
		auto *entC0 = physObj0 ? physObj0->GetOwner() : nullptr;
		auto *touchC0 = entC0 ? static_cast<pragma::BaseTouchComponent*>(entC0->GetEntity().FindComponent("touch").get()) : nullptr;

		auto *physObj1 = o1.GetPhysObj();
		auto *entC1 = physObj1 ? physObj1->GetOwner() : nullptr;
		if(touchC0 == nullptr || entC1 == nullptr)
			return;
		touchC0->StartTouch(entC1->GetEntity(),*physObj1,o0,o1);
	}

	// Called when an actor has stopped touching another actor.
	virtual void OnEndTouch(pragma::physics::ICollisionObject &o0,pragma::physics::ICollisionObject &o1) override
	{
		auto *physObj0 = o0.GetPhysObj();
		auto *entC0 = physObj0 ? physObj0->GetOwner() : nullptr;
		auto *touchC0 = entC0 ? static_cast<pragma::BaseTouchComponent*>(entC0->GetEntity().FindComponent("touch").get()) : nullptr;

		auto *physObj1 = o1.GetPhysObj();
		auto *entC1 = physObj1 ? physObj1->GetOwner() : nullptr;
		if(touchC0 == nullptr || entC1 == nullptr)
			return;
		touchC0->EndTouch(entC1->GetEntity(),*physObj1,o0,o1);
	}

	virtual void OnWake(pragma::physics::ICollisionObject &o) override
	{
		auto *physObj = o.GetPhysObj();
		auto *ent = physObj ? physObj->GetOwner() : nullptr;
		auto *physC = ent ? ent->GetEntity().GetPhysicsComponent().get() : nullptr;
		if(physC == nullptr)
			return;
		physC->OnWake();
	}
	virtual void OnSleep(pragma::physics::ICollisionObject &o) override
	{
		auto *physObj = o.GetPhysObj();
		auto *ent = physObj ? physObj->GetOwner() : nullptr;
		auto *physC = ent ? ent->GetEntity().GetPhysicsComponent().get() : nullptr;
		if(physC == nullptr)
			return;
		physC->OnSleep();
	}
};

void Game::InitializeGame()
{
	InitializeLua(); // Lua has to be initialized completely before any entites are created

	auto physEngineName = GetConVarString("phys_engine");
	auto physEngineLibName = pragma::physics::IEnvironment::GetPhysicsEngineModuleLocation(physEngineName);
	std::string err;
	auto dllHandle = GetNetworkState()->InitializeLibrary(physEngineLibName,&err);
	if(dllHandle)
	{
		auto *fInitPhysicsEngine = dllHandle->FindSymbolAddress<void(*)(NetworkState&,std::unique_ptr<pragma::physics::IEnvironment,void(*)(pragma::physics::IEnvironment*)>&)>("initialize_physics_engine");
		if(fInitPhysicsEngine != nullptr)
			fInitPhysicsEngine(*GetNetworkState(),m_physEnvironment);
		else
			Con::cerr<<"ERROR: Unable to initialize physics engine '"<<physEngineName<<"': Function 'initialize_physics_engine' not found!"<<Con::endl;
	}
	else
		Con::cerr<<"ERROR: Unable to initialize physics engine '"<<physEngineName<<"': "<<err<<Con::endl;
	if(m_physEnvironment)
	{
		m_surfaceMaterialManager = std::make_unique<SurfaceMaterialManager>(*m_physEnvironment);
		m_physEnvironment->SetEventCallback(std::make_unique<PhysEventCallback>());

		auto &tireTypeManager = m_physEnvironment->GetTireTypeManager();
		auto &surfTypeManager = m_physEnvironment->GetSurfaceTypeManager();
		auto data = ds::System::LoadData("scripts/physics/tire_types.txt");
		if(data)
		{
			auto *values = data->GetData();
			for(auto it=values->begin();it!=values->end();it++)
			{
				auto &val = it->second;
				if(val->IsBlock() == false || it->first.empty())
					continue;
				auto hTireType = tireTypeManager.RegisterType(it->first);
				if(hTireType.IsExpired())
					continue;
				auto *blockData = static_cast<ds::Block*>(val.get());
				auto frictionMods = blockData->GetBlock("friction_modifiers",0);
				auto *frictionModsData = frictionMods ? frictionMods->GetData() : nullptr;
				if(frictionModsData == nullptr || frictionMods->IsBlock() == false)
					continue;
				auto *frictionBlock = static_cast<ds::Block*>(frictionMods.get());
				for(auto &pair : *frictionModsData)
				{
					auto &surfaceType = pair.first;
					auto surfType = surfTypeManager.RegisterType(surfaceType);
					auto friction = 1.f;
					if(surfType.IsExpired() || frictionBlock->GetFloat(surfaceType,&friction) == false)
						continue;
					hTireType->SetFrictionModifier(*surfType,friction);
				}
			}
		}
	}

	m_cbProfilingHandle = engine->AddProfilingHandler([this](bool profilingEnabled) {
		if(profilingEnabled == false)
		{
			m_profilingStageManager = nullptr;
			return;
		}
		std::string postFix = IsClient() ? " (CL)" : " (SV)";
		auto &cpuProfiler = engine->GetProfiler();
		m_profilingStageManager = std::make_unique<pragma::debug::ProfilingStageManager<pragma::debug::ProfilingStage,CPUProfilingPhase>>();
		auto stageTick = pragma::debug::ProfilingStage::Create(cpuProfiler,"Tick" +postFix,&engine->GetProfilingStageManager()->GetProfilerStage(Engine::CPUProfilingPhase::Tick));
		auto stagePhysics = pragma::debug::ProfilingStage::Create(cpuProfiler,"Physics" +postFix,stageTick.get());
		m_profilingStageManager->InitializeProfilingStageManager(cpuProfiler,{
			stageTick,
			stagePhysics,
			pragma::debug::ProfilingStage::Create(cpuProfiler,"PhysicsSimulation" +postFix,stagePhysics.get()),
			pragma::debug::ProfilingStage::Create(cpuProfiler,"GameObjectLogic" +postFix,stageTick.get()),
			pragma::debug::ProfilingStage::Create(cpuProfiler,"Timers" +postFix,stageTick.get())
			});
		static_assert(umath::to_integral(CPUProfilingPhase::Count) == 5u,"Added new profiling phase, but did not create associated profiling stage!");
	});
}

void Game::GetEntities(std::vector<BaseEntity*> **ents) {*ents = &m_baseEnts;}
void Game::GetSpawnedEntities(std::vector<BaseEntity*> *ents)
{
	std::vector<BaseEntity*> *baseEnts;
	GetEntities(&baseEnts);
	for(unsigned int i=0;i<baseEnts->size();i++)
	{
		BaseEntity *ent = (*baseEnts)[i];
		if(ent != NULL && ent->IsSpawned())
			ents->push_back(ent);
	}
}

NetworkState *Game::GetNetworkState() {return m_stateNetwork;}

void Game::UpdateTime()
{
	float timeScale = GetTimeScale();
	m_ctCur.Update(timeScale);
	m_ctReal.Update();
	m_tCur = CDouble(m_ctCur());
	m_tReal = CDouble(m_ctReal());
	m_tDelta = CDouble(m_tCur -m_tLast);
	m_tDeltaReal = CDouble(m_tReal -m_tLastReal);
}
void Game::Think()
{
	UpdateTime();

	m_scriptWatcher->Poll(); // TODO: Don't do this every frame?
}
void Game::PostThink()
{
	m_tLast = m_tCur;
	m_tLastReal = m_tReal;
}

double &Game::GetLastThink() {return m_tLast;}
double &Game::GetLastTick() {return m_tLastTick;}

pragma::debug::ProfilingStageManager<pragma::debug::ProfilingStage,Game::CPUProfilingPhase> *Game::GetProfilingStageManager() {return m_profilingStageManager.get();}
bool Game::StartProfilingStage(CPUProfilingPhase stage)
{
	return m_profilingStageManager && m_profilingStageManager->StartProfilerStage(stage);
}
bool Game::StopProfilingStage(CPUProfilingPhase stage)
{
	return m_profilingStageManager && m_profilingStageManager->StopProfilerStage(stage);
}

std::string Game::GetMapName() {return m_mapInfo.name;}

const std::vector<BaseEntity*> &Game::GetBaseEntities() const {return const_cast<Game*>(this)->GetBaseEntities();}
std::vector<BaseEntity*> &Game::GetBaseEntities() {return m_baseEnts;}
std::size_t Game::GetBaseEntityCount() const {return m_numEnts;}

void Game::ScheduleEntityForRemoval(BaseEntity &ent) {m_entsScheduledForRemoval.push(ent.GetHandle());}

void Game::Tick()
{
	StartProfilingStage(CPUProfilingPhase::Tick);
	if((m_flags &GameFlags::InitialTick) != GameFlags::None)
	{
		m_flags &= ~GameFlags::InitialTick;
		m_tDeltaTick = 0.0f; // First tick is essentially 'skipped' to avoid physics errors after the world has been loaded
	}
	else
		m_tDeltaTick = (1.f /engine->GetTickRate()) *GetTimeScale();//m_tCur -m_tLastTick;
	for(auto *ent : m_baseEnts)
	{
		if(ent != nullptr && ent->IsSpawned())
			ent->ResetStateChangeFlags();
	}

	StartProfilingStage(CPUProfilingPhase::Physics);

	auto &awakePhysics = GetAwakePhysicsComponents();
	for(auto &hPhysC : awakePhysics)
	{
		if(hPhysC.expired() || hPhysC->GetPhysicsType() == PHYSICSTYPE::NONE)
			continue;
		hPhysC->PrePhysicsSimulate(); // Has to be called BEFORE PhysicsUpdate (This is where stuff like Character movement is handled)!
	}

	for(auto &hPhysC : awakePhysics)
	{
		if(hPhysC.expired() || hPhysC->GetPhysicsType() == PHYSICSTYPE::NONE)
			continue;
		hPhysC->PhysicsUpdate(m_tDeltaTick); // Has to be called AFTER PrePhysicsSimulate (This is where physics objects are updated)!
	}

	CallCallbacks("PrePhysicsSimulate");
	CallLuaCallbacks("PrePhysicsSimulate");
	StartProfilingStage(CPUProfilingPhase::PhysicsSimulation);
	if(IsPhysicsSimulationEnabled() == true && m_physEnvironment)
	{
		static int maxSteps = 1;
		m_tPhysDeltaRemainder = m_physEnvironment->StepSimulation(CFloat(m_tDeltaTick +m_tPhysDeltaRemainder),maxSteps,CFloat(m_tDeltaTick));
	}
	StopProfilingStage(CPUProfilingPhase::PhysicsSimulation);
	CallCallbacks("PostPhysicsSimulate");
	CallLuaCallbacks("PostPhysicsSimulate");

	for(auto &hPhysC : awakePhysics)
	{
		if(hPhysC.expired() || hPhysC->GetPhysicsType() == PHYSICSTYPE::NONE)
			continue;
		hPhysC->PostPhysicsSimulate();
		hPhysC->UpdatePhysicsData(); // Has to be before Think (Requires updated physics).
	}

	StopProfilingStage(CPUProfilingPhase::Physics);

	while(m_entsScheduledForRemoval.empty() == false)
	{
		auto &hEnt = m_entsScheduledForRemoval.front();
		if(hEnt.IsValid())
			hEnt->Remove();

		m_entsScheduledForRemoval.pop();
	}

	StartProfilingStage(CPUProfilingPhase::GameObjectLogic);
	static std::vector<util::WeakHandle<pragma::LogicComponent>> logicComponents {};
	EntityIterator entItLogic {*this};
	entItLogic.AttachFilter<TEntityIteratorFilterComponent<pragma::LogicComponent>>();
	if(entItLogic.GetCount() > logicComponents.size())
		logicComponents.resize(entItLogic.GetCount(),util::WeakHandle<pragma::LogicComponent>{});

	uint32_t idx = 0u;
	for(auto *ent : entItLogic)
	{
		auto pLogicComponent = ent->GetComponent<pragma::LogicComponent>();
		if(m_tCur < pLogicComponent->GetNextThink())
			continue;
		pLogicComponent->Think(m_tDeltaTick);
		logicComponents.at(idx++) = pLogicComponent;
	}
	if(idx < logicComponents.size())
		logicComponents.at(idx) = {};

	for(auto logicComponent : logicComponents)
	{
		if(logicComponent.expired())
			continue;
		logicComponent->PostThink();
	}
	StopProfilingStage(CPUProfilingPhase::GameObjectLogic);

	StartProfilingStage(CPUProfilingPhase::Timers);
	UpdateTimers();
	StopProfilingStage(CPUProfilingPhase::Timers);
	//if(GetNetworkState()->IsClient())
	//	return;
	StopProfilingStage(CPUProfilingPhase::Tick);
}
void Game::PostTick()
{
	m_tLastTick = m_tCur;
}

void Game::SetGameFlags(GameFlags flags) {m_flags = flags;}
Game::GameFlags Game::GetGameFlags() const {return m_flags;}

bool Game::IsMapInitialized() {return (m_flags &GameFlags::MapInitialized) != GameFlags::None;}

const pragma::NetEventManager &Game::GetEntityNetEventManager() const {return const_cast<Game*>(this)->GetEntityNetEventManager();}
pragma::NetEventManager &Game::GetEntityNetEventManager() {return m_entNetEventManager;}

pragma::NetEventId Game::FindNetEvent(const std::string &name) const {return m_entNetEventManager.FindNetEvent(name);}
const std::vector<std::string> &Game::GetNetEventIds() const {return const_cast<Game*>(this)->GetNetEventIds();}
std::vector<std::string> &Game::GetNetEventIds() {return m_entNetEventManager.GetNetEventIds();}

const MapInfo &Game::GetMapInfo() const {return m_mapInfo;}

bool Game::LoadSoundScripts(const char *file) {return m_stateNetwork->LoadSoundScripts(file,true);}
bool Game::LoadMap(const std::string &map,const Vector3 &origin,std::vector<EntityHandle> *entities)
{
	std::string smap = "maps\\";
	smap += map;
	smap += ".wld";

	m_mapInfo.name = map;
	m_mapInfo.fileName = smap;
	auto f = FileManager::OpenFile(smap.c_str(),"rb");
	if(f == nullptr)
	{
		static auto bPort = true;
		if(bPort == true)
		{
			Con::cwar<<"WARNING: Map '"<<map<<"' not found."<<Con::endl;
			if(util::port_source2_map(GetNetworkState(),smap) == false && util::port_hl2_map(GetNetworkState(),smap) == false)
				Con::cwar<<" Loading empty map..."<<Con::endl;
			else
			{
				Con::cwar<<Con::endl;
				Con::cout<<"Successfully ported HL2 map "<<smap<<"!"<<Con::endl;
				bPort = false;
				auto r = LoadMap(map);
				bPort = true;
				return r;
			}
		}
		m_flags |= GameFlags::MapInitialized;
		return false;
	}
	auto worldData = pragma::asset::WorldData::Create(*GetNetworkState());
	std::string errMsg;
	auto success = worldData->Read(f,pragma::asset::EntityData::Flags::None,&errMsg);

	if(success)
	{
		// Load sound scripts
		std::string soundScript = "soundscapes_";
		soundScript += map;
		soundScript += ".txt";
		LoadSoundScripts(soundScript.c_str());

		// Load entities
		Con::cout<<"Loading entities..."<<Con::endl;

		std::vector<EntityHandle> ents {};
		InitializeMapEntities(*worldData,ents);
		for(auto &hEnt : ents)
		{
			if(hEnt.IsValid() == false)
				continue;
			hEnt->Spawn();
		}
		for(auto &hEnt : ents)
		{
			if(hEnt.IsValid() == false || hEnt->IsSpawned() == false)
				continue;
			hEnt->OnSpawn();
		}
		InitializeWorldData(*worldData);
	}

	m_flags |= GameFlags::MapInitialized;
	if(success == false)
		Con::cwar<<"WARNING: Unable to load map '"<<map<<"': "<<errMsg<<Con::endl;
	return success;
}

void Game::InitializeWorldData(pragma::asset::WorldData &worldData) {}
void Game::InitializeMapEntities(pragma::asset::WorldData &worldData,std::vector<EntityHandle> &outEnt)
{
	auto &entityData = worldData.GetEntities();
	outEnt.reserve(entityData.size());
	for(auto &entData : entityData)
	{
		auto *ent = CreateMapEntity(*entData);
		if(ent == nullptr)
			continue;
		outEnt.push_back(ent->GetHandle());
	}
}

std::shared_ptr<Model> Game::CreateModel(const std::string &mdl) const {return m_stateNetwork->GetModelManager().CreateModel(mdl);}
std::shared_ptr<Model> Game::CreateModel(bool bAddReference) const {return m_stateNetwork->GetModelManager().CreateModel("",bAddReference);}
std::shared_ptr<Model> Game::LoadModel(const std::string &mdl,bool bReload)
{
	auto bNewModel = false;
	auto r = GetNetworkState()->GetModelManager().LoadModel(mdl,bReload,&bNewModel);
	if(bNewModel == true && r != nullptr)
	{
		CallCallbacks<void,std::reference_wrapper<std::shared_ptr<Model>>>("OnModelLoaded",r);
		CallLuaCallbacks<void,std::shared_ptr<Model>>("OnModelLoaded",r);
	}
	return r;
}

void Game::OnGameReady()
{
	// All assets have been loaded, now clear the unused assets (if there was a previous map)
	Con::cout<<"Clearing unused assets..."<<Con::endl;
	auto numModelsCleared = GetNetworkState()->GetModelManager().ClearUnused();
	auto numMaterialsCleared = GetNetworkState()->GetMaterialManager().ClearUnused();
	Con::cout<<numModelsCleared<<" models and "<<numMaterialsCleared<<" materials have been cleared from cache!"<<Con::endl;

	m_ctCur.Reset();
	m_ctReal.Reset();
	CallCallbacks<void>("OnGameReady");
	CallLuaCallbacks("OnGameReady");
}

void Game::SetWorld(pragma::BaseWorldComponent *entWorld) {m_worldComponent = (entWorld != nullptr) ? entWorld->GetHandle<pragma::BaseWorldComponent>() : util::WeakHandle<pragma::BaseWorldComponent>{};}

std::vector<util::WeakHandle<pragma::BasePhysicsComponent>> &Game::GetAwakePhysicsComponents() {return m_awakePhysicsEntities;}

const pragma::EntityComponentManager &Game::GetEntityComponentManager() const {return const_cast<Game*>(this)->GetEntityComponentManager();}
pragma::EntityComponentManager &Game::GetEntityComponentManager() {return *m_componentManager;}

SurfaceMaterial &Game::CreateSurfaceMaterial(const std::string &identifier,Float friction,Float restitution)
{
	return m_surfaceMaterialManager->Create(identifier,friction,restitution);
}
SurfaceMaterial *Game::GetSurfaceMaterial(const std::string &id)
{
	return m_surfaceMaterialManager ? m_surfaceMaterialManager->GetMaterial(id) : nullptr;
}
SurfaceMaterial *Game::GetSurfaceMaterial(UInt32 id)
{
	if(m_surfaceMaterialManager == nullptr)
		return nullptr;
	auto &materials = m_surfaceMaterialManager->GetMaterials();
	if(id >= materials.size())
		return nullptr;
	return &materials[id];
}
std::vector<SurfaceMaterial> &Game::GetSurfaceMaterials()
{
	return m_surfaceMaterialManager->GetMaterials();
}

double &Game::RealTime() {return m_tReal;}
double &Game::CurTime() {return m_tCur;}
double &Game::ServerTime() {return CurTime();}
double &Game::DeltaTime() {return m_tDelta;}
double &Game::DeltaRealTime() {return m_stateNetwork->DeltaTime();}
double &Game::LastThink() {return m_tLast;}
double &Game::LastTick() {return m_tLastTick;}
double &Game::DeltaTickTime() {return m_tDeltaTick;}

float Game::GetTimeScale() {return 1.f;}
void Game::SetTimeScale(float t) {m_stateNetwork->SetConVar("host_timescale",std::to_string(t));}

ConConf *Game::GetConVar(const std::string &scmd) {return m_stateNetwork->GetConVar(scmd);}
int Game::GetConVarInt(const std::string &scmd) {return m_stateNetwork->GetConVarInt(scmd);}
std::string Game::GetConVarString(const std::string &scmd) {return m_stateNetwork->GetConVarString(scmd);}
float Game::GetConVarFloat(const std::string &scmd) {return m_stateNetwork->GetConVarFloat(scmd);}
bool Game::GetConVarBool(const std::string &scmd) {return m_stateNetwork->GetConVarBool(scmd);}
ConVarFlags Game::GetConVarFlags(const std::string &scmd) {return m_stateNetwork->GetConVarFlags(scmd);}

