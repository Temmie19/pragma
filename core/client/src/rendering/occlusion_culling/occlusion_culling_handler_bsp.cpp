#include "stdafx_client.h"
#include "pragma/rendering/occlusion_culling/occlusion_culling_handler_bsp.hpp"
#include "pragma/rendering/renderers/rasterization_renderer.hpp"
#include "pragma/entities/components/c_bsp_leaf_component.hpp"
#include "pragma/model/c_modelmesh.h"
#include "pragma/model/c_model.h"
#include <pragma/math/intersection.h>
#include <util_bsp.hpp>
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

extern DLLCLIENT CGame *c_game;

OcclusionCullingHandlerBSP::OcclusionCullingHandlerBSP(const std::shared_ptr<util::BSPTree> &bspTree)
	: m_bspTree{bspTree}
{}
const util::BSPTree &OcclusionCullingHandlerBSP::GetBSPTree() const {return *m_bspTree;}
void OcclusionCullingHandlerBSP::Update(const Vector3 &camPos)
{
	if(m_bLockCurrentNode)
		return;
	m_pCurrentNode = FindLeafNode(camPos);
}
bool OcclusionCullingHandlerBSP::ShouldExamine(CModelMesh &mesh,const Vector3 &pos,bool bViewModel,std::size_t numMeshes,const std::vector<Plane> &planes) const
{
	return ShouldPass(mesh,pos) && OcclusionCullingHandlerOctTree::ShouldExamine(mesh,pos,bViewModel,numMeshes,planes);
}
bool OcclusionCullingHandlerBSP::ShouldExamine(const rendering::RasterizationRenderer &renderer,CBaseEntity &cent,bool &outViewModel,std::vector<Plane> **outPlanes) const
{
	return ShouldPass(cent) && OcclusionCullingHandlerOctTree::ShouldExamine(renderer,cent,outViewModel,outPlanes);
}
bool OcclusionCullingHandlerBSP::ShouldPass(CBaseEntity &ent) const
{
	auto pRenderComponent = ent.GetRenderComponent();
	if(m_pCurrentNode == nullptr || pRenderComponent.expired())
		return false;
	auto pBspLeafComponent = ent.GetComponent<CBSPLeafComponent>();
	//if(pBspLeafComponent.valid())
		//return false;//pBspLeafComponent->GetLeafVisibility(m_pCurrentNode->cluster); // TODO
	auto pTrComponent = ent.GetTransformComponent();
	auto pos = pTrComponent.valid() ? pTrComponent->GetPosition() : Vector3{};
	Vector3 min;
	Vector3 max;
	pRenderComponent->GetRenderBounds(&min,&max);
	min += pos;
	max += pos;
	return Intersection::AABBAABB(min,max,m_pCurrentNode->minVisible,m_pCurrentNode->maxVisible);
}
bool OcclusionCullingHandlerBSP::ShouldPass(CModelMesh &modelMesh,const Vector3 &entityPos) const
{
	auto &bspNodes = m_bspTree->GetNodes();
	auto clusterIndex = modelMesh.GetReferenceId();
	if(clusterIndex == std::numeric_limits<uint32_t>::max())
	{
		// Probably not a world mesh
		return true; // TODO: Do manual culling by AABB? (calculate AABB for each cluster around all visible clusters)
	}
	return m_bspTree->IsClusterVisible(m_pCurrentNode->cluster,clusterIndex);

	/*Vector3 min,max;
	modelMesh.GetBounds(min,max);
	min += entityPos;
	max += entityPos;
	return Intersection::AABBAABB(min,max,m_pCurrentNode->min,m_pCurrentNode->max);*/
}
bool OcclusionCullingHandlerBSP::ShouldPass(CModelSubMesh &subMesh,const Vector3 &entityPos) const
{
	Vector3 min,max;
	subMesh.GetBounds(min,max);
	min += entityPos;
	max += entityPos;
	return Intersection::AABBAABB(min,max,m_pCurrentNode->minVisible,m_pCurrentNode->maxVisible);
}
const util::BSPTree::Node *OcclusionCullingHandlerBSP::FindLeafNode(const util::BSPTree::Node &node,const Vector3 &point) const
{
	if(node.leaf)
		return &node;
	const auto &n = node.plane.GetNormal();
	auto d = node.plane.GetDistance();
	auto v = point -n *static_cast<float>(d);
	auto dot = uvec::dot(v,n);
	if(dot >= 0.f)
		return FindLeafNode(*node.children.at(0),point);
	return FindLeafNode(*node.children.at(1),point);
}
const util::BSPTree::Node *OcclusionCullingHandlerBSP::FindLeafNode(const Vector3 &point) const {return FindLeafNode(m_bspTree->GetRootNode(),point);}
const util::BSPTree::Node *OcclusionCullingHandlerBSP::GetCurrentNode() const {return m_pCurrentNode;}
void OcclusionCullingHandlerBSP::PerformCulling(const rendering::RasterizationRenderer &renderer,std::vector<OcclusionMeshInfo> &culledMeshesOut)
{
	Update(renderer.GetScene().camera->GetPos());
	return OcclusionCullingHandlerOctTree::PerformCulling(renderer,culledMeshesOut);
}
void OcclusionCullingHandlerBSP::SetCurrentNodeLocked(bool bLocked) {m_bLockCurrentNode = bLocked;}
bool OcclusionCullingHandlerBSP::IsCurrentNodeLocked() const {return m_bLockCurrentNode;}

