/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer */

#ifndef __S_LUTIL_H__
#define __S_LUTIL_H__

#include "pragma/serverdefinitions.h"
#include <pragma/lua/luaapi.h>

namespace Lua
{
	namespace util
	{
		namespace Server
		{
			DLLSERVER int fire_bullets(lua_State *l);
			DLLSERVER int create_explosion(lua_State *l);
			DLLSERVER int create_giblet(lua_State *l);
		};
	};
};

#endif
