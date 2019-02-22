#ifndef __LALSOUND_H__
#define __LALSOUND_H__

#include "pragma/networkdefinitions.h"
#include <pragma/lua/luaapi.h>
#include <mathutil/glmutil.h>

struct lua_State;
class ALSound;
namespace Lua
{
	namespace ALSound
	{
		DLLNETWORK void register_class(luabind::class_<std::shared_ptr<::ALSound>> &classDef);
	};
};

#endif
