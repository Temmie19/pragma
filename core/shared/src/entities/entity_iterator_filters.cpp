#include "stdafx_shared.h"
#include "pragma/entities/entity_iterator.hpp"
#include "pragma/entities/entity_component_manager.hpp"
#include "pragma/entities/components/base_physics_component.hpp"
#include "pragma/entities/components/base_transform_component.hpp"
#include "pragma/entities/components/base_name_component.hpp"
#include "pragma/entities/basefilterentity.h"
#include <pragma/math/intersection.h>

EntityIteratorFilterName::EntityIteratorFilterName(Game &game,const std::string &name,bool caseSensitive,bool exactMatch)
	: m_name(name),m_bCaseSensitive(caseSensitive),m_bExactMatch(exactMatch)
{}
bool EntityIteratorFilterName::ShouldPass(BaseEntity &ent)
{
	auto pNameComponent = static_cast<pragma::BaseNameComponent*>(ent.FindComponent("name").get());
	if(pNameComponent == nullptr)
		return false;
	return m_bExactMatch ? ustring::match(pNameComponent->GetName(),m_name,m_bCaseSensitive) : ustring::compare(pNameComponent->GetName(),m_name,m_bCaseSensitive);
}

/////////////////

EntityIteratorFilterClass::EntityIteratorFilterClass(Game &game,const std::string &name,bool caseSensitive,bool exactMatch)
	: m_name(name),m_bCaseSensitive(caseSensitive),m_bExactMatch(exactMatch)
{}
bool EntityIteratorFilterClass::ShouldPass(BaseEntity &ent)
{
	return m_bExactMatch ? ustring::match(ent.GetClass(),m_name,m_bCaseSensitive) : ustring::compare(ent.GetClass(),m_name,m_bCaseSensitive);
}

/////////////////

EntityIteratorFilterNameOrClass::EntityIteratorFilterNameOrClass(Game &game,const std::string &name,bool caseSensitive,bool exactMatch)
	: m_name(name),m_bCaseSensitive(caseSensitive),m_bExactMatch(exactMatch)
{}
bool EntityIteratorFilterNameOrClass::ShouldPass(BaseEntity &ent)
{
	if(m_bExactMatch ? ustring::match(ent.GetClass(),m_name,m_bCaseSensitive) : ustring::compare(ent.GetClass(),m_name,m_bCaseSensitive))
		return true;
	auto pNameComponent = static_cast<pragma::BaseNameComponent*>(ent.FindComponent("name").get());
	return pNameComponent != nullptr && m_bExactMatch ? ustring::match(pNameComponent->GetName(),m_name,m_bCaseSensitive) : ustring::compare(pNameComponent->GetName(),m_name,m_bCaseSensitive);
}

/////////////////

EntityIteratorFilterEntity::EntityIteratorFilterEntity(Game &game,const std::string &name)
	: m_name(name)
{
	auto &componentManager = game.GetEntityComponentManager();
	componentManager.GetComponentTypeId("filter_name",m_filterNameComponentId);
	componentManager.GetComponentTypeId("filter_class",m_filterClassComponentId);

	EntityIterator entIt{game};
	entIt.AttachFilter<EntityIteratorFilterName>(name);
	for(auto *ent : entIt)
	{
		auto pFilterComponent = dynamic_cast<pragma::BaseFilterComponent*>(ent->FindComponent(m_filterNameComponentId).get());
		if(pFilterComponent == nullptr)
			pFilterComponent = dynamic_cast<pragma::BaseFilterComponent*>(ent->FindComponent(m_filterClassComponentId).get());
		if(pFilterComponent == nullptr)
			continue;
		m_filterEnts.push_back(std::static_pointer_cast<pragma::BaseFilterComponent>(pFilterComponent->shared_from_this()));
	}
}
bool EntityIteratorFilterEntity::ShouldPass(BaseEntity &ent)
{
	for(auto &hFilter : m_filterEnts)
	{
		if(hFilter.expired())
			continue;
		if(hFilter->ShouldPass(ent))
			return true;
	}
	if(ustring::compare(ent.GetClass(),m_name,false))
		return true;
	auto pNameComponent = static_cast<pragma::BaseNameComponent*>(ent.FindComponent("name").get());
	return pNameComponent != nullptr && ustring::compare(pNameComponent->GetName(),m_name,false);
}

