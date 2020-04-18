#include "stdafx_shared.h"
#include "pragma/asset_types/world.hpp"
#include "pragma/level/level_info.hpp"
#include <util_image.hpp>
#include <util_texture_info.hpp>
#include <sharedutils/util_file.h>

extern DLLENGINE Engine *engine;


void pragma::asset::Output::Write(VFilePtrReal &f)
{
	auto lname = name;
	ustring::to_lower(lname);
	f->WriteString(lname);
	f->WriteString(target);
	f->WriteString(input);
	f->WriteString(param);
	f->Write<float>(delay);
	f->Write<int32_t>(times);
}

/////////

void pragma::asset::WorldData::WriteDataOffset(VFilePtrReal &f,uint64_t offsetToOffset)
{
	auto offset = f->Tell();
	f->Seek(offsetToOffset);
	f->Write<uint64_t>(offset);
	f->Seek(offset);
}

bool pragma::asset::WorldData::Write(const std::string &fileName,std::string *optOutErrMsg)
{
	auto nFileName = fileName;
	ufile::remove_extension_from_filename(nFileName);
	nFileName += ".wld";
	auto fullPath = util::IMPORT_PATH +nFileName;
	auto pathNoFile = ufile::get_path_from_filename(fullPath);
	if(FileManager::CreatePath(pathNoFile.c_str()) == false)
	{
		if(optOutErrMsg)
			*optOutErrMsg = "Unable to create path '" +pathNoFile +"'!";
		return false;
	}
	auto fOut = FileManager::OpenFile<VFilePtrReal>(fullPath.c_str(),"wb");
	if(fOut == nullptr)
	{
		if(optOutErrMsg)
			*optOutErrMsg = "Unable to write file '" +nFileName +"'!";
		return false;
	}
	Write(fOut);
	return true;
}

void pragma::asset::WorldData::Write(VFilePtrReal &f)
{
	auto mapName = ufile::get_file_from_filename(f->GetPath());
	ufile::remove_extension_from_filename(mapName);
	ustring::to_lower(mapName);

	const std::array<char,3> header = {'W','L','D'};
	f->Write(header.data(),header.size());

	f->Write<uint32_t>(WLD_VERSION);
	static_assert(WLD_VERSION == 11);
	auto offsetToDataFlags = f->Tell();
	f->Write<DataFlags>(DataFlags::None);
	auto offsetMaterials = f->Tell();
	f->Write<uint64_t>(0ull);
	auto offsetBSPTree = f->Tell();
	f->Write<uint64_t>(0ull);
	auto offsetLightMapData = f->Tell();
	f->Write<uint64_t>(0ull);
	auto offsetEntities = f->Tell();
	f->Write<uint64_t>(0ull);

	auto flags = DataFlags::None;
	m_messageLogger("Writing materials...");
	WriteDataOffset(f,offsetMaterials);
	WriteMaterials(f);

	m_messageLogger("Writing BSP Tree...");
	if(m_bspTree)
	{
		WriteDataOffset(f,offsetBSPTree);
		flags |= DataFlags::HasBSPTree;
		WriteBSPTree(f);
	}

	m_messageLogger("Saving lightmap atlas...");
	SaveLightmapAtlas(mapName);
	if(m_lightMapAtlasEnabled)
		flags |= DataFlags::HasLightmapAtlas;
	if(m_lightMapAtlasEnabled)
	{
		WriteDataOffset(f,offsetLightMapData);
		f->Write<float>(m_lightMapIntensity);
		f->Write<float>(m_lightMapSqrtFactor);
	}

	m_messageLogger("Writing entity data...");
	WriteDataOffset(f,offsetEntities);
	WriteEntities(f);

	// Update flags
	auto cur = f->Tell();
	f->Seek(offsetToDataFlags);
	f->Write<DataFlags>(flags);
	f->Seek(cur);

	m_messageLogger("Done!");
	m_messageLogger("All operations are complete!");
}

