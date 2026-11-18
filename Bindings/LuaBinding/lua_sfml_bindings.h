#ifndef METEORSHIP_LUA_SFML_BINDINGS_H
#define METEORSHIP_LUA_SFML_BINDINGS_H

#include "sol_optional_compat.h"

#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "lua_reflection.h"

namespace ms::scripting {
    template<typename Vector, typename Scalar>
    void bind_vector2(sol::state& lua, const char* name) {
        auto vectorType = new_reflected_usertype<Vector>(lua, name,
            sol::constructors<sol::types<>, sol::types<Scalar, Scalar>>());
        vectorType["__add"] = [](const Vector& lhs, const Vector& rhs) { return lhs + rhs; };
        vectorType["__sub"] = [](const Vector& lhs, const Vector& rhs) { return lhs - rhs; };
        vectorType["__mul"] = [](const Vector& value, Scalar scale) { return value * scale; };
        vectorType["__div"] = [](const Vector& value, Scalar scale) { return value / scale; };
    }

    inline void lua_sfml_bind(sol::state& lua) {
        using namespace sf;
        using Event = sf::Event;

        bind_vector2<Vector2f, float>(lua, "Vector2f");
        lua["Vector2f"]["length"] = [](const Vector2f& value) { return value.length(); };
        lua["Vector2f"]["normalized"] = [](const Vector2f& value) { return value.normalized(); };
        bind_vector2<Vector2i, int>(lua, "Vector2i");
        bind_vector2<Vector2u, unsigned int>(lua, "Vector2u");
        new_reflected_usertype<Vector3f>(lua, "Vector3f",
            sol::constructors<sol::types<>, sol::types<float, float, float>>());

        auto floatRect = new_reflected_usertype<FloatRect>(lua, "FloatRect",
            sol::constructors<sol::types<>, sol::types<Vector2f, Vector2f>>());
        floatRect["find_intersection"] = [](const FloatRect& lhs, const FloatRect& rhs) {
            return lhs.findIntersection(rhs);
        };
        auto intRect = new_reflected_usertype<IntRect>(lua, "IntRect",
            sol::constructors<sol::types<>, sol::types<Vector2i, Vector2i>>());
        intRect["find_intersection"] = [](const IntRect& lhs, const IntRect& rhs) {
            return lhs.findIntersection(rhs);
        };

        lua.new_usertype<Angle>("Angle", sol::no_constructor,
            "as_degrees", &Angle::asDegrees,
            "as_radians", &Angle::asRadians);
        lua.new_usertype<Time>("Time", sol::no_constructor,
            "as_seconds", &Time::asSeconds,
            "as_milliseconds", &Time::asMilliseconds,
            "as_microseconds", &Time::asMicroseconds,
            "__add", [](Time lhs, Time rhs) { return lhs + rhs; },
            "__sub", [](Time lhs, Time rhs) { return lhs - rhs; });
        lua["Time"]["Zero"] = Time::Zero;
        lua.new_usertype<Clock>("Clock", sol::constructors<sol::types<>>(),
            "elapsed_time", &Clock::getElapsedTime,
            "restart", &Clock::restart);

        new_reflected_usertype<Color>(lua, "Color",
            sol::constructors<sol::types<>, sol::types<std::uint8_t, std::uint8_t, std::uint8_t>,
                              sol::types<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>>());
        lua["Color"]["Black"] = Color::Black;
        lua["Color"]["White"] = Color::White;
        lua["Color"]["Red"] = Color::Red;
        lua["Color"]["Green"] = Color::Green;
        lua["Color"]["Blue"] = Color::Blue;
        lua["Color"]["Transparent"] = Color::Transparent;

        lua["degrees"] = [](float value) { return sf::degrees(value); };
        lua["radians"] = [](float value) { return sf::radians(value); };
        lua["seconds"] = [](float value) { return sf::seconds(value); };
        lua["milliseconds"] = [](std::int32_t value) { return sf::milliseconds(value); };
        lua["microseconds"] = [](std::int64_t value) { return sf::microseconds(value); };

        lua.new_usertype<Transformable>("Transformable",
            sol::constructors<sol::types<>>(),
            "position", sol::property(
                [](const Transformable& value) { return value.getPosition(); },
                [](Transformable& value, const Vector2f& position) { value.setPosition(position); }),
            "rotation", sol::property(
                [](const Transformable& value) { return value.getRotation(); },
                [](Transformable& value, Angle angle) { value.setRotation(angle); }),
            "scale", sol::property(
                [](const Transformable& value) { return value.getScale(); },
                [](Transformable& value, const Vector2f& scale) { value.setScale(scale); }),
            "origin", sol::property(
                [](const Transformable& value) { return value.getOrigin(); },
                [](Transformable& value, const Vector2f& origin) { value.setOrigin(origin); }),
            "move", [](Transformable& value, const Vector2f& offset) { value.move(offset); },
            "rotate", [](Transformable& value, Angle angle) { value.rotate(angle); });

        lua.new_usertype<Texture>("Texture",
            sol::constructors<sol::types<>>(),
            "load_from_file", [](Texture& texture, const std::string& path) {
                return texture.loadFromFile(path);
            },
            "size", &Texture::getSize,
            "set_smooth", &Texture::setSmooth,
            "is_smooth", &Texture::isSmooth);

        lua.new_usertype<Drawable>("Drawable", sol::no_constructor);
        lua.new_usertype<RenderTarget>("RenderTarget", sol::no_constructor,
            "size", &RenderTarget::getSize);
        lua.new_usertype<Shape>("Shape", sol::no_constructor,
            sol::base_classes, sol::bases<Drawable, Transformable>(),
            "fill_color", sol::property(&Shape::getFillColor, &Shape::setFillColor),
            "outline_color", sol::property(&Shape::getOutlineColor, &Shape::setOutlineColor),
            "outline_thickness", sol::property(&Shape::getOutlineThickness, &Shape::setOutlineThickness));
        lua.new_usertype<RectangleShape>("RectangleShape",
            sol::constructors<sol::types<>, sol::types<Vector2f>>(),
            sol::base_classes, sol::bases<Shape>(),
            "size", sol::property(&RectangleShape::getSize, &RectangleShape::setSize));
        lua.new_usertype<CircleShape>("CircleShape",
            sol::constructors<sol::types<>, sol::types<float>, sol::types<float, std::size_t>>(),
            sol::base_classes, sol::bases<Shape>(),
            "radius", sol::property(&CircleShape::getRadius, &CircleShape::setRadius),
            "point_count", sol::property(&CircleShape::getPointCount, &CircleShape::setPointCount));
        lua.new_usertype<Sprite>("Sprite",
            sol::constructors<sol::types<const Texture&>, sol::types<const Texture&, const IntRect&>>(),
            sol::base_classes, sol::bases<Drawable, Transformable>(),
            "texture_rect", sol::property(&Sprite::getTextureRect, &Sprite::setTextureRect));

        bind_reflected_enum<PrimitiveType>(lua, "PrimitiveType");
        new_reflected_usertype_snake_case<Vertex>(lua, "Vertex",
            sol::constructors<sol::types<>, sol::types<Vector2f>, sol::types<Vector2f, Color>>());
        lua.new_usertype<VertexArray>("VertexArray",
            sol::constructors<sol::types<>, sol::types<PrimitiveType>,
                              sol::types<PrimitiveType, std::size_t>>(),
            "append", &VertexArray::append,
            "clear", &VertexArray::clear,
            "resize", &VertexArray::resize,
            "vertex_count", &VertexArray::getVertexCount,
            "primitive_type", sol::property(&VertexArray::getPrimitiveType, &VertexArray::setPrimitiveType),
            "get_vertex", [](VertexArray& vertices, std::size_t index) -> Vertex& {
                if (index >= vertices.getVertexCount()) throw std::out_of_range("vertex index out of range");
                return vertices[index];
            });

        lua.new_usertype<Window>("Window", sol::no_constructor,
            "is_open", &Window::isOpen,
            "close", &Window::close,
            "size", &Window::getSize,
            "position", sol::property(
                [](const Window& window) { return window.getPosition(); },
                [](Window& window, const Vector2i& position) { window.setPosition(position); }),
            "set_title", &Window::setTitle,
            "set_size", &Window::setSize,
            "set_framerate_limit", &Window::setFramerateLimit,
            "set_vertical_sync_enabled", &Window::setVerticalSyncEnabled);
        lua.new_usertype<RenderWindow>("RenderWindow", sol::no_constructor,
            sol::base_classes, sol::bases<Window, RenderTarget>(),
            "clear", sol::overload(
                [](RenderWindow& window) { window.clear(); },
                [](RenderWindow& window, const Color& color) { window.clear(color); }),
            "display", &RenderWindow::display,
            "draw", [](RenderWindow& window, const Drawable& drawable) { window.draw(drawable); });

        bind_reflected_enum<Keyboard::Key>(lua, "KeyboardKey");
        bind_reflected_enum<Mouse::Button>(lua, "MouseButton");
        bind_reflected_enum<Mouse::Wheel>(lua, "MouseWheel");
        bind_reflected_enum<Joystick::Axis>(lua, "JoystickAxis");
        bind_reflected_enum<Sensor::Type>(lua, "SensorType");
        auto keyboard = lua.create_named_table("Keyboard");
        keyboard["Key"] = lua.get<sol::object>("KeyboardKey");
        keyboard["is_key_pressed"] = [](Keyboard::Key key) { return Keyboard::isKeyPressed(key); };

        auto mouse = lua.create_named_table("Mouse");
        mouse["Button"] = lua.get<sol::object>("MouseButton");
        mouse["Wheel"] = lua.get<sol::object>("MouseWheel");
        mouse["is_button_pressed"] = [](Mouse::Button button) { return Mouse::isButtonPressed(button); };
        mouse["position"] = sol::overload(
                [](const Window& window) { return Mouse::getPosition(window); },
                []() { return Mouse::getPosition(); });
        mouse["set_position"] = sol::overload(
                [](const Vector2i& position, const Window& window) { Mouse::setPosition(position, window); },
                [](const Vector2i& position) { Mouse::setPosition(position); });

        auto joystick = lua.create_named_table("Joystick");
        joystick["Axis"] = lua.get<sol::object>("JoystickAxis");
        joystick["is_connected"] = [](unsigned int id) { return Joystick::isConnected(id); };
        joystick["button_count"] = [](unsigned int id) { return Joystick::getButtonCount(id); };
        joystick["has_axis"] = [](unsigned int id, Joystick::Axis axis) { return Joystick::hasAxis(id, axis); };
        joystick["axis_position"] = [](unsigned int id, Joystick::Axis axis) {
            return Joystick::getAxisPosition(id, axis);
        };

        auto sensor = lua.create_named_table("Sensor");
        sensor["Type"] = lua.get<sol::object>("SensorType");
        sensor["is_available"] = [](Sensor::Type type) { return Sensor::isAvailable(type); };
        sensor["get_value"] = [](Sensor::Type type) { return Sensor::getValue(type); };
        sensor["set_enabled"] = [](Sensor::Type type, bool enabled) { Sensor::setEnabled(type, enabled); };

        auto events = lua.create_named_table("sf_events");
        bind_reflected_events<
            Event::Closed, Event::FocusLost, Event::FocusGained, Event::MouseEntered, Event::MouseLeft,
            Event::Resized, Event::TextEntered, Event::KeyPressed, Event::KeyReleased,
            Event::MouseWheelScrolled, Event::MouseButtonPressed, Event::MouseButtonReleased,
            Event::MouseMoved, Event::MouseMovedRaw, Event::JoystickButtonPressed,
            Event::JoystickButtonReleased, Event::JoystickMoved, Event::JoystickConnected,
            Event::JoystickDisconnected, Event::TouchBegan, Event::TouchMoved, Event::TouchEnded,
            Event::SensorChanged>(lua, events);
        lua["KeyPressedEvent"]["scancode"] =
            [](const Event::KeyPressed& event) { return static_cast<int>(event.scancode); };
        lua["KeyReleasedEvent"]["scancode"] =
            [](const Event::KeyReleased& event) { return static_cast<int>(event.scancode); };

        auto sf = lua.create_named_table("sf");
        for (const char* name : {"Vector2f", "Vector2i", "Vector2u", "Vector3f", "FloatRect", "IntRect",
                                 "Angle", "Time", "Clock", "Color", "Texture", "Sprite", "Vertex", "VertexArray",
                                 "PrimitiveType", "Drawable", "Shape", "RenderTarget", "Transformable", "Window",
                                 "RectangleShape", "CircleShape", "RenderWindow"}) {
            sf[name] = lua.get<sol::object>(name);
        }
        sf["Keyboard"] = lua.get<sol::object>("Keyboard");
        sf["Mouse"] = lua.get<sol::object>("Mouse");
        sf["Joystick"] = lua.get<sol::object>("Joystick");
        sf["Sensor"] = lua.get<sol::object>("Sensor");
        sf["events"] = lua.get<sol::object>("sf_events");
        sf["degrees"] = lua.get<sol::object>("degrees");
        sf["radians"] = lua.get<sol::object>("radians");
        sf["seconds"] = lua.get<sol::object>("seconds");
        sf["milliseconds"] = lua.get<sol::object>("milliseconds");
        sf["microseconds"] = lua.get<sol::object>("microseconds");
    }
}

#endif // METEORSHIP_LUA_SFML_BINDINGS_H
