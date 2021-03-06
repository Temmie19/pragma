/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_LINPUT_H__
#define __C_LINPUT_H__

#include "pragma/clientdefinitions.h"
#include <pragma/lua/ldefinitions.h>

namespace Lua
{
	namespace input
	{
		DLLCLIENT int get_mouse_button_state(lua_State *l);
		DLLCLIENT int get_key_state(lua_State *l);
		DLLCLIENT int add_callback(lua_State *l);
		DLLCLIENT int get_cursor_pos(lua_State *l);
		DLLCLIENT int get_controller_count(lua_State *l);
		DLLCLIENT int get_controller_name(lua_State *l);
		DLLCLIENT int get_joystick_axes(lua_State *l);
		DLLCLIENT int get_joystick_buttons(lua_State *l);
		DLLCLIENT int set_cursor_pos(lua_State *l);

		DLLCLIENT int key_to_string(lua_State *l);
		DLLCLIENT int key_to_text(lua_State *l);
		DLLCLIENT int string_to_key(lua_State *l);
		DLLCLIENT int get_mapped_keys(lua_State *l);
	};
};

#endif
