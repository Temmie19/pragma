#ifndef __GAME_H__
#define __GAME_H__
#include "pragma/networkdefinitions.h"
#include <mathutil/glmutil.h>
#ifdef PHYS_ENGINE_PHYSX
#include "pragma/physics/physxutil.h"
#include <PxShape.h>
#include <PxQueryFiltering.h>
#endif
#include <pragma/lua/luaapi.h>
#include "pragma/lua/ldefinitions.h"
#include "pragma/level/mapinfo.h"
#include "pragma/lua/d_hooks.h"
#include <vector>
#include <deque>
#include "pragma/physics/physicstypes.h"
#include "pragma/entities/baseentity_handle.h"
#include <pragma/physics/physapi.h>
#include <sharedutils/callback_handler.h>
#include "pragma/lua/lua_callback_handler.h"
#include <sharedutils/chronotime.h>
#include "pragma/util/timertypes.h"
#include <fsys/vfileptr.h>
#include "pragma/lua/sh_lua_entity_manager.h"
#include "pragma/util/ammo_type.h"
#include "pragma/math/surfacematerial.h"
#include "pragma/entities/baseentity_net_event_manager.hpp"
#include <fsys/filesystem.h>
#include <sharedutils/util_weak_handle.hpp>
#ifdef __linux__
#include "pragma/lua/lua_script_watcher.h"
#endif

namespace Lua
{
	DLLNETWORK void StackDump(lua_State *lua);
	DLLNETWORK void TableDump(lua_State *lua,int n=-1);
	DLLNETWORK void VarDump(lua_State *lua,int n=-1);
	class Interface;
}

#undef DrawText

class BrushMesh;
class Side;
class CvarCallback;
class NetworkState;
class ConConf;
struct TraceResult;
class BaseEntity;
class WVPxEventCallback;
class PhysObj;
class TCallback;
class SurfaceMaterial;
class PhysEnv;
class PhysCollisionObject;
class TraceData;
class Timer;
class Model;
class ModelMesh;
class ModelSubMesh;
class LuaDirectoryWatcherManager;
class ResourceWatcherManager;
struct AmmoType;
struct GameModeInfo;
struct Color;
class GibletCreateInfo;
enum class FTRACE;
namespace nwm {enum class ClientDropped : int8_t;};
namespace pragma
{
	class BaseWorldComponent;
	class BaseEntityComponent;
	class EntityComponentManager;
	namespace nav
	{
		class Mesh;
	};
};
#ifdef PHYS_ENGINE_BULLET
#elif PHYS_ENGINE_PHYSX
class PhysXControllerHitReport;
namespace physx
{
	class PxControllerManager;
	class PxFixedJoint;
	class PxDistanceJoint;
	class PxSphericalJoint;
	class PxRevoluteJoint;
	class PxPrismaticJoint;
	class PxD6Joint;
};
#endif

