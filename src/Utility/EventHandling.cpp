//
// Created by iformat on 6/10/26.
//

#include "EventHandling.h"

#include <iostream>
#include <ostream>

#include "Game.h"


void ms::utility::EventHandling::Visitor::operator()(sf::Event::Closed &event) {
    getEventSignal<std::remove_reference_t<decltype(event)>>()();
    std::cout << "should be called" << std::endl;
    game.getMainWindow().close();
}
