/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_SKYBOX_H__
#define __C_SKYBOX_H__
#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include <pragma/entities/baseskybox.h>

namespace pragma
{
	class DLLCLIENT CSkyboxComponent final
		: public BaseSkyboxComponent,
		public CBaseNetComponent
	{
	public:
		CSkyboxComponent(BaseEntity &ent) : BaseSkyboxComponent(ent) {}
		virtual void Initialize() override;
		virtual void OnRemove() override;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;

		virtual Bool ReceiveNetEvent(pragma::NetEventId eventId,NetPacket &packet) override;
		virtual void ReceiveData(NetPacket &packet) override;
		virtual bool ShouldTransmitNetData() const override {return true;}
	private:
		bool CreateCubemapFromIndividualTextures(const std::string &materialPath,const std::string &postfix="") const;
		void ValidateMaterials();
		CallbackHandle m_cbOnModelMaterialsLoaded = {};
	};
};

class DLLCLIENT CSkybox
	: public CBaseEntity
{
public:
	virtual void Initialize() override;
};

#endif