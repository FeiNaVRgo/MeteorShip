//
// Created by iformat on 11/24/26.
//

#ifndef METEORSHIP_SETTINGS_H
#define METEORSHIP_SETTINGS_H
#include <any>
#include <meta>
#include <string>
#include <unordered_map>
#include <variant>

namespace ms::utility
{
    class Settings
    {
    private:
        using SettingValue = std::variant<int, float, std::string>;

        //PATH -- preferably outside of project
        std::unordered_map<std::string_view, SettingValue> settings;
    public:
        Settings() {
            load_vals_to_settings(load_file());
            bind_to_lua();
        }
        Settings(Settings const& settings) = delete;
        ~Settings() {
            save_to_file();
        }

        void bind_to_lua() {}
        std::string_view load_file() {}
        void save_to_file() {}
        void load_vals_to_settings(std::string_view file) {}


        /*
        void apply(std::string_view setting_name, auto&& f) {
            std::visit([&]<typename T0>(T0&& arg) mutable {
                using T = std::decay_t<T0>;
                f<T>();
            }, settings[setting_name]);
        }
        */
    };
}

#endif //METEORSHIP_SETTINGS_H
