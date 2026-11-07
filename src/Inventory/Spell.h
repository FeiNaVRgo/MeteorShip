#ifndef METEORSHIP_SPELL_H
#define METEORSHIP_SPELL_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

#include "ECS/ECS.h"
#include "Utility/AnimatedSprite.h"

namespace ms::inventory {
    class Turret;

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

    enum class TriggerType {
        None,
        Collision,   // On collision with entity or wall (Contact Trigger)
        Timer,       // After timerLifeTime seconds
        Expiration   // When projectile lifetime expires
    };

    // Forward declaration of ShootableSpell
    struct ShootableSpell;

    // Context passed to modifier callbacks
    struct ModifierContext {
        ecs::ECS& entityManager;
        ecs::Entity projectileEntity;
        ecs::Entity targetEntity = 0; // Valid on collision / hit
        sf::Vector2f hitNormal{};
        float deltaTime = 0.f;
    };

    using ModifierInitFunc = std::function<void(ModifierContext&)>;
    using ModifierUpdateFunc = std::function<void(ModifierContext&)>;
    using ModifierCollisionFunc = std::function<void(ModifierContext&)>;
    using ModifierDeathFunc = std::function<void(ModifierContext&)>;

    struct ProjectileModifierHook {
        ModifierInitFunc onInit;           // Called once when projectile is spawned (one-time thing)
        ModifierUpdateFunc onUpdate;       // Called every frame/tick (continuous stuff, e.g. homing, gravity, trails)
        ModifierCollisionFunc onCollision; // Called on collision / hit
        ModifierDeathFunc onDeath;         // Called when projectile dies/expires
    };

    using UtilitySpellFunc = std::function<void()>;

    struct ProjectileStats {
        template<typename T>
        using opt = std::optional<T>;

        opt<int> uses;
        opt<float> manaDrain;
        opt<float> castDelay;
        opt<float> rechargeTime;
        opt<float> spreadModification;
        opt<float> radius; // for explosions
        opt<float> speed;  // refer to https://noita.wiki.gg/wiki/Spells#Spell_List_By_Type
        opt<float> critChangeBonus; // in percents
        opt<float> damage;

        float lifeTime{};
        opt<float> timerLifeTime;
        bool friendlyFire{};
        bool Piercing{};
        bool Penetrating{};

        TriggerType triggerType = TriggerType::None;

        void combine(const ProjectileStats& mod) {
            if (mod.damage.has_value()) {
                damage = damage.value_or(0.f) + *mod.damage;
            }
            if (mod.speed.has_value()) {
                speed = speed.value_or(0.f) + *mod.speed;
            }
            if (mod.castDelay.has_value()) {
                castDelay = castDelay.value_or(0.f) + *mod.castDelay;
            }
            if (mod.rechargeTime.has_value()) {
                rechargeTime = rechargeTime.value_or(0.f) + *mod.rechargeTime;
            }
            if (mod.manaDrain.has_value()) {
                manaDrain = manaDrain.value_or(0.f) + *mod.manaDrain;
            }
            if (mod.spreadModification.has_value()) {
                spreadModification = spreadModification.value_or(0.f) + *mod.spreadModification;
            }
            if (mod.radius.has_value()) {
                radius = radius.value_or(0.f) + *mod.radius;
            }
            if (mod.critChangeBonus.has_value()) {
                critChangeBonus = critChangeBonus.value_or(0.f) + *mod.critChangeBonus;
            }
            if (mod.uses.has_value()) {
                uses = mod.uses;
            }
            if (mod.lifeTime > 0.f) {
                lifeTime += mod.lifeTime;
            }
            if (mod.timerLifeTime.has_value()) {
                timerLifeTime = mod.timerLifeTime;
            }
            if (mod.friendlyFire) friendlyFire = true;
            if (mod.Piercing) Piercing = true;
            if (mod.Penetrating) Penetrating = true;
            if (mod.triggerType != TriggerType::None) {
                triggerType = mod.triggerType;
            }
        }

        ProjectileStats& operator+=(const ProjectileStats& other) {
            combine(other);
            return *this;
        }
    };

    enum class FormationType {
        None,
        Spread,
        BiDirectional,
        TriDirectional,
        AboveAndBelow,
        Hexagonal,
        Behind
    };

    class SpellFormation {
    public:
        FormationType type = FormationType::None;
        float spreadAngleDegrees = 0.f;

        SpellFormation() = default;
        explicit SpellFormation(FormationType type, float spreadAngleDegrees = 0.f)
            : type(type), spreadAngleDegrees(spreadAngleDegrees) {}