namespace pragma {namespace level {class BSPInputData;};};
class DLLNETWORK Game
	: public CallbackHandler,public LuaCallbackHandler
{
public:
#ifdef PHYS_ENGINE_BULLET
	PhysEnv *GetPhysicsEnvironment();
	SurfaceMaterial &CreateSurfaceMaterial(const std::string &identifier,Float friction=0.5f,Float restitution=0.5f);
	SurfaceMaterial *GetSurfaceMaterial(const std::string &id);
	SurfaceMaterial *GetSurfaceMaterial(UInt32 id);
	std::vector<SurfaceMaterial> &GetSurfaceMaterials();
#elif PHYS_ENGINE_PHYSX
	// PhysX
	physx::PxRigidDynamic *CreatePhysXActor(physx::PxGeometry &geometry,physx::PxMaterial &material);
	physx::PxRigidDynamic *CreatePhysXActor(physx::PxGeometry &geometry,physx::PxMaterial &material,physx::PxTransform &transform);
	physx::PxRigidDynamic *CreatePhysXActor(physx::PxGeometry &geometry,physx::PxMaterial &material,Vector3 &pos);
	physx::PxRigidStatic *CreateStaticPhysXActor(physx::PxGeometry &geometry,physx::PxMaterial &material);
	physx::PxRigidStatic *CreateStaticPhysXActor(physx::PxGeometry &geometry,physx::PxMaterial &material,physx::PxTransform &transform);
	physx::PxRigidStatic *CreateStaticPhysXActor(physx::PxGeometry &geometry,physx::PxMaterial &material,Vector3 &pos);
	physx::PxControllerManager *GetPhysXControllerManager();
	physx::PxScene *GetPhysXScene();
	physx::PxFixedJoint *CreateFixedJoint(physx::PxRigidActor *src,physx::PxTransform &tSrc,physx::PxRigidActor *tgt,physx::PxTransform &tTgt);
	physx::PxDistanceJoint *CreateDistanceJoint(physx::PxRigidActor *src,physx::PxTransform &tSrc,physx::PxRigidActor *tgt,physx::PxTransform &tTgt);
	physx::PxSphericalJoint *CreateSphericalJoint(physx::PxRigidActor *src,physx::PxTransform &tSrc,physx::PxRigidActor *tgt,physx::PxTransform &tTgt);
	physx::PxRevoluteJoint *CreateRevoluteJoint(physx::PxRigidActor *src,physx::PxTransform &tSrc,physx::PxRigidActor *tgt,physx::PxTransform &tTgt);
	physx::PxPrismaticJoint *CreatePrismaticJoint(physx::PxRigidActor *src,physx::PxTransform &tSrc,physx::PxRigidActor *tgt,physx::PxTransform &tTgt);
	physx::PxD6Joint *CreateD6Joint(physx::PxRigidActor *src,physx::PxTransform &tSrc,physx::PxRigidActor *tgt,physx::PxTransform &tTgt);
	virtual SurfaceMaterial *CreateSurfaceMaterial(std::string &identifier)=0;
	virtual SurfaceMaterial *CreateSurfaceMaterial(std::string &identifier,float staticFriction,float dynamicFriction,float restitution,std::string footstepType)=0;
	virtual SurfaceMaterial *GetSurfaceMaterial(std::string &identifier)=0;
	virtual std::unordered_map<std::string,SurfaceMaterial*> &GetSurfaceMaterials()=0;
	PhysXControllerHitReport *GetPhysXControllerHitReport();
#endif
	enum class GameFlags : uint32_t
	{
		None = 0u,
		MapInitialized = 1u,
		GameInitialized = MapInitialized<<1u,
		MapLoaded = GameInitialized<<1u,
		InitialTick = MapLoaded<<1u,
		LevelTransition = InitialTick<<1u
	};

	Vector3 &GetGravity();
	void SetGravity(Vector3 &gravity);
	virtual std::shared_ptr<Model> CreateModel(const std::string &mdl) const=0;
	virtual std::shared_ptr<Model> CreateModel(bool bAddReference=true) const=0;
	virtual std::shared_ptr<Model> LoadModel(const std::string &mdl,bool bReload=false)=0;
	virtual std::unordered_map<std::string,std::shared_ptr<Model>> &GetModels() const=0;
	bool RunLua(const std::string &lua,const std::string &chunkName);
	LuaDirectoryWatcherManager &GetLuaScriptWatcher();
	ResourceWatcherManager &GetResourceWatcher();
	void ReloadGameModeScripts();

	virtual std::shared_ptr<BrushMesh> CreateBrushMesh() const=0;
	virtual std::shared_ptr<Side> CreateSide() const=0;
	virtual std::shared_ptr<ModelMesh> CreateModelMesh() const=0;
	virtual std::shared_ptr<ModelSubMesh> CreateModelSubMesh() const=0;

	void ScheduleEntityForRemoval(BaseEntity &ent);

	pragma::NetEventId FindNetEvent(const std::string &name) const;
	std::vector<std::string> &GetNetEventIds();
	const std::vector<std::string> &GetNetEventIds() const;

	virtual pragma::NetEventId SetupNetEvent(const std::string &name)=0;

	bool IsGameModeInitialized() const;
	bool IsGameInitialized() const;
	bool IsMapLoaded() const;
	const std::array<std::string,4> &GetLuaEntityDirectories() const;
	virtual void InitializeLua();
	virtual void SetupLua();
	virtual void SetUp();
	virtual void Initialize();

	const pragma::EntityComponentManager &GetEntityComponentManager() const;
	pragma::EntityComponentManager &GetEntityComponentManager();

	// Entities
	const std::vector<BaseEntity*> &GetBaseEntities() const;
	std::vector<BaseEntity*> &GetBaseEntities();
	std::size_t GetBaseEntityCount() const;
	virtual void GetEntities(std::vector<BaseEntity*> **ents);
	void GetSpawnedEntities(std::vector<BaseEntity*> *ents);

	virtual void GetPlayers(std::vector<BaseEntity*> *ents) = 0;
	virtual void GetNPCs(std::vector<BaseEntity*> *ents) = 0;
	virtual void GetWeapons(std::vector<BaseEntity*> *ents) = 0;
	virtual void GetVehicles(std::vector<BaseEntity*> *ents) = 0;

	virtual void GetPlayers(std::vector<EntityHandle> *ents) = 0;
	virtual void GetNPCs(std::vector<EntityHandle> *ents) = 0;
	virtual void GetWeapons(std::vector<EntityHandle> *ents) = 0;
	virtual void GetVehicles(std::vector<EntityHandle> *ents) = 0;

	virtual BaseEntity *CreateEntity();
	virtual BaseEntity *CreateEntity(std::string classname);
	virtual void RemoveEntity(BaseEntity *ent);
	void RemoveEntities();
	virtual BaseEntity *GetEntity(unsigned int idx);
	pragma::BaseWorldComponent *GetWorld();
	unsigned char GetPlayerCount();
	unsigned int GetEntityCount();
	virtual void SpawnEntity(BaseEntity *ent);
	void SplashDamage(const Vector3 &origin,Float radius,DamageInfo &dmg,const std::function<bool(BaseEntity*,DamageInfo&)> &callback=nullptr);
	void SplashDamage(const Vector3 &origin,Float radius,UInt32 damage,Float force=0.f,BaseEntity *attacker=nullptr,BaseEntity *inflictor=nullptr,const std::function<bool(BaseEntity*,DamageInfo&)> &callback=nullptr);
	void SplashDamage(const Vector3 &origin,Float radius,UInt32 damage,Float force=0.f,const EntityHandle &attacker=EntityHandle(),const EntityHandle &inflictor=EntityHandle(),const std::function<bool(BaseEntity*,DamageInfo&)> &callback=nullptr);

	virtual void CreateGiblet(const GibletCreateInfo &info)=0;

	const std::shared_ptr<pragma::nav::Mesh> &GetNavMesh() const;
	std::shared_ptr<pragma::nav::Mesh> &GetNavMesh();

	bool IsMultiPlayer() const;
	bool IsSinglePlayer() const;
	// Map
	bool IsMapInitialized();
	template<class TWorld,class TPolyMesh,class TPoly,class TBrushMesh>
		void BuildVMF(const char *map);
	virtual bool LoadMap(const char *map,const Vector3 &origin={},std::vector<EntityHandle> *entities=nullptr);
	// Called when map and gamemode has been fully loaded and the game can start proper
	virtual void OnGameReady();
	bool LoadNavMesh(bool bReload=false);
	AmmoTypeManager &GetAmmoTypeManager();
	Bool RegisterAmmoType(const std::string &name,Int32 damage=10,Float force=200.f,DAMAGETYPE dmgType=DAMAGETYPE::BULLET,AmmoType **ammoOut=nullptr);
	AmmoType *GetAmmoType(const std::string &name,UInt32 *ammoId=nullptr);
	AmmoType *GetAmmoType(UInt32 ammoId);
	const GameModeInfo *GetGameMode() const;
	GameModeInfo *GetGameMode();
	void SetGameMode(const std::string &gameMode);
	luabind::object *GetGameModeLuaObject();
	LuaEntityManager &GetLuaEntityManager();

	void SetWorld(pragma::BaseWorldComponent *entWorld);
	void CloseMap();
	void ClearModels();
	virtual void Think();
	virtual void Tick();
	void PostThink();
	void PostTick();
	std::string GetMapName();
	void PrecacheModel(const char *mdl);
	bool LoadSoundScripts(const char *file);
	std::string GetSoundScript(const char *script);
	bool ExecConfig(std::string cfg);
	// Collision Detection
	bool m_bCollisionsEnabled = true;
	bool CollisionTest();
	bool CollisionTest(Vector3 *pos);
	bool CollisionTest(pragma::BasePlayerComponent &pl,float *distance,Vector3 *hitnormal=NULL);
	void CollisionTest(BaseEntity *a,BaseEntity *b);
	void EnableCollisions(bool b);
	//
	// Raytraces
	// Returns true if there's a blocking hit. If 'res' isn't NULL, it will contain the hit information
#ifdef PHYS_ENGINE_BULLET
	Bool Overlap(const TraceData &data,TraceResult *result);
	TraceResult Overlap(const TraceData &data);
	Bool RayCast(const TraceData &data,std::vector<TraceResult> *results);
	TraceResult RayCast(const TraceData &data);
	Bool Sweep(const TraceData &data,TraceResult *result);
	TraceResult Sweep(const TraceData &data);
#elif PHYS_ENGINE_PHYSX
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,TraceResult *res=NULL,unsigned int flags=0);
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,BaseEntity *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,EntityHandle &hFilter,TraceResult *res=NULL,unsigned int flags=0);
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,std::vector<BaseEntity*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,physx::PxRigidActor *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,std::vector<physx::PxRigidActor*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryFilterCallback &filter,TraceResult *res=NULL,unsigned int flags=0);
	// Automatically uses pre-filter
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),TraceResult *res=NULL,unsigned int flags=0);
	// Automatically uses post-filter
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	// Uses both pre- and post-filters
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(BaseEntity*),TraceResult *res=NULL,unsigned int flags=0);
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(const physx::PxRigidActor*),TraceResult *res=NULL,unsigned int flags=0);
	bool RayCast(Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(PhysObj*),TraceResult *res=NULL,unsigned int flags=0);
	
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,BaseEntity *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,EntityHandle &hFilter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,std::vector<BaseEntity*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,physx::PxRigidActor *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,std::vector<physx::PxRigidActor*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,physx::PxQueryFilterCallback &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),TraceResult *res=NULL,unsigned int flags=0);
	// Automatically uses post-filter
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	// Uses both pre- and post-filters
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(BaseEntity*),TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(const physx::PxRigidActor*),TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(PhysObj*),TraceResult *res=NULL,unsigned int flags=0);

	// Axis-aligned
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,BaseEntity *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,EntityHandle &hFilter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,std::vector<BaseEntity*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,physx::PxRigidActor *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,std::vector<physx::PxRigidActor*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryFilterCallback &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(BaseEntity*),TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(const physx::PxRigidActor*),TraceResult *res=NULL,unsigned int flags=0);
	bool Sweep(const physx::PxGeometry &geometry,Vector3 &origin,Vector3 &dir,float distance,physx::PxQueryHitType::Enum(*filter)(PhysObj*),TraceResult *res=NULL,unsigned int flags=0);

	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,BaseEntity *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,EntityHandle &hFilter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,std::vector<BaseEntity*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,physx::PxRigidActor *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,std::vector<physx::PxRigidActor*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,physx::PxQueryFilterCallback &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,physx::PxQueryHitType::Enum(*filter)(BaseEntity*),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,physx::PxQueryHitType::Enum(*filter)(const physx::PxRigidActor*),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,physx::PxQueryHitType::Enum(*filter)(PhysObj*),TraceResult *res=NULL,unsigned int flags=0);

	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,BaseEntity *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,EntityHandle &hFilter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,std::vector<BaseEntity*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,physx::PxRigidActor *filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,std::vector<physx::PxRigidActor*> &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,physx::PxQueryFilterCallback &filter,TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,physx::PxQueryHitType::Enum(*preFilter)(const physx::PxFilterData&,const physx::PxShape*,const physx::PxRigidActor*,physx::PxHitFlags&),physx::PxQueryHitType::Enum(*postFilter)(const physx::PxFilterData&,const physx::PxQueryHit&),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,physx::PxQueryHitType::Enum(*filter)(BaseEntity*),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,physx::PxQueryHitType::Enum(*filter)(const physx::PxRigidActor*),TraceResult *res=NULL,unsigned int flags=0);
	bool Overlap(const physx::PxGeometry &geometry,Vector3 &origin,physx::PxQueryHitType::Enum(*filter)(PhysObj*),TraceResult *res=NULL,unsigned int flags=0);
