#include "stdafx_client.h"
#include "pragma/clientdefinitions.h"
#include "pragma/clientstate/clientstate.h"
#include "pragma/console/c_cvar_global_functions.h"
#include "pragma/debug/c_debug_game_gui.h"
#include "pragma/gui/widebugmipmaps.h"
#include <pragma/game/game_resources.hpp>
#include <wgui/wgui.h>
#include <wgui/fontmanager.h>
#include <cmaterialmanager.h>

extern DLLCENGINE CEngine *c_engine;
extern DLLCLIENT ClientState *client;
void Console::commands::debug_font(NetworkState*,pragma::BasePlayerComponent*,std::vector<std::string> &argv)
{
	if(argv.empty())
	{
		Con::cout<<"Available fonts:"<<Con::endl;
		auto &fonts = FontManager::GetFonts();
		for(auto &pair : fonts)
			Con::cout<<pair.first<<Con::endl;
		Con::cout<<Con::endl;
		return;
	}
	auto &fontName = argv.front();
	auto font = FontManager::GetFont(fontName);
	if(font == nullptr)
	{
		Con::cout<<"No font by that name found!"<<Con::endl;
		return;
	}
	auto glyphMap = font->GetGlyphMap();
	if(glyphMap == nullptr)
	{
		Con::cout<<"Font has invalid glyph map!"<<Con::endl;
		return;
	}
	static std::unique_ptr<DebugGameGUI> dbg = nullptr;
	if(dbg == nullptr)
	{
		dbg = std::make_unique<DebugGameGUI>([glyphMap]() {
			auto &wgui = WGUI::GetInstance();
			auto *r = wgui.Create<WIDebugMipMaps>();
			r->SetTexture(glyphMap);
			r->Update();
			return r->GetHandle();
		});
		return;
	}
	dbg = nullptr;
}

void Console::commands::debug_texture_mipmaps(NetworkState*,pragma::BasePlayerComponent*,std::vector<std::string> &argv)
{
	if(argv.empty())
	{
		Con::cwar<<"WARNING: No texture given!"<<Con::endl;
		return;
	}
	auto &texPath = argv.front();
	auto &materialManager = static_cast<CMaterialManager&>(client->GetMaterialManager());
	auto &textureManager = materialManager.GetTextureManager();
	auto texture = textureManager.FindTexture(texPath,true);

	if(texture == nullptr || texture->HasValidVkTexture() == false)
	{
		auto *mat = materialManager.FindMaterial(texPath);
		auto *diffuseMap = (mat != nullptr) ? mat->GetDiffuseMap() : nullptr;
		if(diffuseMap == nullptr || diffuseMap->texture == nullptr || static_cast<Texture*>(diffuseMap->texture.get())->HasValidVkTexture() == false)
		{
			Con::cwar<<"WARNING: No texture or material with name '"<<texPath<<"' found or loaded!"<<Con::endl;
			return;
		}
		texture = std::static_pointer_cast<Texture>(diffuseMap->texture);
	}
	static std::unique_ptr<DebugGameGUI> dbg = nullptr;
	if(dbg == nullptr)
	{
		auto &vkTexture = texture->GetVkTexture();
		auto numMipmaps = vkTexture->GetImage()->GetMipmapCount();
		Con::cout<<"Mipmap count for '"<<texPath<<"': "<<numMipmaps<<Con::endl;
		dbg = std::make_unique<DebugGameGUI>([vkTexture]() {
			auto &wgui = WGUI::GetInstance();
			auto *r = wgui.Create<WIDebugMipMaps>();
			r->SetTexture(vkTexture);
			r->Update();
			return r->GetHandle();
		});
		return;
	}
	dbg = nullptr;
}
