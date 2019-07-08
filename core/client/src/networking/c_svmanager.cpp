#include "stdafx_client.h"
#include "pragma/clientstate/clientutil.h"
#include "pragma/c_engine.h"
#include "pragma/console/c_cvar_server.h"
#include "pragma/networking/iclient.hpp"
#include "pragma/networking/standard_client.hpp"
#include <pragma/networking/nwm_util.h>
#include <sharedutils/util.h>
#include <pragma/networking/error.hpp>
#include <pragma/networking/enums.hpp>
#include <pragma/networking/portinfo.h>
#include <pragma/networking/netmessages.h>

extern DLLCENGINE CEngine *c_engine;
static std::vector<std::string> s_lastConnection {};
void ClientState::Connect(std::string ip,std::string port)
{
	EndGame();
	s_lastConnection = {ip,port};
#ifdef DEBUG_SOCKET
	Con::ccl<<"Connecting to "<<ip<<":"<<port<<"..."<<Con::endl;
#endif
	Disconnect();
	m_client = std::make_unique<pragma::networking::StandardClient>();
	if(m_client == nullptr)
		return;
	pragma::networking::Error err;
	if(m_client->Connect(ip,CUInt16(util::to_int(port)),err) == false)
		Con::cwar<<"WARNING: Unable to connect to '"<<ip<<":"<<port<<"': "<<err.GetMessage()<<"!"<<Con::endl;
}

///////////////////////////

extern ClientState *client;
DLLCLIENT void CMD_cl_rcon(NetworkState*,pragma::BasePlayerComponent*,std::vector<std::string> &argv)
{
	if(!client->IsConnected() || argv.empty())
		return;
	std::string pass = c_engine->GetConVarString("rcon_password");
	NetPacket p;
	p->WriteString(pass);
	p->WriteString(argv[0]);
	client->SendPacket("rcon",p,pragma::networking::Protocol::SlowReliable);
}

DLLCLIENT void CMD_connect(NetworkState *state,pragma::BasePlayerComponent *pl,std::vector<std::string> &argv)
{
	if(argv.empty())
	{
		if(s_lastConnection.empty())
		{
			Con::cout<<"No previous connection attempt has been made! Please supply a destination address."<<Con::endl;
			return;
		}
		CMD_connect(state,pl,s_lastConnection);
		return;
	}
	if(argv.size() == 1)
	{
		auto &address = argv[0];
		std::string ip;
		std::string port;
		ustring::remove_whitespace(address);
		if(!address.empty() && address[0] == '[') // IPv6
		{
			auto pos = address.find_first_of(']');
			if(pos != ustring::NOT_FOUND)
			{
				ip = address.substr(1,pos -1);
				auto posPort = address.find_first_of(':',pos +1);
				if(posPort != ustring::NOT_FOUND)
					port = address.substr(posPort +1,address.length());
			}
			else
				ip = address;
		}
		else
		{
			auto posPort = address.find_first_of(':');
			if(posPort != ustring::NOT_FOUND)
			{
				auto posIpv6 = address.find_last_of(':');
				if(posIpv6 != posPort) // IPv6
				{
					if(posIpv6 == (posPort +1))
					{
						std::vector<std::string> argv = {std::string("[") +address +std::string("]")};
						CMD_connect(state,pl,argv);
						return;
					}
					std::vector<std::string> argv = {
						address.substr(0,posIpv6),
						address.substr(posIpv6 +1,address.length())
					};
					CMD_connect(state,pl,argv);
					return;
				}
				ip = address.substr(0,posPort);
				port = address.substr(posPort +1,address.length());
			}
			else
				ip = address;
		}
		if(port.empty())
			port = sci::DEFAULT_PORT_TCP;
		c_engine->Connect(ip,port);
		return;
	}
	c_engine->Connect(argv[0],argv[1]);
}

DLLCLIENT void CMD_disconnect(NetworkState*,pragma::BasePlayerComponent*,std::vector<std::string>&)
{
	c_engine->EndGame();
}

DLLCLIENT void CMD_cl_send(NetworkState*,pragma::BasePlayerComponent*,std::vector<std::string> &argv)
{
	if(argv.empty()) return;
	NetPacket packet;
	packet->WriteString(argv[0]);
	client->SendPacket("cl_send",packet,pragma::networking::Protocol::SlowReliable);
}

DLLCLIENT void CMD_cl_send_udp(NetworkState*,pragma::BasePlayerComponent*,std::vector<std::string> &argv)
{
	if(argv.empty())
		return;
	NetPacket packet;
	packet->WriteString(argv[0]);
	client->SendPacket("cl_send",packet,pragma::networking::Protocol::FastUnreliable);
}

void CMD_cl_debug_netmessages(NetworkState *state,pragma::BasePlayerComponent *pl,std::vector<std::string> &argv)
{
	auto *cl = client->GetClient();
	if(cl == nullptr)
	{
		Con::cwar<<"WARNING: No client is active!"<<Con::endl;
		return;
	}
	if(argv.size() > 0)
	{
		auto numBacklog = ustring::to_int(argv.front());
		cl->SetMemoryCount(numBacklog);
		Con::cout<<"Debug backlog has been set to "<<numBacklog<<Con::endl;
		return;
	}
	auto *svMap = GetServerMessageMap();
	std::unordered_map<std::string,uint32_t> *svMsgs;
	svMap->GetNetMessages(&svMsgs);

	auto *clMap = GetClientMessageMap();
	std::unordered_map<std::string,uint32_t> *clMsgs;
	clMap->GetNetMessages(&clMsgs);

	cl->DebugPrint(*clMsgs,*svMsgs);
	cl->DebugDump("cl_netmessages.dump",*clMsgs,*svMsgs);
}