#endif
public:
	Game(NetworkState *state);
	virtual ~Game();
	virtual bool IsServer();
	virtual bool IsClient();
	NetworkState *GetNetworkState();
	// Time
	double &LastThink();
	double &LastTick();
	virtual double &CurTime();
	virtual double &ServerTime();
	virtual double &RealTime();
	virtual double &DeltaRealTime();
	// This is the frame delta time! For tick time, use DeltaTickTime!
	virtual double &DeltaTime();
	virtual double &DeltaTickTime();
	double &GetLastTick();
	double &GetLastThink();
	virtual float GetTimeScale();
	virtual void SetTimeScale(float t);
	// Lua
	lua_State *GetLuaState();
	Lua::Interface &GetLuaInterface();
	virtual void RegisterLua();
	virtual void RegisterLuaGlobals();
	virtual void RegisterLuaClasses();
	void RegisterLuaGameClasses(luabind::module_ &gameMod);
	virtual void RegisterLuaLibraries();
	virtual bool RegisterNetMessage(std::string name);
	void RegisterLuaNetMessage(std::string name,int handler);
	std::vector<std::string> *GetLuaNetMessageIndices();
	bool BroadcastEntityEvent(pragma::BaseEntityComponent &component,uint32_t eventId,int32_t argsIdx);
	bool InjectEntityEvent(pragma::BaseEntityComponent &component,uint32_t eventId,int32_t argsIdx);
	Lua::StatusCode LoadLuaFile(std::string &fInOut,fsys::SearchFlags includeFlags=fsys::SearchFlags::All,fsys::SearchFlags excludeFlags=fsys::SearchFlags::None);
	virtual bool ExecuteLuaFile(std::string &fInOut);
	// Same as ExecuteLuaFile, but uses the last value from the include stack
	//bool IncludeLuaFile(std::string &fInOut); // Deprecated

	virtual bool RunLua(const std::string &lua)=0;
	virtual void RunLuaFiles(const std::string &subPath);
	Lua::StatusCode ProtectedLuaCall(const std::function<Lua::StatusCode(lua_State*)> &pushFuncArgs,int32_t numResults);
	template<class TLuaEntity,class THandle>
		BaseEntity *CreateLuaEntity(std::string classname,luabind::object &oClass,bool bLoadIfNotExists=false);
	virtual BaseEntity *CreateLuaEntity(std::string classname,bool bLoadIfNotExists=false)=0;
	virtual pragma::BaseEntityComponent *CreateLuaEntityComponent(BaseEntity &ent,std::string classname)=0;
	void LoadLuaEntities(std::string subPath);
	void LoadLuaComponents(const std::string &typePath);
	virtual std::string GetLuaNetworkDirectoryName() const=0;
	virtual std::string GetLuaNetworkFileName() const=0;
	std::string GetGameModeScriptDirectoryPath() const;
	std::string GetGameModeScriptDirectoryNetworkPath() const;
	std::string GetGameModeScriptFilePath() const;

	// Path should be path to directory or file of lua entity (relative to "lua" directory)
	bool LoadLuaEntity(std::string path);
	// Path should be path to directory or file of lua component (relative to "lua" directory)
	bool LoadLuaComponent(std::string path);

	bool LoadLuaEntity(const std::string &mainPath,const std::string &className);
	bool LoadLuaComponent(const std::string &mainPath,const std::string &componentName);
	bool LoadLuaEntityByClass(const std::string &className);
	bool LoadLuaComponentByName(const std::string &componentName);

	void AddConVarCallback(const std::string &cvar,LuaFunction function);
	unsigned int GetNetMessageID(std::string name);
	std::string *GetNetMessageIdentifier(unsigned int ID);
	virtual void OnPlayerDropped(pragma::BasePlayerComponent &pl,nwm::ClientDropped reason);
	virtual void OnPlayerReady(pragma::BasePlayerComponent &pl);
	virtual void OnPlayerJoined(pragma::BasePlayerComponent &pl);
	// Timers
	Timer *CreateTimer(float delay,int reps,int fcLua,TimerType timeType=TimerType::CurTime);
	Timer *CreateTimer(float delay,int reps,const CallbackHandle &hCallback,TimerType timeType=TimerType::CurTime);
	void ClearTimers();
	// ConVars
	template<class T>
		T *GetConVar(const std::string &scmd);
	ConConf *GetConVar(const std::string &scmd);
	int GetConVarInt(const std::string &scmd);
	std::string GetConVarString(const std::string &scmd);
	float GetConVarFloat(const std::string &scmd);
	bool GetConVarBool(const std::string &scmd);
	ConVarFlags GetConVarFlags(const std::string &scmd);
	const std::unordered_map<std::string,std::vector<std::shared_ptr<CvarCallback>>> &GetConVarCallbacks() const;

	virtual Float GetFrictionScale() const=0;
	virtual Float GetRestitutionScale() const=0;
	const MapInfo &GetMapInfo() const;
	
	void SetGameFlags(GameFlags flags);
	GameFlags GetGameFlags() const;

	virtual bool IsPhysicsSimulationEnabled() const=0;

	uint32_t GetEntityMapIndexStart() const;
	void SetEntityMapIndexStart(uint32_t start);

	// Debug
	virtual void DrawLine(const Vector3 &start,const Vector3 &end,const Color &color,float duration=0.f)=0;
	virtual void DrawBox(const Vector3 &start,const Vector3 &end,const EulerAngles &ang,const Color &color,float duration=0.f)=0;
	virtual void DrawPlane(const Vector3 &n,float dist,const Color &color,float duration=0.f)=0;

	const util::CPUProfiler::Stage &StartStageProfiling(uint32_t stage);
	std::chrono::nanoseconds EndStageProfiling(uint32_t stage,bool addDuration=false);
