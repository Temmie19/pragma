/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/lua/classes/components/c_lentity_components.hpp"
#include <prosper_command_buffer.hpp>

void Lua::Flex::register_class(lua_State *l,luabind::module_ &entsMod)
{
	auto defCFlex = luabind::class_<CFlexHandle,BaseEntityComponentHandle>("FlexComponent");
	Lua::register_base_flex_component_methods<luabind::class_<CFlexHandle,BaseEntityComponentHandle>,CFlexHandle>(l,defCFlex);
	defCFlex.def("SetFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t,float,float,bool)>(
		[](lua_State *l,CFlexHandle &hEnt,uint32_t flexId,float value,float duration,bool clampToLimits
			) {
				pragma::Lua::check_component(l,hEnt);
				hEnt->SetFlexController(flexId,value,duration,clampToLimits);
		}));
	defCFlex.def("SetFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t,float,float)>(
		[](lua_State *l,CFlexHandle &hEnt,uint32_t flexId,float value,float duration
			) {
				pragma::Lua::check_component(l,hEnt);
				hEnt->SetFlexController(flexId,value,duration);
		}));
	defCFlex.def("SetFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t,float)>(
		[](lua_State *l,CFlexHandle &hEnt,uint32_t flexId,float value
			) {
				pragma::Lua::check_component(l,hEnt);
				hEnt->SetFlexController(flexId,value);
		}));
	defCFlex.def("SetFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,const std::string&,float,float,bool)>(
		[](lua_State *l,CFlexHandle &hEnt,const std::string &flexName,float value,float duration,bool clampToLimits
			) {
				pragma::Lua::check_component(l,hEnt);
				hEnt->SetFlexController(flexName,value,duration,clampToLimits);
		}));
	defCFlex.def("SetFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,const std::string&,float,float)>(
		[](lua_State *l,CFlexHandle &hEnt,const std::string &flexName,float value,float duration
			) {
				pragma::Lua::check_component(l,hEnt);
				hEnt->SetFlexController(flexName,value,duration);
		}));
	defCFlex.def("SetFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,const std::string&,float)>(
		[](lua_State *l,CFlexHandle &hEnt,const std::string &flexName,float value
			) {
				pragma::Lua::check_component(l,hEnt);
				hEnt->SetFlexController(flexName,value);
		}));
	defCFlex.def("GetFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t)>(&Lua::Flex::GetFlexController));
	defCFlex.def("GetFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,const std::string&)>(&Lua::Flex::GetFlexController));
	defCFlex.def("GetScaledFlexController",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t)>(
		[](lua_State *l,CFlexHandle &hEnt,uint32_t flexControllerId
			) {
				pragma::Lua::check_component(l,hEnt);
				float value;
				if(hEnt->GetScaledFlexController(flexControllerId,value) == false)
					return;
				Lua::PushNumber(l,value);
		}));
	defCFlex.def("SetFlexControllerScale",static_cast<void(*)(lua_State*,CFlexHandle&,float)>(
		[](lua_State *l,CFlexHandle &hEnt,float scale
			) {
				pragma::Lua::check_component(l,hEnt);
				hEnt->SetFlexControllerScale(scale);
		}));
	defCFlex.def("GetFlexControllerScale",static_cast<void(*)(lua_State*,CFlexHandle&)>(
		[](lua_State *l,CFlexHandle &hEnt
			) {
				pragma::Lua::check_component(l,hEnt);
				Lua::PushNumber(l,hEnt->GetFlexControllerScale());
		}));
	defCFlex.def("CalcFlexValue",&Lua::Flex::CalcFlexValue);
	defCFlex.def("GetFlexWeights",static_cast<void(*)(lua_State*,CFlexHandle&)>([](lua_State *l,CFlexHandle &hEnt) {
		pragma::Lua::check_component(l,hEnt);
		auto &flexWeights = hEnt->GetFlexWeights();
		auto t = Lua::CreateTable(l);
		for(auto i=decltype(flexWeights.size()){0u};i<flexWeights.size();++i)
		{
			Lua::PushInt(l,i +1);
			Lua::PushNumber(l,flexWeights.at(i));
			Lua::SetTableValue(l,t);
		}
	}));
	defCFlex.def("GetFlexWeight",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t)>([](lua_State *l,CFlexHandle &hEnt,uint32_t flexId) {
		pragma::Lua::check_component(l,hEnt);
		float weight;
		if(hEnt->GetFlexWeight(flexId,weight) == false)
			return;
		Lua::PushNumber(l,weight);
	}));
	defCFlex.def("SetFlexWeight",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t,float)>([](lua_State *l,CFlexHandle &hEnt,uint32_t flexId,float weight) {
		pragma::Lua::check_component(l,hEnt);
		hEnt->SetFlexWeight(flexId,weight);
	}));
	defCFlex.def("ClearFlexWeightOverride",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t)>([](lua_State *l,CFlexHandle &hEnt,uint32_t flexId) {
		pragma::Lua::check_component(l,hEnt);
		hEnt->ClearFlexWeightOverride(flexId);
	}));
	defCFlex.def("HasFlexWeightOverride",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t)>([](lua_State *l,CFlexHandle &hEnt,uint32_t flexId) {
		pragma::Lua::check_component(l,hEnt);
		Lua::PushBool(l,hEnt->HasFlexWeightOverride(flexId));
	}));
	defCFlex.def("SetFlexWeightOverride",static_cast<void(*)(lua_State*,CFlexHandle&,uint32_t,float)>([](lua_State *l,CFlexHandle &hEnt,uint32_t flexId,float weight) {
		pragma::Lua::check_component(l,hEnt);
		hEnt->SetFlexWeightOverride(flexId,weight);
	}));
	entsMod[defCFlex];
}
