//
// Created by iformat on 6/25/26.
//

#ifndef METEORSHIP_ASSETMANAGER_H
#define METEORSHIP_ASSETMANAGER_H
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ms::uti {
    class AssetManager {
        std::filesystem::path assetPath;
        static std::unordered_map<std::string, sf::Texture> textureMap;
        static std::unordered_map<std::string, std::vector<sf::Shader>> shaderMap;

        explicit AssetManager() {
            assetPath = "./assets/";
        }

    public:
        void loadAssets() {
            for (auto it : assetPath) {
                if (std::filesystem::exists(it)) {
                    if (std::filesystem::is_regular_file(it)) {
                        loadTexture(it);
                        loadShader(it);
                    }
                }
            }
        }

        static void loadTexture(std::filesystem::path const& path) {
            if (path.extension() == ".png") {
                textureMap[path.stem()] = sf::Texture{path};
            }
        }

        static void loadShader(std::filesystem::path const& path) {
            if (path.extension() == ".vert") {
                shaderMap[path.stem()].push_back(sf::Shader{path, sf::Shader::Type::Vertex});
            } else if (path.extension() == ".frag") {
                shaderMap[path.stem()].push_back(sf::Shader{path, sf::Shader::Type::Fragment});
            } else if (path.extension() == ".geom") {
                shaderMap[path.stem()].push_back(sf::Shader{path, sf::Shader::Type::Geometry});
            }
        }
    };
}


#endif //METEORSHIP_ASSETMANAGER_H
