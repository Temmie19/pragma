#ifndef __LIMPORT_HPP__
#define __LIMPORT_HPP__

#include "pragma/networkdefinitions.h"
#include "pragma/lua/ldefinitions.h"

namespace Lua
{
	namespace import
	{
		DLLNETWORK int import_wrci(lua_State *l);
		DLLNETWORK int import_wad(lua_State *l);
		DLLNETWORK int import_wrmi(lua_State *l);
		DLLNETWORK int import_smd(lua_State *l);
		DLLNETWORK int import_obj(lua_State *l);
		DLLNETWORK int import_pmx(lua_State *l);
		DLLNETWORK int import_model_asset(lua_State *l);
	};
};

#endif
