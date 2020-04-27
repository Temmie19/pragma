/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/lua/classes/components/c_lentity_components.hpp"
#include "pragma/model/c_modelmesh.h"
#include <prosper_command_buffer.hpp>

void Lua::VertexAnimated::register_class(lua_State *l,luabind::module_ &entsMod)
{
	auto defCVertexAnimated = luabind::class_<CVertexAnimatedHandle,BaseEntityComponentHandle>("VertexAnimatedComponent");
	defCVertexAnimated.def("UpdateVertexAnimationBuffer",static_cast<void(*)(lua_State*,CVertexAnimatedHandle&,std::shared_ptr<prosper::ICommandBuffer>&)>([](lua_State *l,CVertexAnimatedHandle &hAnim,std::shared_ptr<prosper::ICommandBuffer> &drawCmd) {
		pragma::Lua::check_component(l,hAnim);
		if(drawCmd->IsPrimary() == false)
			return;
		hAnim->UpdateVertexAnimationBuffer(std::dynamic_pointer_cast<prosper::IPrimaryCommandBuffer>(drawCmd));
		}));
	defCVertexAnimated.def("GetVertexAnimationBuffer",static_cast<void(*)(lua_State*,CVertexAnimatedHandle&)>([](lua_State *l,CVertexAnimatedHandle &hAnim) {
		pragma::Lua::check_component(l,hAnim);
		auto &buffer = hAnim->GetVertexAnimationBuffer();
		if(buffer == nullptr)
			return;
		Lua::Push<std::shared_ptr<prosper::IBuffer>>(l,buffer);
		}));
	defCVertexAnimated.def("GetVertexAnimationBufferMeshOffset",static_cast<void(*)(lua_State*,CVertexAnimatedHandle&,std::shared_ptr<::ModelSubMesh>&)>([](lua_State *l,CVertexAnimatedHandle &hAnim,std::shared_ptr<::ModelSubMesh> &subMesh) {
		pragma::Lua::check_component(l,hAnim);
		uint32_t offset;
		uint32_t animCount;
		auto b = hAnim->GetVertexAnimationBufferMeshOffset(static_cast<CModelSubMesh&>(*subMesh),offset,animCount);
		if(b == false)
			return;
		Lua::PushInt(l,offset);
		Lua::PushInt(l,animCount);
		}));
	defCVertexAnimated.def("GetLocalVertexPosition",static_cast<void(*)(lua_State*,CVertexAnimatedHandle&,std::shared_ptr<::ModelSubMesh>&,uint32_t)>([](lua_State *l,CVertexAnimatedHandle &hAnim,std::shared_ptr<::ModelSubMesh> &subMesh,uint32_t vertexId) {
		pragma::Lua::check_component(l,hAnim);
		Vector3 pos;
		auto b = hAnim->GetLocalVertexPosition(static_cast<CModelSubMesh&>(*subMesh),vertexId,pos);
		if(b == false)
			return;
		Lua::Push<Vector3>(l,pos);
		}));
	entsMod[defCVertexAnimated];
}
