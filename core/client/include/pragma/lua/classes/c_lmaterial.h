/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_LMATERIAL_H__
#define __C_LMATERIAL_H__

#include "pragma/clientdefinitions.h"
#include "matsysdefinitions.h"
#include <pragma/lua/ldefinitions.h>
#include <texturemanager/texturemanager.h>
#include "pragma/lua/libraries/c_lua_vulkan.h"

class Material;
struct TextureInfo;
namespace Lua
{
	namespace Material
	{
		namespace Client
		{
			DLLCLIENT void SetTexture(lua_State *l,::Material *mat,const std::string &textureID,const std::string &tex);
			DLLCLIENT void SetTexture(lua_State *l,::Material *mat,const std::string &textureID,::Texture &tex);
			DLLCLIENT void SetTexture(lua_State *l,::Material *mat,const std::string &textureID,Lua::Vulkan::Texture &hTex);
			DLLCLIENT void GetTexture(lua_State *l,::Material *mat,const std::string &textureID);
			DLLCLIENT void GetData(lua_State *l,::Material *mat);
			DLLCLIENT void InitializeShaderData(lua_State *l,::Material *mat,bool reload);
			DLLCLIENT void InitializeShaderData(lua_State *l,::Material *mat);
		};
	};
	namespace TextureInfo
	{
		DLLCLIENT void GetTexture(lua_State *l,::TextureInfo *tex);
		DLLCLIENT void GetSize(lua_State *l,::TextureInfo *tex);
		DLLCLIENT void GetWidth(lua_State *l,::TextureInfo *tex);
		DLLCLIENT void GetHeight(lua_State *l,::TextureInfo *tex);
	};
};

#endif
