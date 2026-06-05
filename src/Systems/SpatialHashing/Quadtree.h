//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_QUADTREE_H
#define METEORSHIP_QUADTREE_H

#include <list>
#include <queue>
#include <unordered_set>
#include <SFML/Graphics/Rect.hpp>
#include "ECS/ECS.h"

namespace ms::systems {
    static inline constexpr uint8_t MAX_ENTITIES_IN_NODE = 8;

    enum class ENodesChildren : uint8_t {
        TopRight,
        TopLeft,
        BottomRight,
        BottomLeft,
    };

    struct Node {
        sf::FloatRect bounds;

        Node* parent;
        std::vector<Node*> children;
        std::optional<std::list<std::pair<ecs::Entity, sf::FloatRect>>> entities; // only constructed when children vector is empty, that means this node is a leaf

        Node(ENodesChildren where, Node* _parent, std::list<std::pair<ecs::Entity, sf::FloatRect>> _entities) {
            switch (where) {
                case ENodesChildren::TopRight:
                    bounds = {};
                    break;
                case ENodesChildren::TopLeft:
                    bounds = {};
                    break;
                case ENodesChildren::BottomRight:
                    bounds = {};
                    break;
                case ENodesChildren::BottomLeft:
                    bounds = {};
                    break;
                default:
                    bounds = {};
            }
            parent = _parent;
            if (_entities.empty()) {
                entities = std::nullopt;
            } else {
                entities.emplace();
                for (auto& entity : _entities) {
                    if (bounds.findIntersection(entity.second).has_value()) {
                        entities.value().emplace_back(entity);
                    }
                }
            }
        }
    };

    class Quadtree {
    private:
        Node* root = new Node(static_cast<ENodesChildren>(-1), nullptr, {});
    public:
        void insert(ecs::Entity entity, sf::FloatRect bounds) {
            insert(entity, bounds, root);
        }

        void remove(ecs::Entity entity, sf::FloatRect bounds) {
            remove(entity, bounds, root);
        }

        std::list<ecs::Entity> query(sf::FloatRect bounds) {
            return query(bounds, root);
        }

    private:
        void insert(ecs::Entity entity, sf::FloatRect bounds, Node* node) {
            if (node->entities.has_value()) {
                node->entities.value().emplace_back(entity, bounds);
                if (node->entities.value().size() > MAX_ENTITIES_IN_NODE) {
                    split(node);
                }
                return;
            }
            for (auto const& child : node->children) {
                if (child->bounds.findIntersection(bounds).has_value()) {
                    insert(entity, bounds, child);
                }
            }
        }

        void remove(ecs::Entity entity, sf::FloatRect bounds, Node* node) {
            std::queue<Node*> queue;
            std::unordered_set<Node*> nodesToCheck;
            queue.push(node);

            while (!queue.empty()) {
                auto nq = queue.front();
                if (nq->entities.has_value()) {
                    std::erase_if(nq->entities.value(), [&](auto const& var) {
                       return var.first == entity;
                    });
                    nodesToCheck.insert(nq->parent);
                } else {
                    for (auto const& child : node->children) {
                        if (child->bounds.findIntersection(bounds).has_value()) {
                            queue.push(child);
                        }
                    }
                }
                queue.pop();
            }

            for (auto const& n : nodesToCheck) {
                int count = 0;
                for (auto const& child : n->children) {
                    count += child->entities.value().size();
                }
                if (count <= MAX_ENTITIES_IN_NODE) {
                    join(n);
                }
            }
        }

        std::list<ecs::Entity> query(sf::FloatRect bounds, Node* node) {
            std::queue<Node*> queue;
            std::list<ecs::Entity> entities;
            queue.push(node);
            while (!queue.empty()) {
                auto nq = queue.front();
                if (nq->entities.has_value()) {
                    for (auto const& [ent, bd] : nq->entities.value()) {
                        if (bounds.findIntersection(bd).has_value()) {
                            entities.emplace_back(ent);
                        }
                    }
                } else {
                    for (auto const& child : nq->children) {
                        if (child->bounds.findIntersection(bounds).has_value()) {
                            queue.push(child);
                        }
                    }
                }
                queue.pop();
            }

            return entities;
        }

        void split(Node* node) {
            node->children.reserve(4);
            for (int i = 0; i < 4; ++i) {
                node->children[i] = new Node(static_cast<ENodesChildren>(i), node, node->entities.value());
                if (node->children[i]->entities.value().size() > MAX_ENTITIES_IN_NODE) {
                    split(node->children[i]);
                }
            }
            node->entities.reset();
            node->entities = std::nullopt;
        }

        void join(Node* node) {
            node->entities.emplace();
            for (auto const& child : node->children) {
                auto& _ent = node->entities.value();
                _ent.splice(_ent.end(), child->entities.value());
                child->entities.reset();
                child->entities = std::nullopt;
            }
            node->children.clear();
        }
    };
}


#endif //METEORSHIP_QUADTREE_H
