#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sfm::scene
{

enum class PixelFormat
{
    GRAY8,
    BGR8
};

struct Image
{
    std::uint32_t id_ = 0;
    int width_ = 0;
    int height_ = 0;
    std::size_t rowStrideBytes_ = 0;
    PixelFormat pixelFormat_ = PixelFormat::GRAY8;
    std::vector<std::uint8_t> pixels_;

    bool empty() const noexcept;
    bool isValid() const noexcept;
    int channels() const noexcept;
};

} // namespace sfm::scene
