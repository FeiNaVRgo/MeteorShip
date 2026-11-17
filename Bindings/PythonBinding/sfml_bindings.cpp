//
// Created by iformat on 10/20/26.
//

#include "sfml_bindings.h"
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/Graphics/Rect.hpp>

namespace py = pybind11;

void bind_sfml(py::module_& m) {
    py::class_<sf::Vector2f, py::smart_holder>(m, "Vector2f")
        .def(py::init<>())
        .def(py::init<float, float>(), py::arg("x"), py::arg("y"))
        .def_readwrite("x", &sf::Vector2f::x)
        .def_readwrite("y", &sf::Vector2f::y)
        .def("__repr__", [](const sf::Vector2f& v) {
            return "<Vector2f(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")>";
        });

    py::class_<sf::FloatRect, py::smart_holder>(m, "FloatRect")
        .def(py::init<>())
        .def(py::init<sf::Vector2f, sf::Vector2f>(),
             py::arg("position"), py::arg("size"))
        .def_readwrite("position",   &sf::FloatRect::position)
        .def_readwrite("size",    &sf::FloatRect::size)

        .def("__repr__", [](const sf::FloatRect& r) {
            return "<FloatRect(" + std::to_string(r.position.x) + ", " +
                   std::to_string(r.position.y) + ", " + std::to_string(r.size.x) +
                   ", " + std::to_string(r.size.y) + ")>";
        });

    py::class_<sf::Angle, py::smart_holder>(m, "Angle")
        .def(py::init<>())
        .def("asDegrees", &sf::Angle::asDegrees)
        .def("asRadians", &sf::Angle::asRadians);
}