#include "Spell.h"
#include "Turret.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "Components/Enemy.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Utility/UtiFuncs.h"

namespace ms::inventory {

    // -------------------------------------------------------------
    // SpellFormation
    // -------------------------------------------------------------
    std::vector<float> SpellFormation::calculateAngles(std::size_t count, float baseAngleRadians) const {
        std::vector<float> angles;
        if (count == 0) return angles;
        angles.reserve(count);

        if (count == 1 || type == FormationType::None) {
            angles.push_back(baseAngleRadians);
            return angles;
        }

        constexpr float PI = std::numbers::pi_v<float>;

        switch (type) {
            case FormationType::Behind: {
                angles.push_back(baseAngleRadians);
                for (std::size_t i = 1; i < count; ++i) {
                    angles.push_back(baseAngleRadians + PI);
                }
                break;
            }
            case FormationType::BiDirectional: {
                for (std::size_t i = 0; i < count; ++i) {
                    angles.push_back(baseAngleRadians + (i % 2 == 0 ? 0.f : PI));
                }
                break;
            }
            case FormationType::TriDirectional: {
                float step = (2.f * PI) / 3.f;
                for (std::size_t i = 0; i < count; ++i) {
                    angles.push_back(baseAngleRadians + static_cast<float>(i % 3) * step);
                }
                break;
            }
            case FormationType::AboveAndBelow: {
                for (std::size_t i = 0; i < count; ++i) {
                    angles.push_back(baseAngleRadians + (i % 2 == 0 ? PI / 2.f : -PI / 2.f));
                }
                break;
            }
            case FormationType::Hexagonal: {
                float step = (2.f * PI) / 6.f;
                for (std::size_t i = 0; i < count; ++i) {
                    angles.push_back(baseAngleRadians + static_cast<float>(i % 6) * step);
                }
                break;
            }
            case FormationType::Spread:
            default: {
                float spread = (spreadAngleDegrees > 0.f)
                    ? (spreadAngleDegrees * PI / 180.f)
                    : (count > 1 ? static_cast<float>(count) * 0.15f : 0.f);
                float startAngle = baseAngleRadians - spread / 2.f;
                float step = (count > 1) ? (spread / static_cast<float>(count - 1)) : 0.f;
                for (std::size_t i = 0; i < count; ++i) {
                    angles.push_back(startAngle + static_cast<float>(i) * step);
                }
                break;
            }
        }
        return angles;
    }

    // -------------------------------------------------------------
    // SpellFactory
    // -------------------------------------------------------------
    Spell SpellFactory::makeSparkBolt(std::optional<sf::Sprite> icon,
                                     std::optional<utility::AnimatedSprite> projSprite) {
        ProjectileStats stats;
        stats.damage = 3.f;
        stats.speed = 800.f;
        stats.lifeTime = 0.8f;
        stats.castDelay = 0.05f;
        stats.rechargeTime = 0.f;
        stats.manaDrain = 5.f;
        stats.critChangeBonus = 5.f;
        stats.spreadModification = 1.f;
        return Spell("Spark Bolt", Projectile(stats), std::move(icon), std::move(projSprite));
    }

    Spell SpellFactory::makeSparkBoltWithTrigger(std::optional<sf::Sprite> icon,
                                                std::optional<utility::AnimatedSprite> projSprite) {
        ProjectileStats stats;
        stats.damage = 3.f;
        stats.speed = 800.f;
        stats.lifeTime = 0.8f;
        stats.castDelay = 0.05f;
        stats.rechargeTime = 0.f;
        stats.manaDrain = 10.f;
        stats.critChangeBonus = 5.f;
        stats.triggerType = TriggerType::Collision;
        return Spell("Spark Bolt with Trigger", Projectile(stats), std::move(icon), std::move(projSprite));
    }

    Spell SpellFactory::makeSparkBoltWithTimer(float timer,
                                              std::optional<sf::Sprite> icon,
                                              std::optional<utility::AnimatedSprite> projSprite) {
        ProjectileStats stats;
        stats.damage = 3.f;
        stats.speed = 800.f;
        stats.lifeTime = 0.8f;
        stats.timerLifeTime = timer;
        stats.castDelay = 0.05f;
        stats.rechargeTime = 0.f;
        stats.manaDrain = 10.f;
        stats.critChangeBonus = 5.f;
        stats.triggerType = TriggerType::Timer;
        return Spell("Spark Bolt with Timer", Projectile(stats), std::move(icon), std::move(projSprite));
    }

