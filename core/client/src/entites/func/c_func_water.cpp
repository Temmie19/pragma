#include "stdafx_client.h"
#include "pragma/entities/func/c_func_water.h"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/rendering/c_rendermode.h"
#include <pragma/physics/physobj.h>
#include "pragma/model/c_modelmesh.h"
#include "pragma/physics/c_phys_water_surface_simulator.hpp"
#include "pragma/entities/baseentity_luaobject.h"
#include "pragma/entities/c_water_surface.hpp"
#include "pragma/entities/components/c_render_component.hpp"
#include "pragma/entities/components/c_model_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/model/model.h>
#include <pragma/physics/raytraces.h>
#include <pragma/entities/components/base_physics_component.hpp>
#include <pragma/audio/alsound_type.h>
#include <pragma/entities/components/base_physics_component.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(func_water,CFuncWater);

extern DLLCLIENT CGame *c_game;

static std::vector<CWaterComponent*> s_waterEntities = {};
REGISTER_CONVAR_CALLBACK_CL(cl_water_surface_simulation_spacing,[](NetworkState*,ConVar*,int,int val) {
	for(auto *entWater : s_waterEntities)
		entWater->ReloadSurfaceSimulator();
});

REGISTER_CONVAR_CALLBACK_CL(cl_water_surface_simulation_enable_gpu_acceleration,[](NetworkState*,ConVar*,bool,bool val) {
	for(auto *entWater : s_waterEntities)
		entWater->ReloadSurfaceSimulator();
});

CWaterComponent::CWaterComponent(BaseEntity &ent)
	: BaseFuncWaterComponent(ent)
{
	pragma::CParticleSystemComponent::Precache("water");

	s_waterEntities.push_back(this);
}
CWaterComponent::~CWaterComponent()
{
	auto it = std::find(s_waterEntities.begin(),s_waterEntities.end(),this);
	if(it == s_waterEntities.end())
		return;
	s_waterEntities.erase(it);
}
void CWaterComponent::Initialize()
{
	BaseFuncWaterComponent::Initialize();

	BindEvent(CRenderComponent::EVENT_SHOULD_DRAW,[this](std::reference_wrapper<pragma::ComponentEvent> evData) -> util::EventReply {
		if(!(m_hWaterSurface.IsValid() == false && (m_waterScene == nullptr || m_waterScene->descSetGroupTexEffects != nullptr)))
		{
			static_cast<CEShouldDraw&>(evData.get()).shouldDraw = CEShouldDraw::ShouldDraw::No;
			return util::EventReply::Handled;
		}
		return util::EventReply::Unhandled;
	});
	BindEventUnhandled(CModelComponent::EVENT_ON_MODEL_CHANGED,[this](std::reference_wrapper<pragma::ComponentEvent> evData) {
		if(GetEntity().IsSpawned() == false)
			return;
		BaseFuncWaterComponent::InitializeWaterSurface();
		SetupWater();
		ReloadSurfaceSimulator();
	});
}
const Vector3 &CWaterComponent::GetPosition() const
{
	auto pTrComponent = GetEntity().GetTransformComponent();
	return pTrComponent.valid() ? pTrComponent->GetPosition() : uvec::ORIGIN;
}
const Quat &CWaterComponent::GetOrientation() const
{
	auto pTrComponent = GetEntity().GetTransformComponent();
	static auto identity = uquat::identity();
	return pTrComponent.valid() ? pTrComponent->GetOrientation() : identity;
}
void CWaterComponent::OnEntitySpawn()
{
	BaseFuncWaterComponent::OnEntitySpawn();
	auto &ent = static_cast<CBaseEntity&>(GetEntity());
	//auto pPhysComponent = ent.GetPhysicsComponent();
	//if(pPhysComponent.valid())
	//	pPhysComponent->InitializePhysics(PHYSICSTYPE::STATIC);
	auto pRenderComponent = ent.GetRenderComponent();
	if(pRenderComponent.valid())
	{
		pRenderComponent->SetRenderMode(RenderMode::Water);
		pRenderComponent->GetRenderModeProperty()->SetLocked(true);
	}

	auto mdlComponent = ent.GetModelComponent();
	auto mdl = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(mdl == nullptr)
		return;
	BaseFuncWaterComponent::InitializeWaterSurface();
	SetupWater();
	ReloadSurfaceSimulator();
}
void CWaterComponent::ReceiveData(NetPacket &packet)
{
	auto surfMat = packet->ReadString();
	GetEntity().SetKeyValue("surface_material",surfMat);
	auto height = packet->Read<float>();
	SetMaxWaveHeight(height);
}
CWaterSurface *CWaterComponent::GetSurfaceEntity() const {return static_cast<CWaterSurface*>(m_hWaterSurface.get());}

