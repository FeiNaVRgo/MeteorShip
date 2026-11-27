//
// Created by iformat on 11/27/26.
//

#ifndef METEORSHIP_SETTINGSDATA_H
#define METEORSHIP_SETTINGSDATA_H

namespace ms::utility {
    struct SettingsData
    {
        struct Window {
            int  width      = 1280;
            int  height     = 720;
            bool fullscreen = false;
        } window;

        struct Audio {
            double master_volume = 0.8;
        } audio;

        std::string language = "en";
    };
}
#endif //METEORSHIP_SETTINGSDATA_H
