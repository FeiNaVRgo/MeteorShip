//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_ECS_H
#define METEORSHIP_ECS_H
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ms {
    constexpr static inline uint8_t MAX_COMPONENTS = 32;
    namespace ecs {
        using Entity = uint32_t;
        struct Component {};

        class ComponentManager {
            std::vector<std::string> components;
            std::vector<std::unordered_map<std::string, void*>> entities;//not aligned but meh
        public:
            ComponentManager() {
                components.reserve(MAX_COMPONENTS);
            }

            template<class T>
            void registerComponent() {
                if (std::ranges::find(components, typeid(T).name()) == components.end()) {
                    components.emplace_back(typeid(T).name());
                }
            }

            template<class T>
            void registerComponentToEntity(Entity entity, T* component) {
                entities[entity].emplace(typeid(T).name(), component);
            }

            void insertEntity(Entity entity) {
                if (entities.size() <= entity) {
                    entities.emplace_back();
                }
            }

            template<class T>
            [[nodiscard]] uint32_t getComponent() const {
                auto t_name = typeid(T).name();
                int i = 0;
                for (const auto& component : components) {
                    if (component == t_name) {
                        return i;
                    }
                    i++;
                }
                return -1;
            }

            [[nodiscard]] bool hasComponent(Entity entity, std::string const& str) const {
                return entities[entity].contains(str);
            }

            template<class T>
            T& getComponent(Entity entity) {
                return *static_cast<T*>(entities[entity].at(typeid(T).name()));
            }
        };

        class EntityManager {
            std::unordered_map<Entity, std::bitset<MAX_COMPONENTS>> entities;
        public:
            void insertEntity(Entity entity) {
                entities.insert({entity, {}});
            }

            template<class T>
            void registerComponentToEntity(Entity entity, T* component, ComponentManager& componentManager) {
                auto entityIt = entities.find(entity);
                if (entityIt != entities.end()) {
                    auto i = componentManager.getComponent<T>();
                    if (i != -1) {
                        entityIt->second.set(i, true);
                    }
                    componentManager.registerComponentToEntity<T>(entity, component);
                }
            }

            template<class T>
            T& getComponent(Entity entity, ComponentManager& componentManager) {
                auto entityIt = entities.find(entity);
                if (entityIt != entities.end()) {
                    auto iC = componentManager.getComponent<T>();
                    if (iC != -1 && entityIt->second.test(iC)) {
                        return componentManager.getComponent<T>(entity);
                    }
                }
            }

            Entity getNextEntity() const {
                return entities.size();
            }
        };

        class System {
            std::unordered_set<std::string> components;
        public:
            std::unordered_set<Entity> _entities;

            void registerComponent(std::string const& componentName) {
                components.insert(componentName);
            }

            void insertEntity(Entity entity, const ComponentManager& componentManager) {
                for (auto& component : components) {
                    if (componentManager.hasComponent(entity, component)) {
                        _entities.insert(entity);
                    }
                }
            }
            virtual void run() {};
        };

        class SystemManager {
        public:
            std::vector<System*> systems;

            template<class S>
            void registerSystem(S& system) {
                systems.push_back(&system);
            }

            void insertEntity(Entity entity, const ComponentManager& componentManager) const {
                for (const auto& system : systems) {
                    system->insertEntity(entity, componentManager);
                }
            }
        };

        class ECS {
            EntityManager entityManager;
            SystemManager systemManager;
        public:
            ComponentManager componentManager;

            template<class S>
            void registerSystem(S& system) {
                systemManager.registerSystem<S>(system);
            }

            template<class C, class S>
            void registerComponentToSystem(S& system) {
                system.registerComponent(typeid(C).name());
            }

            template<class C>
            void registerComponent() {
                componentManager.registerComponent<C>();
            }

            Entity createEntity() {
                Entity entity = entityManager.getNextEntity();
                entityManager.insertEntity(entity);
                componentManager.insertEntity(entity);
                return entity;
            }

            template<class C>
            void registerComponentToEntity(Entity entity, C* component) {
                entityManager.registerComponentToEntity<C>(entity, component, componentManager);
                systemManager.insertEntity(entity, componentManager);
            }

            template<class C>
            C& getComponent(Entity entity) {
                return entityManager.getComponent<C>(entity, componentManager);
            }
        };
    } // ecs
} // ms

#endif //METEORSHIP_ECS_H
