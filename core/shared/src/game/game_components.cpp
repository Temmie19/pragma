#include "stdafx_shared.h"
#include <pragma/game/game.h>
#include "pragma/entities/entity_component_manager.hpp"
#include "pragma/entities/components/logic_component.hpp"
#include "pragma/entities/components/damageable_component.hpp"
#include "pragma/entities/components/global_component.hpp"
#include "pragma/entities/components/ik_component.hpp"
#include "pragma/entities/components/map_component.hpp"
#include "pragma/entities/components/submergible_component.hpp"
#include "pragma/entities/components/velocity_component.hpp"
#include "pragma/entities/components/usable_component.hpp"
#include "pragma/entities/components/basegravity.h"

void Game::InitializeEntityComponents(pragma::EntityComponentManager &componentManager)
{
	componentManager.RegisterComponentType<pragma::DamageableComponent>("damageable");
	componentManager.RegisterComponentType<pragma::IKComponent>("ik");
	componentManager.RegisterComponentType<pragma::LogicComponent>("logic");
	componentManager.RegisterComponentType<pragma::GravityComponent>("gravity");
	componentManager.RegisterComponentType<pragma::MapComponent>("map");
	componentManager.RegisterComponentType<pragma::SubmergibleComponent>("submergible");
	componentManager.RegisterComponentType<pragma::VelocityComponent>("velocity");
	componentManager.RegisterComponentType<pragma::UsableComponent>("usable");
	componentManager.RegisterComponentType<pragma::GlobalNameComponent>("global");

	pragma::BaseEntityComponent::RegisterEvents(componentManager);
	BaseEntity::RegisterEvents(componentManager);
}
