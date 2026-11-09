#ifndef METEORSHIP_COLLISIONGEOMETRY_H
#define METEORSHIP_COLLISIONGEOMETRY_H

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>

namespace ms::systems::collision_geometry {
    using Polygon = std::vector<sf::Vector2f>;

    struct Contact {
        sf::Vector2f normal;
        float penetration;
    };

    inline float cross(const sf::Vector2f a, const sf::Vector2f b) {
        return a.x * b.y - a.y * b.x;
    }

    inline float signedArea(const Polygon& polygon) {
        float area = 0.f;
        for (std::size_t i = 0; i < polygon.size(); ++i) {
            area += cross(polygon[i], polygon[(i + 1) % polygon.size()]);
        }
        return area / 2.f;
    }

    inline bool isConvex(const Polygon& polygon) {
        constexpr float epsilon = 1e-6f;
        if (polygon.size() < 3) {
            return false;
        }

        float orientation = 0.f;
        for (std::size_t i = 0; i < polygon.size(); ++i) {
            const auto first = polygon[(i + 1) % polygon.size()] - polygon[i];
            const auto second =
                polygon[(i + 2) % polygon.size()] - polygon[(i + 1) % polygon.size()];
            const float turn = cross(first, second);
            if (std::abs(turn) <= epsilon) {
                continue;
            }
            if (orientation == 0.f) {
                orientation = turn;
            } else if ((orientation > 0.f) != (turn > 0.f)) {
                return false;
            }
        }
        return orientation != 0.f;
    }

    inline bool pointInTriangle(
        const sf::Vector2f point,
        const sf::Vector2f a,
        const sf::Vector2f b,
        const sf::Vector2f c,
        const float orientation) {
        constexpr float epsilon = 1e-6f;
        return orientation * cross(b - a, point - a) >= -epsilon &&
               orientation * cross(c - b, point - b) >= -epsilon &&
               orientation * cross(a - c, point - c) >= -epsilon;
    }

    inline std::vector<Polygon> triangulate(const Polygon& polygon) {
        constexpr float epsilon = 1e-6f;
        Polygon vertices;
        vertices.reserve(polygon.size());
        for (const auto point : polygon) {
            if (vertices.empty()) {
                vertices.push_back(point);
                continue;
            }
            const auto delta = point - vertices.back();
            if (delta.x * delta.x + delta.y * delta.y > epsilon * epsilon) {
                vertices.push_back(point);
            }
        }
        if (vertices.size() > 1) {
            const auto closingDelta = vertices.front() - vertices.back();
            if (closingDelta.x * closingDelta.x + closingDelta.y * closingDelta.y <=
                epsilon * epsilon) {
                vertices.pop_back();
            }
        }

        bool removedCollinearVertex = true;
        while (removedCollinearVertex && vertices.size() > 3) {
            removedCollinearVertex = false;
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                const auto previous = vertices[(i + vertices.size() - 1) % vertices.size()];
                const auto current = vertices[i];
                const auto next = vertices[(i + 1) % vertices.size()];
                const auto incoming = current - previous;
                const auto outgoing = next - current;
                if (std::abs(cross(incoming, outgoing)) <= epsilon &&
                    incoming.x * outgoing.x + incoming.y * outgoing.y >= 0.f) {
                    vertices.erase(vertices.begin() + static_cast<std::ptrdiff_t>(i));
                    removedCollinearVertex = true;
                    break;
                }
            }
        }
        if (vertices.size() < 3 || std::abs(signedArea(vertices)) <= epsilon) {
            return {};
        }

        const float orientation = signedArea(vertices) > 0.f ? 1.f : -1.f;
        std::vector<std::size_t> indices(vertices.size());
        for (std::size_t i = 0; i < indices.size(); ++i) {
            indices[i] = i;
        }

