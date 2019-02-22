#ifndef __SAVEGAME_HPP__
#define __SAVEGAME_HPP__

#include "pragma/networkdefinitions.h"
#include <memory>

class Game;
namespace pragma
{
	namespace savegame
	{
		const uint32_t VERSION = 1u;
		static bool save(Game &game,const std::string &fileName);
		static bool load(Game &game,const std::string &fileName);
	};
};

#endif
