//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_ECS_H
#define METEORSHIP_ECS_H
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <functional>
#include <iostream>
#include <stack>

namespace ms {
    class World;

    constexpr static inline uint8_t MAX_COMPONENTS = 32;

    namespace ecs {
        using Entity = uint32_t;
        using Version = uint32_t;
        using ComponentType = uint8_t;
        using Signature = std::bitset<MAX_COMPONENTS>;

        inline ComponentType getNextComponentTypeId() {
            static ComponentType nextId = 0;
            return nextId++;
        }

        template<typename T>
        struct ComponentTypeId {
            static ComponentType get() {
                static const ComponentType id = getNextComponentTypeId();
                return id;
            }
        };

        class IComponentPool {
        public:
            virtual ~IComponentPool() = default;
            virtual void remove(Entity entity) = 0;
            [[nodiscard]] virtual bool has(Entity entity) const = 0;
            [[nodiscard]] virtual size_t size() const = 0;
        };

        template<typename T>
        class SparseSet : public IComponentPool {
        private:
            std::vector<Entity> sparse;
            std::vector<Entity> dense;
            std::vector<T> components;
        public:
            explicit SparseSet() = default;

            [[nodiscard]] bool has(Entity const entity) const override {
                if (entity >= sparse.size()) return false;
                const size_t pos = sparse[entity];
                return pos != static_cast<Entity>(-1) && pos < dense.size() && dense[pos] == entity;
            }

            T& get(Entity const entity) {
                assert(has(entity));
                return components[sparse[entity]];
            }

            [[nodiscard]] const T& get(Entity const entity) const {
                assert(has(entity));
                return components[sparse[entity]];
            }

            template<typename ...Args>
            T& emplace(Entity const entity, Args&& ...args) {
                if (has(entity)) {
                    T& comp = components[sparse[entity]];
                    comp = T(std::forward<Args>(args)...);
                    return comp;
                }

                if (entity >= sparse.size()) {
                    sparse.resize(entity + 1, static_cast<Entity>(-1));
                }

                const size_t pos = dense.size();
                sparse[entity] = static_cast<Entity>(pos);
                dense.push_back(entity);
                components.emplace_back(std::forward<Args>(args)...);
                return components.back();
            }

            void remove(Entity const entity) override {
                if (!has(entity)) return;
                size_t pos = sparse[entity];
                size_t lastPos = dense.size() - 1;
                const auto lastEntity = dense[lastPos];

                dense[pos] = lastEntity;
                components[pos] = std::move(components[lastPos]);
                sparse[lastEntity] = pos;

                dense.pop_back();
                components.pop_back();
                sparse[entity] = static_cast<Entity>(-1);
            }

            T* raw_data() { return components.data(); }
            [[nodiscard]] const T* raw_data() const { return components.data(); }

            template<typename Func>
            void each(Func&& func) {
                for (size_t i = 0; i < dense.size(); ++i) {
                    func(dense[i], components[i]);
                }
            }

            [[nodiscard]] const std::vector<T>& view() const { return components; }
            [[nodiscard]] const std::vector<Entity>& entities() const { return dense; }

            [[nodiscard]] size_t size() const override { return dense.size(); }
            [[nodiscard]] bool empty() const { return dense.empty(); }

            void clear() {
                sparse.clear();
                dense.clear();
                components.clear();
            }
        };

        class ComponentManager {
        private:
            std::array<std::unique_ptr<IComponentPool>, MAX_COMPONENTS> componentPools{};
            ComponentType numComponents = 0;

            template<typename T>
            SparseSet<T>& getPool() {
                const ComponentType id = ComponentTypeId<T>::get();

                if (id >= MAX_COMPONENTS) {
                    throw std::runtime_error("Component type ID out of range!");
                }

                if (!componentPools[id]) {
                    componentPools[id] = std::make_unique<SparseSet<T>>();
                }

                return *static_cast<SparseSet<T>*>(componentPools[id].get());
            }

