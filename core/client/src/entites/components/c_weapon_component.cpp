#include "stdafx_client.h"
#include "pragma/entities/components/c_weapon_component.hpp"
#include "pragma/entities/c_viewmodel.h"
#include "pragma/console/c_cvar.h"
#include "pragma/entities/components/c_player_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"
#include "pragma/entities/components/c_render_component.hpp"
#include "pragma/entities/components/c_ownable_component.hpp"
#include <pragma/entities/components/base_transform_component.hpp>
#include <pragma/entities/observermode.h>

using namespace pragma;

std::vector<CWeaponComponent*> CWeaponComponent::s_weapons;
const std::vector<CWeaponComponent*> &CWeaponComponent::GetAll() {return s_weapons;}
unsigned int CWeaponComponent::GetWeaponCount() {return CUInt32(s_weapons.size());}

extern DLLCLIENT CGame *c_game;
extern DLLCLIENT ClientState *client;

CWeaponComponent::CWeaponComponent(BaseEntity &ent)
	: BaseWeaponComponent(ent),
	CBaseNetComponent()
{
	s_weapons.push_back(this);
}

CWeaponComponent::~CWeaponComponent()
{
	auto it = std::find(s_weapons.begin(),s_weapons.end(),this);
	if(it != s_weapons.end())
		s_weapons.erase(it);
	ClearOwnerCallbacks();
}

luabind::object CWeaponComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CWeaponHandle>(l);}

void CWeaponComponent::ReceiveData(NetPacket &packet)
{
	auto primAmmoType = packet->Read<UInt32>();
	auto secAmmoType = packet->Read<UInt32>();
	auto primClipSize = packet->Read<UInt16>();
	auto secClipSize = packet->Read<UInt16>();
	auto primMaxClipSize = packet->Read<UInt16>();
	auto secMaxClipSize = packet->Read<UInt16>();
	SetPrimaryAmmoType(primAmmoType);
	SetSecondaryAmmoType(secAmmoType);
	SetPrimaryClipSize(primClipSize);
	SetSecondaryClipSize(secClipSize);
	SetMaxPrimaryClipSize(primMaxClipSize);
	SetMaxSecondaryClipSize(secMaxClipSize);
}

bool CWeaponComponent::HandleViewModelAnimationEvent(pragma::CViewModelComponent*,const AnimationEvent&) {return false;}

void CWeaponComponent::UpdateViewModel()
{
	if(IsDeployed() == false)
		return;
	auto *vm = GetViewModel();
	if(vm == nullptr)
		return;
	auto &vmEnt = static_cast<CBaseEntity&>(vm->GetEntity());
	auto pRenderComponentVm = vmEnt.GetRenderComponent();
	if(m_viewModel.empty() == true)
	{
		if(pRenderComponentVm.valid())
			pRenderComponentVm->SetRenderMode(RenderMode::None);
		vm->SetViewModelOffset({});
		return;
	}
	auto mdlComponentVm = vmEnt.GetModelComponent();
	if(mdlComponentVm.valid())
		mdlComponentVm->SetModel(m_viewModel);
	if(pRenderComponentVm.valid())
		pRenderComponentVm->SetRenderMode(RenderMode::View);
	vm->SetViewModelOffset(GetViewModelOffset());
	vm->SetViewFOV(GetViewFOV());
}

