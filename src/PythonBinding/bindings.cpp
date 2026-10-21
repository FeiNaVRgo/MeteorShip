//
// Created by iformat on 10/20/26.
//

#include <pybind11/pybind11.h>
#include "sfml_bindings.h"

#include "Components/Transform.h"
#include "Components/Rigidbody.h"

namespace py = pybind11;

void bind_components(py::module_& m) {
    using namespace ms::components;

    py::class_<Transform>(m, "Transform")
        .def(py::init<>())
        .def_property("position",
            [](const Transform& t) { return t.getPosition(); },
            [](Transform& t, const sf::Vector2f& v) { t.setPosition(v); })
        .def_property("rotation",
            [](const Transform& t) { return t.getRotation().asDegrees(); },
            [](Transform& t, float deg) { t.setRotation(sf::degrees(deg)); })
        .def_property("scale",
            [](const Transform& t) { return t.getScale(); },
            [](Transform& t, const sf::Vector2f& v) { t.setScale(v); })
        .def_property("origin",
            [](const Transform& t) { return t.getOrigin(); },
            [](Transform& t, const sf::Vector2f& v) { t.setOrigin(v); });

    py::class_<Rigidbody>(m, "Rigidbody")
        .def(py::init<>())
        .def_readwrite("velocity", &Rigidbody::velocity)
        .def_readwrite("acceleration", &Rigidbody::acceleration)
        .def_readwrite("bounds", &Rigidbody::bounds);
}

PYBIND11_MODULE(CPPModule, m) {
    bind_sfml(m);
    bind_components(m);
}