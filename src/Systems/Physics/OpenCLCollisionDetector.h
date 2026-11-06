#ifndef METEORSHIP_OPENCLCOLLISIONDETECTOR_H
#define METEORSHIP_OPENCLCOLLISIONDETECTOR_H

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ms::systems {
    class OpenCLCollisionDetector {
        class Impl;

        std::unique_ptr<Impl> impl;
        std::vector<std::string> options;
        int selectedOption{};

    public:
        OpenCLCollisionDetector();

        ~OpenCLCollisionDetector();

        OpenCLCollisionDetector(const OpenCLCollisionDetector &) = delete;

        OpenCLCollisionDetector &operator=(const OpenCLCollisionDetector &) = delete;

        [[nodiscard]] const std::vector<std::string> &getOptions() const { return options; }
        [[nodiscard]] int getSelectedOption() const { return selectedOption; }

        void selectOption(int option);

        [[nodiscard]] std::optional<std::vector<std::uint8_t> > detect(
            const std::vector<std::array<float, 4> > &boxes, std::size_t pairCount);
    };
}

#endif //METEORSHIP_OPENCLCOLLISIONDETECTOR_H
