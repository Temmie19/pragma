#ifndef __C_ENV_TIMESCALE_H__
#define __C_ENV_TIMESCALE_H__
#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/entities/environment/env_timescale.h"
#include "pragma/entities/components/c_entity_component.hpp"
#include <memory>

namespace pragma
{
	class DLLCLIENT CEnvTimescaleComponent final
		: public BaseEnvTimescaleComponent,
		public CBaseNetComponent
	{
	public:
		CEnvTimescaleComponent(BaseEntity &ent) : BaseEnvTimescaleComponent(ent) {}
		virtual void ReceiveData(NetPacket &packet) override;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
		virtual bool ShouldTransmitNetData() const override {return true;}
	};
};

class DLLCLIENT CEnvTimescale
	: public CBaseEntity
{
public:
	virtual void Initialize() override;
};

#endif