void pragma::asset::WorldData::WriteMaterials(VFilePtrReal &f)
{
	f->Write<uint32_t>(m_materialTable.size());
	for(auto &str : m_materialTable)
	{
		util::Path path{str};
		if(ustring::compare(path.GetFront(),"materials",false))
			path.PopFront();

		auto strPath = path.GetString();
		ustring::to_lower(strPath);
		f->WriteString(strPath);
	}
}

void pragma::asset::WorldData::WriteBSPTree(VFilePtrReal &f)
{
	auto &bspTree = *m_bspTree;
	auto numClusters = bspTree.GetClusterCount();
	auto &bspNodes = bspTree.GetNodes();

	// Pre-processing to speed up some calculations
	std::vector<std::vector<size_t>> clusterNodes {};
	std::vector<std::vector<uint16_t>> clusterToClusterVisibility {};
	{
		// Nodes per cluster
		clusterNodes.resize(numClusters);
		for(auto i=decltype(bspNodes.size()){0u};i<bspNodes.size();++i)
		{
			auto &bspNode = bspNodes.at(i);
			if(bspNode->cluster >= clusterNodes.size())
				continue;
			auto &nodeList = clusterNodes.at(bspNode->cluster);
			if(nodeList.size() == nodeList.capacity())
				nodeList.reserve(nodeList.size() *1.5f +100);
			nodeList.push_back(i);
		}

		// List of clusters visible from every other cluster
		clusterToClusterVisibility.resize(numClusters);
		for(auto cluster0=decltype(numClusters){0u};cluster0<numClusters;++cluster0)
		{
			auto &visibleClusters = clusterToClusterVisibility.at(cluster0);
			for(auto cluster1=decltype(numClusters){0u};cluster1<numClusters;++cluster1)
			{
				if(bspTree.IsClusterVisible(cluster0,cluster1) == false)
					continue;
				if(visibleClusters.size() == visibleClusters.capacity())
					visibleClusters.reserve(visibleClusters.size() *1.5f +50);
				visibleClusters.push_back(cluster1);
			}
		}
	}
	//

	auto &clusterVisibility = bspTree.GetClusterVisibility();
	std::function<void(const util::BSPTree::Node&)> fWriteNode = nullptr;
	fWriteNode = [&f,&fWriteNode,&clusterVisibility,&clusterToClusterVisibility,&bspNodes,&clusterNodes,&bspTree,numClusters](const util::BSPTree::Node &node) {
		f->Write<bool>(node.leaf);
		f->Write<Vector3>(node.min);
		f->Write<Vector3>(node.max);
		f->Write<int32_t>(node.firstFace);
		f->Write<int32_t>(node.numFaces);
		f->Write<int32_t>(node.originalNodeIndex);
		if(node.leaf)
		{
			f->Write<uint16_t>(node.cluster);
			auto itNode = std::find_if(bspNodes.begin(),bspNodes.end(),[&node](const std::shared_ptr<util::BSPTree::Node> &nodeOther) {
				return nodeOther.get() == &node;
				});
			// Calculate AABB encompassing all nodes visible by this node
			auto min = node.min;
			auto max = node.max;
			if(itNode != bspNodes.end() && node.cluster != std::numeric_limits<uint16_t>::max())
			{
				for(auto clusterDst : clusterToClusterVisibility.at(node.cluster))
				{
					for(auto nodeOtherIdx : clusterNodes.at(clusterDst))
					{
						auto &nodeOther = bspNodes.at(nodeOtherIdx);
						uvec::to_min_max(min,max,nodeOther->min,nodeOther->max);
					}
				}
			}
			uvec::to_min_max(min,max); // Vertex conversion rotates the vectors, which will change the signs, so we have to re-order the vector components
			f->Write<Vector3>(min);
			f->Write<Vector3>(max);
			return;
		}
		f->Write<Vector3>(node.plane.GetNormal());
		f->Write<float>(node.plane.GetDistance());

		fWriteNode(*node.children.at(0));
		fWriteNode(*node.children.at(1));
	};
	fWriteNode(bspTree.GetRootNode());

	f->Write<uint64_t>(bspTree.GetClusterCount());
	f->Write(clusterVisibility.data(),clusterVisibility.size() *sizeof(clusterVisibility.front()));
}

