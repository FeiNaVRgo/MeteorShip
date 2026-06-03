//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_ECS_H
#define METEORSHIP_ECS_H
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <memory>
#include <ostream>
#include <ranges>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>

namespace ms {
    constexpr static inline uint8_t MAX_COMPONENTS = 32;
    constexpr static inline size_t MAX_ENTITIES = 10000;

    namespace ecs {
        using Entity = uint32_t;
        using Version = uint32_t;
        using ComponentType = uint8_t;

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

        struct IComponent {};


        template<typename T>
        class SparseSet {
        private:
            std::vector<Entity> sparse;
            std::vector<Entity> dense;
            std::vector<T> components;
            std::vector<size_t> entityToIndex;

            size_t maxEntities;
        public:
            explicit SparseSet(size_t maxEnt = MAX_ENTITIES) : maxEntities(maxEnt) {
                sparse.resize(maxEnt, static_cast<Entity>(-1));
                dense.reserve(maxEnt);
                components.reserve(maxEnt);
            }

            [[nodiscard]] bool has(Entity entity) const {
                if (entity >= sparse.size()) return false;
                size_t pos = sparse[entity];
                return pos != static_cast<Entity>(-1) && pos < dense.size() && dense[pos] == entity;
            }

            T& get(Entity entity) {
                assert(has(entity));
                return components[sparse[entity]];
            }

            [[nodiscard]] const T& get(Entity entity) const {
                assert(has(entity));
                return components[sparse[entity]];
            }

            template<typename ...Args>
            T& emplace(Entity entity, Args&& ...args) {
                std::cout << std::endl;
                if (has(entity)) {
                    T& comp = components[sparse[entity]];
                    comp = T(std::forward<Args>(args)...);
                    return comp;
                }

                size_t pos = dense.size();
                sparse[entity] = pos;
                dense.push_back(entity);
                components.emplace_back(std::forward<Args>(args)...);
                return components.back();
            }

            void remove(Entity entity) {
                if (!has(entity)) return;
                size_t pos = sparse[entity];
                size_t lastPos = dense.size() - 1;
                Entity lastEntity = dense[lastPos];

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

            [[nodiscard]] size_t size() const { return dense.size(); }
            [[nodiscard]] bool empty() const { return dense.empty(); }

            void clear() {
                sparse.assign(MAX_ENTITIES, static_cast<Entity>(-1));
                dense.clear();
                components.clear();
            }
        };

        class ComponentManager {
        private:
            std::array<void*, MAX_COMPONENTS> componentPools;
            std::array<size_t, MAX_COMPONENTS> componentSizes;
            std::array<ComponentType, MAX_COMPONENTS> componentIds;
            ComponentType numComponents = 0;

            template<typename T>
            SparseSet<T>& getPool() {
                ComponentType id = ComponentTypeId<T>::get();

                if (id >= MAX_COMPONENTS) {
                    throw std::runtime_error("Component type ID out of range!");
                }

                if (componentPools[id] == nullptr) {
                    componentPools[id] = new SparseSet<T>();
                    componentSizes[id] = sizeof(T);
                }

                return *static_cast<SparseSet<T>*>(componentPools[id]);
            }

            template<typename T>
            const SparseSet<T>& getPool() const {
                ComponentType id = ComponentTypeId<T>::get();
                assert(id < numComponents && componentPools[id] != nullptr);
                return *static_cast<const SparseSet<T>*>(componentPools[id]);
            }
        public:
            ComponentManager() {
                componentPools.fill(nullptr);
                componentSizes.fill(0);
            }

            ~ComponentManager() {
                for (auto & componentPool : componentPools) {
                    delete static_cast<SparseSet<uint8_t>*>(componentPool);
                }
            }

