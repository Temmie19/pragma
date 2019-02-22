#ifndef __ENTITY_ITERATOR_HPP__
#define __ENTITY_ITERATOR_HPP__

#include "pragma/networkdefinitions.h"
#include "pragma/entities/entity_component_manager.hpp"
#include <vector>

class BaseEntity;
class Game;
namespace pragma
{
	using ComponentId = uint32_t;
	class BaseFilterComponent;
};

struct DLLNETWORK IEntityIteratorFilter
{
	IEntityIteratorFilter()=default;
	IEntityIteratorFilter(Game &game) {}
	virtual bool ShouldPass(BaseEntity &ent)=0;
};

#pragma warning(push)
#pragma warning(disable : 4251)
struct BaseEntityContainer
{
	BaseEntityContainer(std::size_t count)
		: count{count}
	{}
	// Returns number of actual (non-NULL) items
	std::size_t Count() const {return count;}
	virtual std::size_t Size() const=0;
	virtual BaseEntity *At(std::size_t index)=0;
private:
	std::size_t count = 0ull;
};
struct EntityContainer
	: public BaseEntityContainer
{
	EntityContainer(std::vector<BaseEntity*> &ents,std::size_t count)
		: BaseEntityContainer(count),ents{ents}
	{}
	// Returns container size (potentially includes NULL elements)
	virtual std::size_t Size() const override;
	virtual BaseEntity *At(std::size_t index) override;
private:
	std::vector<BaseEntity*> &ents;
};
struct ComponentContainer
	: public BaseEntityContainer
{
	ComponentContainer(const std::vector<pragma::BaseEntityComponent*> &components,std::size_t count)
		: BaseEntityContainer(count),components{components}
	{}
	virtual std::size_t Size() const override;
	virtual BaseEntity *At(std::size_t index) override;
private:
	const std::vector<pragma::BaseEntityComponent*> &components;
};
struct EntityIteratorData
{
	EntityIteratorData(Game &game);
	EntityIteratorData(Game &game,const std::vector<pragma::BaseEntityComponent*> &components,std::size_t count);
	bool ShouldPass(BaseEntity &ent) const;
	std::size_t GetCount() const;
	Game &game;
	std::unique_ptr<BaseEntityContainer> entities = nullptr;
	std::vector<std::shared_ptr<IEntityIteratorFilter>> filters = {};
};

class EntityIterator;
class DLLNETWORK BaseEntityIterator
{
public:
	BaseEntityIterator(const std::shared_ptr<EntityIteratorData> &itData,bool bEndIterator);
	BaseEntityIterator(const BaseEntityIterator &other)=default;
	BaseEntityIterator &operator=(const BaseEntityIterator &other)=default;

	BaseEntityIterator &operator++();
	BaseEntityIterator operator++(int);
	BaseEntity *operator*();
	BaseEntity *operator->();
	bool operator==(const BaseEntityIterator &other);
	bool operator!=(const BaseEntityIterator &other);

	std::size_t GetCount() const;
private:
	bool ShouldPass(BaseEntity &ent) const;

	std::shared_ptr<EntityIteratorData> m_iteratorData;
	std::size_t m_currentIndex = 0ull;
};

class DLLNETWORK EntityIterator
{
public:
	friend BaseEntityIterator;
	enum class FilterFlags : uint32_t
	{
		None = 0u,
		Spawned = 1u,
		Pending = Spawned<<1u, // All entities that aren't spawned yet
		IncludeShared = Pending<<1u, // Include shared entities
		IncludeNetworkLocal = IncludeShared<<1u, // Include all entities that are either serverside- or clientside-only

		// Type flags; If none of these are set, ALL types will pass.
		// If at least one of these is set, only the set types will pass!
		Character = IncludeNetworkLocal<<1u,
		Player = Character<<1u,
		Weapon = Player<<1u,
		Vehicle = Weapon<<1u,
		NPC = Vehicle<<1u,
		Physical = NPC<<1u,
		Scripted = Physical<<1u,
		MapEntity = Scripted<<1u,
		//

		HasTransform = MapEntity<<1u,
		HasModel = HasTransform<<1u,

		AnyType = Character | Player | Weapon | Vehicle | NPC | Physical | Scripted | MapEntity,
		Default = Spawned | IncludeShared | IncludeNetworkLocal,
		Any = Spawned | Pending | IncludeShared | IncludeNetworkLocal | AnyType
	};

