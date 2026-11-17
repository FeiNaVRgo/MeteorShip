#ifndef METEORSHIP_QUADTREE_H
#define METEORSHIP_QUADTREE_H

#include <algorithm>
#include <list>
#include <memory>
#include <unordered_set>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics.hpp>

#include "ECS/ECS.h"

namespace ms::systems {
    static inline constexpr uint8_t MAX_ENTITIES_IN_NODE = 8;
    static inline constexpr uint8_t MAX_QUADTREE_DEPTH = 4;

    enum class ENodesChildren : uint8_t {
        TopLeft = 0,
        TopRight = 1,
        BottomLeft = 2,
        BottomRight = 3,
    };

    struct EntityBounds {
        ecs::Entity entity;
        sf::FloatRect bounds;
    };

    struct Node {
        sf::FloatRect bounds;
        Node* parent{};
        std::vector<std::unique_ptr<Node>> children;
        std::list<EntityBounds> entities;
        uint8_t depth{};

        explicit Node(Node* parent = nullptr, const uint8_t depth = 0)
            : parent(parent), depth(depth) {}

        [[nodiscard]] bool isLeaf() const {
            return children.empty();
        }
    };

    class Quadtree {
        std::unique_ptr<Node> root;

    public:
        sf::FloatRect worldBounds;

        explicit Quadtree(const sf::FloatRect& bounds) : worldBounds(bounds) {
            resetRoot();
        }

        Quadtree(
            const float x,
            const float y,
            const float width,
            const float height)
            : worldBounds({{x, y}, {width, height}}) {
            resetRoot();
        }

        Quadtree(const Quadtree&) = delete;
        Quadtree& operator=(const Quadtree&) = delete;
        Quadtree(Quadtree&&) = default;
        Quadtree& operator=(Quadtree&&) = default;

        void insert(const ecs::Entity entity, const sf::FloatRect& bounds) {
            remove(entity);
            if (root->bounds.findIntersection(bounds)) {
                insert({entity, bounds}, root.get());
            }
        }

        void remove(const ecs::Entity entity) {
            remove(entity, root.get());
        }

        void update(const ecs::Entity entity, const sf::FloatRect& newBounds) {
            insert(entity, newBounds);
        }

        [[nodiscard]] std::vector<ecs::Entity> query(
            const sf::FloatRect& bounds) const {
            std::vector<ecs::Entity> result;
            std::unordered_set<ecs::Entity> seen;
            query(bounds, root.get(), result, seen);
            return result;
        }

        [[nodiscard]] std::vector<ecs::Entity> query(const float radius, const sf::Vector2f point) const {
            const sf::Vector2f half = {radius, -radius};
            return query(sf::FloatRect(point - half, {2.f * radius, 2.f * radius}));
        }

        [[nodiscard]] std::vector<ecs::Entity> queryPotential(
            const sf::FloatRect& bounds) const {
            return query(bounds);
        }

        [[nodiscard]] std::vector<ecs::Entity> queryAll() const {
            std::vector<ecs::Entity> result;
            std::unordered_set<ecs::Entity> seen;
            queryAll(root.get(), result, seen);
            return result;
        }

        void clear() {
            resetRoot();
        }

        void draw(sf::RenderTarget& renderTarget) const {
            draw(root.get(), renderTarget);
        }

    private:
        void resetRoot() {
            root = std::make_unique<Node>();
            root->bounds = worldBounds;
        }

        static void drawRect(
            const sf::FloatRect& rect,
            sf::RenderTarget& renderTarget) {
            constexpr float thickness = 2.f;
            sf::RectangleShape rectangle;
            rectangle.setSize(rect.size - sf::Vector2{thickness * 2.f, thickness * 2.f});
            rectangle.setFillColor(sf::Color::Transparent);
            rectangle.setOutlineColor(sf::Color::Red);
            rectangle.setOutlineThickness(thickness);
            rectangle.setPosition(rect.position + sf::Vector2(thickness, thickness));
            renderTarget.draw(rectangle);
        }

