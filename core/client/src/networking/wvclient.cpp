#include "stdafx_client.h"
#include "pragma/networking/wvclient.h"
#include <iostream>
#include <pragma/networking/netmessages.h>

#define DEBUG_CLIENT_VERBOSE 0

extern DLLCLIENT ClientState *client;

void WVClient::OnPacketSent(const NWMEndpoint &ep,const NetPacket &packet)
{
	nwm::Client::OnPacketSent(ep,packet);
#if DEBUG_CLIENT_VERBOSE == 1
	auto id = packet.GetMessageID();
	auto *svMap = GetServerMessageMap();
	std::unordered_map<std::string,uint32_t> *svMsgs;
	svMap->GetNetMessages(&svMsgs);
	auto it = std::find_if(svMsgs->begin(),svMsgs->end(),[id](const std::pair<std::string,uint32_t> &pair) {
		return (pair.second == id) ? true : false;
	});
	std::string msgName = (it != svMsgs->end()) ? it->first : "Unknown";
	Con::ccl<<"OnPacketSent: "<<msgName<<" ("<<id<<")"<<Con::endl;
#endif
	MemorizeNetMessage(MessageTracker::MessageType::Outgoing,packet.GetMessageID(),ep,packet);
}
void WVClient::OnPacketReceived(const NWMEndpoint &ep,unsigned int id,NetPacket &packet)
{
	nwm::Client::OnPacketReceived(ep,id,packet);
#if DEBUG_CLIENT_VERBOSE == 1
	auto *clMap = GetClientMessageMap();
	std::unordered_map<std::string,uint32_t> *clMsgs;
	clMap->GetNetMessages(&clMsgs);
	auto it = std::find_if(clMsgs->begin(),clMsgs->end(),[id](const std::pair<std::string,uint32_t> &pair) {
		return (pair.second == id) ? true : false;
	});
	std::string msgName = (it != clMsgs->end()) ? it->first : "Unknown";
	Con::ccl<<"OnPacketReceived: "<<msgName<<" ("<<id<<")"<<Con::endl;
#endif
	MemorizeNetMessage(MessageTracker::MessageType::Incoming,id,ep,packet);
}

bool WVClient::HandlePacket(const NWMEndpoint &ep,unsigned int id,NetPacket &packet)
{
#if DEBUG_CLIENT_VERBOSE == 1
	Con::ccl<<"HandlePacket: "<<id<<Con::endl;
#endif
	if(nwm::Client::HandlePacket(ep,id,packet) == true)
		return true;
	client->HandlePacket(packet);
	return true;
}
void WVClient::OnConnected()
{
#if DEBUG_CLIENT_VERBOSE == 1
	Con::ccl<<"Connected to server..."<<Con::endl;
#endif
	client->HandleConnect();
}
void WVClient::OnClosed()
{
	auto err = GetLastError();
#if DEBUG_CLIENT_VERBOSE == 1
	Con::ccl<<"Connection to server has closed: "<<err.message()<<" ("<<err.value()<<")"<<Con::endl;
#endif
}
void WVClient::OnDisconnected(nwm::ClientDropped reason)
{
#if DEBUG_CLIENT_VERBOSE == 1
	Con::ccl<<"Disconnected from server ("<<nwm::client_dropped_enum_to_string(reason)<<")..."<<Con::endl;
#endif
	m_bDisconnected = true;
}

WVClient::WVClient(const std::shared_ptr<CLNWMUDPConnection> &udp,std::shared_ptr<CLNWMTCPConnection> &tcp)
	: nwm::Client(udp,tcp)
{}

std::unique_ptr<WVClient> WVClient::Create(const std::string &serverIp,unsigned short serverPort)
{
	std::unique_ptr<WVClient> cl = nullptr;
	NWMException lastException("No error.");
	for(unsigned short localPort=27018;localPort<27034;localPort++)
	{
		try
		{
			cl = nwm::Client::Create<WVClient>(serverIp,serverPort,localPort,nwm::ConnectionType::TCPUDP);
			break;
		}
		catch(NWMException &e)
		{
			lastException = e;
		}
	}
	if(cl == nullptr)
	{
		Con::cwar<<"WARNING: Unable to connect to server '"<<serverIp<<":"<<serverPort<<": "<<lastException.what()<<Con::endl;
		return nullptr;
	}
#ifdef _DEBUG
	cl->SetTimeoutDuration(0.f);
#else
	cl->SetTimeoutDuration(client->GetConVarFloat("sv_timeout_duration"));
#endif
	//cl->SetPingEnabled(false);
	cl->Start();
	return cl;
}

bool WVClient::IsDisconnected() const {return m_bDisconnected;}

REGISTER_CONVAR_CALLBACK_CL(sv_timeout_duration,[](NetworkState*,ConVar*,float,float val) {
	if(client == nullptr)
		return;
	auto *cl = client->GetClient();
	if(cl == nullptr)
		return;
	cl->SetTimeoutDuration(val);
});