        std::vector<Polygon> triangles;
        triangles.reserve(vertices.size() - 2);
        while (indices.size() > 3) {
            bool clippedEar = false;
            for (std::size_t i = 0; i < indices.size(); ++i) {
                const auto previous = indices[(i + indices.size() - 1) % indices.size()];
                const auto current = indices[i];
                const auto next = indices[(i + 1) % indices.size()];
                if (orientation * cross(
                        vertices[current] - vertices[previous],
                        vertices[next] - vertices[current]) <= epsilon) {
                    continue;
                }

                bool containsVertex = false;
                for (const auto candidate : indices) {
                    if (candidate == previous || candidate == current || candidate == next) {
                        continue;
                    }
                    if (pointInTriangle(
                            vertices[candidate], vertices[previous], vertices[current],
                            vertices[next], orientation)) {
                        containsVertex = true;
                        break;
                    }
                }
                if (containsVertex) {
                    continue;
                }

                triangles.push_back(
                    {vertices[previous], vertices[current], vertices[next]});
                indices.erase(indices.begin() + static_cast<std::ptrdiff_t>(i));
                clippedEar = true;
                break;
            }
            if (!clippedEar) {
                return {};
            }
        }

        triangles.push_back({
            vertices[indices[0]], vertices[indices[1]], vertices[indices[2]]
        });
        return triangles;
    }

    inline std::optional<Contact> intersectConvex(
        const Polygon& first,
        const Polygon& second) {
        constexpr float epsilon = 1e-6f;
        float minimumOverlap = std::numeric_limits<float>::max();
        sf::Vector2f minimumAxis{};

        const auto testAxes = [&](const Polygon& source) {
            for (std::size_t i = 0; i < source.size(); ++i) {
                const auto edge = source[(i + 1) % source.size()] - source[i];
                const float length = std::sqrt(edge.x * edge.x + edge.y * edge.y);
                if (length <= epsilon) {
                    continue;
                }
                const sf::Vector2f axis(edge.y / length, -edge.x / length);
                float firstMin = first.front().x * axis.x + first.front().y * axis.y;
                float firstMax = firstMin;
                float secondMin = second.front().x * axis.x + second.front().y * axis.y;
                float secondMax = secondMin;
                for (const auto point : first) {
                    const float projection = point.x * axis.x + point.y * axis.y;
                    firstMin = std::min(firstMin, projection);
                    firstMax = std::max(firstMax, projection);
                }
                for (const auto point : second) {
                    const float projection = point.x * axis.x + point.y * axis.y;
                    secondMin = std::min(secondMin, projection);
                    secondMax = std::max(secondMax, projection);
                }
                const float positiveOverlap = firstMax - secondMin;
                const float negativeOverlap = secondMax - firstMin;
                if (positiveOverlap <= epsilon || negativeOverlap <= epsilon) {
                    return false;
                }
                const float overlap = std::min(positiveOverlap, negativeOverlap);
                if (overlap < minimumOverlap) {
                    minimumOverlap = overlap;
                    minimumAxis = positiveOverlap < negativeOverlap ? axis : -axis;
                }
            }
            return true;
        };

        if (!testAxes(first) || !testAxes(second) ||
            minimumOverlap == std::numeric_limits<float>::max()) {
            return std::nullopt;
        }

        return Contact{minimumAxis, minimumOverlap};
    }

    inline std::optional<Contact> intersect(
        const std::vector<Polygon>& first,
        const std::vector<Polygon>& second) {
        std::optional<Contact> bestContact;
        for (const auto& firstTriangle : first) {
            for (const auto& secondTriangle : second) {
                const auto contact = intersectConvex(firstTriangle, secondTriangle);
                if (contact && (!bestContact ||
                                contact->penetration > bestContact->penetration)) {
                    bestContact = contact;
                }
            }
        }
        return bestContact;
    }

    inline Polygon transformPolygon(
        const Polygon& polygon,
        const sf::Transform& transform) {
        Polygon result;
        result.reserve(polygon.size());
        for (const auto point : polygon) {
            result.push_back(transform.transformPoint(point));
        }
        return result;
    }

    inline Polygon transformRectangle(
        const sf::FloatRect& rectangle,
        const sf::Transform& transform) {
        const auto left = rectangle.position.x;
        const auto top = rectangle.position.y;
        const auto right = left + rectangle.size.x;
        const auto bottom = top + rectangle.size.y;
        return transformPolygon(
            {{left, top}, {right, top}, {right, bottom}, {left, bottom}}, transform);
    }
}

#endif //METEORSHIP_COLLISIONGEOMETRY_H