            template<typename T>
            const SparseSet<T>& getPool() const {
                const ComponentType id = ComponentTypeId<T>::get();
                assert(id < numComponents && componentPools[id] != nullptr);
                return *static_cast<const SparseSet<T>*>(componentPools[id].get());
            }
        public:
            void removeComponentByType(const Entity entity, const ComponentType type) const {
                if (type >= numComponents || !componentPools[type]) return;
                componentPools[type]->remove(entity);
            }

            void removeAllComponentsFromEntity(const Entity entity) const {
                for (ComponentType type = 0; type < numComponents; ++type) {
                    if (componentPools[type] && componentPools[type]->has(entity)) {
                        componentPools[type]->remove(entity);
                    }
                }
            }

            template<typename T>
            void registerComponent() {
                const ComponentType id = ComponentTypeId<T>::get();
                if (id >= MAX_COMPONENTS) {
                    throw std::runtime_error("Component type ID out of range!");
                }
                if (!componentPools[id]) {
                    componentPools[id] = std::make_unique<SparseSet<T>>();
                    numComponents++;
                }
            }

            template<typename T, typename... Args>
            T& addComponent(Entity entity, Args&& ...args) {
                return getPool<T>().emplace(entity, std::forward<Args>(args)...);
            }

            template<typename T>
            bool hasComponent(Entity entity) {
                return getPool<T>().has(entity);
            }

            template<typename T>
            T& getComponent(Entity entity) {
                return getPool<T>().get(entity);
            }

            template<typename T>
            void removeComponent(Entity entity) {
                getPool<T>().remove(entity);
            }

            template<typename T, typename Func>
            void forEach(Func&& func) {
                getPool<T>().each(std::forward<Func>(func));
            }

            template<typename T>
            T* getComponentArray() {
                return getPool<T>().raw_data();
            }

            template<typename T>
            [[nodiscard]] std::vector<Entity> const& getEntitiesWithComponent() const {
                return getPool<T>().entities();
            }
        };

        class EntityManager {
        private:
            struct EntityData {
                Version version = 0;
                bool alive = false;
            };

            std::vector<EntityData> entities;
            //entities vector will not shrink, if entity is to destroyed its only
            //going to set alive to false and push free entity to use to stack
            //using stack minimalizes amount of memory used at minimal cost
            //versions are only there to count how many times an entity was
            //reused, in other terms pulled from stack of free entities
            std::stack<Entity> freeList;
            size_t aliveCount = 0;

        public:
            Entity create() {
                Entity entity;

                if (!freeList.empty()) {
                    entity = freeList.top();
                    freeList.pop();
                    entities[entity].alive = true;
                    entities[entity].version++;
                    aliveCount++;
                } else {
                    entity = entities.size();
                    //it can reallocate too much memory if we operate on MANY entities
                    //IDK how urgent it is tho, I could probably use a mix of allocate
                    //and/or shrink_to_fit
                    entities.emplace_back();
                    entities[entity].alive = true;
                    entities[entity].version = 0;
                    aliveCount++;
                }

                return entity;
            }

            void destroy(const Entity entity) {
                if (entity >= entities.size() || !entities[entity].alive) return;

                entities[entity].alive = false;
                freeList.push(entity);
                aliveCount--;
            }

            [[nodiscard]] bool isAlive(const Entity entity) const {
                return entity < entities.size() && entities[entity].alive;
            }

            [[nodiscard]] Version getVersion(const Entity entity) const {
                return entity < entities.size() ? entities[entity].version : 0;
            }

            [[nodiscard]] size_t getAliveCount() const { return aliveCount; }
            [[nodiscard]] size_t capacity() const { return entities.size(); }
        };

        template<typename ...Components>
        class View {
        private:
            using Entities = std::vector<Entity>;

            ComponentManager* cm;
            Entities const& entities;

            template<typename T>
            [[nodiscard]] auto& getEntitiesFromType() const {
                return cm->getEntitiesWithComponent<T>();
            }

            template<typename First, typename Second, typename ...Rest>
            [[nodiscard]] auto& findPoolWithMinEntities() {
                auto& firstVal = getEntitiesFromType<First>();
                auto& secondVal = findPoolWithMinEntities<Second>();
                return firstVal.size() < secondVal.size() ? firstVal : secondVal;
            }

