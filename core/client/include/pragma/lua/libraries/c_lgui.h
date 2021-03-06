/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef _C_LGUI_H__
#define _C_LGUI_H__

#include "pragma/clientdefinitions.h"
#include <pragma/lua/ldefinitions.h>

namespace Lua
{
	namespace gui
	{
		DLLCLIENT int create(lua_State *l);
		DLLCLIENT int create_label(lua_State *l);
		DLLCLIENT int create_button(lua_State *l);
		DLLCLIENT int create_checkbox(lua_State *l);
		DLLCLIENT int register_element(lua_State *l);
		DLLCLIENT int get_base_element(lua_State *l);
		DLLCLIENT int get_element_at_position(lua_State *l,int32_t *optX=nullptr,int32_t *optY=nullptr);
		DLLCLIENT int get_element_under_cursor(lua_State *l);
		DLLCLIENT int get_focused_element(lua_State *l);
		DLLCLIENT int register_skin(lua_State *l);
		DLLCLIENT int load_skin(lua_State *l);
		DLLCLIENT int set_skin(lua_State *l);
		DLLCLIENT int skin_exists(lua_State *l);
		DLLCLIENT int get_cursor(lua_State *l);
		DLLCLIENT int set_cursor(lua_State *l);
		DLLCLIENT int get_cursor_input_mode(lua_State *l);
		DLLCLIENT int set_cursor_input_mode(lua_State *l);
		DLLCLIENT int get_window_size(lua_State *l);
		DLLCLIENT int inject_mouse_input(lua_State *l);
		DLLCLIENT int inject_keyboard_input(lua_State *l);
		DLLCLIENT int inject_char_input(lua_State *l);
		DLLCLIENT int inject_scroll_input(lua_State *l);
	};
};

DLLCLIENT int Lua_gui_RealTime(lua_State *l);
DLLCLIENT int Lua_gui_DeltaTime(lua_State *l);
DLLCLIENT int Lua_gui_LastThink(lua_State *l);

#endif