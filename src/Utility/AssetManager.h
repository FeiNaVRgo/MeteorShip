//
// Created by iformat on 6/25/26.
//

#ifndef METEORSHIP_ASSETMANAGER_H
#define METEORSHIP_ASSETMANAGER_H
#include <filesystem>
#include <unordered_map>
#include <SFML/Graphics/Texture.hpp>

namespace ms::uti {
    class AssetManager {
        std::filesystem::path assetPath;
        std::unordered_map<std::string, sf::Texture> textureMap;

        explicit AssetManager() {
            assetPath = "./assets/";
        }

    public:
        void loadTextures() {
            for (auto it : assetPath) {
                if (std::filesystem::exists(it)) {
                    if (std::filesystem::is_regular_file(it)) {
                        if (it.extension() == ".png") {
                            const auto texture = sf::Texture{it};
                            textureMap[it.stem()] = texture;
                        }
                    }
                }
            }
        }

        void loadShaders() {

        }
    };
}


#endif //METEORSHIP_ASSETMANAGER_H
