//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_TRANSFORM_H
#define METEORSHIP_TRANSFORM_H

#include <SFML/Graphics/Transformable.hpp>
#include "Utility/SaveState/SaveState.h"

namespace ms::components {
    struct Transform : public sf::Transformable {};
}

namespace ms::utility::savestate {
    inline boost::json::value tag_invoke(to_json_tag, const components::Transform& t) {
        return boost::json::object{
            {"position", encode(t.getPosition())},
            {"rotation", t.getRotation().asDegrees()},
            {"scale",    encode(t.getScale())},
            {"origin",   encode(t.getOrigin())},
        };
    }

    inline components::Transform tag_invoke(from_json_tag, std::type_identity<components::Transform>,
                                        const boost::json::value& jv) {
        components::Transform t;
        t.setPosition(decode_field<sf::Vector2f>(jv, "position"));
        t.setRotation(sf::degrees(decode_field<float>(jv, "rotation")));
        t.setScale(decode_field<sf::Vector2f>(jv, "scale"));
        t.setOrigin(decode_field<sf::Vector2f>(jv, "origin"));
        return t;
    }
}

#endif //METEORSHIP_TRANSFORM_H