    Spell SpellFactory::makeBomb(std::optional<sf::Sprite> icon,
                                std::optional<utility::AnimatedSprite> projSprite) {
        ProjectileStats stats;
        stats.damage = 25.f;
        stats.speed = 200.f;
        stats.radius = 50.f;
        stats.lifeTime = 3.f;
        stats.castDelay = 0.5f;
        stats.rechargeTime = 0.8f;
        stats.manaDrain = 25.f;
        stats.friendlyFire = true;
        return Spell("Bomb", Projectile(stats), std::move(icon), std::move(projSprite));
    }

    Spell SpellFactory::makeDamagePlus(float bonusDamage, std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.damage = bonusDamage;
        stats.manaDrain = 5.f;
        stats.castDelay = -0.03f;
        return Spell("Damage Plus", ProjectileModifier({}, stats), std::move(icon));
    }

    Spell SpellFactory::makeSpeedPlus(float bonusSpeed, std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.speed = bonusSpeed;
        stats.manaDrain = 4.f;
        stats.castDelay = -0.02f;
        return Spell("Speed Plus", ProjectileModifier({}, stats), std::move(icon));
    }

    Spell SpellFactory::makeCriticalPlus(float critChance, std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.critChangeBonus = critChance;
        stats.manaDrain = 10.f;
        return Spell("Critical Plus", ProjectileModifier({}, stats), std::move(icon));
    }

    Spell SpellFactory::makeHoming(float turnRate, float searchRadius, std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.manaDrain = 30.f;
        stats.castDelay = 0.1f;

        ProjectileModifierHook hook;
        // Continuous update hook: steers velocity towards nearest enemy
        hook.onUpdate = [turnRate, searchRadius](ModifierContext& ctx) {
            auto& ecs = ctx.entityManager;
            if (!ecs.isEntityAlive(ctx.projectileEntity) ||
                !ecs.hasComponent<components::Transform>(ctx.projectileEntity) ||
                !ecs.hasComponent<components::Rigidbody>(ctx.projectileEntity)) {
                return;
            }

            auto& tf = ecs.getComponent<components::Transform>(ctx.projectileEntity);
            auto& rb = ecs.getComponent<components::Rigidbody>(ctx.projectileEntity);
            sf::Vector2f myPos = tf.getPosition();

            // Find closest enemy within search radius
            ecs::Entity closestEnemy = 0;
            float closestDistSq = searchRadius * searchRadius;

            ecs.view<components::Enemy, components::Transform>().each(
                [&](const ecs::Entity enemy, const components::Enemy&, const components::Transform& enemyTf) {
                    if (!ecs.isEntityAlive(enemy)) return;
                    float distSq = utility::squaredDistance(myPos, enemyTf.getPosition());
                    if (distSq < closestDistSq) {
                        closestDistSq = distSq;
                        closestEnemy = enemy;
                    }
                }
            );

            if (closestEnemy != 0 && ecs.isEntityAlive(closestEnemy)) {
                sf::Vector2f targetPos = ecs.getComponent<components::Transform>(closestEnemy).getPosition();
                sf::Vector2f diff = targetPos - myPos;
                float currentSpeed = std::hypot(rb.velocity.x, rb.velocity.y);
                if (currentSpeed > 1e-4f) {
                    float currentAngle = std::atan2(rb.velocity.y, rb.velocity.x);
                    float targetAngle = std::atan2(diff.y, diff.x);

                    // Angle difference normalized to [-PI, PI]
                    constexpr float PI = std::numbers::pi_v<float>;
                    float angleDiff = targetAngle - currentAngle;
                    while (angleDiff > PI) angleDiff -= 2.f * PI;
                    while (angleDiff < -PI) angleDiff += 2.f * PI;

                    float maxTurn = turnRate * ctx.deltaTime;
                    float step = std::clamp(angleDiff, -maxTurn, maxTurn);
                    float newAngle = currentAngle + step;

                    rb.velocity = sf::Vector2f(std::cos(newAngle) * currentSpeed, std::sin(newAngle) * currentSpeed);
                    tf.setRotation(sf::radians(newAngle));
                }
            }
        };

        return Spell("Homing", ProjectileModifier(std::move(hook), stats), std::move(icon));
    }