static void debug_bsp_nodes(NetworkState*,ConVar*,int32_t,int32_t val)
{
	if(c_game == nullptr)
		return;
	auto &scene = c_game->GetScene();
	auto *renderer = scene ? scene->GetRenderer() : nullptr;
	if(renderer == nullptr || renderer->IsRasterizationRenderer() == false)
		return;
	auto *rasterizer = static_cast<pragma::rendering::RasterizationRenderer*>(renderer);
	auto *pHandler = dynamic_cast<OcclusionCullingHandlerBSP*>(&rasterizer->GetOcclusionCullingHandler());
	if(pHandler == nullptr)
	{
		Con::cwar<<"WARNING: Scene does not have BSP occlusion culling handler!"<<Con::endl;
		return;
	}
	auto *pCurrentNode = pHandler->GetCurrentNode();
	if(pCurrentNode == nullptr)
	{
		Con::cwar<<"WARNING: Camera not located in any leaf node!"<<Con::endl;
		return;
	}
	auto &bspTree = pHandler->GetBSPTree();
	auto &clusterVisibility = bspTree.GetClusterVisibility();
	auto &camPos = scene->camera->GetPos();
	Con::cout<<"Camera position: ("<<camPos.x<<" "<<camPos.y<<" "<<camPos.z<<")"<<Con::endl;
	Con::cout<<"Leaf cluster id: "<<pCurrentNode->cluster<<Con::endl;
	if(pCurrentNode->cluster >= clusterVisibility.size())
		Con::cwar<<"WARNING: Invalid cluster id "<<pCurrentNode->cluster<<"!"<<Con::endl;
	else
	{
		auto numClusters = bspTree.GetClusterCount();
		std::vector<std::vector<uint8_t>> decompressedClusters(numClusters,std::vector<uint8_t>(numClusters,0u));
		auto cluster0 = 0ull;
		auto cluster1 = 0ull;
		for(auto i=decltype(clusterVisibility.size()){0u};i<clusterVisibility.size();++i)
		{
			auto vis = clusterVisibility.at(i);
			for(auto j=0u;j<8u;++j)
			{
				if(vis &(1<<j))
					decompressedClusters.at(cluster0).at(cluster1) = 1u;
				if(++cluster1 == numClusters)
				{
					++cluster0;
					cluster1 = 0ull;
				}
			}
		}

		auto &visArray = decompressedClusters.at(pCurrentNode->cluster);
		std::unordered_set<size_t> visClusters {};
		std::vector<size_t> clusterIdToVisClusterId(visArray.size(),std::numeric_limits<size_t>::max());
		for(auto i=decltype(visArray.size()){0u};i<visArray.size();++i)
		{
			auto visibility = visArray.at(i);
			if(visibility == 0u)
				continue;
			clusterIdToVisClusterId.at(i) = visClusters.size();
			visClusters.insert(i);
		}
		Con::cout<<"Cluster bounds: ("<<pCurrentNode->min.x<<" "<<pCurrentNode->min.y<<" "<<pCurrentNode->min.z<<") ("<<pCurrentNode->max.x<<" "<<pCurrentNode->max.y<<" "<<pCurrentNode->max.z<<")"<<Con::endl;
		Con::cout<<"Number of clusters visible from this cluster: "<<visClusters.size()<<Con::endl;

		static std::vector<std::shared_ptr<DebugRenderer::BaseObject>> dbgObjects {};
		dbgObjects.clear();
		auto *pWorld = c_game->GetWorld();
		if(pWorld == nullptr)
			Con::cwar<<"WARNING: No world entity found!"<<Con::endl;
		else
		{
			auto &entWorld = pWorld->GetEntity();
			auto mdlComponent = entWorld.GetModelComponent();
			auto mdl = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
			auto meshGroup = (mdl != nullptr) ? mdl->GetMeshGroup(0u) : nullptr;
			if(meshGroup != nullptr)
			{
				auto &meshes = meshGroup->GetMeshes();
				auto numTotalMeshes = meshes.size();
				auto numTotalVerts = 0ull;
				auto numTotalTris = 0ull;
				for(auto &mesh : meshes)
				{
					numTotalVerts += mesh->GetTriangleVertexCount();
					numTotalTris += mesh->GetTriangleCount();
				}

				const auto fFindVisInfo = [&meshes,pCurrentNode,numTotalVerts,numTotalTris](const std::unordered_set<size_t> &visClusters) {
					auto numVerts = 0ull;
					auto numTris = 0ull;
					auto numMeshes = std::count_if(meshes.begin(),meshes.end(),[pCurrentNode,&numVerts,&numTris,numTotalVerts,numTotalTris,&visClusters](const std::shared_ptr<ModelMesh> &mesh) {
						auto it = visClusters.find(mesh->GetReferenceId());
						if(it == visClusters.end())
							return false;
						numVerts += mesh->GetTriangleVertexCount();
						numTris += mesh->GetTriangleCount();
						return true;
					});
					return std::array<uint64_t,3>{static_cast<uint64_t>(numMeshes),numVerts,numTris};
				};
				const auto fGetPercentage = [](uint32_t val,uint32_t total) -> std::string {
					if(total == 0u)
						return "0%";
					return std::to_string(umath::round(val /static_cast<float>(total) *100.f,2)) +"%";
				};
				auto visInfo = fFindVisInfo({pCurrentNode->cluster});
				Con::cout<<"Number of meshes inside this cluster: "<<visInfo.at(0)<<" / "<<numTotalMeshes<<" ("<<fGetPercentage(visInfo.at(0),numTotalMeshes)<<")"<<Con::endl;
				//Con::cout<<"Number of vertices inside this cluster: "<<visInfo.at(1)<<" / "<<numTotalVerts<<" ("<<fGetPercentage(visInfo.at(1),numTotalVerts)<<")"<<Con::endl;
				Con::cout<<"Number of triangles inside this cluster: "<<visInfo.at(2)<<" / "<<numTotalTris<<" ("<<fGetPercentage(visInfo.at(2),numTotalTris)<<")"<<Con::endl;
				Con::cout<<Con::endl;

				visInfo = fFindVisInfo(visClusters);
				Con::cout<<"Number of meshes visible from this cluster: "<<visInfo.at(0)<<" / "<<numTotalMeshes<<" ("<<fGetPercentage(visInfo.at(0),numTotalMeshes)<<")"<<Con::endl;
				//Con::cout<<"Number of vertices visible from this cluster: "<<visInfo.at(1)<<" / "<<numTotalVerts<<" ("<<fGetPercentage(visInfo.at(1),numTotalVerts)<<")"<<Con::endl;
				Con::cout<<"Number of triangles visible from this cluster: "<<visInfo.at(2)<<" / "<<numTotalTris<<" ("<<fGetPercentage(visInfo.at(2),numTotalTris)<<")"<<Con::endl;
				
				if(val == 0)
				{
					auto &posWorld = entWorld.GetPosition();
					auto pTrComponent = entWorld.GetTransformComponent();
					auto rotWorld = pTrComponent.valid() ? pTrComponent->GetOrientation() : uquat::identity();
					auto clusterCenter = (pCurrentNode->min +pCurrentNode->max) *0.5f;
					for(auto &mesh : meshes)
					{
						auto it = visClusters.find(mesh->GetReferenceId());
						if(it == visClusters.end())
							continue;
						auto pos = mesh->GetCenter();
						uvec::local_to_world(posWorld,rotWorld,pos);
						dbgObjects.push_back(DebugRenderer::DrawLine(clusterCenter,pos,(*it == pCurrentNode->cluster) ? Color::Red : Color::Lime));
					}
				}
			}
		}

		if(val == 1)
		{
			// Draw vis leafs
			dbgObjects.push_back(DebugRenderer::DrawBox(pCurrentNode->min,pCurrentNode->max,EulerAngles{},Color{255,0,0,255},Color::Aqua));
			auto &nodes = bspTree.GetNodes();
			std::vector<util::BSPTree::Node*> clusterNodes {};
			clusterNodes.reserve(visClusters.size());
			for(auto clusterId : visClusters)
			{
				auto itNode = std::find_if(nodes.begin(),nodes.end(),[clusterId](const std::shared_ptr<util::BSPTree::Node> &node) {
					return node->cluster == clusterId;
				});
				clusterNodes.push_back((itNode != nodes.end()) ? itNode->get() : nullptr);
			}
			for(auto clusterId : visClusters)
			{
				auto visClusterId = clusterIdToVisClusterId.at(clusterId);
				auto *pNode = (visClusterId < clusterNodes.size()) ? clusterNodes.at(visClusterId) : nullptr;
				if(pNode == nullptr)
				{
					Con::cwar<<"WARNING: Reference to invalid cluster id "<<clusterId<<Con::endl;
					continue;
				}
				dbgObjects.push_back(DebugRenderer::DrawBox(pNode->min,pNode->max,EulerAngles{},Color{0,255,0,64},Color::ForestGreen));
			}
		}
	}
}
REGISTER_CONVAR_CALLBACK_CL(debug_bsp_nodes,debug_bsp_nodes);

static void debug_bsp_lock_callback(NetworkState*,ConVar*,int32_t,int32_t val)
{
	if(c_game == nullptr)
		return;
	auto &scene = c_game->GetScene();
	auto *renderer = scene ? scene->GetRenderer() : nullptr;
	if(renderer == nullptr || renderer->IsRasterizationRenderer() == false)
		return;
	auto *rasterizer = static_cast<pragma::rendering::RasterizationRenderer*>(renderer);
	auto *pHandler = dynamic_cast<OcclusionCullingHandlerBSP*>(&rasterizer->GetOcclusionCullingHandler());
	if(pHandler == nullptr)
	{
		Con::cwar<<"WARNING: Scene does not have BSP occlusion culling handler!"<<Con::endl;
		return;
	}
	pHandler->SetCurrentNodeLocked(val != 0);
}
REGISTER_CONVAR_CALLBACK_CL(debug_bsp_lock,debug_bsp_lock_callback);
