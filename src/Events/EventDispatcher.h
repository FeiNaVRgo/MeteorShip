//
// Created by iformat on 6/15/26.
//

#ifndef METEORSHIP_EVENTDISPATCHER_H
#define METEORSHIP_EVENTDISPATCHER_H
#include <queue>
#include <utility>

#include "Event.h"

namespace ms {
    class EventDispatcher {
    private:
        std::queue<Event> events{};
    public:
        [[nodiscard]] constexpr std::optional<Event> poll() {
            if (events.empty()) return std::nullopt;

            auto event = std::move(events.front());
            events.pop();
            return event;
        }

        constexpr void push(Event event) {
            events.push(std::move(event));
        }

        [[nodiscard]] constexpr bool empty() const {
            return events.empty();
        }
    };
}

#endif //METEORSHIP_EVENTDISPATCHER_H
