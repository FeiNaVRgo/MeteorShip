//
// Created by iformat on 11/24/26.
//

#ifndef METEORSHIP_SAVESTATE_H
#define METEORSHIP_SAVESTATE_H

#include <any>
#include <boost/json.hpp>
#include <concepts>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <meta> // C++26 Static Reflection (P2996)
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
    namespace detail {
        struct TransparentStringHash {
            using is_transparent = void;
            size_t operator()(std::string_view sv) const noexcept {
                return std::hash<std::string_view>{}(sv);
            }
        };

        template<typename T>
        struct is_std_vector : std::false_type {};
        template<typename T, typename A>
        struct is_std_vector<std::vector<T, A>> : std::true_type {};

        template<typename T>
        struct is_std_optional : std::false_type {};
        template<typename T>
        struct is_std_optional<std::optional<T>> : std::true_type {};

        template<typename T>
        struct is_string_map : std::false_type {};
        template<typename V, typename C, typename A>
        struct is_string_map<std::map<std::string, V, C, A>> : std::true_type {};
        template<typename V, typename H, typename E, typename A>
        struct is_string_map<std::unordered_map<std::string, V, H, E, A>> : std::true_type {};

        inline boost::json::string_view to_boost_sv(std::string_view sv) noexcept {
            return {sv.data(), sv.size()};
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
                        out += boost::json::serialize(kv.key());
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
                    for (size_t i = 0; i < arr.size(); ++i) {
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
        void from_json(const boost::json::value& jv, T& val);

        template<typename T>
        boost::json::value to_json(const T& val) {
            if constexpr (std::same_as<T, bool>) {
                return boost::json::value(val);
            } else if constexpr (std::integral<T>) {
                if constexpr (std::is_signed_v<T>) {
                    return boost::json::value(static_cast<std::int64_t>(val));
                } else {
                    return boost::json::value(static_cast<std::uint64_t>(val));
                }
            } else if constexpr (std::floating_point<T>) {
                return boost::json::value(static_cast<double>(val));
            } else if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
                return boost::json::value(to_boost_sv(val));
            } else if constexpr (std::is_enum_v<T>) {
                using Underlying = std::underlying_type_t<T>;
                return to_json(static_cast<Underlying>(val));
            } else if constexpr (is_std_optional<T>::value) {
                return val.has_value() ? to_json(*val) : boost::json::value(nullptr);
            } else if constexpr (is_std_vector<T>::value) {
                boost::json::array arr;
                arr.reserve(val.size());
                for (const auto& elem : val) {
                    arr.push_back(to_json(elem));
                }
                return arr;
            } else if constexpr (is_string_map<T>::value) {
                boost::json::object obj;
                obj.reserve(val.size());
                for (const auto& [k, v] : val) {
                    obj.emplace(to_boost_sv(k), to_json(v));
                }
                return obj;
            } else if constexpr (std::is_class_v<T>) {
                boost::json::object obj;
                constexpr auto ctx = std::meta::access_context::unchecked();
                template for (constexpr auto mem : std::define_static_array(
                                  std::meta::nonstatic_data_members_of(^^T, ctx))) {
                    obj.emplace(to_boost_sv(std::meta::identifier_of(mem)), to_json(val.[:mem:]));
                }
                return obj;
            } else {
                static_assert(false, "Type is not supported by SaveState Boost.JSON reflection serializer.");
            }
        }

        template<typename T>
        void from_json(const boost::json::value& jv, T& val) {
            if constexpr (std::same_as<T, bool>) {
                val = jv.as_bool();
            } else if constexpr (std::is_arithmetic_v<T>) {
                val = jv.to_number<T>();
            } else if constexpr (std::same_as<T, std::string>) {
                const auto& str = jv.as_string();
                val.assign(str.data(), str.size());
            } else if constexpr (std::is_enum_v<T>) {
                using Underlying = std::underlying_type_t<T>;
                val = static_cast<T>(jv.to_number<Underlying>());
            } else if constexpr (is_std_optional<T>::value) {
                if (jv.is_null()) {
                    val.reset();
                } else {
                    typename T::value_type inner{};
                    from_json(jv, inner);
                    val = std::move(inner);
                }
            } else if constexpr (is_std_vector<T>::value) {
                const auto& arr = jv.as_array();
                val.clear();
                val.reserve(arr.size());
                for (const auto& elem : arr) {
                    typename T::value_type item{};
                    from_json(elem, item);
                    val.push_back(std::move(item));
                }
            } else if constexpr (is_string_map<T>::value) {
                const auto& obj = jv.as_object();
                val.clear();
                for (const auto& kv : obj) {
                    typename T::mapped_type item{};
                    from_json(kv.value(), item);
                    val.emplace(std::string(kv.key().data(), kv.key().size()), std::move(item));
                }
            } else if constexpr (std::is_class_v<T>) {
                const auto& obj = jv.as_object();
                constexpr auto ctx = std::meta::access_context::unchecked();
                template for (constexpr auto mem : std::define_static_array(
                                  std::meta::nonstatic_data_members_of(^^T, ctx))) {
                    if (const auto* field = obj.if_contains(to_boost_sv(std::meta::identifier_of(mem)))) {
                        from_json(*field, val.[:mem:]);
                    }
                }
            } else {
                static_assert(false, "Type is not supported by SaveState Boost.JSON reflection deserializer.");
            }
        }
    } // namespace detail

    class SerializableImpl {
    public:
        virtual ~SerializableImpl() = default;
        virtual boost::json::value serialize() const = 0;
        virtual std::any deserialize(const boost::json::value& jv) = 0;
    };

    template<typename T>
    class Serializable : public SerializableImpl {
        std::optional<T> owned_value_;
        T* target_ptr_ = nullptr;

    public:
        Serializable() : owned_value_(T{}), target_ptr_(&*owned_value_) {}
        explicit Serializable(T val) : owned_value_(std::move(val)), target_ptr_(&*owned_value_) {}
        explicit Serializable(T* external_ref) : target_ptr_(external_ref) {}

        T& value() { return *target_ptr_; }
        const T& value() const { return *target_ptr_; }

        boost::json::value serialize() const override {
            return detail::to_json(*target_ptr_);
        }

        std::any deserialize(const boost::json::value& jv) override {
            detail::from_json(jv, *target_ptr_);
            if constexpr (std::is_copy_constructible_v<T>) {
                return *target_ptr_;
            } else {
                return {};
            }
        }
    };

    class SaveState {
    public:
        using Key = std::string;
        using ValuePtr = std::unique_ptr<SerializableImpl>;

    private:
        std::unordered_map<Key, ValuePtr, detail::TransparentStringHash, std::equal_to<>> map_serializable_;
        std::unordered_map<Key, boost::json::value, detail::TransparentStringHash, std::equal_to<>> pending_values_;

        class ConstEntryProxy {
            const SaveState& state_;
            std::string_view key_;

        public:
            ConstEntryProxy(const SaveState& state, std::string_view key) : state_(state), key_(key) {}

            template<typename T>
            T as() const {
                return state_.at<T>(key_);
            }

            template<typename T>
            T value_or(T default_val) const {
                return state_.get_or<T>(key_, std::move(default_val));
            }

            template<typename T>
                requires (!std::same_as<std::remove_cvref_t<T>, ConstEntryProxy>)
            operator T() const {
                return state_.at<T>(key_);
            }

            bool exists() const { return state_.contains(key_); }
        };

        class EntryProxy {
            SaveState& state_;
            std::string_view key_;

        public:
            EntryProxy(SaveState& state, std::string_view key) : state_(state), key_(key) {}

            template<typename T>
            EntryProxy& operator=(T&& value) {
                state_.set<std::decay_t<T>>(key_, std::forward<T>(value));
                return *this;
            }

            template<typename T>
            T as() const {
                return state_.at<T>(key_);
            }

            template<typename T>
            T value_or(T default_val) const {
                return state_.get_or<T>(key_, std::move(default_val));
            }

            template<typename T>
                requires (!std::same_as<std::remove_cvref_t<T>, EntryProxy>)
            operator T() const {
                return state_.at<T>(key_);
            }

            bool exists() const { return state_.contains(key_); }
        };

    public:
        SaveState() = default;

        void push_serializable(std::string_view key, ValuePtr ptr) {
            if (auto it = pending_values_.find(key); it != pending_values_.end()) {
                ptr->deserialize(it->second);
                pending_values_.erase(it);
            }
            map_serializable_.insert_or_assign(std::string(key), std::move(ptr));
        }

        std::optional<SerializableImpl*> get_serializable(std::string_view key) {
            if (auto it = map_serializable_.find(key); it != map_serializable_.end()) {
                return it->second.get();
            }
            return std::nullopt;
        }

        std::optional<const SerializableImpl*> get_serializable(std::string_view key) const {
            if (auto it = map_serializable_.find(key); it != map_serializable_.end()) {
                return it->second.get();
            }
            return std::nullopt;
        }

        template<typename T>
        SaveState& set(std::string_view key, T value) {
            pending_values_.erase(std::string(key));
            auto ptr = std::make_unique<Serializable<T>>(std::move(value));
            map_serializable_.insert_or_assign(std::string(key), std::move(ptr));
            return *this;
        }

        template<typename T, typename... Args>
        SaveState& emplace(std::string_view key, Args&&... args) {
            return set<T>(key, T(std::forward<Args>(args)...));
        }

        template<typename T>
        SaveState& bind(std::string_view key, T& target) {
            auto ptr = std::make_unique<Serializable<T>>(&target);
            push_serializable(key, std::move(ptr));
            return *this;
        }

        template<typename T, typename Func>
            requires std::invocable<Func, T&>
        bool modify(std::string_view key, Func&& fn) {
            if (auto it = map_serializable_.find(key); it != map_serializable_.end()) {
                if (auto* typed = dynamic_cast<Serializable<T>*>(it->second.get())) {
                    std::invoke(std::forward<Func>(fn), typed->value());
                    return true;
                }
                return false;
            }
            if (auto it = pending_values_.find(key); it != pending_values_.end()) {
                auto ptr = std::make_unique<Serializable<T>>();
                ptr->deserialize(it->second);
                std::invoke(std::forward<Func>(fn), ptr->value());
                pending_values_.erase(it);
                map_serializable_.emplace(std::string(key), std::move(ptr));
                return true;
            }
            return false;
        }

        template<typename T>
        std::optional<T> get(std::string_view key) const {
            if (auto it = map_serializable_.find(key); it != map_serializable_.end()) {
                if (const auto* typed = dynamic_cast<const Serializable<T>*>(it->second.get())) {
                    return typed->value(); // Returns a newly constructed copy of T
                }
                return std::nullopt;
            }
            if (auto it = pending_values_.find(key); it != pending_values_.end()) {
                T obj{};
                detail::from_json(it->second, obj);
                return obj;
            }
            return std::nullopt;
        }

        template<typename T>
        T at(std::string_view key) const {
            auto opt = get<T>(key);
            if (!opt) {
                throw std::out_of_range("SaveState key missing or type mismatch: " + std::string(key));
            }
            return std::move(*opt);
        }

        template<typename T>
        T get_or(std::string_view key, T default_value) const {
            if (auto opt = get<T>(key)) {
                return std::move(*opt);
            }
            return default_value;
        }

        EntryProxy operator[](std::string_view key) {
            return EntryProxy(*this, key);
        }

        ConstEntryProxy operator[](std::string_view key) const {
            return ConstEntryProxy(*this, key);
        }

        bool contains(std::string_view key) const {
            return map_serializable_.contains(key) || pending_values_.contains(key);
        }

        bool erase(std::string_view key) {
            bool removed = map_serializable_.erase(std::string(key)) > 0;
            removed |= pending_values_.erase(std::string(key)) > 0;
            return removed;
        }

        void clear() {
            map_serializable_.clear();
            pending_values_.clear();
        }

        size_t size() const { return map_serializable_.size() + pending_values_.size(); }
        bool empty() const { return size() == 0; }

        std::vector<std::string> keys() const {
            std::vector<std::string> result;
            result.reserve(size());
            for (const auto& [k, _] : map_serializable_) result.push_back(k);
            for (const auto& [k, _] : pending_values_) result.push_back(k);
            return result;
        }

        boost::json::object to_json_object() const {
            boost::json::object root;
            root.reserve(size());

            for (const auto& [key, val] : map_serializable_) {
                root.emplace(detail::to_boost_sv(key), val->serialize());
            }
            for (const auto& [key, jv] : pending_values_) {
                root.emplace(detail::to_boost_sv(key), jv);
            }
            return root;
        }

        std::string dump(bool pretty = true) const {
            boost::json::value root = to_json_object();
            if (!pretty) {
                return boost::json::serialize(root);
            }
            std::string out;
            detail::pretty_print(out, root);
            out.push_back('\n');
            return out;
        }

        std::expected<void, std::string> parse(std::string_view content) {
            boost::system::error_code ec;
            boost::json::value parsed = boost::json::parse(detail::to_boost_sv(content), ec);
            if (ec) {
                return std::unexpected("Boost.JSON parse error: " + ec.message());
            }
            if (!parsed.is_object()) {
                return std::unexpected("Boost.JSON parse error: root element must be an object");
            }

            try {
                for (auto& kv : parsed.get_object()) {
                    std::string_view key(kv.key().data(), kv.key().size());
                    if (auto it = map_serializable_.find(key); it != map_serializable_.end()) {
                        it->second->deserialize(kv.value());
                    } else {
                        pending_values_.insert_or_assign(std::string(key), std::move(kv.value()));
                    }
                }
                return {};
            } catch (const std::exception& e) {
                return std::unexpected(std::string("Deserialization error: ") + e.what());
            }
        }

        std::expected<void, std::string> save_to_file(const std::filesystem::path& path, bool pretty = true) const {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file) {
                return std::unexpected("Failed to open file for writing: " + path.string());
            }
            std::string data = dump(pretty);
            file.write(data.data(), static_cast<std::streamsize>(data.size()));
            if (!file) {
                return std::unexpected("Failed to write data to file: " + path.string());
            }
            return {};
        }

        std::expected<void, std::string> load_from_file(const std::filesystem::path& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                return std::unexpected("Failed to open file for reading: " + path.string());
            }
            std::ostringstream ss;
            ss << file.rdbuf();
            return parse(ss.str());
        }
    };

} // namespace ms::utility::savestate

#endif // METEORSHIP_SAVESTATE_H