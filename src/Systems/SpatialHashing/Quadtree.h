//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_QUADTREE_H
#define METEORSHIP_QUADTREE_H

#include <list>
#include <queue>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <memory>
#include <vector>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics.hpp>
#include "ECS/ECS.h"

namespace ms::systems {
    static inline constexpr uint8_t MAX_ENTITIES_IN_NODE = 1;
    static inline constexpr uint8_t MAX_QUADTREE_DEPTH = 4;

    enum class ENodesChildren : uint8_t {
        TopLeft = 0,
        TopRight = 1,
        BottomLeft = 2,
        BottomRight = 3,
    };

    struct Node {
        sf::FloatRect bounds;
        Node* parent;
        std::vector<std::unique_ptr<Node>> children;//I could use array I think?
        std::optional<std::list<ecs::Entity>> entities;//only leafs are allowed to have entities
        uint8_t depth;

        explicit Node(Node* _parent = nullptr, uint8_t const _depth = 0)
            : parent(_parent), depth(_depth) {
            entities.emplace();
        }

        Node(ENodesChildren const where, Node* _parent, const std::list<ecs::Entity>& _entities, uint8_t const _depth, ecs::SparseSet<sf::FloatRect> const& boundsOfEntities)
            : parent(_parent), depth(_depth) {

            if (parent) {
                const auto& parentBounds = parent->bounds;
                const float halfWidth = parentBounds.size.x / 2.0f;
                const float halfHeight = parentBounds.size.y / 2.0f;

                switch (where) {
                    case ENodesChildren::TopLeft:
                        bounds = sf::FloatRect(parentBounds.position, sf::Vector2f(halfWidth, halfHeight));
                        break;
                    case ENodesChildren::TopRight:
                        bounds = sf::FloatRect(
                            sf::Vector2f(parentBounds.position.x + halfWidth, parentBounds.position.y),
                            sf::Vector2f(halfWidth, halfHeight)
                        );
                        break;
                    case ENodesChildren::BottomLeft:
                        bounds = sf::FloatRect(
                            sf::Vector2f(parentBounds.position.x, parentBounds.position.y + halfHeight),
                            sf::Vector2f(halfWidth, halfHeight)
                        );
                        break;
                    case ENodesChildren::BottomRight:
                        bounds = sf::FloatRect(
                            sf::Vector2f(parentBounds.position.x + halfWidth, parentBounds.position.y + halfHeight),
                            sf::Vector2f(halfWidth, halfHeight)
                        );
                        break;
                }
            }

            entities.emplace();
            for (const auto& entity : _entities) {
                if (bounds.findIntersection(boundsOfEntities.get(entity)).has_value()) {
                    entities->push_back(entity);
                }
            }
        }

        [[nodiscard]] bool isLeaf() const {
            return children.empty();
        }

        [[nodiscard]] bool hasEntities() const {
            return entities.has_value() && !entities->empty();
        }
    };

    //TODO: replace all findIntersection functions to isIntersecting as it requires much less computational power
    class Quadtree {
        std::unique_ptr<Node> root;
        ecs::SparseSet<sf::FloatRect> sparse_set;
    public:
        sf::FloatRect worldBounds;
        explicit Quadtree(const sf::FloatRect& bounds) : worldBounds(bounds) {
            root = std::make_unique<Node>(nullptr, 0);
            root->bounds = bounds;
        }

        Quadtree(float const x, float const y, float const width, float const height)
            : worldBounds(sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(width, height))) {
            root = std::make_unique<Node>(nullptr, 0);
            root->bounds = worldBounds;
        }

        Quadtree(const Quadtree&) = delete;
        Quadtree& operator=(const Quadtree&) = delete;

        Quadtree(Quadtree&&) = default;
        Quadtree& operator=(Quadtree&&) = default;

        void insert(ecs::Entity const entity, sf::FloatRect const& bounds) {
            if (!root->bounds.findIntersection(bounds).has_value()) {
                return;
            }
            sparse_set.emplace(entity, bounds);
            insert(entity, bounds, root.get());
        }

        void remove(ecs::Entity const entity) {
            remove(entity, root.get());
            sparse_set.remove(entity);
        }

        void update(ecs::Entity const entity, sf::FloatRect const& newBounds) {
            remove(entity);
            insert(entity, newBounds);
        }

        [[nodiscard]] std::vector<ecs::Entity> query(const sf::FloatRect& bounds) const {
            return query(bounds, root.get());
        }

        [[nodiscard]] std::vector<ecs::Entity> queryPotential(const sf::FloatRect& bounds) const {
            std::vector<ecs::Entity> result;
            queryPotential(bounds, root.get(), result);
            return result;
        }

        [[nodiscard]] std::vector<ecs::Entity> queryAll() const {
            std::vector<ecs::Entity> result;
            queryAll(root.get(), result);
            return result;
        }

        void clear() {
            root = std::make_unique<Node>(nullptr, 0);
            root->bounds = worldBounds;
        }

        void draw(sf::RenderTarget& renderTarget) const {
            draw(root.get(), renderTarget);
        }
    private:
        static void     drawRect(sf::FloatRect const& rect, sf::RenderTarget& renderTarget) {
            constexpr float thickness = 2.f;
            sf::RectangleShape rectangle;
            rectangle.setSize(sf::Vector2f(rect.size - sf::Vector2{thickness * 2.f, thickness * 2.f}));
            rectangle.setFillColor(sf::Color::Transparent);
            rectangle.setOutlineColor(sf::Color::Red);
            rectangle.setOutlineThickness(thickness);
            rectangle.setPosition(rect.position + sf::Vector2(thickness, thickness));
            renderTarget.draw(rectangle);
        }

