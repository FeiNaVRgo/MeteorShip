#ifndef METEORSHIP_SPELL_H
#define METEORSHIP_SPELL_H

#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "Components/Damage.h"
#include "ECS/ECS.h"
#include "Utility/AnimatedSprite.h"

namespace ms::inventory {
    struct SpellCastingStats {
        std::size_t uses = std::numeric_limits<std::size_t>::max();
        float castDelay = 0.f;
        float rechargeTime = 0.f;
        float manaCost = 1.f;
    };

    struct ProjectileStats {
        float spreadModification = 0.f;
        float radius = 0.f;
        float speed = 260.f;
        float critChanceBonus = 0.f;
        float lifeTime = 0.f;
        float timerLifeTime = 0.f;
        bool friendlyFire = false;
        bool piercing = false;
        bool penetrating = false;
        bool homing = false;
        float homingTurnRate = 0.f;
        std::optional<float> maxTravelDistance;
        std::vector<components::DamageAmount> damage{
            {.type = components::DamageType::Projectile, .amount = 1.f}
        };
    };

    using ProjectileEffect = std::function<void(ecs::ECS&, ecs::Entity)>;

    enum class ProjectileKind {
        Moving,
        Stationary,
        Material
    };

    struct ProjectileSpell {
        ProjectileStats stats;
        ProjectileKind kind = ProjectileKind::Moving;
    };

    struct ModifierSpell {
        ProjectileEffect effect;
    };

    using UtilitySpell = ModifierSpell;

    struct MulticastSpell {
        std::size_t followingSpells = 1;
    };

    struct PassiveSpell {};

    using SpellBehavior = std::variant<
        ProjectileSpell,
        ModifierSpell,
        MulticastSpell,
        PassiveSpell
    >;

    class Spell {
        std::string name;
        std::optional<sf::Sprite> icon;
        std::optional<utility::AnimatedSprite> projectileAnimation;
        SpellBehavior behavior;
        SpellCastingStats castingStats;
        sf::Color projectileTint = sf::Color::White;
        std::size_t usesRemaining;

    public:
        explicit Spell(
            std::string name,
            SpellBehavior behavior = ProjectileSpell{},
            std::optional<sf::Sprite> icon = std::nullopt,
            std::optional<utility::AnimatedSprite> projectileAnimation = std::nullopt,
            const sf::Color projectileTint = sf::Color::White,
            SpellCastingStats castingStats = {})
            : name(std::move(name)),
              icon(std::move(icon)),
              projectileAnimation(std::move(projectileAnimation)),
              behavior(std::move(behavior)),
              castingStats(std::move(castingStats)),
              projectileTint(projectileTint),
              usesRemaining(this->castingStats.uses) {}

        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }
        std::optional<utility::AnimatedSprite> createProjectileSprite() const {
            if (projectileAnimation) return projectileAnimation->createInstance();
            if (!icon) return std::nullopt;
            return utility::AnimatedSprite(
                icon->getTexture(), icon->getTextureRect(), 1, std::nullopt);
        }
        const SpellBehavior& getBehavior() const { return behavior; }
        const SpellCastingStats& getCastingStats() const { return castingStats; }
        sf::Color getProjectileTint() const { return projectileTint; }
        bool canCast() const {
            return std::holds_alternative<PassiveSpell>(behavior) || usesRemaining > 0;
        }

        bool consumeUse() {
            if (!canCast()) return false;
            if (std::holds_alternative<PassiveSpell>(behavior)) return true;
            if (usesRemaining != std::numeric_limits<std::size_t>::max()) --usesRemaining;
            return true;
        }
    };

    struct PayloadSpell {
        ProjectileStats stats;
        sf::Color tint;
        std::optional<utility::AnimatedSprite> projectileAnimation;
        std::vector<ProjectileEffect> effects;

        std::optional<utility::AnimatedSprite> createProjectileSprite() const {
            if (!projectileAnimation) return std::nullopt;
            return projectileAnimation->createInstance();
        }
    };

    class Payload {
        std::vector<PayloadSpell> spells;
        std::vector<ProjectileEffect> pendingEffects;

    public:
        [[nodiscard]] const std::vector<PayloadSpell>& getSpells() const { return spells; }

        [[nodiscard]] std::size_t addSpell(const Spell& spell) {
            const auto& behavior = spell.getBehavior();
            return std::visit([this, &spell](const auto& spellBehavior) -> std::size_t {
                using T = std::decay_t<decltype(spellBehavior)>;
                if constexpr (std::is_same_v<T, ModifierSpell>) {
                    if (spellBehavior.effect) pendingEffects.push_back(spellBehavior.effect);
                    return 0;
                } else if constexpr (std::is_same_v<T, MulticastSpell>) {
                    return spellBehavior.followingSpells;
                } else if constexpr (std::is_same_v<T, ProjectileSpell>) {
                    auto stats = spellBehavior.stats;
                    if (spellBehavior.kind == ProjectileKind::Stationary) stats.speed = 0.f;
                    addProjectile(
                        spell, std::move(stats),
                        spellBehavior.kind != ProjectileKind::Material);
                    return 0;
                } else {
                    return 0;
                }
            }, behavior);
        }

    private:
        void addProjectile(const Spell& spell, ProjectileStats stats,
                           const bool applyEffects = true) {
            spells.push_back({
                .stats = std::move(stats),
                .tint = spell.getProjectileTint(),
                .projectileAnimation = spell.createProjectileSprite(),
                .effects = applyEffects ? pendingEffects : std::vector<ProjectileEffect>{}
            });
        }
    };
}

#endif //METEORSHIP_SPELL_H
