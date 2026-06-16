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
    private:
        using events_t = std::variant<RemoveEntity>;
        events_t event_data;
    public:
        template<typename TEventSubVariant> //I probably should check if the data type is in variant
        constexpr Event(TEventSubVariant data) : event_data(data) {}

        template <typename Visitor>
        decltype(auto) visit(Visitor&& visitor) const {
            return std::visit(visitor, event_data);
        }
    };
}

#endif //METEORSHIP_EVENT_H