        std::vector<float> calculateAngles(std::size_t count, float baseAngleRadians) const;
    };

    class ProjectileBase {
    protected:
        ProjectileStats stats;
    public:
        ProjectileBase() = default;
        explicit ProjectileBase(ProjectileStats stats) : stats(std::move(stats)) {}

        const ProjectileStats& getStats() const { return stats; }
        ProjectileStats& getStats() { return stats; }
        void setStats(ProjectileStats s) { stats = std::move(s); }
    };

    class Projectile : public ProjectileBase {
    public:
        using ProjectileBase::ProjectileBase;
    };

    class StaticProjectile : public ProjectileBase {
    public:
        using ProjectileBase::ProjectileBase;
    };

    class Utility : public ProjectileBase {
    public:
        UtilitySpellFunc uti;
        using ProjectileBase::ProjectileBase;
        Utility(UtilitySpellFunc f, ProjectileStats stats = {})
            : ProjectileBase(std::move(stats)), uti(std::move(f)) {}
    };

    class ProjectileModifier : public ProjectileBase {
    public:
        ProjectileModifierHook hook;
        using ProjectileBase::ProjectileBase;

        ProjectileModifier(ProjectileModifierHook h = {}, ProjectileStats stats = {})
            : ProjectileBase(std::move(stats)), hook(std::move(h)) {}

        // Helper constructors for specific lifecycle hooks:
        static ProjectileModifier makeInitModifier(ModifierInitFunc f, ProjectileStats stats = {}) {
            return ProjectileModifier(ProjectileModifierHook{.onInit = std::move(f)}, std::move(stats));
        }

        static ProjectileModifier makeUpdateModifier(ModifierUpdateFunc f, ProjectileStats stats = {}) {
            return ProjectileModifier(ProjectileModifierHook{.onUpdate = std::move(f)}, std::move(stats));
        }

        static ProjectileModifier makeCollisionModifier(ModifierCollisionFunc f, ProjectileStats stats = {}) {
            return ProjectileModifier(ProjectileModifierHook{.onCollision = std::move(f)}, std::move(stats));
        }
    };

    class Material : public ProjectileBase {
    public:
        using ProjectileBase::ProjectileBase;
    };

    class Multicast : public ProjectileBase {
    public:
        std::optional<SpellFormation> formation;
        int draw = 1;

        Multicast() = default;
        explicit Multicast(int drawCount,
                           std::optional<SpellFormation> formation = std::nullopt,
                           ProjectileStats stats = {})
            : ProjectileBase(std::move(stats)), formation(std::move(formation)), draw(drawCount) {}
    };

    class Other : public ProjectileBase {
    public:
        using ProjectileBase::ProjectileBase;
    };

    using ProjectileType = std::variant<Projectile,
                                        StaticProjectile,
                                        Utility,
                                        ProjectileModifier,
                                        Material,
                                        Multicast,
                                        Other>;

    class Spell {
        std::string name;
        std::optional<sf::Sprite> icon;
        std::optional<utility::AnimatedSprite> projectileSprite;
        ProjectileType type = Projectile{};
    public:
        Spell() : name("Unnamed"), type(Projectile{}) {}

        explicit Spell(
            std::string name,
            ProjectileType type = Projectile{},
            std::optional<sf::Sprite> icon = std::nullopt,
            std::optional<utility::AnimatedSprite> projectileSprite = std::nullopt)
            : name(std::move(name)),
              icon(std::move(icon)),
              projectileSprite(std::move(projectileSprite)),
              type(std::move(type)) {}

        explicit Spell(
            std::string name,
            std::optional<sf::Sprite> icon)
            : name(std::move(name)),
              icon(std::move(icon)),
              type(Projectile{}) {}

        ProjectileType& getProjectileType() { return type; }
        const ProjectileType& getProjectileType() const { return type; }
        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }
        void setIcon(sf::Sprite s) { icon = std::move(s); }

        const utility::AnimatedSprite* getProjectileSprite() const {
            return projectileSprite ? &*projectileSprite : nullptr;
        }
        void setProjectileSprite(utility::AnimatedSprite s) { projectileSprite = std::move(s); }

        ProjectileStats& getStats() {
            return std::visit([](auto& p) -> ProjectileStats& { return p.getStats(); }, type);
        }

        const ProjectileStats& getStats() const {
            return std::visit([](const auto& p) -> const ProjectileStats& { return p.getStats(); }, type);
        }

