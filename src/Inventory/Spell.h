#ifndef METEORSHIP_SPELL_H
#define METEORSHIP_SPELL_H

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <SFML/Graphics/Sprite.hpp>

#include "Turret.h"

namespace ms::inventory {
    using ModifierSpellFunc = std::function<void()>;
    using UtilitySpellFunc = std::function<void()>;


    enum class DamageType {
        Fire,
        Ice,
        Melee,
        Projectile,
        Explosion,
        Electricity,
        Drill,
        Slice,
        Healing,
        Radioactive,
        Poison,
        Curse,
        Holy,
        Bite,
        None,
        MAX
    };

    struct ProjectileStats {
        template<typename T>
        using opt = std::optional<T>;

        //rethink in future if we want opt or like default vals idk

        opt<int> uses;
        opt<float> manaDrain;
        opt<float> castDelay;
        opt<float> rechargeTime;
        opt<float> spreadModification;
        opt<float> radius; //for explosions
        opt<float> speed; //refer to https://noita.wiki.gg/wiki/Spells#Spell_List_By_Type
        opt<float> critChangeBonus; // in percents ig
        opt<float> damage;

        float lifeTime{};
        opt<float> timerLifeTime;
        bool friendlyFire{};
        bool Piercing{};
        bool Penetrating{};

        //think about how to handle trigger spells
    };

    enum class PLACEHOLDER_ {
        PhysicalSpell,
        ModifySpell,
        ChangeDrawSpell
    };


    class SpellFormation {
        void applyFormation() {}
    };

    class ProjectileBase {
        ProjectileStats stats;
    };

    class Projectile : public ProjectileBase {

    };
    class StaticProjectile : public ProjectileBase {

    };

    //for now we will not implement passive spells like from noita
    //oh my fucking god this is cancer

    class Utility : public ProjectileBase {
        UtilitySpellFunc uti;
    };
    class ProjectileModifier : public ProjectileBase {
        ModifierSpellFunc uti;
    };
    class Material : public ProjectileBase {

    };
    class Multicast : public ProjectileBase {
    public:
        std::optional<SpellFormation> formation;
        int draw{};

    };
    class Other : public ProjectileBase {

    };

    using ProjectileType = std::variant<Projectile,
                                        StaticProjectile,
                                        Utility,
                                        ProjectileModifier,
                                        Material,
                                        Multicast,
                                        Other>;



    class SpellFactory {

    };

    class Spell {
        std::string name;
        std::optional<sf::Sprite> icon;
        ProjectileType type;
    public:
        explicit Spell(
            std::string name,
            std::optional<sf::Sprite> icon = std::nullopt)
            : name(std::move(name)),
              icon(std::move(icon)) {}

        ProjectileType& getProjectileType() { return type; }
        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }
    };

    class Payload {
        //Turret::TurretIterator iterator; //hihi i could name it tit
        Turret* turret;
        ecs::ECS* ecs;
        //every type of spells tells payload how much it can move TurretIterator

        //how do i hold all stuf here

        struct PayloadedSpells {
            SpellFormation formation;
            std::vector<Spell> spellsToShot;
            std::vector<ModifierSpellFunc> modifiers; // those can on bullet creation or constant
            std::vector<UtilitySpellFunc> utility; // same as modifiers, can be also a normal shootable spell idk
        };

        struct PayloadNode {
            Spell spell;
            std::vector<std::unique_ptr<PayloadNode>> children;
        };
        //there is some template magic that i am missing ong

        //maybe applying spell traits with templates?
        //then having a SpellBase that can pull all those traits
        std::vector<PayloadedSpells> toShoot;
        std::unique_ptr<PayloadNode> rootNode;


        int getAmountToMoveForward(ProjectileType const& type) {
            if (const auto* multicast = std::get_if<Multicast>(&type)) {
                return multicast->draw;
            }

            if (std::holds_alternative<Utility>(type) ||
                std::holds_alternative<ProjectileModifier>(type)) {
                return 1;
            }

            return 0;
        }

        void pushToTree(Spell const& spell) {

        }

        void flattenTree() {

        }

    public:
        //i think i should do the think in the constructor
        Payload(Turret* turret, ecs::ECS* ecs)
            : turret(turret), ecs(ecs) {}

        //im gonna represent a payload as a tree
        //then flatten it to the toShoot thingy

        void gatherPayloadedSpells() {
            int howFarToMove = 0;
            for (const auto ent_spell : *turret) {
                auto& spell = ecs->getComponent<Spell>(ent_spell);
                howFarToMove += getAmountToMoveForward(spell.getProjectileType());
                //how the fuck am i supposed to do this ong
                pushToTree(spell);

                //hmm how to do spell wrapping


                if (howFarToMove == 0) {
                    break;
                }
            }
        }
    };
}

#endif //METEORSHIP_SPELL_H
