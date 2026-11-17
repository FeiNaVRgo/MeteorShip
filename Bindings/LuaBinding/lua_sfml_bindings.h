#ifndef METEORSHIP_LUA_SFML_BINDINGS_H
#define METEORSHIP_LUA_SFML_BINDINGS_H

#include <sol/sol.hpp>

#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ms::scripting {
    template<typename Vector, typename Scalar>
    void bind_vector2(sol::state& lua, const char* name) {
        lua.new_usertype<Vector>(name,
            sol::constructors<sol::types<>, sol::types<Scalar, Scalar>>(),
            "x", &Vector::x,
            "y", &Vector::y,
            "__add", [](const Vector& lhs, const Vector& rhs) { return lhs + rhs; },
            "__sub", [](const Vector& lhs, const Vector& rhs) { return lhs - rhs; },
            "__mul", [](const Vector& value, Scalar scale) { return value * scale; },
            "__div", [](const Vector& value, Scalar scale) { return value / scale; });
    }

    inline void lua_sfml_bind(sol::state& lua) {
        using namespace sf;
        using Event = sf::Event;

        bind_vector2<Vector2f, float>(lua, "Vector2f");
        lua["Vector2f"]["length"] = [](const Vector2f& value) { return value.length(); };
        lua["Vector2f"]["normalized"] = [](const Vector2f& value) { return value.normalized(); };
        bind_vector2<Vector2i, int>(lua, "Vector2i");
        bind_vector2<Vector2u, unsigned int>(lua, "Vector2u");
        lua.new_usertype<Vector3f>("Vector3f",
            sol::constructors<sol::types<>, sol::types<float, float, float>>(),
            "x", &Vector3f::x,
            "y", &Vector3f::y,
            "z", &Vector3f::z);

        lua.new_usertype<FloatRect>("FloatRect",
            sol::constructors<sol::types<>, sol::types<Vector2f, Vector2f>>(),
            "position", &FloatRect::position,
            "size", &FloatRect::size,
            "find_intersection", [](const FloatRect& lhs, const FloatRect& rhs) {
                return lhs.findIntersection(rhs);
            });
        lua.new_usertype<IntRect>("IntRect",
            sol::constructors<sol::types<>, sol::types<Vector2i, Vector2i>>(),
            "position", &IntRect::position,
            "size", &IntRect::size,
            "find_intersection", [](const IntRect& lhs, const IntRect& rhs) {
                return lhs.findIntersection(rhs);
            });

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

        lua.new_usertype<Color>("Color",
            sol::constructors<sol::types<>, sol::types<std::uint8_t, std::uint8_t, std::uint8_t>,
                              sol::types<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>>(),
            "r", &Color::r,
            "g", &Color::g,
            "b", &Color::b,
            "a", &Color::a);
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

        lua.new_enum("PrimitiveType",
            "Points", PrimitiveType::Points,
            "Lines", PrimitiveType::Lines,
            "LineStrip", PrimitiveType::LineStrip,
            "Triangles", PrimitiveType::Triangles,
            "TriangleStrip", PrimitiveType::TriangleStrip,
            "TriangleFan", PrimitiveType::TriangleFan);
        lua.new_usertype<Vertex>("Vertex",
            sol::constructors<sol::types<>, sol::types<Vector2f>, sol::types<Vector2f, Color>>(),
            "position", &Vertex::position,
            "color", &Vertex::color,
            "tex_coords", &Vertex::texCoords);
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

        lua.new_enum("KeyboardKey",
            "Unknown", Keyboard::Key::Unknown,
            "A", Keyboard::Key::A, "B", Keyboard::Key::B, "C", Keyboard::Key::C,
            "D", Keyboard::Key::D, "E", Keyboard::Key::E, "F", Keyboard::Key::F,
            "G", Keyboard::Key::G, "H", Keyboard::Key::H, "I", Keyboard::Key::I,
            "J", Keyboard::Key::J, "K", Keyboard::Key::K, "L", Keyboard::Key::L,
            "M", Keyboard::Key::M, "N", Keyboard::Key::N, "O", Keyboard::Key::O,
            "P", Keyboard::Key::P, "Q", Keyboard::Key::Q, "R", Keyboard::Key::R,
            "S", Keyboard::Key::S, "T", Keyboard::Key::T, "U", Keyboard::Key::U,
            "V", Keyboard::Key::V, "W", Keyboard::Key::W, "X", Keyboard::Key::X,
            "Y", Keyboard::Key::Y, "Z", Keyboard::Key::Z,
            "Num0", Keyboard::Key::Num0, "Num1", Keyboard::Key::Num1,
            "Num2", Keyboard::Key::Num2, "Num3", Keyboard::Key::Num3,
            "Num4", Keyboard::Key::Num4, "Num5", Keyboard::Key::Num5,
            "Num6", Keyboard::Key::Num6, "Num7", Keyboard::Key::Num7,
            "Num8", Keyboard::Key::Num8, "Num9", Keyboard::Key::Num9,
            "Escape", Keyboard::Key::Escape, "LControl", Keyboard::Key::LControl,
            "LShift", Keyboard::Key::LShift, "LAlt", Keyboard::Key::LAlt,
            "LSystem", Keyboard::Key::LSystem, "RControl", Keyboard::Key::RControl,
            "RShift", Keyboard::Key::RShift, "RAlt", Keyboard::Key::RAlt,
            "RSystem", Keyboard::Key::RSystem, "Menu", Keyboard::Key::Menu,
            "LBracket", Keyboard::Key::LBracket, "RBracket", Keyboard::Key::RBracket,
            "Semicolon", Keyboard::Key::Semicolon, "Comma", Keyboard::Key::Comma,
            "Period", Keyboard::Key::Period, "Apostrophe", Keyboard::Key::Apostrophe,
            "Slash", Keyboard::Key::Slash, "Backslash", Keyboard::Key::Backslash,
            "Grave", Keyboard::Key::Grave, "Equal", Keyboard::Key::Equal,
            "Hyphen", Keyboard::Key::Hyphen, "Space", Keyboard::Key::Space,
            "Enter", Keyboard::Key::Enter, "Tab", Keyboard::Key::Tab,
            "Backspace", Keyboard::Key::Backspace, "PageUp", Keyboard::Key::PageUp,
            "PageDown", Keyboard::Key::PageDown, "End", Keyboard::Key::End,
            "Home", Keyboard::Key::Home, "Insert", Keyboard::Key::Insert,
            "Delete", Keyboard::Key::Delete, "Add", Keyboard::Key::Add,
            "Subtract", Keyboard::Key::Subtract, "Multiply", Keyboard::Key::Multiply,
            "Divide", Keyboard::Key::Divide,
            "Left", Keyboard::Key::Left, "Right", Keyboard::Key::Right,
            "Up", Keyboard::Key::Up, "Down", Keyboard::Key::Down,
            "Numpad0", Keyboard::Key::Numpad0, "Numpad1", Keyboard::Key::Numpad1,
            "Numpad2", Keyboard::Key::Numpad2, "Numpad3", Keyboard::Key::Numpad3,
            "Numpad4", Keyboard::Key::Numpad4, "Numpad5", Keyboard::Key::Numpad5,
            "Numpad6", Keyboard::Key::Numpad6, "Numpad7", Keyboard::Key::Numpad7,
            "Numpad8", Keyboard::Key::Numpad8, "Numpad9", Keyboard::Key::Numpad9,
            "F1", Keyboard::Key::F1, "F2", Keyboard::Key::F2,
            "F3", Keyboard::Key::F3, "F4", Keyboard::Key::F4,
            "F5", Keyboard::Key::F5, "F6", Keyboard::Key::F6,
            "F7", Keyboard::Key::F7, "F8", Keyboard::Key::F8,
            "F9", Keyboard::Key::F9, "F10", Keyboard::Key::F10,
            "F11", Keyboard::Key::F11, "F12", Keyboard::Key::F12,
            "F13", Keyboard::Key::F13, "F14", Keyboard::Key::F14,
            "F15", Keyboard::Key::F15, "Pause", Keyboard::Key::Pause);
        lua.new_enum("MouseButton",
            "Left", Mouse::Button::Left,
            "Right", Mouse::Button::Right,
            "Middle", Mouse::Button::Middle,
            "Extra1", Mouse::Button::Extra1,
            "Extra2", Mouse::Button::Extra2);
        lua.new_enum("MouseWheel", "Vertical", Mouse::Wheel::Vertical, "Horizontal", Mouse::Wheel::Horizontal);
        lua.new_enum("JoystickAxis",
            "X", Joystick::Axis::X, "Y", Joystick::Axis::Y, "Z", Joystick::Axis::Z,
            "R", Joystick::Axis::R, "U", Joystick::Axis::U, "V", Joystick::Axis::V,
            "PovX", Joystick::Axis::PovX, "PovY", Joystick::Axis::PovY);
        lua.new_enum("SensorType",
            "Accelerometer", Sensor::Type::Accelerometer,
            "Gyroscope", Sensor::Type::Gyroscope,
            "Magnetometer", Sensor::Type::Magnetometer,
            "Gravity", Sensor::Type::Gravity,
            "UserAcceleration", Sensor::Type::UserAcceleration,
            "Orientation", Sensor::Type::Orientation);
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
        const auto bind_empty_event = [&lua, &events](const char* alias, const char* typeName, auto typeTag) {
            using T = decltype(typeTag);
            lua.new_usertype<T>(typeName, sol::no_constructor);
            events[alias] = lua.get<sol::object>(typeName);
        };
        bind_empty_event("Closed", "ClosedEvent", Event::Closed{});
        bind_empty_event("FocusLost", "FocusLostEvent", Event::FocusLost{});
        bind_empty_event("FocusGained", "FocusGainedEvent", Event::FocusGained{});
        bind_empty_event("MouseEntered", "MouseEnteredEvent", Event::MouseEntered{});
        bind_empty_event("MouseLeft", "MouseLeftEvent", Event::MouseLeft{});

        lua.new_usertype<Event::Resized>("ResizedEvent", sol::no_constructor, "size", &Event::Resized::size);
        lua.new_usertype<Event::TextEntered>("TextEnteredEvent", sol::no_constructor, "unicode", &Event::TextEntered::unicode);
        lua.new_usertype<Event::KeyPressed>("KeyPressedEvent", sol::no_constructor,
            "code", &Event::KeyPressed::code,
            "scancode", [](const Event::KeyPressed& event) { return static_cast<int>(event.scancode); },
            "alt", &Event::KeyPressed::alt,
            "control", &Event::KeyPressed::control,
            "shift", &Event::KeyPressed::shift,
            "system", &Event::KeyPressed::system);
        lua.new_usertype<Event::KeyReleased>("KeyReleasedEvent", sol::no_constructor,
            "code", &Event::KeyReleased::code,
            "scancode", [](const Event::KeyReleased& event) { return static_cast<int>(event.scancode); },
            "alt", &Event::KeyReleased::alt,
            "control", &Event::KeyReleased::control,
            "shift", &Event::KeyReleased::shift,
            "system", &Event::KeyReleased::system);
        lua.new_usertype<Event::MouseWheelScrolled>("MouseWheelScrolledEvent", sol::no_constructor,
            "wheel", &Event::MouseWheelScrolled::wheel,
            "delta", &Event::MouseWheelScrolled::delta,
            "position", &Event::MouseWheelScrolled::position);
        lua.new_usertype<Event::MouseButtonPressed>("MouseButtonPressedEvent", sol::no_constructor,
            "button", &Event::MouseButtonPressed::button, "position", &Event::MouseButtonPressed::position);
        lua.new_usertype<Event::MouseButtonReleased>("MouseButtonReleasedEvent", sol::no_constructor,
            "button", &Event::MouseButtonReleased::button, "position", &Event::MouseButtonReleased::position);
        lua.new_usertype<Event::MouseMoved>("MouseMovedEvent", sol::no_constructor, "position", &Event::MouseMoved::position);
        lua.new_usertype<Event::MouseMovedRaw>("MouseMovedRawEvent", sol::no_constructor, "delta", &Event::MouseMovedRaw::delta);
        lua.new_usertype<Event::JoystickButtonPressed>("JoystickButtonPressedEvent", sol::no_constructor,
            "joystick_id", &Event::JoystickButtonPressed::joystickId, "button", &Event::JoystickButtonPressed::button);
        lua.new_usertype<Event::JoystickButtonReleased>("JoystickButtonReleasedEvent", sol::no_constructor,
            "joystick_id", &Event::JoystickButtonReleased::joystickId, "button", &Event::JoystickButtonReleased::button);
        lua.new_usertype<Event::JoystickMoved>("JoystickMovedEvent", sol::no_constructor,
            "joystick_id", &Event::JoystickMoved::joystickId, "axis", &Event::JoystickMoved::axis,
            "position", &Event::JoystickMoved::position);
        lua.new_usertype<Event::JoystickConnected>("JoystickConnectedEvent", sol::no_constructor,
            "joystick_id", &Event::JoystickConnected::joystickId);
        lua.new_usertype<Event::JoystickDisconnected>("JoystickDisconnectedEvent", sol::no_constructor,
            "joystick_id", &Event::JoystickDisconnected::joystickId);
        lua.new_usertype<Event::TouchBegan>("TouchBeganEvent", sol::no_constructor,
            "finger", &Event::TouchBegan::finger, "position", &Event::TouchBegan::position);
        lua.new_usertype<Event::TouchMoved>("TouchMovedEvent", sol::no_constructor,
            "finger", &Event::TouchMoved::finger, "position", &Event::TouchMoved::position);
        lua.new_usertype<Event::TouchEnded>("TouchEndedEvent", sol::no_constructor,
            "finger", &Event::TouchEnded::finger, "position", &Event::TouchEnded::position);
        lua.new_usertype<Event::SensorChanged>("SensorChangedEvent", sol::no_constructor,
            "type", &Event::SensorChanged::type, "value", &Event::SensorChanged::value);

        events["Resized"] = lua.get<sol::object>("ResizedEvent");
        events["TextEntered"] = lua.get<sol::object>("TextEnteredEvent");
        events["KeyPressed"] = lua.get<sol::object>("KeyPressedEvent");
        events["KeyReleased"] = lua.get<sol::object>("KeyReleasedEvent");
        events["MouseWheelScrolled"] = lua.get<sol::object>("MouseWheelScrolledEvent");
        events["MouseButtonPressed"] = lua.get<sol::object>("MouseButtonPressedEvent");
        events["MouseButtonReleased"] = lua.get<sol::object>("MouseButtonReleasedEvent");
        events["MouseMoved"] = lua.get<sol::object>("MouseMovedEvent");
        events["MouseMovedRaw"] = lua.get<sol::object>("MouseMovedRawEvent");
        events["JoystickButtonPressed"] = lua.get<sol::object>("JoystickButtonPressedEvent");
        events["JoystickButtonReleased"] = lua.get<sol::object>("JoystickButtonReleasedEvent");
        events["JoystickMoved"] = lua.get<sol::object>("JoystickMovedEvent");
        events["JoystickConnected"] = lua.get<sol::object>("JoystickConnectedEvent");
        events["JoystickDisconnected"] = lua.get<sol::object>("JoystickDisconnectedEvent");
        events["TouchBegan"] = lua.get<sol::object>("TouchBeganEvent");
        events["TouchMoved"] = lua.get<sol::object>("TouchMovedEvent");
        events["TouchEnded"] = lua.get<sol::object>("TouchEndedEvent");
        events["SensorChanged"] = lua.get<sol::object>("SensorChangedEvent");

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