    Spell SpellFactory::makeShortRangeHoming(float turnRate, float searchRadius, std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.manaDrain = 20.f;
        stats.castDelay = 0.05f;

        // Reuse makeHoming logic with different parameters
        auto homingSpell = makeHoming(turnRate, searchRadius, std::move(icon));
        auto* mod = std::get_if<ProjectileModifier>(&homingSpell.getProjectileType());
        ProjectileModifierHook hook = mod ? mod->hook : ProjectileModifierHook{};
        return Spell("Short-Range Homing", ProjectileModifier(std::move(hook), stats), homingSpell.getIcon() ? std::optional{*homingSpell.getIcon()} : std::nullopt);
    }

    Spell SpellFactory::makeDoubleSpell(std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.manaDrain = 2.f;
        return Spell("Double Spell", Multicast(2, std::nullopt, stats), std::move(icon));
    }

    Spell SpellFactory::makeTripleSpell(std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.manaDrain = 5.f;
        return Spell("Triple Spell", Multicast(3, std::nullopt, stats), std::move(icon));
    }

    Spell SpellFactory::makeFormationHexagonal(std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.manaDrain = 10.f;
        return Spell("Formation - Hexagonal", Multicast(6, SpellFormation(FormationType::Hexagonal), stats), std::move(icon));
    }

    Spell SpellFactory::makeFormationBehind(std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.manaDrain = 5.f;
        return Spell("Formation - Behind", Multicast(2, SpellFormation(FormationType::Behind), stats), std::move(icon));
    }

    Spell SpellFactory::makeFormationBiDirectional(std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.manaDrain = 5.f;
        return Spell("Formation - Bifurcated", Multicast(2, SpellFormation(FormationType::BiDirectional), stats), std::move(icon));
    }

    Spell SpellFactory::makeFormationTriDirectional(std::optional<sf::Sprite> icon) {
        ProjectileStats stats;
        stats.manaDrain = 8.f;
        return Spell("Formation - Trifurcated", Multicast(3, SpellFormation(FormationType::TriDirectional), stats), std::move(icon));
    }

    std::vector<Spell> SpellFactory::makeExampleDeck() {
        return {
            makeDamagePlus(15.f),
            makeHoming(10.f, 450.f),
            makeSparkBoltWithTimer(),
            makeFormationTriDirectional(),
            makeSparkBolt(),
            makeSparkBolt(),
            makeSparkBolt()
        };
    }

    // -------------------------------------------------------------
    // Payload Implementation
    // -------------------------------------------------------------
    Spell* Payload::drawNextSpell(bool& wrapped) {
        if (!turret) return nullptr;

        while (turret->deckCursor < turret->deckOrder.size()) {
            std::size_t slotIdx = turret->deckOrder[turret->deckCursor++];
            if (slotIdx < turret->spellOrder.size() && turret->spellOrder[slotIdx].has_value()) {
                return &(*turret->spellOrder[slotIdx]);
            }
        }

        // Deck is exhausted; check if wand wrapping is allowed in this cast
        if (!wrapped && !turret->deckOrder.empty()) {
            wrapped = true;
            turret->deckCursor = 0;
            while (turret->deckCursor < turret->deckOrder.size()) {
                std::size_t slotIdx = turret->deckOrder[turret->deckCursor++];
                if (slotIdx < turret->spellOrder.size() && turret->spellOrder[slotIdx].has_value()) {
                    return &(*turret->spellOrder[slotIdx]);
                }
            }
        }

        return nullptr;
    }

