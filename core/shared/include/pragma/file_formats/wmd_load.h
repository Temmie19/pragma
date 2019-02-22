#ifndef __WMD_LOAD_H__
#define __WMD_LOAD_H__

#include "pragma/file_formats/wmd.h"
#include "pragma/physics/collisionmesh.h"
#include <sharedutils/util_string.h>
#include "pragma/model/vertex.h"
#include "pragma/game/game_resources.hpp"
#include "pragma/physics/physsoftbodyinfo.hpp"
#include "pragma/model/animation/vertex_animation.hpp"
#include <sharedutils/util_file.h>

template<class TModel,class TModelMesh,class TModelSubMesh>
	TModel *FWMD::Load(Game *game,const std::string &model,const std::function<Material*(const std::string&,bool)> &loadMaterial,const std::function<std::shared_ptr<Model>(const std::string&)> &loadModel)
{
	std::string pathCache(model);
	std::transform(pathCache.begin(),pathCache.end(),pathCache.begin(),::tolower);

	std::string path = "models\\";
	path += model;
	const char *cPath = path.c_str();
	
	m_file = FileManager::OpenFile(cPath,"rb");
	if(m_file == NULL)
	{
		Con::cwar<<"WARNING: Unable to open model '"<<model<<"': File not found!"<<Con::endl;

		// Check if external model available, and convert if it is
		static auto bSkipPort = false;
		if(bSkipPort == false)
		{
			auto mdlName = model;
			ufile::remove_extension_from_filename(mdlName);
			auto *nw = game->GetNetworkState();
			if(util::port_hl2_model(nw,"models\\",mdlName +".mdl") == true || util::port_nif_model(nw,"models\\",mdlName +".nif"))
			{
				bSkipPort = true; // Safety flag to make sure we never end up in an infinite recursion
				auto r = Load<TModel,TModelMesh,TModelSubMesh>(game,model,loadMaterial,loadModel);
				bSkipPort = false;
				return r;
			}
		}
		return NULL;
	}
	std::array<char,4> hd;
	for(int i=0;i<3;i++)
		hd[i] = Read<char>();
	if(hd[0] != 'W' || hd[1] != 'M' || hd[2] != 'D')
	{
		Con::cwar<<"WARNING: Invalid file format for model '"<<model<<"'!"<<Con::endl;
		return NULL;
	}
	unsigned short ver = Read<unsigned short>();
	if(ver > WMD_VERSION)
	{
		Con::cwar<<"WARNING: Incompatible model format version "<<ver<<"!"<<Con::endl;
		return nullptr;
	}
	unsigned int flags = Read<unsigned int>();
	m_bStatic = ((flags &FWMD_STATIC) == FWMD_STATIC) ? true : false;

	Vector3 eyeOffset {};
	if(ver > 0x0007)
		eyeOffset = Read<Vector3>();

	unsigned long long offModelData = Read<unsigned long long>();
	UNUSED(offModelData);
	unsigned long long offMeshes = Read<unsigned long long>();
	UNUSED(offMeshes);
	unsigned long long offLODData = Read<unsigned long long>();
	UNUSED(offLODData);
	if(ver >= 0x0004)
	{
		auto offBodygroups = Read<unsigned long long>();
		UNUSED(offBodygroups);
	}
	unsigned long long offCollisionMesh = Read<unsigned long long>();
	if(!m_bStatic)
	{
		m_file->Seek(m_file->Tell() +sizeof(unsigned long long) *2);
		if(ver >= 0x0015)
			m_file->Seek(m_file->Tell() +sizeof(uint64_t) *4u);
		if(ver >= 0x0016)
			m_file->Seek(m_file->Tell() +sizeof(uint64_t) *1u);
	}

	unsigned char numTexturePaths = Read<unsigned char>();
	std::vector<std::string> texturePaths;
	texturePaths.reserve(numTexturePaths);
	for(unsigned char i=0;i<numTexturePaths;i++)
	{
		std::string texturePath = ReadString();
		texturePaths.push_back(texturePath);
	}
	// Bones
	unsigned int numBones = 0;
	if(!m_bStatic)
		numBones = Read<unsigned int>();
	TModel *mdl = new TModel(game->GetNetworkState(),umath::max(numBones,(unsigned int)(1)),model);
	mdl->SetEyeOffset(eyeOffset);
	auto &meta = mdl->GetMetaInfo();
	meta.flags = flags;
	for(auto &path : texturePaths)
		mdl->AddTexturePath(path);
	LoadBones(ver,numBones,mdl);
	if(!m_bStatic)
	{
		LoadAttachments(mdl);
		if(ver >= 0x0017)
			LoadObjectAttachments(mdl);
		LoadHitboxes(ver,mdl);
	}

	// Textures
	unsigned short numBaseTextures = Read<unsigned short>();
	unsigned short numTextures = Read<unsigned short>();
	meta.textures.reserve(numTextures);
	for(unsigned short i=0;i<numTextures;i++)
	{
		std::string texture = ReadString();
		meta.textures.push_back(texture);
	}
	
	unsigned short numSkins = Read<unsigned short>();
	for(unsigned short i=0;i<numSkins;i++)
	{
		TextureGroup *texGroup = mdl->CreateTextureGroup();
		for(unsigned short j=0;j<numBaseTextures;j++)
		{
			unsigned short texID = Read<unsigned short>();
			texGroup->textures.push_back(texID);
		}
	}
	mdl->LoadMaterials(loadMaterial);

	// Determine default surface material from material info if possible
	SurfaceMaterial *smDefault = nullptr;
	auto &materials = mdl->GetMaterials();
	for(auto &hMat : materials)
	{
		auto *mat = hMat.get();
		if(mat != nullptr)
		{
			auto &data = mat->GetDataBlock();
			if(data != nullptr)
			{
				std::string surfaceIdentifier;
				if(data->GetString("surfacematerial",&surfaceIdentifier) == true)
				{
					auto *sm = m_gameState->GetSurfaceMaterial(surfaceIdentifier);
					if(sm != nullptr && sm->GetIndex() != 0)
					{
						smDefault = sm;
						break;
					}
				}
			}
		}
	}
	//
	// Meshes
	LoadMeshes(ver,mdl,[]() {return std::make_shared<TModelMesh>();},[]() {return std::make_shared<TModelSubMesh>();});

	// LOD Data
	LoadLODData(ver,mdl);

	// Bodygroups
	if(ver >= 0x0004)
		LoadBodygroups(mdl);
	
	// Collision Meshes
	if(offCollisionMesh > 0)
		LoadCollisionMeshes(game,ver,mdl,smDefault);
	else
		mdl->SetCollisionBounds(Vector3(0,0,0),Vector3(0,0,0));
	
	if(!m_bStatic)
	{
		LoadBlendControllers(mdl);
		LoadIKControllers(ver,mdl);
		LoadAnimations(ver,mdl);
	}
	unsigned char numIncludes = Read<unsigned char>();
	for(unsigned char i=0;i<numIncludes;i++)
	{
		auto inc = ReadString();
		meta.includes.push_back(inc);
		auto mdlOther = loadModel(inc);
		mdl->Merge(*mdlOther);
	}
	m_file.reset();

	mdl->Update(ModelUpdateFlags::UpdateVertexAnimationBuffer);
	return mdl;
}

#endif