        static void draw(Node const* node, sf::RenderTarget& renderTarget) {
            if (node->isLeaf()) {
                drawRect(node->bounds, renderTarget);
            } else {
                drawRect(node->bounds, renderTarget);
                for (const auto& child : node->children) {
                    draw(child.get(), renderTarget);
                }
            }
        }

        void insert(ecs::Entity entity, const sf::FloatRect& bounds, Node* node) {
            if (node->isLeaf()) {
                node->entities->emplace_back(entity);

                if (node->entities->size() > MAX_ENTITIES_IN_NODE && node->depth < MAX_QUADTREE_DEPTH) {
                    split(node);
                }
            } else {
                for (const auto& child : node->children) {
                    if (child->bounds.findIntersection(bounds).has_value()) {
                        insert(entity, bounds, child.get());
                    }
                }
            }
        }

        void remove(ecs::Entity entity, Node* node) {
            std::queue<Node*> queue;
            std::unordered_set<Node*> nodesToCheck;
            queue.push(node);

            while (!queue.empty()) {
                auto* current = queue.front();
                queue.pop();

                if (current->isLeaf()) {
                    if (current->entities.has_value()) {
                        auto it = std::ranges::remove_if(*current->entities,
                                                         [&](const auto& ent) { return ent == entity; }).begin();

                        if (it != current->entities->end()) {
                            current->entities->erase(it, current->entities->end());
                            if (current->parent) {
                                nodesToCheck.insert(current->parent);
                            }
                        }
                    }
                } else {
                    for (const auto& child : current->children) {
                        if (sparse_set.has(entity) && child->bounds.findIntersection(sparse_set.get(entity)).has_value()) {
                            queue.push(child.get());
                        }
                    }
                }
            }

            for (auto* n : nodesToCheck) {
                if (canJoin(n)) {
                    join(n);
                }
            }
        }

        static bool canSplit(Node const* node) {
            return node->entities->size() > MAX_ENTITIES_IN_NODE && node->depth < MAX_QUADTREE_DEPTH;
        }

        static bool canJoin(Node const* node) {
            if (!node || node->isLeaf()) return false;

            size_t totalEntities = 0;
            for (const auto& child : node->children) {
                if (child->isLeaf() && child->entities.has_value()) {
                    totalEntities += child->entities->size();
                } else {
                    return false;
                }
                if (totalEntities > MAX_ENTITIES_IN_NODE) {
                    return false;
                }
            }
            return true;
        }

        std::vector<ecs::Entity> query(const sf::FloatRect& bounds, Node* node) const {
            std::queue<Node*> queue;
            std::vector<ecs::Entity> entities;
            queue.push(node);

            while (!queue.empty()) {
                const auto* current = queue.front();
                queue.pop();

                if (current->isLeaf()) {
                    if (current->entities.has_value()) {
                        for (const auto& ent: *current->entities) {
                            if (bounds.findIntersection(sparse_set.get(ent)).has_value()) {
                                entities.push_back(ent);
                            }
                        }
                    }
                } else {
                    for (const auto& child : current->children) {
                        if (child->bounds.findIntersection(bounds).has_value()) {
                            queue.push(child.get());
                        }
                    }
                }
            }

            return entities;
        }

        static void queryPotential(const sf::FloatRect& bounds, Node const* node,
                                   std::vector<ecs::Entity>& result) {
            if (!node->bounds.findIntersection(bounds).has_value()) {
                return;
            }

            if (node->isLeaf()) {
                if (node->entities.has_value()) {
                    result.insert(result.end(), node->entities->begin(), node->entities->end());
                }
                return;
            }

            for (const auto& child : node->children) {
                queryPotential(bounds, child.get(), result);
            }
        }

        static void queryAll(Node const* node, std::vector<ecs::Entity>& result) {
            if (node->isLeaf() && node->entities.has_value()) {
                for (const auto &ent: *node->entities) {
                    result.push_back(ent);
                }
            } else {
                for (const auto& child : node->children) {
                    queryAll(child.get(), result);
                }
            }
        }

        void split(Node* node) {
            auto entitiesToDistribute = std::move(*node->entities);

            node->children.reserve(4);
            for (int i = 0; i < 4; ++i) {
                auto child = std::make_unique<Node>(
                    static_cast<ENodesChildren>(i),
                    node,
                    entitiesToDistribute,
                    node->depth + 1,
                    sparse_set
                );
                node->children.push_back(std::move(child));
            }
            node->entities.reset();
            for (auto const& child : node->children) {
                if (canSplit(child.get())) {
                    split(child.get());
                }
            }
        }

        static void join(Node* node) {
            node->entities.emplace();
            for (auto& child : node->children) {
                if (child->isLeaf() && child->entities.has_value()) {
                    node->entities->splice(node->entities->end(), *child->entities);
                    child->entities.reset();
                }
            }

            node->children.clear();
            if (canJoin(node->parent)) {
                join(node->parent);
            }
        }
    };
}

#endif //METEORSHIP_QUADTREE_H