protected:
	GameFlags m_flags = GameFlags::InitialTick;
	std::vector<BaseEntity*> m_baseEnts;
	std::queue<EntityHandle> m_entsScheduledForRemoval;
	std::shared_ptr<Lua::Interface> m_lua = nullptr;
	uint32_t m_mapEntityIdx = 1u;
	std::unique_ptr<LuaDirectoryWatcherManager> m_scriptWatcher = nullptr;
	SurfaceMaterialManager m_surfaceMaterials = {};
	std::unordered_map<std::string,std::vector<std::shared_ptr<CvarCallback>>> m_cvarCallbacks;
	std::vector<std::unique_ptr<Timer>> m_timers;
	std::unordered_map<std::string,int> m_luaNetMessages;
	std::vector<std::string> m_luaNetMessageIndex;
	MapInfo m_mapInfo = {};
	std::deque<unsigned int> m_entIndices;
	uint32_t m_numEnts = 0u;
	uint8_t m_numPlayers = 0u;
	NetworkState *m_stateNetwork = nullptr;
	ChronoTime m_ctCur = {};
	ChronoTime m_ctReal = {};
	double m_tCur = 0.0;
	double m_tLast = 0.0;
	double m_tDelta = 0.0;
	double m_tReal = 0.0;
	double m_tLastTick = 0.0;
	double m_tDeltaTick = 0.0;
	double m_tDeltaReal = 0.0;
	double m_tLastReal = 0.0;
	Vector3 m_gravity = {0,-600,0};
	util::WeakHandle<pragma::BaseWorldComponent> m_worldComponent = {};
	pragma::NetEventManager m_entNetEventManager = {};
	GameModeInfo *m_gameMode = nullptr;
	std::shared_ptr<pragma::nav::Mesh> m_navMesh = nullptr;
	std::unique_ptr<AmmoTypeManager> m_ammoTypes = nullptr;
	std::unique_ptr<LuaEntityManager> m_luaEnts = nullptr;
	std::unique_ptr<luabind::object> m_luaGameMode = nullptr;
	std::shared_ptr<pragma::EntityComponentManager> m_componentManager = nullptr;

	// Raytraces
