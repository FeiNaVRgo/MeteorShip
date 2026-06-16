//
// Created by iformat on 6/10/26.
//

#include "EventHandling.h"
#include "Game.h"


void ms::utility::EventHandling::Visitor::operator()(sf::Event::Closed const& event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
    game.getMainWindow().close();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::Resized const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::FocusLost const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::FocusGained const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::TextEntered const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::KeyPressed const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::KeyReleased const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::MouseWheelScrolled const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::MouseButtonPressed const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::MouseButtonReleased const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::MouseMoved const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::MouseMovedRaw const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::MouseEntered const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::MouseLeft const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::JoystickButtonPressed const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::JoystickButtonReleased const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::JoystickMoved const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::JoystickConnected const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::JoystickDisconnected const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::TouchBegan const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::TouchMoved const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::TouchEnded const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}

void ms::utility::EventHandling::Visitor::operator()(sf::Event::SensorChanged const &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
}
