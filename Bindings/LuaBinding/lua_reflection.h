#ifndef METEORSHIP_LUA_REFLECTION_H
#define METEORSHIP_LUA_REFLECTION_H

#include "sol_optional_compat.h"

#include <meta>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace ms::scripting {
    inline std::string to_lua_snake_case(std::string_view name) {
        std::string result;
        result.reserve(name.size() + 4);

        for (std::size_t i = 0; i < name.size(); ++i) {
            const char character = name[i];
            const bool uppercase = character >= 'A' && character <= 'Z';
            const bool previous_is_lower_or_digit = i > 0 &&
                ((name[i - 1] >= 'a' && name[i - 1] <= 'z') || (name[i - 1] >= '0' && name[i - 1] <= '9'));
            const bool acronym_ends = i > 0 && i + 1 < name.size() &&
                name[i - 1] >= 'A' && name[i - 1] <= 'Z' &&
                name[i + 1] >= 'a' && name[i + 1] <= 'z';

            if (uppercase && (previous_is_lower_or_digit || acronym_ends)) {
                result.push_back('_');
            }
            result.push_back(uppercase ? static_cast<char>(character - 'A' + 'a') : character);
        }

        return result;
    }

    inline std::string reflected_lua_name(std::string_view name) {
        return std::string(name);
    }

    namespace detail {
        template<typename Members>
        consteval std::size_t reflected_method_count(const Members& members, std::string_view name) {
            std::size_t count = 0;
            for (const auto member : members) {
                if (std::meta::is_function(member) && std::meta::has_identifier(member) &&
                    std::meta::is_public(member) && !std::meta::is_special_member_function(member) &&
                    !std::meta::is_function_template(member) && !std::meta::is_operator_function(member) &&
                    !std::meta::is_static_member(member) && !std::meta::is_deleted(member) &&
                    std::meta::identifier_of(member) == name) {
                    ++count;
                }
            }
            return count;
        }
    }

    template<typename T, typename NameTransform = decltype(&reflected_lua_name)>
    void bind_reflected_fields(sol::usertype<T>& usertype, NameTransform name_transform = reflected_lua_name) {
        static constexpr auto fields = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));

        template for (constexpr auto field : fields) {
            if constexpr (std::meta::has_identifier(field) && std::meta::is_public(field) &&
                          !std::meta::is_bit_field(field)) {
                usertype[name_transform(std::meta::identifier_of(field))] = &[:field:];
            }
        }
    }

    template<typename T, typename NameTransform = decltype(&to_lua_snake_case)>
    void bind_reflected_methods(sol::usertype<T>& usertype, NameTransform name_transform = to_lua_snake_case) {
        static constexpr auto members = std::define_static_array(
            std::meta::members_of(^^T, std::meta::access_context::current()));

        template for (constexpr auto member : members) {
            if constexpr (std::meta::is_function(member) && std::meta::has_identifier(member) &&
                          std::meta::is_public(member) && !std::meta::is_special_member_function(member) &&
                          !std::meta::is_function_template(member) && !std::meta::is_operator_function(member) &&
                          !std::meta::is_static_member(member) && !std::meta::is_deleted(member)) {
                constexpr auto name = std::meta::identifier_of(member);
                if constexpr (detail::reflected_method_count(members, name) == 1) {
                    usertype[std::string(name_transform(name))] = &[:member:];
                }
            }
        }
    }

    template<typename T>
    sol::usertype<T> new_reflected_view(sol::state& lua, std::string_view type_name) {
        auto usertype = lua.new_usertype<T>(std::string(type_name), sol::no_constructor);
        bind_reflected_fields(usertype, to_lua_snake_case);
        return usertype;
    }

    template<typename T>
    sol::usertype<T> bind_reflected_event(sol::state& lua, sol::table& events) {
        const std::string event_name(std::meta::identifier_of(^^T));
        const std::string type_name = event_name + "Event";
        auto usertype = new_reflected_view<T>(lua, type_name);
        events[event_name] = lua.get<sol::object>(type_name);
        return usertype;
    }

    template<typename... Events>
    void bind_reflected_events(sol::state& lua, sol::table& events) {
        (bind_reflected_event<Events>(lua, events), ...);
    }

    template<typename Enum>
    auto bind_reflected_enum(sol::state& lua, std::string_view name) {
        static constexpr auto enumerators = std::define_static_array(std::meta::enumerators_of(^^Enum));

        [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
            lua.template new_enum<Enum, false>(
                name,
                {std::pair<sol::string_view, Enum>{std::meta::identifier_of(enumerators[Indices]), [:enumerators[Indices]:]}...});
        }(std::make_index_sequence<enumerators.size()>{});

        return lua.get<sol::table>(std::string(name));
    }

    template<typename T, typename... Args>
    sol::usertype<T> new_reflected_usertype(sol::state& lua, std::string_view name, Args&&... args) {
        auto usertype = lua.new_usertype<T>(std::string(name), std::forward<Args>(args)...);
        bind_reflected_fields(usertype);
        return usertype;
    }

    template<typename T, typename... Args>
    sol::usertype<T> new_reflected_usertype_snake_case(sol::state& lua, std::string_view name, Args&&... args) {
        auto usertype = lua.new_usertype<T>(std::string(name), std::forward<Args>(args)...);
        bind_reflected_fields(usertype, to_lua_snake_case);
        return usertype;
    }
}

#endif // METEORSHIP_LUA_REFLECTION_H
