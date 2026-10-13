//
// Created by iformat on 10/8/26.
//

#ifndef METEORSHIP_EVENTSIGNALFACTORY_H
#define METEORSHIP_EVENTSIGNALFACTORY_H

#include "EventHandling.h"

namespace ms::utility {
    class EventSignalFactory {
    public:
        template<typename T, typename F>
        static decltype(auto) createEventSignal(EventHandling& event_handling, F&& func) {
            return event_handling.bindToSignal<T>([func](void* event) {
                func(*static_cast<T*>(event));
            });
        }
    };
}

#endif //METEORSHIP_EVENTSIGNALFACTORY_H
