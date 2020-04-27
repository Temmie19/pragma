/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_WATER_SURFACE_HPP__
#define __C_WATER_SURFACE_HPP__

#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/c_water_object.hpp"

class PhysWaterSurfaceSimulator;
namespace pragma
{
	class CWaterComponent;
	class DLLCLIENT CWaterSurfaceComponent final
		: public BaseEntityComponent,
		public CWaterObject
	{
	public:
		CWaterSurfaceComponent(BaseEntity &ent) : BaseEntityComponent(ent) {}
		virtual void Initialize() override;

		virtual ~CWaterSurfaceComponent() override;
		void SetSurfaceSimulator(const std::shared_ptr<PhysWaterSurfaceSimulator> &simulator);
		virtual CMaterial *GetWaterMaterial() const override;
		void SetWaterObject(CWaterComponent *ent);
		CModelSubMesh *GetWaterSurfaceMesh() const;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
		virtual const Vector3 &GetPosition() const override;
		virtual const Quat &GetOrientation() const override;
		virtual void OnEntitySpawn() override;
	protected:
		std::shared_ptr<PhysWaterSurfaceSimulator> m_surfaceSimulator = nullptr;
		mutable std::weak_ptr<CModelSubMesh> m_waterSurfaceMesh = {};
		CallbackHandle m_cbRenderSurface = {};
		util::WeakHandle<CWaterComponent> m_hFuncWater = {};
		void UpdateSurfaceMesh();
		void InitializeSurface();
		void DestroySurface();
	};
};

class PhysWaterSurfaceSimulator;
class CFuncWater;
class DLLCLIENT CWaterSurface
	: public CBaseEntity
{
public:
	virtual void Initialize() override;
};

#endif
