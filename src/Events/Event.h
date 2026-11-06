//
// Created by iformat on 6/15/26.
//

#ifndef METEORSHIP_EVENT_H
#define METEORSHIP_EVENT_H

#include <variant>
#include "ECS/ECS.h"

namespace ms {
    class Event {
    public:
        struct RemoveEntity {
            ecs::Entity entity;
        };

        struct CollisionDetected {
            ecs::Entity first;
            ecs::Entity second;
        };

        using events_t = std::variant<RemoveEntity, CollisionDetected>;
        events_t event_data;

        template<typename TEventSubVariant> //I probably should check if the data type is in variant
        constexpr Event(TEventSubVariant data) : event_data(data) {}

        template <typename Visitor>
        decltype(auto) visit(Visitor&& visitor) const {
            return std::visit(std::forward<Visitor>(visitor), event_data);
        }
    };
}

#endif //METEORSHIP_EVENT_H
