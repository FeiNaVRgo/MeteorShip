//
// Created by iformat on 6/15/26.
//

#ifndef METEORSHIP_EVENTDISPATCHER_H
#define METEORSHIP_EVENTDISPATCHER_H
#include <queue>

#include "Event.h"

namespace ms {
    class EventDispatcher {
    private:
        //optional is only to match sfml way of handling events
        //bc sfml can push an event to queue that's not initialized
        //IDK why It's that
        //"these two functions might return an event, but they might not"
        //this is what sfml tutorial says about sf::WindowBase::pollEvent
        std::queue<std::optional<Event>> events{};
    public:
        [[nodiscard]] constexpr std::optional<Event> poll() {
            //I think we need to copy the event?
            //to not invalidate the reference or smth
            if (events.empty()) return std::nullopt;

            const auto event = events.front();
            events.pop();
            return event;
        }

        constexpr void push(std::optional<Event> const event) {
            events.push(event);
        }

        [[nodiscard]] constexpr bool empty() const {
            return events.empty();
        }
    };
}

#endif //METEORSHIP_EVENTDISPATCHER_H
