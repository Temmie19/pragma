/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __PHYS_BASE_HPP__
#define __PHYS_BASE_HPP__

#include "pragma/networkdefinitions.h"
#include <pragma/lua/luaapi.h>
#include <memory>
#include <sharedutils/util_shared_handle.hpp>
#include <vector>

class PhysObj;
namespace pragma::physics
{
	class IEnvironment;
	class DLLNETWORK IBase
		: public std::enable_shared_from_this<IBase>
	{
	public:
		IBase(const IBase&)=delete;
		IBase &operator=(const IBase&)=delete;
		virtual ~IBase()=default;

		util::TWeakSharedHandle<IBase> GetHandle() const;
		util::TSharedHandle<IBase> ClaimOwnership() const;

		virtual bool IsConstraint() const;
		virtual bool IsCollisionObject() const;
		virtual bool IsController() const;

		virtual void OnRemove();
		virtual void Initialize();
		virtual void InitializeLuaObject(lua_State *lua);
		luabind::object &GetLuaObject();
		const luabind::object &GetLuaObject() const;
		void Push(lua_State *l);
		
		void *GetUserData() const;
		PhysObj *GetPhysObj() const;
		void SetPhysObj(PhysObj &physObj);
	protected:
		friend IEnvironment;
		friend PhysObj;
		IBase(IEnvironment &env);
		void SetUserData(void *userData) const;
		virtual void InitializeLuaHandle(lua_State *l,const util::TWeakSharedHandle<IBase> &handle);
		template<class T>
			void InitializeLuaObject(lua_State *lua);

		IEnvironment &m_physEnv;
		util::TWeakSharedHandle<IBase> m_handle = {};

		std::unique_ptr<luabind::object> m_luaObj = nullptr;
	private:
		mutable void *m_userData = nullptr;
		mutable PhysObj *m_physObj = nullptr;
	};

	class DLLNETWORK IWorldObject
	{
	public:
		bool IsSpawned() const;
		void AddWorldObject();
		void Spawn();
		virtual void RemoveWorldObject()=0;
		virtual void DoAddWorldObject()=0;
	protected:
		virtual void DoSpawn();
	private:
		bool m_bSpawned = false;
	};
};
template<class T>
	void pragma::physics::IBase::InitializeLuaObject(lua_State *lua)
{
	auto handle = ClaimOwnership();
	if(handle.IsValid())
		m_luaObj = std::make_unique<luabind::object>(lua,util::shared_handle_cast<IBase,T>(handle));
	else
		m_luaObj = std::make_unique<luabind::object>(lua,std::dynamic_pointer_cast<T>(shared_from_this()));
}

#endif