static auto cvViewFov = GetClientConVar("cl_fov_viewmodel");
void CWeaponComponent::SetViewModelOffset(const Vector3 &offset)
{
	m_viewModelOffset = offset;
	auto *vm = GetViewModel();
	if(vm == nullptr)
		return;
	vm->SetViewModelOffset(offset);
}
const Vector3 &CWeaponComponent::GetViewModelOffset() const {return m_viewModelOffset;}
void CWeaponComponent::SetViewFOV(float fov)
{
	m_viewFov = fov;
	auto *vm = GetViewModel();
	if(vm == nullptr)
		return;
	vm->SetViewFOV(fov);
}
void CWeaponComponent::Initialize()
{
	BaseWeaponComponent::Initialize();

	BindEvent(CRenderComponent::EVENT_SHOULD_DRAW,[this](std::reference_wrapper<pragma::ComponentEvent> evData) -> util::EventReply {
		auto &shouldDrawData = static_cast<CEShouldDraw&>(evData.get());
		auto &ent = static_cast<CBaseEntity&>(GetEntity());
		auto pRenderComponent = ent.GetComponent<pragma::CRenderComponent>();
		auto renderMode = pRenderComponent.valid() ? pRenderComponent->GetRenderMode() : RenderMode::None;
		if(renderMode != RenderMode::None)
		{
			if(renderMode == RenderMode::View)
			{
				auto *pl = c_game->GetLocalPlayer();
				auto *plComponent = static_cast<CPlayerComponent*>(pl->GetEntity().GetPlayerComponent().get());
				if(pl->IsInFirstPersonMode() == false)
				{
					shouldDrawData.shouldDraw = CEShouldDraw::ShouldDraw::No;
					return util::EventReply::Handled;
				}
				return util::EventReply::Unhandled;
			}
			auto *owner = m_whOwnerComponent.valid() ? m_whOwnerComponent->GetOwner() : nullptr;
			if(owner != nullptr)
			{
				auto charComponent = owner->GetCharacterComponent();
				if(charComponent.valid())
				{
					if(charComponent->GetActiveWeapon() != &GetEntity())
					{
						shouldDrawData.shouldDraw = CEShouldDraw::ShouldDraw::No;
						return util::EventReply::Handled;
					}
					return util::EventReply::Unhandled;
				}
			}
		}
		return util::EventReply::Unhandled;
	});
	BindEvent(CRenderComponent::EVENT_SHOULD_DRAW_SHADOW,[this](std::reference_wrapper<pragma::ComponentEvent> evData) -> util::EventReply {
		if(m_bDeployed == false)
		{
			static_cast<CEShouldDraw&>(evData.get()).shouldDraw = CEShouldDraw::ShouldDraw::No;
			return util::EventReply::Handled;
		}
		return util::EventReply::Unhandled;
	});
	BindEventUnhandled(COwnableComponent::EVENT_ON_OWNER_CHANGED,[this](std::reference_wrapper<pragma::ComponentEvent> evData) {
		UpdateOwnerAttachment();
		ClearOwnerCallbacks();
		auto &ownerChangedData = static_cast<pragma::CEOnOwnerChanged&>(evData.get());
		if(ownerChangedData.newOwner != nullptr && ownerChangedData.newOwner->IsPlayer() == true && static_cast<CPlayerComponent*>(ownerChangedData.newOwner->GetPlayerComponent().get())->IsLocalPlayer() == true)
		{
			auto plComponent = ownerChangedData.newOwner->GetPlayerComponent();
			m_cbOnOwnerObserverModeChanged = plComponent->GetObserverModeProperty()->AddCallback([this](const std::reference_wrapper<const OBSERVERMODE> oldObserverMode,const std::reference_wrapper<const OBSERVERMODE> observerMode) {
				UpdateOwnerAttachment();
			});
			FlagCallbackForRemoval(m_cbOnOwnerObserverModeChanged,CallbackType::Component);
		}
	});
}
float CWeaponComponent::GetViewFOV() const
{
	if(std::isnan(m_viewFov) == true)
		return cvViewFov->GetFloat();
	return m_viewFov;
}

bool CWeaponComponent::IsInFirstPersonMode() const
{
	if(IsDeployed() == false)
		return false;
	auto *owner = m_whOwnerComponent.valid() ? m_whOwnerComponent->GetOwner() : nullptr;
	if(owner == nullptr || owner->IsPlayer() == false)
		return false;
	auto *plComponent = static_cast<CPlayerComponent*>(owner->GetPlayerComponent().get());
	return plComponent->IsLocalPlayer() && plComponent->IsInFirstPersonMode();
}

void CWeaponComponent::UpdateWorldModel()
{
	auto &ent = static_cast<CBaseEntity&>(GetEntity());
	auto pRenderComponent = ent.GetRenderComponent();
	if(pRenderComponent.expired())
		return;
	pRenderComponent->SetRenderMode(IsInFirstPersonMode() ? ((m_bHideWorldModelInFirstPerson == true) ? RenderMode::None : RenderMode::View) : RenderMode::World);
}

void CWeaponComponent::SetViewModel(const std::string &mdl)
{
	m_viewModel = mdl;
	UpdateViewModel();
}
const std::string &CWeaponComponent::GetViewModelName() const {return m_viewModel;}

void CWeaponComponent::OnFireBullets(const BulletInfo &bulletInfo,Vector3 &bulletOrigin,Vector3 &bulletDir,Vector3 *effectsOrigin)
{
	BaseWeaponComponent::OnFireBullets(bulletInfo,bulletOrigin,bulletDir,effectsOrigin);
	auto *owner = m_whOwnerComponent.valid() ? m_whOwnerComponent->GetOwner() : nullptr;
	auto &ent = GetEntity();
	if(owner != nullptr && owner->IsPlayer())
	{
		auto *plComponent = static_cast<CPlayerComponent*>(owner->GetPlayerComponent().get());
		if(plComponent->IsLocalPlayer())
		{
			auto charComponent = owner->GetCharacterComponent();
			auto pTrComponent = owner->GetTransformComponent();
			bulletDir = charComponent.valid() ? charComponent->GetViewForward() : pTrComponent.valid() ? pTrComponent->GetForward() : uvec::FORWARD;
			bulletOrigin = plComponent->GetViewPos();
		}
	}
	if(effectsOrigin == nullptr)
		return;
	if(std::isnan(bulletInfo.effectOrigin.x) == false)
	{
		*effectsOrigin = bulletInfo.effectOrigin;
		return;
	}
	auto pAnimComponent = ent.GetAnimatedComponent();
	if(pAnimComponent.valid() && pAnimComponent->GetAttachment(m_attMuzzle,effectsOrigin,static_cast<Quat*>(nullptr)) == true)
	{
		auto pTrComponent = ent.GetTransformComponent();
		if(pTrComponent.valid())
			pTrComponent->LocalToWorld(effectsOrigin);
	}
}

