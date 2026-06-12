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

            void operator()(sf::Event::Closed const& event);
            void operator()(sf::Event::Resized const& event);
            void operator()(sf::Event::FocusLost const& event);
            void operator()(sf::Event::FocusGained const& event);
            void operator()(sf::Event::TextEntered const& event);
            void operator()(sf::Event::KeyPressed const& event);
            void operator()(sf::Event::KeyReleased const& event);
            void operator()(sf::Event::MouseWheelScrolled const& event);
            void operator()(sf::Event::MouseButtonPressed const& event);
            void operator()(sf::Event::MouseButtonReleased const& event);
            void operator()(sf::Event::MouseMoved const& event);
            void operator()(sf::Event::MouseMovedRaw const& event);
            void operator()(sf::Event::MouseEntered const& event);
            void operator()(sf::Event::MouseLeft const& event);
            void operator()(sf::Event::JoystickButtonPressed const& event);
            void operator()(sf::Event::JoystickButtonReleased const& event);
            void operator()(sf::Event::JoystickMoved const& event);
            void operator()(sf::Event::JoystickConnected const& event);
            void operator()(sf::Event::JoystickDisconnected const& event);
            void operator()(sf::Event::TouchBegan const& event);
            void operator()(sf::Event::TouchMoved const& event);
            void operator()(sf::Event::TouchEnded const& event);
            void operator()(sf::Event::SensorChanged const& event);

            template<typename T>
            void operator()(T const& event) {}

        } visitor;

    public:
        explicit EventHandling(Game& game) : game(game), visitor(eventSignals, game) {
            eventSignals.emplace(typeid(sf::Event::Closed).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::Resized).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::FocusLost).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::FocusGained).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::TextEntered).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::KeyPressed).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::KeyReleased).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::MouseWheelScrolled).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::MouseButtonPressed).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::MouseButtonReleased).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::MouseMoved).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::MouseMovedRaw).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::MouseEntered).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::MouseLeft).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::JoystickButtonPressed).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::JoystickButtonReleased).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::JoystickMoved).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::JoystickConnected).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::JoystickDisconnected).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::TouchBegan).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::TouchMoved).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::TouchEnded).name(), Signal{});
            eventSignals.emplace(typeid(sf::Event::SensorChanged).name(), Signal{});
        }

        template<typename T, typename Func>
        decltype(auto) bindToSignal(Func&& func) {
            return eventSignals.at(typeid(T).name()).connect(std::forward<Func>(func));
        }

        void handleEvent(auto& event) {
            event->visit(visitor);
        }

    };
}

#endif //METEORSHIP_EVENTHANDLING_H
