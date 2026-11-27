//
// Created by iformat on 11/24/26.
//

#ifndef METEORSHIP_SAVESTATE_H
#define METEORSHIP_SAVESTATE_H

#include <array>
#include <boost/json.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <meta>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ms::utility::savestate {

    using Error = std::string;

    struct to_json_tag {};
    struct from_json_tag {};

    class JsonError : public std::runtime_error {
        std::string path_;
        std::string message_;
        std::string full_;

        void rebuild() {
            std::string_view p = path_;
            if (!p.empty() && p.front() == '.') p.remove_prefix(1);
            full_.clear();
            if (!p.empty()) {
                full_.append(p);
                full_ += ": ";
            }
            full_ += message_;
        }

    public:
        explicit JsonError(std::string message) : std::runtime_error(""), message_(std::move(message)) {
            rebuild();
        }

        void prepend_field(std::string_view name) {
            path_.insert(0, name.data(), name.size());
            path_.insert(path_.begin(), '.');
            rebuild();
        }
        void prepend_index(std::size_t i) {
            path_.insert(0, "[" + std::to_string(i) + "]");
            rebuild();
        }
        void prepend_key(std::string_view key) {
            path_.insert(0, "[\"" + std::string(key) + "\"]");
            rebuild();
        }

        const std::string& path() const noexcept { return path_; }
        const std::string& message() const noexcept { return message_; }
        const char* what() const noexcept override { return full_.c_str(); }
    };

    namespace detail {
        template<typename...>
        inline constexpr bool dependent_false_v = false;

        template<typename T> struct is_std_vector : std::false_type {};
        template<typename T, typename A> struct is_std_vector<std::vector<T, A>> : std::true_type {};

        template<typename T> struct is_std_array : std::false_type {};
        template<typename T, std::size_t N> struct is_std_array<std::array<T, N>> : std::true_type {};

        template<typename T> struct is_std_optional : std::false_type {};
        template<typename T> struct is_std_optional<std::optional<T>> : std::true_type {};

        template<typename T> struct is_std_pair : std::false_type {};
        template<typename A, typename B> struct is_std_pair<std::pair<A, B>> : std::true_type {};

        template<typename T> struct is_unique_ptr : std::false_type {};
        template<typename T, typename D> struct is_unique_ptr<std::unique_ptr<T, D>> : std::true_type {};

        template<typename T> struct is_string_map : std::false_type {};
        template<typename V, typename C, typename A>
        struct is_string_map<std::map<std::string, V, C, A>> : std::true_type {};
        template<typename V, typename H, typename E, typename A>
        struct is_string_map<std::unordered_map<std::string, V, H, E, A>> : std::true_type {};

        template<typename T>
        concept has_custom_to_json = requires(const T& v) {
            { tag_invoke(to_json_tag{}, v) } -> std::convertible_to<boost::json::value>;
        };

        template<typename T>
        concept has_custom_from_json = requires(const boost::json::value& jv) {
            { tag_invoke(from_json_tag{}, std::type_identity<T>{}, jv) } -> std::same_as<T>;
        };

        template<typename T>
        inline constexpr auto members_v = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));

        template<typename T>
        inline constexpr auto bases_v = std::define_static_array(
            std::meta::bases_of(^^T, std::meta::access_context::unchecked()));

        inline boost::json::string_view to_boost_sv(std::string_view sv) noexcept {
            return {sv.data(), sv.size()};
        }

        inline std::string kind_name(const boost::json::value& jv) {
            const auto sv = boost::json::to_string(jv.kind());
            return std::string(sv.data(), sv.size());
        }

        inline const boost::json::object& expect_object(const boost::json::value& jv) {
            if (!jv.is_object()) throw JsonError("expected object, got " + kind_name(jv));
            return jv.get_object();
        }

        inline const boost::json::array& expect_array(const boost::json::value& jv) {
            if (!jv.is_array()) throw JsonError("expected array, got " + kind_name(jv));
            return jv.get_array();
        }

        inline const boost::json::array& expect_array_of_size(const boost::json::value& jv, std::size_t n) {
            const auto& arr = expect_array(jv);
            if (arr.size() != n) {
                throw JsonError("expected array of " + std::to_string(n) + " elements, got " +
                                std::to_string(arr.size()));
            }
            return arr;
        }

        template<typename N>
        N read_number(const boost::json::value& jv) {
            if (!jv.is_number()) throw JsonError("expected number, got " + kind_name(jv));
            boost::system::error_code ec;
            const N n = jv.to_number<N>(ec);
            if (ec) throw JsonError("number not representable: " + ec.message());
            return n;
        }

        inline void pretty_print(std::string& out, const boost::json::value& jv, int indent = 0) {
            const std::string pad(indent, ' ');
            const std::string inner_pad(indent + 2, ' ');

            switch (jv.kind()) {
                case boost::json::kind::object: {
                    const auto& obj = jv.get_object();
                    if (obj.empty()) {
                        out += "{}";
                        return;
                    }
                    out += "{\n";
                    bool first = true;
                    for (const auto& kv : obj) {
                        if (!first) out += ",\n";
                        first = false;
                        out += inner_pad;
                        out += boost::json::serialize(boost::json::value(kv.key()));
                        out += ": ";
                        pretty_print(out, kv.value(), indent + 2);
                    }
                    out += "\n" + pad + "}";
                    break;
                }
                case boost::json::kind::array: {
                    const auto& arr = jv.get_array();
                    if (arr.empty()) {
                        out += "[]";
                        return;
                    }
                    out += "[\n";
                    for (std::size_t i = 0; i < arr.size(); ++i) {
                        if (i > 0) out += ",\n";
                        out += inner_pad;
                        pretty_print(out, arr[i], indent + 2);
                    }
                    out += "\n" + pad + "]";
                    break;
                }
                default:
                    out += boost::json::serialize(jv);
                    break;
            }
        }

        template<typename T>
        boost::json::value to_json(const T& val);

        template<typename T>
        void from_json_into(const boost::json::value& jv, T& val);

        template<typename T>
        T from_json_new(const boost::json::value& jv);

        template<typename V>
        V read_element(const boost::json::array& arr, std::size_t i) {
            try {
                return from_json_new<V>(arr[i]);
            } catch (JsonError& e) {
                e.prepend_index(i);
                throw;
            }
        }

        template<typename M>
        M read_member(const boost::json::object& obj, std::string_view name) {
            const auto* field = obj.if_contains(to_boost_sv(name));
            if (!field) throw JsonError("missing required field '" + std::string(name) + "'");
            try {
                return from_json_new<M>(*field);
            } catch (JsonError& e) {
                e.prepend_field(name);
                throw;
            }
        }

        template<typename T>
        void members_to_json(const T& val, boost::json::object& obj) {
            template for (constexpr auto base : bases_v<T>) {
                static_assert(std::meta::is_public(base), "SaveState: base classes must be public");
                using B = typename [:std::meta::type_of(base):];
                members_to_json<B>(static_cast<const B&>(val), obj);
            }
            template for (constexpr auto mem : members_v<T>) {
                obj.insert_or_assign(to_boost_sv(std::meta::identifier_of(mem)), to_json(val.[:mem:]));
            }
        }

        template<typename T>
        void members_from_json(const boost::json::object& obj, T& val) {
            template for (constexpr auto base : bases_v<T>) {
                static_assert(std::meta::is_public(base), "SaveState: base classes must be public");
                using B = typename [:std::meta::type_of(base):];
                members_from_json<B>(obj, static_cast<B&>(val));
            }
            template for (constexpr auto mem : members_v<T>) {
                if (const auto* field = obj.if_contains(to_boost_sv(std::meta::identifier_of(mem)))) {
                    try {
                        from_json_into(*field, val.[:mem:]);
                    } catch (JsonError& e) {
                        e.prepend_field(std::meta::identifier_of(mem));
                        throw;
                    }
                }
            }
        }

        template<typename T>
        T object_to_new(const boost::json::object& obj);

        template<typename T, std::size_t... B, std::size_t... M>
        T construct_aggregate(const boost::json::object& obj, std::index_sequence<B...>,
                              std::index_sequence<M...>) {
            return T{object_to_new<typename [:std::meta::type_of(bases_v<T>[B]):]>(obj)...,
                     read_member<typename [:std::meta::type_of(members_v<T>[M]):]>(
                         obj, std::meta::identifier_of(members_v<T>[M]))...};
        }

        template<typename T>
        T object_to_new(const boost::json::object& obj) {
            if constexpr (std::default_initializable<T>) {
                T out{};
                members_from_json(obj, out);
                return out;
            } else if constexpr (std::is_aggregate_v<T>) {
                return construct_aggregate<T>(obj, std::make_index_sequence<bases_v<T>.size()>{},
                                              std::make_index_sequence<members_v<T>.size()>{});
            } else {
                static_assert(dependent_false_v<T>,
                              "SaveState: type has no default constructor and is not an aggregate; "
                              "provide a tag_invoke(from_json_tag, ...) overload");
            }
        }

        template<typename T>
        boost::json::value to_json(const T& val) {
            if constexpr (has_custom_to_json<T>) {
                return boost::json::value(tag_invoke(to_json_tag{}, val));
            } else if constexpr (std::same_as<T, bool>) {
                return boost::json::value(val);
            } else if constexpr (std::integral<T>) {
                if constexpr (std::is_signed_v<T>) {
                    return boost::json::value(static_cast<std::int64_t>(val));
                } else {
                    return boost::json::value(static_cast<std::uint64_t>(val));
                }
            } else if constexpr (std::floating_point<T>) {
                return boost::json::value(static_cast<double>(val));
            } else if constexpr (std::is_enum_v<T>) {
                return to_json(static_cast<std::underlying_type_t<T>>(val));
            } else if constexpr (std::convertible_to<const T&, std::string_view>) {
                const std::string_view sv = val;
                return boost::json::value(to_boost_sv(sv));
            } else if constexpr (is_std_optional<T>::value) {
                return val.has_value() ? to_json(*val) : boost::json::value(nullptr);
            } else if constexpr (is_unique_ptr<T>::value) {
                static_assert(!std::is_polymorphic_v<typename T::element_type>,
                              "SaveState: unique_ptr to a polymorphic type is not supported");
                return val ? to_json(*val) : boost::json::value(nullptr);
            } else if constexpr (is_std_vector<T>::value || is_std_array<T>::value) {
                boost::json::array arr;
                arr.reserve(val.size());
                for (const auto& elem : val) arr.push_back(to_json(elem));
                return boost::json::value(std::move(arr));
            } else if constexpr (is_std_pair<T>::value) {
                boost::json::array arr;
                arr.push_back(to_json(val.first));
                arr.push_back(to_json(val.second));
                return boost::json::value(std::move(arr));
            } else if constexpr (is_string_map<T>::value) {
                boost::json::object obj;
                obj.reserve(val.size());
                for (const auto& [k, v] : val) obj.emplace(to_boost_sv(k), to_json(v));
                return boost::json::value(std::move(obj));
            } else if constexpr (std::is_class_v<T>) {
                static_assert(!std::is_polymorphic_v<T>,
                              "SaveState: polymorphic types are not supported; provide a tag_invoke overload");
                boost::json::object obj;
                members_to_json(val, obj);
                return boost::json::value(std::move(obj));
            } else {
                static_assert(dependent_false_v<T>, "Type is not supported by the SaveState serializer.");
            }
        }

        template<typename T>
        void from_json_into(const boost::json::value& jv, T& val) {
            static_assert(!std::is_const_v<T>, "SaveState: cannot deserialize into a const object");

            if constexpr (has_custom_from_json<T>) {
                val = tag_invoke(from_json_tag{}, std::type_identity<T>{}, jv);
            } else if constexpr (std::same_as<T, bool>) {
                if (!jv.is_bool()) throw JsonError("expected bool, got " + kind_name(jv));
                val = jv.get_bool();
            } else if constexpr (std::is_arithmetic_v<T>) {
                val = read_number<T>(jv);
            } else if constexpr (std::is_enum_v<T>) {
                val = static_cast<T>(read_number<std::underlying_type_t<T>>(jv));
            } else if constexpr (std::same_as<T, std::string>) {
                if (!jv.is_string()) throw JsonError("expected string, got " + kind_name(jv));
                const auto& str = jv.get_string();
                val.assign(str.data(), str.size());
            } else if constexpr (is_std_optional<T>::value) {
                using V = typename T::value_type;
                if (jv.is_null()) {
                    val.reset();
                } else if (val.has_value()) {
                    from_json_into(jv, *val);
                } else {
                    val.emplace(from_json_new<V>(jv));
                }
            } else if constexpr (is_unique_ptr<T>::value) {
                using V = typename T::element_type;
                static_assert(!std::is_polymorphic_v<V>,
                              "SaveState: unique_ptr to a polymorphic type is not supported");
                if (jv.is_null()) {
                    val.reset();
                } else if (val) {
                    from_json_into(jv, *val);
                } else {
                    val = std::make_unique<V>(from_json_new<V>(jv));
                }
            } else if constexpr (is_std_vector<T>::value) {
                using V = typename T::value_type;
                const auto& arr = expect_array(jv);
                val.clear();
                val.reserve(arr.size());
                for (std::size_t i = 0; i < arr.size(); ++i) val.push_back(read_element<V>(arr, i));
            } else if constexpr (is_std_array<T>::value) {
                const auto& arr = expect_array_of_size(jv, val.size());
                for (std::size_t i = 0; i < arr.size(); ++i) {
                    try {
                        from_json_into(arr[i], val[i]);
                    } catch (JsonError& e) {
                        e.prepend_index(i);
                        throw;
                    }
                }
            } else if constexpr (is_std_pair<T>::value) {
                const auto& arr = expect_array_of_size(jv, 2);
                try {
                    from_json_into(arr[0], val.first);
                } catch (JsonError& e) {
                    e.prepend_index(0);
                    throw;
                }
                try {
                    from_json_into(arr[1], val.second);
                } catch (JsonError& e) {
                    e.prepend_index(1);
                    throw;
                }
            } else if constexpr (is_string_map<T>::value) {
                using M = typename T::mapped_type;
                const auto& obj = expect_object(jv);
                val.clear();
                for (const auto& kv : obj) {
                    try {
                        auto item = from_json_new<M>(kv.value());
                        val.emplace(std::string(kv.key().data(), kv.key().size()), std::move(item));
                    } catch (JsonError& e) {
                        e.prepend_key(std::string_view(kv.key().data(), kv.key().size()));
                        throw;
                    }
                }
            } else if constexpr (std::is_class_v<T>) {
                static_assert(!std::is_polymorphic_v<T>,
                              "SaveState: polymorphic types are not supported; provide a tag_invoke overload");
                members_from_json(expect_object(jv), val);
            } else {
                static_assert(dependent_false_v<T>, "Type is not supported by the SaveState deserializer.");
            }
        }

        template<typename T>
        T from_json_new(const boost::json::value& jv) {
            static_assert(!std::is_reference_v<T>, "SaveState: cannot construct a reference from JSON");

            if constexpr (!std::same_as<T, std::remove_cv_t<T>>) {
                return from_json_new<std::remove_cv_t<T>>(jv);
            } else if constexpr (has_custom_from_json<T>) {
                return tag_invoke(from_json_tag{}, std::type_identity<T>{}, jv);
            } else if constexpr (is_std_array<T>::value) {
                using V = typename T::value_type;
                constexpr std::size_t N = std::tuple_size_v<T>;
                const auto& arr = expect_array_of_size(jv, N);
                return [&]<std::size_t... I>(std::index_sequence<I...>) {
                    return T{read_element<V>(arr, I)...};
                }(std::make_index_sequence<N>{});
            } else if constexpr (is_std_pair<T>::value) {
                const auto& arr = expect_array_of_size(jv, 2);
                return T{read_element<typename T::first_type>(arr, 0),
                         read_element<typename T::second_type>(arr, 1)};
            } else if constexpr (std::default_initializable<T>) {
                T out{};
                from_json_into(jv, out);
                return out;
            } else if constexpr (std::is_aggregate_v<T>) {
                return object_to_new<T>(expect_object(jv));
            } else {
                static_assert(dependent_false_v<T>,
                              "SaveState: type has no default constructor and is not an aggregate; "
                              "provide a tag_invoke(from_json_tag, ...) overload");
            }
        }
    } // namespace detail

    template<typename T>
    boost::json::value encode(const T& value) {
        return detail::to_json(value);
    }

    template<typename T>
    T decode(const boost::json::value& jv) {
        return detail::from_json_new<T>(jv);
    }

    template<typename T>
    T decode_field(const boost::json::value& jv, std::string_view name) {
        return detail::read_member<T>(detail::expect_object(jv), name);
    }

    class SaveState {
    public:
        class EntryProxy;
        class ConstEntryProxy;

    private:
        struct Binding {
            std::function<boost::json::value()> save;
            std::function<void(const boost::json::value&)> load;
        };

        boost::json::object data_;
        std::map<std::string, Binding, std::less<>> bindings_;

        template<typename T>
        static std::expected<T, Error> convert(std::string_view key, const boost::json::value& jv) {
            try {
                return detail::from_json_new<T>(jv);
            } catch (JsonError& e) {
                e.prepend_field(key);
                return std::unexpected(std::string(e.what()));
            } catch (const std::exception& e) {
                return std::unexpected(std::string(key) + ": " + e.what());
            }
        }

        static std::expected<void, Error> load_into(std::string_view key, const Binding& b,
                                                    const boost::json::value& jv) {
            try {
                b.load(jv);
                return {};
            } catch (JsonError& e) {
                e.prepend_field(key);
                return std::unexpected(std::string(e.what()));
            } catch (const std::exception& e) {
                return std::unexpected(std::string(key) + ": " + e.what());
            }
        }

        template<typename F>
        auto with_value(std::string_view key, F&& f) const
            -> std::optional<std::invoke_result_t<F, const boost::json::value&>> {
            if (auto it = bindings_.find(key); it != bindings_.end()) {
                const boost::json::value tmp = it->second.save();
                return std::invoke(std::forward<F>(f), tmp);
            }
            if (const auto* jv = data_.if_contains(detail::to_boost_sv(key))) {
                return std::invoke(std::forward<F>(f), *jv);
            }
            return std::nullopt;
        }

    public:
        class BindingGuard {
            SaveState* state_ = nullptr;
            std::string key_;

        public:
            BindingGuard() = default;
            BindingGuard(SaveState& state, std::string key) : state_(&state), key_(std::move(key)) {}
            BindingGuard(const BindingGuard&) = delete;
            BindingGuard& operator=(const BindingGuard&) = delete;
            BindingGuard(BindingGuard&& o) noexcept
                : state_(std::exchange(o.state_, nullptr)), key_(std::move(o.key_)) {}
            BindingGuard& operator=(BindingGuard&& o) noexcept {
                if (this != &o) {
                    release();
                    state_ = std::exchange(o.state_, nullptr);
                    key_ = std::move(o.key_);
                }
                return *this;
            }
            ~BindingGuard() { release(); }

            void release() {
                if (state_) {
                    state_->unbind(key_);
                    state_ = nullptr;
                }
            }
        };

        class ConstEntryProxy {
            const SaveState& state_;
            std::string key_;

        public:
            ConstEntryProxy(const SaveState& state, std::string_view key) : state_(state), key_(key) {}

            template<typename T> T as() const { return state_.at<T>(key_); }
            template<typename T> std::expected<T, Error> get() const { return state_.get<T>(key_); }
            template<typename T> T value_or(T default_val) const {
                return state_.get_or<T>(key_, std::move(default_val));
            }
            bool exists() const { return state_.contains(key_); }
        };

        class EntryProxy {
            SaveState& state_;
            std::string key_;

        public:
            EntryProxy(SaveState& state, std::string_view key) : state_(state), key_(key) {}
            EntryProxy(const EntryProxy&) = default;
            EntryProxy& operator=(const EntryProxy&) = delete;

            template<typename T>
                requires(!std::same_as<std::remove_cvref_t<T>, EntryProxy> &&
                         !std::same_as<std::remove_cvref_t<T>, ConstEntryProxy>)
            EntryProxy& operator=(T&& value) {
                state_.set(key_, value);
                return *this;
            }

            template<typename T> T as() const { return state_.at<T>(key_); }
            template<typename T> std::expected<T, Error> get() const { return state_.get<T>(key_); }
            template<typename T> T value_or(T default_val) const {
                return state_.get_or<T>(key_, std::move(default_val));
            }
            bool exists() const { return state_.contains(key_); }
        };

        SaveState() = default;

        template<typename T>
        SaveState& set(std::string_view key, const T& value) {
            boost::json::value jv = detail::to_json(value);
            if (auto it = bindings_.find(key); it != bindings_.end()) {
                if (auto r = load_into(key, it->second, jv); !r) {
                    throw std::runtime_error("SaveState: " + r.error());
                }
                return *this;
            }
            data_.insert_or_assign(detail::to_boost_sv(key), std::move(jv));
            return *this;
        }

        template<typename T, typename... Args>
        SaveState& emplace(std::string_view key, Args&&... args) {
            return set<T>(key, T(std::forward<Args>(args)...));
        }

        template<typename T>
        std::expected<T, Error> get(std::string_view key) const {
            auto result = with_value(key, [&](const boost::json::value& jv) { return convert<T>(key, jv); });
            if (!result) return std::unexpected("missing key: " + std::string(key));
            return std::move(*result);
        }

        template<typename T>
        T at(std::string_view key) const {
            auto r = get<T>(key);
            if (!r) throw std::runtime_error("SaveState: " + r.error());
            return std::move(*r);
        }

        template<typename T>
        T get_or(std::string_view key, T default_value) const {
            if (auto r = get<T>(key)) return std::move(*r);
            return default_value;
        }

        template<typename T, typename Func>
            requires std::invocable<Func, T&>
        std::expected<void, Error> modify(std::string_view key, Func&& fn) {
            auto current = get<T>(key);
            if (!current) return std::unexpected(std::move(current.error()));
            std::invoke(std::forward<Func>(fn), *current);
            set(key, *current);
            return {};
        }

        EntryProxy operator[](std::string_view key) { return EntryProxy(*this, key); }
        ConstEntryProxy operator[](std::string_view key) const { return ConstEntryProxy(*this, key); }

        template<typename T>
        [[nodiscard]] std::expected<void, Error> bind(std::string_view key, T& target) {
            static_assert(!std::is_const_v<T>, "SaveState::bind needs a mutable object");

            Binding b{
                [&target] { return detail::to_json(target); },
                [&target](const boost::json::value& jv) { detail::from_json_into(jv, target); },
            };

            std::optional<boost::json::value> existing;
            if (auto it = bindings_.find(key); it != bindings_.end()) {
                existing = it->second.save();
            } else if (const auto* jv = data_.if_contains(detail::to_boost_sv(key))) {
                existing = *jv;
            }
            if (existing) {
                if (auto r = load_into(key, b, *existing); !r) return r;
            }

            bindings_.insert_or_assign(std::string(key), std::move(b));
            return {};
        }

        template<typename T>
        [[nodiscard]] std::expected<BindingGuard, Error> bind_scoped(std::string_view key, T& target) {
            if (auto r = bind(key, target); !r) return std::unexpected(std::move(r.error()));
            return BindingGuard(*this, std::string(key));
        }

        bool unbind(std::string_view key) {
            auto it = bindings_.find(key);
            if (it == bindings_.end()) return false;
            data_.insert_or_assign(detail::to_boost_sv(key), it->second.save());
            bindings_.erase(it);
            return true;
        }

        bool is_bound(std::string_view key) const { return bindings_.contains(key); }

        bool contains(std::string_view key) const {
            return bindings_.contains(key) || data_.contains(detail::to_boost_sv(key));
        }

        bool erase(std::string_view key) {
            bool removed = data_.erase(detail::to_boost_sv(key)) > 0;
            if (auto it = bindings_.find(key); it != bindings_.end()) {
                bindings_.erase(it);
                removed = true;
            }
            return removed;
        }

        void clear() {
            data_.clear();
            bindings_.clear();
        }

        std::size_t size() const {
            std::size_t n = data_.size();
            for (const auto& kv : bindings_) {
                if (!data_.contains(detail::to_boost_sv(kv.first))) ++n;
            }
            return n;
        }
        bool empty() const { return size() == 0; }

        std::vector<std::string> keys() const {
            std::vector<std::string> result;
            result.reserve(size());
            for (const auto& kv : data_) result.emplace_back(kv.key().data(), kv.key().size());
            for (const auto& kv : bindings_) {
                if (!data_.contains(detail::to_boost_sv(kv.first))) result.push_back(kv.first);
            }
            return result;
        }

        boost::json::object to_json_object() const {
            boost::json::object root = data_;
            for (const auto& [key, b] : bindings_) {
                root.insert_or_assign(detail::to_boost_sv(key), b.save());
            }
            return root;
        }

        std::string dump(bool pretty = true) const {
            const boost::json::value root(to_json_object());
            if (!pretty) return boost::json::serialize(root);
            std::string out;
            detail::pretty_print(out, root);
            out.push_back('\n');
            return out;
        }

        std::expected<void, Error> parse(std::string_view content) {
            boost::system::error_code ec;
            boost::json::value parsed = boost::json::parse(detail::to_boost_sv(content), ec);
            if (ec) return std::unexpected("Boost.JSON parse error: " + ec.message());
            if (!parsed.is_object()) {
                return std::unexpected("Boost.JSON parse error: root element must be an object");
            }

            for (auto& kv : parsed.get_object()) {
                const std::string_view key(kv.key().data(), kv.key().size());
                if (auto it = bindings_.find(key); it != bindings_.end()) {
                    if (auto r = load_into(key, it->second, kv.value()); !r) return r;
                } else {
                    data_.insert_or_assign(kv.key(), std::move(kv.value()));
                }
            }
            return {};
        }

        std::expected<void, Error> save_to_file(const std::filesystem::path& path, bool pretty = true) const {
            const std::string data = dump(pretty);
            std::filesystem::path tmp = path;
            tmp += ".tmp";

            {
                std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
                if (!file) return std::unexpected("Failed to open file for writing: " + tmp.string());
                file.write(data.data(), static_cast<std::streamsize>(data.size()));
                file.flush();
                if (!file) {
                    file.close();
                    std::error_code ignore;
                    std::filesystem::remove(tmp, ignore);
                    return std::unexpected("Failed to write data to file: " + tmp.string());
                }
            }

            std::error_code ec;
            std::filesystem::rename(tmp, path, ec);
            if (ec) {
                std::error_code ignore;
                std::filesystem::remove(tmp, ignore);
                return std::unexpected("Failed to replace " + path.string() + ": " + ec.message());
            }
            return {};
        }

        std::expected<void, Error> load_from_file(const std::filesystem::path& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) return std::unexpected("Failed to open file for reading: " + path.string());
            std::ostringstream ss;
            ss << file.rdbuf();
            return parse(ss.str());
        }
    };

} // namespace ms::utility::savestate

#endif // METEORSHIP_SAVESTATE_H