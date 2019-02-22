#ifndef __C_PROP_BASE_HPP__
#define __C_PROP_BASE_HPP__

#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/entities/components/c_entity_component.hpp"
#include <pragma/entities/prop/prop_base.h>
#include <pragma/physics/movetypes.h>

namespace pragma
{
	class DLLCLIENT CPropComponent final
		: public BasePropComponent,
		public CBaseNetComponent
	{
	public:
		CPropComponent(BaseEntity &ent) : BasePropComponent(ent) {}
		virtual void Initialize() override;
		virtual void ReceiveData(NetPacket &packet) override;
		virtual void OnEntitySpawn() override;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
		virtual bool ShouldTransmitNetData() const override {return true;}
	protected:
		double m_sqrMaxVisibleDist = 0.f;
		PHYSICSTYPE m_propPhysType = PHYSICSTYPE::NONE;
		MOVETYPE m_propMoveType = MOVETYPE::NONE;
	};
};

class DLLCLIENT CBaseProp
	: public CBaseEntity
{
protected:
};

#endif