            template<typename T>
            void registerComponent() {
                ComponentType id = ComponentTypeId<T>::get();
                if (componentPools[id] == nullptr) {
                    componentPools[id] = new SparseSet<T>();
                    componentSizes[id] = sizeof(T);
                    componentIds[numComponents] = id;
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
            [[nodiscard]] const std::vector<Entity>& getEntitiesWithComponent() const {
                return getPool<T>().entities();
            }

            [[nodiscard]] size_t getComponentCount(ComponentType type) const {
                if (type >= numComponents || componentPools[type] == nullptr) return 0;
                return static_cast<SparseSet<uint8_t>*>(componentPools[type])->size();
            }
        };

        class EntityManager {
        private:
            std::vector<Version> versions;
            std::vector<bool> alive;
            std::vector<Entity> freeList;
            size_t nextEntity = 0;

        public:
            EntityManager(size_t maxEntities = MAX_ENTITIES) {
                versions.resize(maxEntities, 0);
                alive.resize(maxEntities, false);
                freeList.reserve(maxEntities);
            }

            Entity create() {
                Entity entity;
                if (!freeList.empty()) {
                    entity = freeList.back();
                    freeList.pop_back();
                    versions[entity]++;
                    alive[entity] = true;
                } else {
                    entity = nextEntity++;
                    versions[entity] = 0;
                    alive[entity] = true;
                }
                return entity;
            }

            void destroy(Entity entity) {
                if (entity >= alive.size() || !alive[entity]) return;
                alive[entity] = false;
                freeList.push_back(entity);
            }

            [[nodiscard]] bool isAlive(Entity entity) const {
                return entity < alive.size() && alive[entity];
            }

            [[nodiscard]] Version getVersion(Entity entity) const {
                return entity < versions.size() ? versions[entity] : 0;
            }

            [[nodiscard]] size_t aliveCount() const {
                return nextEntity - freeList.size();
            }
        };

        template<typename ...Components>
        class View {
        private:
            ComponentManager* cm;

            template<typename T>
            static constexpr size_t getPoolSize() {
                return sizeof(T);
            }

            template<typename T, typename... Rest>
            static constexpr size_t getSmallestPool() {
                if constexpr (sizeof(T) < getSmallestPool<Rest...>())
                    return sizeof(T);
                else
                    return getSmallestPool<Rest...>();
            }
        public:
            explicit View(ComponentManager* cm) : cm(cm) {}

            class Iterator {
            private:
                std::vector<Entity>::const_iterator entityIt;
                std::vector<Entity>::const_iterator end;
                ComponentManager* cm;

                void skipMissing() {
                    while (entityIt != end && !hasAllComponents(*entityIt)) {
                        ++entityIt;
                    }
                }

                [[nodiscard]] bool hasAllComponents(Entity entity) const {
                    return (cm->hasComponent<Components>(entity) && ...);
                }
            public:
                Iterator(ComponentManager* mgr,
                         std::vector<Entity>::const_iterator it,
                         std::vector<Entity>::const_iterator endIt)
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
                const auto& entities = cm->getEntitiesWithComponent<std::tuple_element_t<0, std::tuple<Components...>>>();
                return Iterator(cm, entities.begin(), entities.end());
            }

            Iterator end() {
                const auto& entities = cm->getEntitiesWithComponent<std::tuple_element_t<0, std::tuple<Components...>>>();
                return Iterator(cm, entities.end(), entities.end());
            }

            template<typename Func>
            void each(Func&& func) {
                using Primary = std::tuple_element_t<0, std::tuple<Components...>>;
                const auto& entities = cm->getEntitiesWithComponent<Primary>();

                for (Entity entity : entities) {
                    if ((cm->hasComponent<Components>(entity) && ...)) {
                        func(entity, cm->getComponent<Components>(entity)...);
                    }
                }
            }
        };

        class ECS;

        class System {
        protected:
            ECS* ecs{};

        public:
            virtual ~System() = default;
            virtual void setEcs(ECS* _ecs) {};
            virtual void update(float deltaTime) = 0;
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
                return componentManager.addComponent<T>(entity, std::forward<Args>(args)...);
            }

            template<typename T>
            bool hasComponent(Entity entity) {
                return componentManager.hasComponent<T>(entity);
            }

            template<typename T>
            T& getComponent(Entity entity) {
                return componentManager.getComponent<T>(entity);
            }

            template<typename T>
            void removeComponent(Entity entity) {
                componentManager.removeComponent<T>(entity);
            }

            Entity createEntity() {
                return entityManager.create();
            }

            void destroyEntity(Entity entity) {
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
            S& registerSystem(Args&& ...args) {
                auto system = std::make_unique<S>(std::forward<Args>(args)...);
                system->setEcs(this);
                S& ref = *system;
                systems.emplace_back(std::move(system));
                return ref;
            }

            void update(float deltaTime) {
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