	EntityIterator(Game &game,FilterFlags filterFlags=FilterFlags::Default);
	EntityIterator(Game &game,pragma::ComponentId componentId,FilterFlags filterFlags=FilterFlags::Default);
	EntityIterator(Game &game,const std::string &componentName,FilterFlags filterFlags=FilterFlags::Default);
	EntityIterator(const EntityIterator&)=default;
	EntityIterator(EntityIterator&&)=delete;
	EntityIterator &operator=(const EntityIterator&)=default;
	EntityIterator &operator=(EntityIterator&&)=default;

	BaseEntityIterator begin() const;
	BaseEntityIterator end() const;
	std::size_t GetCount() const;

	template<class TFilter,typename... TARGS>
		void AttachFilter(TARGS ...args);
private:
	EntityIterator(Game &game,bool /* dummy */);

	template <typename T, typename = int>
		struct HasGetType : std::false_type { };

	template <typename T>
		struct HasGetType <T, decltype(&T::GetType, 0)> : std::true_type { };

	template<typename T>
		typename std::enable_if<HasGetType<T>::value>::type
			GetIteratorFilterComponentType(std::optional<std::type_index> &typeIndex)
	{
		typeIndex = T::GetType();
	}

	template<typename T>
		typename std::enable_if<!HasGetType<T>::value>::type
			GetIteratorFilterComponentType(std::optional<std::type_index> &typeIndex)
	{}

	void SetBaseComponentType(pragma::ComponentId componentId);
	void SetBaseComponentType(std::type_index typeIndex);
	void SetBaseComponentType(const std::string &componentName);

	std::shared_ptr<EntityIteratorData> m_iteratorData;
};
REGISTER_BASIC_BITWISE_OPERATORS(EntityIterator::FilterFlags)
#pragma warning(pop)

///////////////

template<class TFilter,typename... TARGS>
	void EntityIterator::AttachFilter(TARGS ...args)
{
	static_assert(std::is_base_of<IEntityIteratorFilter,TFilter>::value,"TFilter must be a descendant of IEntityIteratorFilter!");
	if(typeid(*m_iteratorData->entities) == typeid(EntityContainer))
	{
		if constexpr (std::is_same_v<EntityIteratorFilterComponent,TFilter>)
		{
			// If a component filter was attached, we can optimize by only iterating the components of
			// that type (instead of iterating over all entities). In this case we don't actually need to
			// attach a filter.
			SetBaseComponentType(std::forward<TARGS>(args)...);
			return;
		}

		std::optional<std::type_index> componentTypeIndex {};
		GetIteratorFilterComponentType<TFilter>(componentTypeIndex);
		if(componentTypeIndex.has_value())
		{
			// Same as the block above, but for TEntityIteratorFilterComponent
			SetBaseComponentType(*componentTypeIndex);
			return;
		}
	}
	m_iteratorData->filters.emplace_back(std::make_unique<TFilter>(m_iteratorData->game,std::forward<TARGS>(args)...));
}

#pragma warning(push)
#pragma warning(disable : 4251)
struct DLLNETWORK EntityIteratorFilterName
	: public IEntityIteratorFilter
{
	EntityIteratorFilterName(Game &game,const std::string &name,bool caseSensitive=false,bool exactMatch=true);
	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	std::string m_name;
	bool m_bCaseSensitive = false;
	bool m_bExactMatch = true;
};

struct DLLNETWORK EntityIteratorFilterClass
	: public IEntityIteratorFilter
{
	EntityIteratorFilterClass(Game &game,const std::string &name,bool caseSensitive=false,bool exactMatch=true);
	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	std::string m_name;
	bool m_bCaseSensitive = false;
	bool m_bExactMatch = true;
};

struct DLLNETWORK EntityIteratorFilterNameOrClass
	: public IEntityIteratorFilter
{
	EntityIteratorFilterNameOrClass(Game &game,const std::string &name,bool caseSensitive=false,bool exactMatch=true);
	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	std::string m_name;
	bool m_bCaseSensitive = false;
	bool m_bExactMatch = true;
};

struct DLLNETWORK EntityIteratorFilterEntity
	: public IEntityIteratorFilter
{
	EntityIteratorFilterEntity(Game &game,const std::string &name);
	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	std::vector<util::WeakHandle<pragma::BaseFilterComponent>> m_filterEnts;
	pragma::ComponentId m_filterNameComponentId = pragma::INVALID_COMPONENT_ID;
	pragma::ComponentId m_filterClassComponentId = pragma::INVALID_COMPONENT_ID;
	std::string m_name;
};

