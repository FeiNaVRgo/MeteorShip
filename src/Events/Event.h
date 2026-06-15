//
// Created by iformat on 6/15/26.
//

#ifndef METEORSHIP_EVENT_H
#define METEORSHIP_EVENT_H
#include <variant>

namespace ms {
    class Event {
    public:
        //here i can declare specific events
        struct Something {};
    private:
        using events_t = std::variant<Something>;
        events_t event_data;
    public:
        template<typename TEventSubVariant> //I probably should check if the data type is in variant
        constexpr Event(TEventSubVariant data) : event_data(data) {}

        template <typename Visitor>
        decltype(auto) visit(Visitor&& visitor) {
            return std::visit(visitor, event_data);
        }
    };
}

#endif //METEORSHIP_EVENT_H