void CWaterComponent::CreateSplash(const Vector3 &origin,float radius,float force)
{
	auto pPhysComponent = GetEntity().GetPhysicsComponent();
	if(pPhysComponent.expired() || pPhysComponent->GetPhysicsObject() == nullptr)
		return;
	BaseFuncWaterComponent::CreateSplash(origin,radius,force); // TODO
	if(m_physSurfaceSim != nullptr)
		static_cast<CPhysWaterSurfaceSimulator&>(*m_physSurfaceSim).CreateSplash(origin,radius,force);
}
Bool CWaterComponent::ReceiveNetEvent(pragma::NetEventId eventId,NetPacket &packet)
{
	if(eventId == m_netEvSetWaterPlane)
	{
		//auto n = packet->Read<Vector3>();
		//auto d = packet->Read<double>();
		//m_waterPlane = Plane{n,d};
	}
	else if(eventId == m_netEvCreateSplash)
	{
		auto origin = packet->Read<Vector3>();
		auto radius = packet->Read<float>();
		auto force = packet->Read<float>();
		CreateSplash(origin,radius,force);
	}
	else
		return CWaterComponent::ReceiveNetEvent(eventId,packet);
	return true;
}

bool CWaterComponent::OnBulletHit(const BulletInfo &bulletInfo,const TraceData &data,PhysObj *phys,PhysCollisionObject *col,const btCollisionWorld::LocalRayResult &result)
{
	auto srcOrigin = data.GetSourceOrigin();
	auto dir = data.GetDirection();
	auto dist = data.GetDistance();
	auto hitPos = srcOrigin +dir *(dist *static_cast<float>(result.m_hitFraction));

	auto surfMatId = col->GetSurfaceMaterial();
	auto *surfMat = c_game->GetSurfaceMaterial(surfMatId);
	if(surfMat != nullptr)
	{
		auto &ptEffect = surfMat->GetImpactParticleEffect();
		if(ptEffect.empty() == false)
		{
			auto *pt = pragma::CParticleSystemComponent::Create(ptEffect);
			if(pt != nullptr)
			{
				auto pTrComponent = pt->GetEntity().GetTransformComponent();
				if(pTrComponent.valid())
				{
					pTrComponent->SetPosition(hitPos);

					auto up = uvec::create(result.m_hitNormalLocal);
					uvec::normalize(&up);
					const auto rot = Quat{0.5f,-0.5f,-0.5f,-0.5f};
					pTrComponent->SetOrientation(uquat::create_look_rotation(uvec::get_perpendicular(up),up) *rot);
				}
				pt->SetRemoveOnComplete(true);
				pt->Start();
			}
		}

		auto &bulletImpactSnd = surfMat->GetBulletImpactSound();
		if(bulletImpactSnd.empty() == false)
		{
			auto snd = client->CreateSound(bulletImpactSnd,ALSoundType::Effect | ALSoundType::Physics,ALCreateFlags::Mono);
			if(snd != nullptr)
			{
				snd->SetPosition(hitPos);
				snd->SetType(ALSoundType::Effect);
				snd->Play();
			}
		}
	}
	return BaseFuncWaterComponent::OnBulletHit(bulletInfo,data,phys,col,result);
}

CMaterial *CWaterComponent::GetWaterMaterial() const
{
	if(m_waterMesh.expired() == true)
		return nullptr;
	auto &ent = GetEntity();
	auto mdlComponent = GetEntity().GetModelComponent();
	auto mdl = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(mdl == nullptr)
		return nullptr;
	auto matIdx = m_waterMesh.lock()->GetTexture();
	auto &mats = mdl->GetMaterials();
	return (matIdx < mats.size()) ? static_cast<CMaterial*>(mats.at(matIdx).get()) : nullptr;
}

std::shared_ptr<PhysWaterSurfaceSimulator> CWaterComponent::InitializeSurfaceSimulator(const Vector2 &min,const Vector2 &max,float originY) {return std::make_shared<CPhysWaterSurfaceSimulator>(min,max,originY,GetSpacing(),GetStiffness(),GetPropagation());}

void CWaterComponent::SetupWater()
{
	m_waterScene = nullptr;
	/*auto &mdl = GetModel();
	if(mdl == nullptr)
		return;
	auto &mats = mdl->GetMaterials();
	std::unordered_map<std::size_t,bool> waterMaterialIds;
	waterMaterialIds.reserve(mats.size());
	for(auto it=mats.begin();it!=mats.end();++it)
	{
		auto &hMat = *it;
		if(hMat.IsValid() == false)
			continue;
		auto *mat = static_cast<CMaterial*>(hMat.get());
		auto hShader = mat->GetShader();
		if(hShader.IsValid() == false || dynamic_cast<Shader::Water*>(hShader.get()) == nullptr)
			continue;
		waterMaterialIds.insert(decltype(waterMaterialIds)::value_type({it -mats.begin(),true}));
	}
	if(waterMaterialIds.empty() == true)
		return;
	std::vector<CModelSubMesh*> waterSurfaces;
	for(auto meshId : mdl->GetBaseMeshes())
	{
		auto meshGroup = mdl->GetMeshGroup(meshId);
		if(meshGroup == nullptr)
			continue;
		for(auto &mesh : meshGroup->GetMeshes())
		{
			for(auto &subMesh : mesh->GetSubMeshes())
			{
				auto *cSubMesh = static_cast<CModelSubMesh*>(subMesh.get());
				auto texId = cSubMesh->GetTexture();
				auto it = waterMaterialIds.find(texId);
				if(it != waterMaterialIds.end())
					waterSurfaces.push_back(cSubMesh);
			}
		}
	}
	if(waterSurfaces.empty() == true)
		return;
	auto *meshSurface = waterSurfaces.front(); // TODO: All surfaces?
	*/
	auto mdlComponent = GetEntity().GetModelComponent();
	auto mdl = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(mdl == nullptr)
		return;
	auto &mats = mdl->GetMaterials();
	if(m_waterMesh.expired() == true)
		return;
	auto *meshSurface = static_cast<CModelSubMesh*>(m_waterMesh.lock().get());
	auto *mat = static_cast<CMaterial*>(mats.at(meshSurface->GetTexture()).get());
	InitializeWaterScene(meshSurface->GetVertexPosition(0),meshSurface->GetVertexNormal(0));
}