#ifdef PHYS_ENGINE_PHYSX
	template<class T>
		bool FilteredRayCast(Vector3 &origin,Vector3 &dir,float distance,T filter,TraceResult *res=NULL,unsigned int flags=0);
	template<class T>
		bool FilteredSweep(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,Vector3 &dir,float distance,T filter,TraceResult *res=NULL,unsigned int flags=0);
	template<class T>
		bool FilteredOverlap(const physx::PxGeometry &geometry,Vector3 &origin,Quat &rot,T filter,TraceResult *res=NULL,unsigned int flags=0);
#endif
	// Lua
	std::vector<std::string> m_luaIncludeStack = {};

	virtual bool LoadLuaComponent(const std::string &luaFilePath,const std::string &mainPath,const std::string &componentName);
	virtual bool InitializeGameMode();
	template<class TComponent>
		pragma::BaseEntityComponent *CreateLuaEntityComponent(BaseEntity &ent,std::string classname);

	const pragma::NetEventManager &GetEntityNetEventManager() const;
	pragma::NetEventManager &GetEntityNetEventManager();

	virtual bool InvokeEntityEvent(pragma::BaseEntityComponent &component,uint32_t eventId,int32_t argsIdx,bool bInject);
	virtual void RegisterLuaEntityComponents(luabind::module_ &gameMod);
	void LoadConfig();
	void SaveConfig();
	void UpdateTimers();
	virtual void InitializeLuaScriptWatcher();
	template<class TModelManager>
		void ClearResources();

	// Map
	virtual void LoadMapEntities(uint32_t version,const char *map,VFilePtr f,const pragma::level::BSPInputData &bspInputData,std::vector<Material*> &materials,const Vector3 &origin={},std::vector<EntityHandle> *entities=nullptr)=0;
	virtual void LoadMapMaterials(uint32_t version,VFilePtr f,std::vector<Material*> &materials);
	BaseEntity *CreateMapEntity(uint32_t version,const std::string &classname,VFilePtr f,const pragma::level::BSPInputData &bspInputData,std::vector<Material*> &materials,const Vector3 &origin,uint64_t offsetToEndOfEntity,std::vector<EntityHandle> &ents,std::vector<EntityHandle> *entities=nullptr);
