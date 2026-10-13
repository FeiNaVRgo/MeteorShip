//
// Created by iformat on 6/10/26.
//

#ifndef METEORSHIP_EVENTHANDLING_H
#define METEORSHIP_EVENTHANDLING_H

#include <typeindex>
#include <boost/signals2/signal.hpp>
#include <SFML/Window/Event.hpp>
#include <unordered_map>

#include "Event.h"

namespace ms {
    class Game;
}

namespace ms::utility {
    class EventHandling {
    private:
        using Signature     = void(void*);
        using Signal        = boost::signals2::signal<Signature>;
        using EventSignals  = std::unordered_map<std::type_index, Signal>;

        using SFMLEvents    = std::variant<sf::Event::Closed,
                                           sf::Event::Resized,
                                           sf::Event::FocusLost,
                                           sf::Event::FocusGained,
                                           sf::Event::TextEntered,
                                           sf::Event::KeyPressed,
                                           sf::Event::KeyReleased,
                                           sf::Event::MouseWheelScrolled,
                                           sf::Event::MouseButtonPressed,
                                           sf::Event::MouseButtonReleased,
                                           sf::Event::MouseMoved,
                                           sf::Event::MouseMovedRaw,
                                           sf::Event::MouseEntered,
                                           sf::Event::MouseLeft,
                                           sf::Event::JoystickButtonPressed,
                                           sf::Event::JoystickButtonReleased,
                                           sf::Event::JoystickMoved,
                                           sf::Event::JoystickConnected,
                                           sf::Event::JoystickDisconnected,
                                           sf::Event::TouchBegan,
                                           sf::Event::TouchMoved,
                                           sf::Event::TouchEnded,
                                           sf::Event::SensorChanged>;

        EventSignals    eventSignals{};

        struct Visitor {
            EventSignals&   eventSignals;
            Game&           game;

            explicit Visitor(EventSignals& eventSignals, Game& _game) : eventSignals(eventSignals), game(_game) {}

            template<typename T>
            [[nodiscard]] std::optional<std::reference_wrapper<Signal>> getEventSignal() const {
                if (!eventSignals.contains(typeid(T))) return std::nullopt;
                return eventSignals.at(typeid(T));
            }

            //sfml events
            void handle(sf::Event::Closed const& event);
            void handle(sf::Event::Resized const& event);
            void handle(sf::Event::FocusLost const& event);
            void handle(sf::Event::FocusGained const& event);
            void handle(sf::Event::TextEntered const& event);
            void handle(sf::Event::KeyPressed const& event);
            void handle(sf::Event::KeyReleased const& event);
            void handle(sf::Event::MouseWheelScrolled const& event);
            void handle(sf::Event::MouseButtonPressed const& event);
            void handle(sf::Event::MouseButtonReleased const& event);
            void handle(sf::Event::MouseMoved const& event);
            void handle(sf::Event::MouseMovedRaw const& event);
            void handle(sf::Event::MouseEntered const& event);
            void handle(sf::Event::MouseLeft const& event);
            void handle(sf::Event::JoystickButtonPressed const& event);
            void handle(sf::Event::JoystickButtonReleased const& event);
            void handle(sf::Event::JoystickMoved const& event);
            void handle(sf::Event::JoystickConnected const& event);
            void handle(sf::Event::JoystickDisconnected const& event);
            void handle(sf::Event::TouchBegan const& event);
            void handle(sf::Event::TouchMoved const& event);
            void handle(sf::Event::TouchEnded const& event);
            void handle(sf::Event::SensorChanged const& event);

            //now in game events
            void handle(Event::RemoveEntity const& event);
            void handle(Event::PlayerShot const& event);

            template<typename T>//for all unhandled types
            void handle(T const& event) {}

            template<typename T>
            void operator()(T const& event) {
                if (auto opt_sig = getEventSignal<std::remove_all_extents_t<T>>(); opt_sig.has_value()) {
                    auto& sig = opt_sig.value().get();
                    sig((void*)(&event));
                }
                handle(event);
            }
        } visitor;

    public:
        explicit EventHandling(Game& game) : visitor(eventSignals, game) {
            initEventSignalsFromV<SFMLEvents>();//sfml events
            initEventSignalsFromV<Event::events_t>();//game events
        }

        template<typename T, typename Func>
        decltype(auto) bindToSignal(Func&& func) {
            return eventSignals.at(typeid(T)).connect(std::forward<Func>(func));
        }

        template<typename Event>
        decltype(auto) handleEvent(Event const& event) {
            return event->visit(std::move(visitor)); // for sfml events and normal events
        }
    private:
        template<typename V, std::size_t I = 0>
        constexpr void initEventSignalsFromV() {
            if constexpr (I < std::variant_size_v<V>) {
                eventSignals.emplace(typeid(std::variant_alternative_t<I, V>), Signal{});
                initEventSignalsFromV<V, I + 1>();
            }
        }
    };
}

#endif //METEORSHIP_EVENTHANDLING_H