void CWaterComponent::ReloadSurfaceSimulator()
{
	SetSpacing(c_game->GetConVarInt("cl_water_surface_simulation_spacing"));
	BaseFuncWaterComponent::ReloadSurfaceSimulator();
	if(m_hWaterSurface.IsValid())
		m_hWaterSurface->Remove();
	if(m_physSurfaceSim == nullptr)
		return;
	auto *entSurface = c_game->CreateEntity<CWaterSurface>();
	if(entSurface == nullptr)
		return;
	auto &ent = GetEntity();
	ent.RemoveEntityOnRemoval(entSurface);
	m_hWaterSurface = entSurface->GetHandle();
	auto pWaterSurfComponent = entSurface->GetComponent<pragma::CWaterSurfaceComponent>();
	if(pWaterSurfComponent.valid())
	{
		pWaterSurfComponent->SetWaterObject(this);
		pWaterSurfComponent->SetSurfaceSimulator(m_physSurfaceSim);
	}
	entSurface->Spawn();
}

void CWaterComponent::SetStiffness(float stiffness)
{
	BaseFuncWaterComponent::SetStiffness(stiffness);
	if(m_physSurfaceSim == nullptr)
		return;
	m_physSurfaceSim->SetStiffness(stiffness);
}
void CWaterComponent::SetPropagation(float propagation)
{
	BaseFuncWaterComponent::SetPropagation(propagation);
	if(m_physSurfaceSim == nullptr)
		return;
	m_physSurfaceSim->SetPropagation(propagation);
}
luabind::object CWaterComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CWaterComponentHandleWrapper>(l);}

///////////////

void CFuncWater::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CWaterComponent>();
}


/*


	DECLARE_ENTITY_HANDLE;
	CFuncWater();
	virtual ~CFuncWater() override;
	virtual void Spawn() override;

	virtual void Initialize() override;
	virtual void OnInitializeModelComponent() override;
	virtual bool OnBulletHit(const BulletInfo &bulletInfo,const TraceData &data,PhysObj *phys,PhysCollisionObject *col,const btCollisionWorld::LocalRayResult &result) override;
	virtual void ReceiveData(NetPacket &packet) override;
	virtual Bool ReceiveNetEvent(UInt32 eventId,NetPacket &packet) override;
	virtual void PhysicsUpdate(double tDelta) override;
	virtual void OnEndTouch(BaseEntity *ent,PhysObj *phys) override;

	virtual bool CanTrigger(BaseEntity *ent,PhysObj *phys) override;
	virtual void SetStiffness(float stiffness) override;
	virtual void SetPropagation(float propagation) override;
	virtual const Vector3 &GetPosition() const override;
	virtual const Quat &GetOrientation() const override;
	virtual bool ShouldDraw(const Vector3 &camOrigin) const override;
	virtual CMaterial *GetWaterMaterial() const override;
	virtual void CreateSplash(const Vector3 &origin,float radius,float force) override;
	virtual void PrePhysicsSimulate() override;
	CWaterSurface *GetSurfaceEntity() const;
	virtual void ReloadSurfaceSimulator() override;
	virtual void SetKeyValue(std::string key,std::string val) override;
	virtual bool RayResultCallback(CollisionMask rayCollisionGroup,CollisionMask rayCollisionMask,btVector3 &rayFromWorld,btVector3 &rayToWorld,btVector3 &hitNormalWorld,btVector3 &hitPointWorld,btCollisionWorld::LocalRayResult &rayResult) override;
protected:
	virtual void OnPhysicsInitialized() override;
	virtual std::shared_ptr<PhysWaterSurfaceSimulator> InitializeSurfaceSimulator(const Vector2 &min,const Vector2 &max,float originY) override;
	void SetupWater();
	
	*/

/*
	: public CTriggerTouch,
	public BaseFuncWater,
	public CWaterObject
	*/