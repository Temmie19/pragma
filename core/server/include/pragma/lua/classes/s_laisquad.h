#ifndef __S_LAISQUAD_H__
#define __S_LAISQUAD_H__

#include "pragma/serverdefinitions.h"
#include <pragma/lua/ldefinitions.h>

namespace Lua
{
	namespace AISquad
	{
		DLLSERVER void register_class(lua_State *l,luabind::module_ &mod);
	};
};

#endif