void pragma::asset::WorldData::WriteEntities(VFilePtrReal &f)
{
	f->Write<uint32_t>(m_entities.size());

	for(auto &entData : m_entities)
	{
		auto offsetEndOfEntity = f->Tell();
		f->Write<uint64_t>(0u); // Offset to end of entity
		auto offsetEntityMeshes = f->Tell();
		f->Write<uint64_t>(0u); // Offset to entity meshes
		auto offsetEntityLeaves = f->Tell();
		f->Write<uint64_t>(0u); // Offset to entity leaves

		f->Write<uint64_t>(umath::to_integral(entData->GetFlags()));

		f->WriteString(entData->GetClassName());
		f->Write<Vector3>(entData->GetOrigin());

		// Keyvalues
		auto &keyValues = entData->GetKeyValues();
		f->Write<uint32_t>(keyValues.size());
		for(auto &pair : keyValues)
		{
			f->WriteString(pair.first);
			f->WriteString(pair.second);
		}

		// Outputs
		auto &outputs = entData->GetOutputs();
		f->Write<uint32_t>(outputs.size());
		for(auto &output : outputs)
			output.Write(f);

		// Custom Components
		auto &components = entData->GetComponents();
		f->Write<uint32_t>(components.size());
		for(auto &name : components)
			f->WriteString(name);

		// Leaves
		auto cur = f->Tell();
		f->Seek(offsetEntityLeaves);
		f->Write<uint64_t>(cur -offsetEntityLeaves);
		f->Seek(cur);

		uint32_t firstLeaf,numLeaves;
		entData->GetLeafData(firstLeaf,numLeaves);
		f->Write<uint32_t>(numLeaves);
		std::vector<uint16_t> leaves {};
		leaves.resize(numLeaves);
		if(numLeaves > 0u)
			memcpy(leaves.data(),GetStaticPropLeaves().data() +firstLeaf,leaves.size() *sizeof(leaves.front()));
		f->Write(leaves.data(),leaves.size() *sizeof(leaves.front()));

		cur = f->Tell();
		f->Seek(offsetEndOfEntity);
		f->Write<uint64_t>(cur -offsetEndOfEntity);
		f->Seek(cur);
	}
}

bool pragma::asset::WorldData::SaveLightmapAtlas(const std::string &mapName)
{
	if(m_lightMapAtlas == nullptr)
	{
		m_messageLogger("No lightmap atlas has been specified! Lightmaps will not be available.");
		return false;
	}
	// Build lightmap texture
	uimg::TextureInfo texInfo {};
	texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
	// texInfo.flags = uimg::TextureInfo::Flags::SRGB;
	texInfo.inputFormat = uimg::TextureInfo::InputFormat::R16G16B16A16_Float;
	texInfo.outputFormat = uimg::TextureInfo::OutputFormat::HDRColorMap;
	auto matPath = "materials/maps/" +mapName;
	FileManager::CreatePath(matPath.c_str());
	auto filePath = matPath +"/lightmap_atlas.dds";
	auto result = uimg::save_texture(filePath,*m_lightMapAtlas,texInfo,false,[](const std::string &msg) {
		Con::cwar<<"WARNING: Unable to save lightmap atlas: "<<msg<<Con::endl;
	});
	if(result)
		m_messageLogger("Lightmap atlas has been saved as '" +filePath +"'!");
	else
		m_messageLogger("Lightmap atlas could not be saved as '" +filePath +"'! Lightmaps will not be available.");
	return result;
}

