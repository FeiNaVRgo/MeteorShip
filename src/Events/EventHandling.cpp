//
// Created by iformat on 6/10/26.
//

#include "EventHandling.h"
#include "Game.h"


void ms::utility::EventHandling::Visitor::handle(sf::Event::Closed const& event) {
    game.getMainWindow().close();
}
void ms::utility::EventHandling::Visitor::handle(sf::Event::Resized const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::FocusLost const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::FocusGained const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::TextEntered const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::KeyPressed const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::KeyReleased const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::MouseWheelScrolled const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::MouseButtonPressed const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::MouseButtonReleased const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::MouseMoved const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::MouseMovedRaw const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::MouseEntered const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::MouseLeft const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::JoystickButtonPressed const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::JoystickButtonReleased const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::JoystickMoved const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::JoystickConnected const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::JoystickDisconnected const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::TouchBegan const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::TouchMoved const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::TouchEnded const &event) {}
void ms::utility::EventHandling::Visitor::handle(sf::Event::SensorChanged const &event) {}
void ms::utility::EventHandling::Visitor::handle(Event::RemoveEntity const &event) {}