void CWeaponComponent::ClearOwnerCallbacks()
{
	if(m_cbOnOwnerObserverModeChanged.IsValid() == false)
		return;
	m_cbOnOwnerObserverModeChanged.Remove();
}

void CWeaponComponent::UpdateOwnerAttachment()
{
	m_hTarget = {};
	UpdateWorldModel();
	auto &ent = GetEntity();
	auto *owner = m_whOwnerComponent.valid() ? m_whOwnerComponent->GetOwner() : nullptr;
	if(owner == nullptr)
	{
		auto pAttComponent = ent.GetComponent<CAttachableComponent>();
		if(pAttComponent.valid())
			pAttComponent->ClearAttachment();
		return;
	}
	auto *game = client->GetGameState();
	BaseEntity *parent = owner;
	if(owner->IsPlayer())
	{
		auto *plComponent = static_cast<CPlayerComponent*>(owner->GetPlayerComponent().get());
		auto charComponent = owner->GetCharacterComponent();
		if(plComponent->IsLocalPlayer() && charComponent.valid() && charComponent->GetActiveWeapon() == &ent && IsInFirstPersonMode() == true)
		{
			auto *vm = game->GetViewModel();
			if(vm == nullptr)
				return;
			parent = &vm->GetEntity();
		}
	}
	m_hTarget = parent->GetHandle();
	auto pTransformComponent = ent.GetTransformComponent();
	auto pTransformComponentParent = parent->GetTransformComponent();
	if(pTransformComponent.valid() && pTransformComponentParent.valid())
	{
		pTransformComponent->SetPosition(pTransformComponentParent->GetPosition());
		pTransformComponent->SetOrientation(pTransformComponentParent->GetOrientation());
	}
	auto pAttComponent = ent.AddComponent<CAttachableComponent>();
	if(pAttComponent.valid())
	{
		auto pMdlComponent = parent->GetModelComponent();
		auto attId = pMdlComponent.valid() ? pMdlComponent->LookupAttachment("weapon") : -1;
		AttachmentInfo attInfo {};
		attInfo.flags |= FAttachmentMode::SnapToOrigin | FAttachmentMode::UpdateEachFrame;
		if(attId != -1)
			pAttComponent->AttachToAttachment(parent,"weapon",attInfo);
		else
			pAttComponent->AttachToEntity(parent,attInfo);
	}
	//SetParent(parent,FPARENT_BONEMERGE | FPARENT_UPDATE_EACH_FRAME);
	//SetAnimated(true);
}

void CWeaponComponent::SetHideWorldModelInFirstPerson(bool b)
{
	m_bHideWorldModelInFirstPerson = b;
	UpdateWorldModel();
}
bool CWeaponComponent::GetHideWorldModelInFirstPerson() const {return m_bHideWorldModelInFirstPerson;}

void CWeaponComponent::Deploy()
{
	BaseWeaponComponent::Deploy();
	UpdateOwnerAttachment();
	UpdateViewModel();
	auto *vm = GetViewModel();
	if(vm == nullptr)
		return;
	CGame *game = client->GetGameState();
	if(PlayViewActivity(Activity::VmDeploy) == false)
		PlayViewActivity(Activity::VmIdle);
}

void CWeaponComponent::Holster()
{
	BaseWeaponComponent::Holster();
	auto *vm = GetViewModel();
	if(vm == NULL)
		return;
	CGame *game = client->GetGameState();
	PlayViewActivity(Activity::VmHolster);
}

Activity CWeaponComponent::TranslateViewActivity(Activity act) {return act;}

pragma::CViewModelComponent *CWeaponComponent::GetViewModel()
{
	BaseEntity *parent = m_hTarget.get();
	if(parent == NULL)
		return NULL;
	CGame *game = client->GetGameState();
	auto *vm = game->GetViewModel();
	if(vm == nullptr || &vm->GetEntity() != parent)
		return NULL;
	return vm;
}

bool CWeaponComponent::PlayViewActivity(Activity activity,pragma::FPlayAnim flags)
{
	auto *vm = GetViewModel();
	if(vm == nullptr)
		return false;
	activity = TranslateViewActivity(activity);
	auto pAnimComponent = vm->GetEntity().GetAnimatedComponent();
	if(pAnimComponent.valid())
		pAnimComponent->PlayActivity(activity,flags);
	return true;
}

void CWeaponComponent::PrimaryAttack()
{
	if(!CanPrimaryAttack())
		return;
	BaseWeaponComponent::PrimaryAttack();
}
void CWeaponComponent::SecondaryAttack()
{
	if(!CanSecondaryAttack())
		return;
	BaseWeaponComponent::SecondaryAttack();
}
void CWeaponComponent::TertiaryAttack()
{
	BaseWeaponComponent::TertiaryAttack();
}
void CWeaponComponent::Attack4()
{
	BaseWeaponComponent::Attack4();
}
void CWeaponComponent::Reload()
{
	BaseWeaponComponent::Reload();
}