        static void draw(const Node* node, sf::RenderTarget& renderTarget) {
            drawRect(node->bounds, renderTarget);
            for (const auto& child : node->children) {
                draw(child.get(), renderTarget);
            }
        }

        static std::unique_ptr<Node> makeChild(
            Node* parent,
            const ENodesChildren childType) {
            const auto halfWidth = parent->bounds.size.x / 2.f;
            const auto halfHeight = parent->bounds.size.y / 2.f;
            sf::Vector2f position = parent->bounds.position;
            switch (childType) {
                case ENodesChildren::TopLeft:
                    break;
                case ENodesChildren::TopRight:
                    position.x += halfWidth;
                    break;
                case ENodesChildren::BottomLeft:
                    position.y += halfHeight;
                    break;
                case ENodesChildren::BottomRight:
                    position += sf::Vector2f(halfWidth, halfHeight);
                    break;
            }
            auto child = std::make_unique<Node>(parent, parent->depth + 1);
            child->bounds = {position, {halfWidth, halfHeight}};
            return child;
        }

        static void insert(const EntityBounds& entity, Node* node) {
            if (node->isLeaf()) {
                node->entities.push_back(entity);
                if (node->entities.size() > MAX_ENTITIES_IN_NODE &&
                    node->depth < MAX_QUADTREE_DEPTH) {
                    split(node);
                }
                return;
            }

            for (const auto& child : node->children) {
                if (child->bounds.findIntersection(entity.bounds)) {
                    insert(entity, child.get());
                }
            }
        }

        static void remove(const ecs::Entity entity, Node* node) {
            std::erase_if(node->entities, [entity](const EntityBounds& entry) {
                return entry.entity == entity;
            });
            for (const auto& child : node->children) {
                remove(entity, child.get());
            }
            if (canJoin(node)) {
                join(node);
            }
        }

        static bool canJoin(const Node* node) {
            if (node->isLeaf()) {
                return false;
            }
            std::unordered_set<ecs::Entity> entities;
            for (const auto& child : node->children) {
                if (!child->isLeaf()) {
                    return false;
                }
                for (const auto& entry : child->entities) {
                    entities.insert(entry.entity);
                    if (entities.size() > MAX_ENTITIES_IN_NODE) {
                        return false;
                    }
                }
            }
            return true;
        }

        static void query(
            const sf::FloatRect& bounds,
            const Node* node,
            std::vector<ecs::Entity>& result,
            std::unordered_set<ecs::Entity>& seen) {
            if (!node->bounds.findIntersection(bounds)) {
                return;
            }
            for (const auto& entry : node->entities) {
                if (entry.bounds.findIntersection(bounds) &&
                    seen.insert(entry.entity).second) {
                    result.push_back(entry.entity);
                }
            }
            for (const auto& child : node->children) {
                query(bounds, child.get(), result, seen);
            }
        }

        static void queryAll(
            const Node* node,
            std::vector<ecs::Entity>& result,
            std::unordered_set<ecs::Entity>& seen) {
            for (const auto& entry : node->entities) {
                if (seen.insert(entry.entity).second) {
                    result.push_back(entry.entity);
                }
            }
            for (const auto& child : node->children) {
                queryAll(child.get(), result, seen);
            }
        }

        static void split(Node* node) {
            auto entitiesToDistribute = std::move(node->entities);
            node->children.reserve(4);
            for (uint8_t i = 0; i < 4; ++i) {
                node->children.push_back(
                    makeChild(node, static_cast<ENodesChildren>(i)));
            }
            for (const auto& entity : entitiesToDistribute) {
                for (const auto& child : node->children) {
                    if (child->bounds.findIntersection(entity.bounds)) {
                        insert(entity, child.get());
                    }
                }
            }
        }

        static void join(Node* node) {
            std::unordered_set<ecs::Entity> seen;
            for (const auto& child : node->children) {
                for (const auto& entry : child->entities) {
                    if (seen.insert(entry.entity).second) {
                        node->entities.push_back(entry);
                    }
                }
            }
            node->children.clear();
        }
    };
}

#endif //METEORSHIP_QUADTREE_H
