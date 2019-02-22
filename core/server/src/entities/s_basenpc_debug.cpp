#include "stdafx_server.h"
#if 0
#include "pragma/lua/classes/s_lai_behavior.h"
#include "pragma/ai/ai_schedule.h"
#include "pragma/entities/player.h"
#include <pragma/debug/debugbehaviortree.h>

extern DLLSERVER ServerState *server;
extern DLLSERVER SGame *s_game;

void SBaseNPC::_debugSendNavInfo(Player *pl)
{
	if(m_navInfo.pathInfo == nullptr)
		return;
	auto &path = *m_navInfo.pathInfo->path;

	auto nodePrev = GetPosition();
	NetPacket p {};
	nwm::write_entity(p,this);
	p->Write<uint32_t>(path.pathCount);
	p->Write<uint32_t>(m_navInfo.pathInfo->pathIdx);
	for(auto i=decltype(path.pathCount){0};i<path.pathCount;++i)
	{
		Vector3 node {};
		path.GetNode(i,nodePrev,node);
		p->Write<Vector3>(node);
		nodePrev = node;
	}
	server->SendPacketTCP("debug_ai_navigation",p,pl->GetClientSession());
}

void SBaseNPC::_debugSendScheduleInfo(Player *pl,std::shared_ptr<DebugBehaviorTreeNode> &dbgTree,std::shared_ptr<::ai::Schedule> &aiSchedule,float &tLastSchedUpdate)
{
	auto sched = GetCurrentSchedule();
	if(sched == nullptr)
	{
		if(aiSchedule != nullptr)
		{
			auto tDelta = s_game->CurTime() -tLastSchedUpdate;
			if(tDelta >= 4.f) // Only actually clear schedule if schedule has been finished for at least 4 seconds
			{
				NetPacket p {};
				p->Write<uint8_t>(static_cast<uint8_t>(0));
				server->SendPacketTCP("debug_ai_schedule_tree",p,pl->GetClientSession());
				aiSchedule = nullptr;
			}
		}
		return;
	}

	tLastSchedUpdate = s_game->CurTime();
	NetPacket p {};
	auto bFullUpdate = (sched != aiSchedule) ? true : false;
	if(bFullUpdate == false)
	{
		auto tCur = static_cast<float>(s_game->CurTime());
		std::function<int8_t(DebugBehaviorTreeNode&,const ai::BehaviorNode&,float)> fMarkChangedNodes = nullptr;
		fMarkChangedNodes = [&fMarkChangedNodes](DebugBehaviorTreeNode &dbgNode,const ai::BehaviorNode &taskNode,float t) -> int8_t {
			auto &childTaskNodes = taskNode.GetNodes();
			if(dbgNode.children.size() != childTaskNodes.size())
				return -1;
			int8_t r = 0;
			auto &dbgInfo = taskNode.GetDebugInfo();
			for(auto i=decltype(dbgNode.children.size()){0};i<dbgNode.children.size();++i)
			{
				auto rChild = fMarkChangedNodes(*dbgNode.children[i],*childTaskNodes[i],t);
				if(rChild == -1)
					return -1;
				if(rChild == 1)
					r = 1;
			}
			if(dbgNode.state != static_cast<DebugBehaviorTreeNode::State>(dbgInfo.lastResult))
			{
				dbgNode.state = static_cast<DebugBehaviorTreeNode::State>(dbgInfo.lastResult);
				dbgNode.lastUpdate = t;
				r = 1;
			}
			else if(r == 1)
				dbgNode.lastUpdate = t;
			return r;
		};
		auto r = fMarkChangedNodes(*dbgTree,sched->GetRootNode(),tCur);
		if(r == -1)
			bFullUpdate = true;
		else
		{
			if(r == 0) // No update required
				return;
			p->Write<uint8_t>(2);

			std::function<void(NetPacket&,DebugBehaviorTreeNode&,float)> fWriteChanges = nullptr;
			fWriteChanges = [&fWriteChanges](NetPacket &p,DebugBehaviorTreeNode &dbgNode,float t) {
				if(dbgNode.lastUpdate != t)
					p->Write<DebugBehaviorTreeNode::State>(DebugBehaviorTreeNode::State::Invalid); // Keep old state; Skip entire branch
				else
				{
					p->Write<DebugBehaviorTreeNode::State>(dbgNode.state);
					for(auto &child : dbgNode.children)
						fWriteChanges(p,*child,t);
				}
			};
			fWriteChanges(p,*dbgTree,tCur);
		}
	}
	if(bFullUpdate == true)
	{
		aiSchedule = sched;

		std::function<std::shared_ptr<DebugBehaviorTreeNode>(const ai::BehaviorNode&)> fAddNode = nullptr;
		fAddNode = [&fAddNode,&sched](const ai::BehaviorNode &node) {
			auto &dbgInfo = node.GetDebugInfo();
			auto dbgChildNode = std::make_shared<DebugBehaviorTreeNode>();
			auto *luaTask = dynamic_cast<const AILuaBehaviorNode*>(&node);
			if(dbgInfo.debugName.empty() == false)
				dbgChildNode->name = dbgInfo.debugName;
			else
			{
				std::stringstream ss;
				node.Print(sched.get(),ss);
				dbgChildNode->name = ss.str();
			}
			dbgChildNode->nodeType = static_cast<DebugBehaviorTreeNode::BehaviorNodeType>(node.GetType());
			dbgChildNode->selectorType = static_cast<DebugBehaviorTreeNode::SelectorType>(node.GetSelectorType());
			dbgChildNode->state = static_cast<DebugBehaviorTreeNode::State>(dbgInfo.lastResult);

			for(auto &child : node.GetNodes())
				dbgChildNode->children.push_back(fAddNode(*child));
			return dbgChildNode;
		};
		auto dbgRootNode = fAddNode(sched->GetRootNode());

		p->Write<uint8_t>(1);
		*dbgTree = *dbgRootNode;
		nwm::write_entity(p,this);
		std::function<void(NetPacket&,const DebugBehaviorTreeNode&)> fWriteTree = nullptr;
		fWriteTree = [&fWriteTree](NetPacket &p,const DebugBehaviorTreeNode &node) {
			p->WriteString(node.name);
			p->Write<uint32_t>(umath::to_integral(node.nodeType));
			p->Write<uint32_t>(umath::to_integral(node.selectorType));
			p->Write<DebugBehaviorTreeNode::State>(node.state);
			p->Write<uint32_t>(node.children.size());
			for(auto &child : node.children)
				fWriteTree(p,*child);
		};
		fWriteTree(p,*dbgRootNode);

		dbgTree = dbgRootNode;
	}
	server->SendPacketTCP("debug_ai_schedule_tree",p,pl->GetClientSession());
}

void NET_sv_debug_ai_navigation(WVServerClient *session,NetPacket packet)
{
	if(!server->CheatsEnabled() || s_game == nullptr)
		return;
	auto *pl = server->GetPlayer(session);
	if(pl == nullptr)
		return;
	auto b = packet->Read<bool>();
	if(b == false)
	{
		auto it = std::find_if(SBaseNPC::s_plDebugAiNav.begin(),SBaseNPC::s_plDebugAiNav.end(),[pl](const EntityHandle &hEnt) {
			return (hEnt.get() == pl) ? true : false;
		});
		if(it != SBaseNPC::s_plDebugAiNav.end())
			SBaseNPC::s_plDebugAiNav.erase(it);
		return;
	}
	SBaseNPC::s_plDebugAiNav.push_back(pl->GetHandle());
	std::vector<SBaseNPC*> *npcs;
	SBaseNPC::GetAll(&npcs);
	for(auto *npc : *npcs)
		npc->_debugSendNavInfo(pl);
}
#endif