        bool isModifier() const {
            return std::holds_alternative<ProjectileModifier>(type);
        }

        bool isMulticast() const {
            return std::holds_alternative<Multicast>(type);
        }

        bool isProjectile() const {
            return std::holds_alternative<Projectile>(type) ||
                   std::holds_alternative<StaticProjectile>(type) ||
                   std::holds_alternative<Material>(type);
        }

        bool hasTrigger() const {
            return getStats().triggerType != TriggerType::None;
        }
    };

    struct ShootableSpell {
        Spell spell;
        ProjectileStats finalStats;
        std::vector<ProjectileModifierHook> modifierHooks;
        std::vector<UtilitySpellFunc> utility;
        // Nested payload: spells triggered on collision/timer/death
        std::vector<ShootableSpell> payload;
    };

    class SpellFactory {
    public:
        static Spell makeSparkBolt(std::optional<sf::Sprite> icon = std::nullopt,
                                   std::optional<utility::AnimatedSprite> projSprite = std::nullopt);
        static Spell makeSparkBoltWithTrigger(std::optional<sf::Sprite> icon = std::nullopt,
                                             std::optional<utility::AnimatedSprite> projSprite = std::nullopt);
        static Spell makeSparkBoltWithTimer(float timer = 0.5f,
                                           std::optional<sf::Sprite> icon = std::nullopt,
                                           std::optional<utility::AnimatedSprite> projSprite = std::nullopt);
        static Spell makeBomb(std::optional<sf::Sprite> icon = std::nullopt,
                             std::optional<utility::AnimatedSprite> projSprite = std::nullopt);
        static Spell makeDamagePlus(float bonusDamage = 10.f, std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeSpeedPlus(float bonusSpeed = 100.f, std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeCriticalPlus(float critChance = 15.f, std::optional<sf::Sprite> icon = std::nullopt);
        // Homing implemented as a continuous onUpdate modifier hook:
        static Spell makeHoming(float turnRate = 8.f, float searchRadius = 400.f, std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeShortRangeHoming(float turnRate = 16.f, float searchRadius = 150.f, std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeDoubleSpell(std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeTripleSpell(std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeFormationHexagonal(std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeFormationBehind(std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeFormationBiDirectional(std::optional<sf::Sprite> icon = std::nullopt);
        static Spell makeFormationTriDirectional(std::optional<sf::Sprite> icon = std::nullopt);

        // Example Wand / Deck setup demonstrating Noita mechanics
        static std::vector<Spell> makeExampleDeck();
    };

    class Payload {
    public:
        struct PayloadedSpells {
            SpellFormation formation;
            std::vector<Spell> spellsToShot; // for compatibility
            std::vector<ProjectileModifierHook> modifierHooks;
            std::vector<UtilitySpellFunc> utility;

            // Fully resolved shootable spells ready for spawning:
            std::vector<ShootableSpell> resolvedSpells;
            float castDelay = 0.f;
            float rechargeTime = 0.f;
            float totalManaDrain = 0.f;
        };

        struct PayloadNode {
            Spell spell;
            ProjectileStats combinedStats;
            std::vector<ProjectileModifierHook> modifierHooks;
            std::vector<UtilitySpellFunc> utility;
            std::vector<std::unique_ptr<PayloadNode>> children;

            explicit PayloadNode(Spell s)
                : spell(std::move(s)), combinedStats(spell.getStats()) {}
        };

    private:
        Turret* turret = nullptr;
        std::vector<PayloadedSpells> toShoot;
        std::vector<std::unique_ptr<PayloadNode>> rootNodes;

        struct CastState {
            ProjectileStats modifiers;
            std::vector<ProjectileModifierHook> modifierHooks;
            std::vector<UtilitySpellFunc> utilityFuncs;
            std::optional<SpellFormation> formation;
            float castDelay = 0.f;
            float rechargeTime = 0.f;
            float manaDrain = 0.f;
        };

        Spell* drawNextSpell(bool& wrapped);

        std::vector<std::unique_ptr<PayloadNode>> evaluateDraw(
            int drawCount,
            CastState& state,
            bool& wrapped,
            std::size_t recursionDepth = 0);

        void flattenTree(const std::vector<std::unique_ptr<PayloadNode>>& nodes, const CastState& state);

    public:
        explicit Payload(Turret* turret)
            : turret(turret) {}

        std::vector<PayloadedSpells>& gatherPayloadedSpells();
        const std::vector<PayloadedSpells>& getPayloadedSpells() const { return toShoot; }
    };
}

#endif //METEORSHIP_SPELL_H