struct DLLNETWORK EntityIteratorFilterFlags
	: public IEntityIteratorFilter
{
	EntityIteratorFilterFlags(Game &game,EntityIterator::FilterFlags flags);
	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	EntityIterator::FilterFlags m_flags = EntityIterator::FilterFlags::None;
};

struct DLLNETWORK EntityIteratorFilterComponent
	: public IEntityIteratorFilter
{
	EntityIteratorFilterComponent(Game &game,pragma::ComponentId componentId);
	EntityIteratorFilterComponent(Game &game,const std::string &componentName);

	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	pragma::ComponentId m_componentId = pragma::INVALID_COMPONENT_ID;
};

struct DLLNETWORK EntityIteratorFilterUser
	: public IEntityIteratorFilter
{
	EntityIteratorFilterUser(Game &game,const std::function<bool(BaseEntity&)> &fUserFilter);

	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	std::function<bool(BaseEntity&)> m_fUserFilter = nullptr;
};

struct DLLNETWORK EntityIteratorFilterSphere
	: public IEntityIteratorFilter
{
	EntityIteratorFilterSphere(Game &game,const Vector3 &origin,float radius);

	virtual bool ShouldPass(BaseEntity &ent) override;
protected:
	bool ShouldPass(BaseEntity &ent,Vector3 &outClosestPointOnEntityBounds,float &outDistToEntity) const;

	Vector3 m_origin;
	float m_radius = 0.f;
};

struct DLLNETWORK EntityIteratorFilterBox
	: public IEntityIteratorFilter
{
	EntityIteratorFilterBox(Game &game,const Vector3 &min,const Vector3 &max);

	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	Vector3 m_min;
	Vector3 m_max;
};

struct DLLNETWORK EntityIteratorFilterCone
	: public EntityIteratorFilterSphere
{
	EntityIteratorFilterCone(Game &game,const Vector3 &origin,const Vector3 &dir,float radius,float angle);

	virtual bool ShouldPass(BaseEntity &ent) override;
private:
	Vector3 m_direction;
	float m_angle = 0.f;
};
#pragma warning(pop)

template<class TComponent>
	struct TEntityIteratorFilterComponent
		: public IEntityIteratorFilter
{
	using IEntityIteratorFilter::IEntityIteratorFilter;
	static std::type_index GetType() {return std::type_index(typeid(TComponent));}
	virtual bool ShouldPass(BaseEntity &ent) override
	{
		return ent.HasComponent<TComponent>();
	}
};

///////////////

template<class TComponent>
	class EntityComponentIterator
{
public:
	EntityComponentIterator()=default;
	EntityComponentIterator(std::vector<BaseEntity*> &ents)
		: m_ents(&ents)
	{}
	EntityComponentIterator &operator++()
	{
		while(++m_currentIndex < m_ents->size())
		{
			auto *ent = m_ents->at(m_currentIndex);
			if(ent != nullptr && ent->HasComponent<TComponent>())
				break;
		}
		return *this;
	}
	EntityComponentIterator operator++(int)
	{
		auto r = *this;
		operator++();
		return r;
	}
	TComponent &operator*() {return *operator->();}
	TComponent *operator->() {return m_ents->at(m_currentIndex)->GetComponent<TComponent>().get();}
	bool operator==(const EntityComponentIterator &other) {return m_ents == other.m_ents && m_currentIndex == other.m_currentIndex;}
	bool operator!=(const EntityComponentIterator &other) {return !operator==(other);}
private:
	std::size_t m_currentIndex = std::numeric_limits<std::size_t>::max(); // Note: Intentional overflow at first iteration
	std::vector<BaseEntity*> *m_ents = nullptr;
};

template<class TComponent>
	class TEntityIterator
{
public:
	TEntityIterator(Game &game)
		: m_game(game)
	{}
	EntityComponentIterator<TComponent> begin()
	{
		return EntityComponentIterator<TComponent>{m_game.GetBaseEntities()};
	}
	EntityComponentIterator<TComponent> end()
	{
		return EntityComponentIterator<TComponent>{};
	}
private:
	Game &m_game;
};

#endif