    std::vector<std::unique_ptr<Payload::PayloadNode>> Payload::evaluateDraw(
        int drawCount,
        CastState& state,
        bool& wrapped,
        std::size_t recursionDepth) {

        std::vector<std::unique_ptr<PayloadNode>> nodes;
        if (recursionDepth > 16) return nodes;

        while (drawCount > 0) {
            Spell* spell = drawNextSpell(wrapped);
            if (!spell) break;

            const auto& stats = spell->getStats();
            float cost = stats.manaDrain.value_or(0.f);

            // Mana check: If not enough mana, spell cannot be cast
            if (turret->mana < cost) {
                --drawCount;
                continue;
            }

            turret->mana -= cost;
            state.manaDrain += cost;
            state.castDelay += stats.castDelay.value_or(0.f);
            state.rechargeTime += stats.rechargeTime.value_or(0.f);

            auto& projType = spell->getProjectileType();

            // 1. Modifiers: Modify the current cast state and draw +1 spell
            if (auto* mod = std::get_if<ProjectileModifier>(&projType)) {
                state.modifiers.combine(mod->getStats());
                if (mod->hook.onInit || mod->hook.onUpdate || mod->hook.onCollision || mod->hook.onDeath) {
                    state.modifierHooks.push_back(mod->hook);
                }
                // In Noita, modifier takes 1 draw action and grants +1 draw action -> net change 0
                continue;
            }

            // 2. Multicast: Consumes 1 draw action and adds multicast->draw
            if (auto* multi = std::get_if<Multicast>(&projType)) {
                --drawCount;
                drawCount += multi->draw;
                if (multi->formation.has_value()) {
                    state.formation = multi->formation;
                }
                state.modifiers.combine(multi->getStats());
                continue;
            }

            // 3. Utility
            if (auto* uti = std::get_if<Utility>(&projType)) {
                if (uti->uti) {
                    state.utilityFuncs.push_back(uti->uti);
                }
                --drawCount;
                auto node = std::make_unique<PayloadNode>(*spell);
                node->combinedStats.combine(state.modifiers);
                node->modifierHooks = state.modifierHooks;
                node->utility = state.utilityFuncs;
                nodes.push_back(std::move(node));
                continue;
            }

            // 4. Projectile / StaticProjectile / Material / Other
            --drawCount;
            auto node = std::make_unique<PayloadNode>(*spell);
            node->combinedStats.combine(state.modifiers);
            node->modifierHooks = state.modifierHooks;
            node->utility = state.utilityFuncs;

            // Check if this projectile is a trigger spell:
            if (node->combinedStats.triggerType != TriggerType::None) {
                // In Noita, a trigger spell's payload starts a sub-draw from the deck
                CastState subState;
                node->children = evaluateDraw(1, subState, wrapped, recursionDepth + 1);
                node->payloadFormation = subState.formation.value_or(SpellFormation{});

                state.manaDrain += subState.manaDrain;
                state.castDelay += subState.castDelay;
                state.rechargeTime += subState.rechargeTime;
            }

            nodes.push_back(std::move(node));
        }

        return nodes;
    }

    namespace {
        ShootableSpell convertNodeToShootable(const Payload::PayloadNode& node) {
            ShootableSpell shootable;
            shootable.spell = node.spell;
            shootable.finalStats = node.combinedStats;
            shootable.payloadFormation = node.payloadFormation;
            shootable.modifierHooks = node.modifierHooks;
            shootable.utility = node.utility;
            for (const auto& child : node.children) {
                if (child) {
                    shootable.payload.push_back(convertNodeToShootable(*child));
                }
            }
            return shootable;
        }
    }

    void Payload::flattenTree(const std::vector<std::unique_ptr<PayloadNode>>& nodes, const CastState& state) {
        if (nodes.empty()) return;

        PayloadedSpells group;
        group.formation = state.formation.value_or(SpellFormation{});
        group.castDelay = std::max(0.f, turret->stats.castDelay + state.castDelay);
        group.rechargeTime = std::max(0.f, turret->stats.rechargeTime + state.rechargeTime);
        group.totalManaDrain = state.manaDrain;

        for (const auto& node : nodes) {
            if (!node) continue;
            group.spellsToShot.push_back(node->spell);
            group.resolvedSpells.push_back(convertNodeToShootable(*node));
        }

        toShoot.push_back(std::move(group));
    }

    std::vector<Payload::PayloadedSpells>& Payload::gatherPayloadedSpells() {
        toShoot.clear();
        rootNodes.clear();

        if (!turret || turret->deckOrder.empty()) {
            return toShoot;
        }

        // If deck was at the end when firing, trigger recharge immediately
        if (turret->deckCursor >= turret->deckOrder.size()) {
            turret->rechargeRemaining = std::max(0.f, turret->stats.rechargeTime + turret->cycleRechargeModifier);
            turret->resetDeck();
            return toShoot;
        }

        bool wrapped = false;
        CastState state;
        int initialDraw = std::max<int>(1, static_cast<int>(turret->stats.spellsPerCast));

        rootNodes = evaluateDraw(initialDraw, state, wrapped, 0);
        flattenTree(rootNodes, state);

        // Apply wand cooldowns according to Noita rules:
        if (wrapped || turret->deckCursor >= turret->deckOrder.size()) {
            turret->cycleRechargeModifier += state.rechargeTime;
            turret->rechargeRemaining = std::max(0.f, turret->stats.rechargeTime + turret->cycleRechargeModifier);
            turret->resetDeck();
        } else {
            turret->castDelayRemaining = std::max(0.f, turret->stats.castDelay + state.castDelay);
        }

        return toShoot;
    }
}