#ifdef PHYS_ENGINE_BULLET
	std::unique_ptr<PhysEnv> m_physEnvironment = nullptr;
#elif PHYS_ENGINE_PHYSX
	// PhysX
	physx::PxScene *m_pxScene = nullptr;
	physx::PxControllerManager *m_pxControllerManager = nullptr;
	double m_pxAccumulator = 0.0;
	WVPxEventCallback *m_eventCallback = nullptr;
	PhysXControllerHitReport *m_controllerHitReport = nullptr;
#endif

	virtual std::shared_ptr<pragma::EntityComponentManager> InitializeEntityComponentManager()=0;
	virtual void OnEntityCreated(BaseEntity *ent);
	virtual void SetupEntity(BaseEntity *ent,unsigned int idx)=0;
	virtual unsigned int GetFreeEntityIndex()=0;
	virtual std::shared_ptr<pragma::nav::Mesh> LoadNavMesh(const std::string &fname);
	void SetupEntity(BaseEntity *ent);
	virtual void InitializeEntityComponents(pragma::EntityComponentManager &componentManager);
};
REGISTER_BASIC_BITWISE_OPERATORS(Game::GameFlags)

template<class T>
	T *Game::GetConVar(const std::string &scmd)
	{
		ConConf *cv = GetConVar(scmd);
		if(cv == NULL)
			return NULL;
		return static_cast<T*>(cv);
	}

DLLNETWORK void IncludeLuaEntityBaseClasses(lua_State *l,int refEntities,int obj,int data);

#endif