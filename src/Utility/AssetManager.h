//
// Created by iformat on 6/25/26.
//

#ifndef METEORSHIP_ASSETMANAGER_H
#define METEORSHIP_ASSETMANAGER_H
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <boost/json.hpp>

namespace ms::utility {
    class AssetManager {
        std::string assetPath;
    public:
        std::unordered_map<std::string, std::vector<sf::Shader>> shaderMap;
        std::unordered_map<std::string, sf::Texture> textureMap;//used for spritesheets usually

        // change of plans
        // i will just load spritesheets
        // and have a json that specifies how to use them
        // loading sprites will be on demand

    public:
        explicit AssetManager() {
            assetPath = "./assets";
        }

        void loadAssets() {
            using it = std::filesystem::recursive_directory_iterator;

            for (auto const& path : it(assetPath)) {
                if (std::filesystem::exists(path)) {
                    if (std::filesystem::is_regular_file(path)) {
                        loadSpritesFromJSON(path);
                        loadShader(path);
                    }
                }
            }
        }

    private:
        void loadShader(std::filesystem::path const& path) {
            if (path.extension() == ".vert") {
                shaderMap[path.stem()].push_back(sf::Shader{path, sf::Shader::Type::Vertex});
            } else if (path.extension() == ".frag") {
                shaderMap[path.stem()].push_back(sf::Shader{path, sf::Shader::Type::Fragment});
            } else if (path.extension() == ".geom") {
                shaderMap[path.stem()].push_back(sf::Shader{path, sf::Shader::Type::Geometry});
            }
        }

        void loadSpritesFromJSON(std::filesystem::path const& path) {
            if (path.extension() == ".json") {
                std::ifstream file(path, std::ios::binary);
                if (!file)
                    throw std::runtime_error("Failed to open file: " + path.string());

                std::stringstream ss;
                ss << file.rdbuf();

                for (boost::json::value json = boost::json::parse(ss.str()); auto it : json.as_array()) {
                    boost::json::object sprite_object = it.as_object();

                    boost::json::value name_as_val = sprite_object["name"];
                    auto height_as_val = sprite_object["height"];
                    auto width_as_val = sprite_object["width"];

                    boost::json::string name = name_as_val.as_string();

                    std::string png_sheet_name = std::string(name.c_str()).append(".png");
                    auto rect = sf::IntRect(
                        {0, 0},
                          {static_cast<int>(width_as_val.as_int64()), static_cast<int>(height_as_val.as_int64())}
                          );

                    textureMap.emplace(name, sf::Texture(
                        path.parent_path().append(png_sheet_name),
                           false,
                                rect
                        ));
                }
            }
        }
    };
}


#endif //METEORSHIP_ASSETMANAGER_H
