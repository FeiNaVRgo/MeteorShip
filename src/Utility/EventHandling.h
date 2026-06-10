//
// Created by iformat on 6/10/26.
//

#ifndef METEORSHIP_EVENTHANDLING_H
#define METEORSHIP_EVENTHANDLING_H

#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <boost/signals2/signal.hpp>
#include <SFML/Window/Event.hpp>

namespace ms {
    class Game;
}

namespace ms::utility {
    class EventHandling {
    private:
        using Signature     = void();
        using Signal        = boost::signals2::signal<Signature>;
        using EventSignals  = std::unordered_map<const char*, Signal>;

        Game&           game;
        EventSignals    eventSignals{};

        struct Visitor {
            EventSignals& eventSignals;
            Game& game;

            explicit Visitor(EventSignals& eventSignals, Game& game) : eventSignals(eventSignals), game(game) {}

            template<typename T>
            [[nodiscard]] Signal& getEventSignal() const {
                return eventSignals.at(typeid(T).name());
            }

            //itd
            void operator()(sf::Event::Closed &event);
            void operator()(sf::Event::KeyPressed& event) {
                std::cout << "Key pressed: " << static_cast<int>(event.scancode) <<  std::endl;
            }


            template<typename T>
            void operator()(T const& event) {}
        } visitor;

    public:
        explicit EventHandling(Game& game) : game(game), visitor(eventSignals, game) {
            eventSignals.emplace(typeid(sf::Event::Closed).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::KeyPressed).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::KeyReleased).name(), Signal{});
        }

        template<typename T, typename Func>
        auto& bindToSignal(Func&& func) {
            return eventSignals.at(typeid(T).name()).connect(std::forward<Func>(func));
        }


        void handleEvent(auto& event) {
            event->visit(visitor);
        }

    };
}

#endif //METEORSHIP_EVENTHANDLING_H