/////////////////

EntityIteratorFilterFlags::EntityIteratorFilterFlags(Game &game,EntityIterator::FilterFlags flags)
	: m_flags(flags)
{}
bool EntityIteratorFilterFlags::ShouldPass(BaseEntity &ent)
{
	auto bIncludeEntity = false;
	if(ent.IsSpawned())
	{
		if((m_flags &EntityIterator::FilterFlags::Spawned) != EntityIterator::FilterFlags::None)
			bIncludeEntity = true;
	}
	else if((m_flags &EntityIterator::FilterFlags::Pending) != EntityIterator::FilterFlags::None)
		bIncludeEntity = true;
	if(bIncludeEntity == false)
		return false;

	if(ent.IsNetworkLocal() == false)
	{
		if((m_flags &EntityIterator::FilterFlags::IncludeShared) != EntityIterator::FilterFlags::None)
			bIncludeEntity = true;
	}
	else if((m_flags &EntityIterator::FilterFlags::IncludeNetworkLocal) != EntityIterator::FilterFlags::None)
		bIncludeEntity = true;
	if(bIncludeEntity == false || (m_flags &EntityIterator::FilterFlags::AnyType) == EntityIterator::FilterFlags::AnyType || (m_flags &EntityIterator::FilterFlags::AnyType) == EntityIterator::FilterFlags::None)
	{
		if((m_flags &EntityIterator::FilterFlags::HasTransform) != EntityIterator::FilterFlags::None && ent.GetTransformComponent().valid())
			return true;

		if((m_flags &EntityIterator::FilterFlags::HasModel) != EntityIterator::FilterFlags::None && ent.GetModelComponent().valid())
			return true;
		return bIncludeEntity;
	}
	if((m_flags &EntityIterator::FilterFlags::HasTransform) != EntityIterator::FilterFlags::None && ent.GetTransformComponent().expired())
		return false;
	if((m_flags &EntityIterator::FilterFlags::HasModel) != EntityIterator::FilterFlags::None && ent.GetModelComponent().expired())
		return false;

	if((m_flags &EntityIterator::FilterFlags::Character) != EntityIterator::FilterFlags::None && ent.IsCharacter())
		return true;
	if((m_flags &EntityIterator::FilterFlags::Player) != EntityIterator::FilterFlags::None && ent.IsPlayer())
		return true;
	if((m_flags &EntityIterator::FilterFlags::Weapon) != EntityIterator::FilterFlags::None && ent.IsWeapon())
		return true;
	if((m_flags &EntityIterator::FilterFlags::Vehicle) != EntityIterator::FilterFlags::None && ent.IsVehicle())
		return true;
	if((m_flags &EntityIterator::FilterFlags::NPC) != EntityIterator::FilterFlags::None && ent.IsNPC())
		return true;
	if((m_flags &EntityIterator::FilterFlags::Scripted) != EntityIterator::FilterFlags::None && ent.IsScripted())
		return true;
	if((m_flags &EntityIterator::FilterFlags::MapEntity) != EntityIterator::FilterFlags::None && ent.IsMapEntity())
		return true;
	if((m_flags &EntityIterator::FilterFlags::Physical) != EntityIterator::FilterFlags::None)
	{
		auto pPhysComponent = ent.GetPhysicsComponent();
		if(pPhysComponent.valid() && pPhysComponent->GetPhysicsObject() != nullptr)
			return true;
	}
	return false;
}

/////////////////

