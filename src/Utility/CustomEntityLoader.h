#ifndef METEORSHIP_CUSTOM_ENTITY_LOADER_H
#define METEORSHIP_CUSTOM_ENTITY_LOADER_H

#include <boost/json.hpp>

#include <memory>
#include <string>
#include <vector>

#include "Components/BehaviorTree.h"
#include "ECS/ECS.h"
#include "Inventory/Spell.h"
#include "Inventory/Turret.h"

namespace ms {
    class Game;
    class World;
}

namespace ms::utility {
    struct CustomSpellDefinition {
        std::string id;
        inventory::Spell spell;
    };

    struct CustomTurretDefinition {
        std::string id;
        inventory::Turret turret;
        std::vector<std::string> spells;
    };

    struct CustomEnemyDefinition {
        std::string id;
        std::string name;
        std::string sprite;
        std::string behaviorTree;
    };

    class CustomEntityLoader {
        Game& game;

    public:
        explicit CustomEntityLoader(Game& game) : game(game) {}

        std::vector<CustomSpellDefinition> loadSpells() const;
        std::vector<CustomTurretDefinition> loadTurrets() const;
        std::vector<CustomEnemyDefinition> loadEnemies() const;

        static components::BehaviorTree createEnemyBehaviorTree(
            Game& game, const std::shared_ptr<World>& world, ecs::Entity entity,
            const std::string& behaviorTree);
    };
}

#endif // METEORSHIP_CUSTOM_ENTITY_LOADER_H