            template<typename T>
            [[nodiscard]] auto& findPoolWithMinEntities() const {
                return getEntitiesFromType<T>();
            }

        public:
            explicit View(ComponentManager* cm) : cm(cm), entities(findPoolWithMinEntities<Components...>()) {}

            class Iterator {
            private:
                Entities::const_iterator entityIt;
                Entities::const_iterator end;
                ComponentManager* cm;

                void skipMissing() {
                    while (entityIt != end && !hasAllComponents(*entityIt)) {
                        ++entityIt;
                    }
                }

                [[nodiscard]] bool hasAllComponents(const Entity entity) const {
                    return (cm->hasComponent<Components>(entity) && ...);
                }
            public:
                Iterator(ComponentManager* mgr,
                         const Entities::const_iterator it,
                         const Entities::const_iterator endIt)
                    : entityIt(it), end(endIt), cm(mgr) {
                    skipMissing();
                }

                Iterator& operator++() {
                    ++entityIt;
                    skipMissing();
                    return *this;
                }

                std::tuple<Components&...> operator*() {
                    return {cm->getComponent<Components>(*entityIt)...};
                }

                bool operator!=(const Iterator& other) const {
                    return entityIt != other.entityIt;
                }
            };

            Iterator begin() {
                return Iterator(cm, entities.begin(), entities.end());
            }

            Iterator end() {
                return Iterator(cm, entities.end(), entities.end());
            }

            template<typename Func>
            void each(Func&& func) {
                for (Entity entity : entities) {
                    if ((cm->hasComponent<Components>(entity) && ...)) {
                        func(entity, cm->getComponent<Components>(entity)...);
                    }
                }
            }
        };

        class ECS;

        class System {
        private:
            World* world = nullptr;
            std::function<bool()> conditionToUpdate;

            friend class ECS;

        public:
            virtual ~System() = default;

            [[nodiscard]] World& getWorld() const {
                assert(world != nullptr);
                return *world;
            }

            virtual void update(float deltaTime) {}
        };

        class ECS {
        private:
            ComponentManager componentManager;
            EntityManager entityManager;
            std::vector<std::unique_ptr<System>> systems;
        public:

            template<typename T>
            void registerComponent() {
                componentManager.registerComponent<T>();
            }

            template<typename T, typename ... Args>
            T& addComponent(Entity entity, Args&& ...args) {
                T& component = componentManager.addComponent<T>(entity, std::forward<Args>(args)...);
                return component;
            }

            template<typename T>
            bool hasComponent(const Entity entity) {
                return componentManager.hasComponent<T>(entity);
            }

            template<typename T>
            T& getComponent(const Entity entity) {
                return componentManager.getComponent<T>(entity);
            }

            template<typename T>
            void removeComponent(const Entity entity) {
                if (hasComponent<T>(entity)) {
                    componentManager.removeComponent<T>(entity);
                }
            }

            Entity createEntity() {
                return entityManager.create();
            }

            void destroyEntity(const Entity entity) {
                if (!entityManager.isAlive(entity)) return;
                componentManager.removeAllComponentsFromEntity(entity);
                entityManager.destroy(entity);
            }

            [[nodiscard]] bool isEntityAlive(Entity entity) const {
                return entityManager.isAlive(entity);
            }

            template<typename ... Components>
            View<Components...> view() {
                return View<Components...>(&componentManager);
            }

            template<typename S, typename ...Args>
            S& registerSystem(World& world, Args&& ...args) {
                auto system = std::make_unique<S>(std::forward<Args>(args)...);
                system->world = &world;
                S& ref = *system;
                systems.emplace_back(std::move(system));
                return ref;
            }

            void update(float const deltaTime) const {
                for (auto& system : systems) {
                    system->update(deltaTime);
                }
            }

            template<typename T, typename Func>
            void forEach(Func&& func) {
                componentManager.forEach<T>(std::forward<Func>(func));
            }
        };
    } // ecs
} // ms

#endif //METEORSHIP_ECS_H
