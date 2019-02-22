#include "stdafx_client.h"
#include "pragma/entities/point/constraints/c_point_constraint_ballsocket.h"
#include "pragma/entities/c_entityfactories.h"
#include <pragma/networking/nwm_util.h>
#include "pragma/lua/c_lentity_handles.hpp"

using namespace pragma;

LINK_ENTITY_TO_CLASS(point_constraint_ballsocket,CPointConstraintBallSocket);

void CPointConstraintBallSocketComponent::ReceiveData(NetPacket &packet)
{
	m_kvSource = packet->ReadString();
	m_kvTarget = packet->ReadString();
	m_posTarget = nwm::read_vector(packet);
}
luabind::object CPointConstraintBallSocketComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CPointConstraintBallSocketComponentHandleWrapper>(l);}

void CPointConstraintBallSocket::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CPointConstraintBallSocketComponent>();
}