EntityIteratorFilterComponent::EntityIteratorFilterComponent(Game &game,pragma::ComponentId componentId)
	: m_componentId(componentId)
{}
EntityIteratorFilterComponent::EntityIteratorFilterComponent(Game &game,const std::string &componentName)
{
	auto &componentManager = game.GetEntityComponentManager();
	componentManager.GetComponentTypeId(componentName,m_componentId);
}

bool EntityIteratorFilterComponent::ShouldPass(BaseEntity &ent)
{
	return ent.HasComponent(m_componentId);
}

/////////////////

EntityIteratorFilterUser::EntityIteratorFilterUser(Game &game,const std::function<bool(BaseEntity&)> &fUserFilter)
	: m_fUserFilter(fUserFilter)
{}

bool EntityIteratorFilterUser::ShouldPass(BaseEntity &ent)
{
	return m_fUserFilter(ent);
}

/////////////////

EntityIteratorFilterSphere::EntityIteratorFilterSphere(Game &game,const Vector3 &origin,float radius)
	: m_origin(origin),m_radius(radius)
{}

bool EntityIteratorFilterSphere::ShouldPass(BaseEntity &ent,Vector3 &outClosestPointOnEntityBounds,float &outDistToEntity) const
{
	auto pTrComponent = ent.GetTransformComponent();
	if(pTrComponent.expired())
		return false;
	auto pPhysComponent = ent.GetPhysicsComponent();
	Vector3 colCenter;
	auto &pos = pTrComponent->GetPosition();
	auto colRadius = pPhysComponent.valid() ? pPhysComponent->GetCollisionRadius(&colCenter) : 0.f;
	colCenter += pos;
	auto distToCol = uvec::distance(m_origin,colCenter) -colRadius;
	if(distToCol > m_radius)
		return false;
	Vector3 min {};
	Vector3 max {};
	if(pPhysComponent.valid())
		pPhysComponent->GetCollisionBounds(&min,&max);
	Geometry::ClosestPointOnAABBToPoint(min,max,m_origin -pos,&outClosestPointOnEntityBounds);
	outDistToEntity = uvec::length(outClosestPointOnEntityBounds);
	return outDistToEntity <= m_radius;
}

bool EntityIteratorFilterSphere::ShouldPass(BaseEntity &ent)
{
	Vector3 r;
	float d;
	return ShouldPass(ent,r,d);
}

/////////////////

EntityIteratorFilterBox::EntityIteratorFilterBox(Game &game,const Vector3 &min,const Vector3 &max)
	: m_min(min),m_max(max)
{}

bool EntityIteratorFilterBox::ShouldPass(BaseEntity &ent)
{
	auto pTrComponent = ent.GetTransformComponent();
	if(pTrComponent.expired())
		return false;
	auto pPhysComponent = ent.GetPhysicsComponent();
	Vector3 entMin {};
	Vector3 entMax {};
	if(pPhysComponent.valid())
		pPhysComponent->GetCollisionBounds(&entMin,&entMax);
	return Intersection::AABBAABB(m_min,m_max,entMin,entMax) != INTERSECT_OUTSIDE;
}

/////////////////

EntityIteratorFilterCone::EntityIteratorFilterCone(Game &game,const Vector3 &origin,const Vector3 &dir,float radius,float angle)
	: EntityIteratorFilterSphere(game,origin,radius),m_direction(dir),m_angle(static_cast<float>(umath::cos(static_cast<float>(umath::deg_to_rad(angle)))))
{}

bool EntityIteratorFilterCone::ShouldPass(BaseEntity &ent)
{
	Vector3 pos;
	float dist;
	if(EntityIteratorFilterSphere::ShouldPass(ent,pos,dist) == false)
		return false;
	auto dirToOrigin = pos -m_origin;
	uvec::normalize(&dirToOrigin);
	auto dot = uvec::dot(m_direction,dirToOrigin);
	return dot < m_angle;
}
