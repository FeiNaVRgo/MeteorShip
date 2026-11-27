#ifndef METEORSHIP_SAVEPATH_H
#define METEORSHIP_SAVEPATH_H

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <shlobj.h>
    #include <knownfolders.h>
#else
    #include <pwd.h>
    #include <unistd.h>
#endif

namespace ms::utility
{
    namespace detail
    {
        namespace fs = std::filesystem;

        inline bool valid_app_name(std::string_view n) {
            return !n.empty()
                && n != "." && n != ".."
                && n.find_first_of("/\\") == std::string_view::npos
                && n.find('\0') == std::string_view::npos;
        }

#if !defined(_WIN32)
        inline fs::path home_dir() {
            if (const char* h = std::getenv("HOME"); h && *h) {
                fs::path p = h;
                if (p.is_absolute()) return p;
            }
            if (const passwd* pw = getpwuid(getuid()); pw && pw->pw_dir && *pw->pw_dir) {
                fs::path p = pw->pw_dir;
                if (p.is_absolute()) return p;
            }
            return {};
        }
#endif

        inline fs::path base_data_dir() {
#if defined(_WIN32)
            PWSTR raw = nullptr;
            fs::path base;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &raw)) && raw)
                base = fs::path(raw);
            CoTaskMemFree(raw);
            return base;
#elif defined(__APPLE__)
            fs::path home = home_dir();
            return home.empty() ? fs::path{} : home / "Library" / "Application Support";
#else
            if (const char* x = std::getenv("XDG_DATA_HOME"); x && *x) {
                fs::path p = x;
                if (p.is_absolute()) return p;
            }
            fs::path home = home_dir();
            return home.empty() ? fs::path{} : home / ".local" / "share";
#endif
        }
    }

    [[nodiscard]] inline std::optional<std::filesystem::path> save_dir(std::string_view app_name) {
        namespace fs = std::filesystem;

        if (!detail::valid_app_name(app_name)) return std::nullopt;

        fs::path base = detail::base_data_dir();
        if (base.empty()) return std::nullopt;

        fs::path dir = base / std::string(app_name);

        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec || !fs::is_directory(dir, ec)) return std::nullopt;

        return dir;
    }
}

#endif //METEORSHIP_SAVEPATH_H