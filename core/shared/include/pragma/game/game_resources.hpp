#ifndef __GAME_RESOURCES_HP__
#define __GAME_RESOURCES_HP__

#include "pragma/networkdefinitions.h"
#include "pragma/model/vertex.h"
#include <string>
#include <optional>
#include <fsys/filesystem.h>

class NetworkState;
class Model;
namespace bsp {class File;};
namespace util
{
	static const std::string IMPORT_PATH = "addons\\imported\\";
	namespace impl
	{
		void *get_module_func(NetworkState *nw,const std::string &name);
		void init_custom_mount_directories(NetworkState &nw);
	};
	DLLNETWORK void initialize_external_archive_manager(NetworkState *nw);
	DLLNETWORK void close_external_archive_manager();

	DLLNETWORK bool port_nif_model(NetworkState *nw,const std::string &path,std::string mdlName);
	DLLNETWORK bool port_hl2_smd(NetworkState &nw,Model &mdl,VFilePtr &f,const std::string &animName,bool isCollisionMesh,std::vector<std::string> &outTextures);
	DLLNETWORK bool port_hl2_model(NetworkState *nw,const std::string &path,std::string mdlName);
	DLLNETWORK bool port_source2_model(NetworkState *nw,const std::string &path,std::string mdlName);
	DLLNETWORK bool port_hl2_particle(NetworkState *nw,const std::string &path);
	DLLNETWORK bool port_hl2_map(NetworkState *nw,const std::string &path);
	DLLNETWORK bool port_source2_map(NetworkState *nw,const std::string &path);
	DLLNETWORK bool port_file(NetworkState *nw,const std::string &path,const std::optional<std::string> &outputPath={});
	DLLNETWORK bool port_sound_script(NetworkState *nw,const std::string &path);
};

#endif
