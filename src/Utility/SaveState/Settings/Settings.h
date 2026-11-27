#ifndef METEORSHIP_SETTINGS_H
#define METEORSHIP_SETTINGS_H

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

#include "SettingsData.h"
#include "Utility/SaveState/SaveState.h"

namespace ms::utility
{
    class Settings
    {
        SettingsData data_;
        savestate::SaveState st_;
        std::filesystem::path path_;

        static constexpr const char* kKey = "settings";

    public:
        explicit Settings(std::filesystem::path path = "settings.json")
            : path_(std::move(path)) {
            if (auto r = st_.bind(kKey, data_); !r) {
                std::cerr << "Settings: " << r.error() << '\n';
            }

            if (std::filesystem::exists(path_)) {
                if (auto r = st_.load_from_file(path_); !r) {
                    std::cerr << "Settings: " << r.error() << " (using defaults where load failed)\n";
                }
            }
        }

        Settings(const Settings&) = delete;
        Settings& operator=(const Settings&) = delete;
        Settings(Settings&&) = delete;
        Settings& operator=(Settings&&) = delete;

        ~Settings() {
            if (auto r = save(); !r) {
                std::cerr << "Settings: " << r.error() << '\n';
            }
        }

        SettingsData*       operator->()       { return &data_; }
        const SettingsData* operator->() const { return &data_; }

        std::expected<void, savestate::Error> save() const {
            return st_.save_to_file(path_);
        }
    };
}

#endif //METEORSHIP_SETTINGS_H