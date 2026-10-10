#include "scene/image.h"

#include <limits>

namespace sfm::scene
{

bool Image::empty() const noexcept
{
    return width_ <= 0 || height_ <= 0 || pixels_.empty();
}

int Image::channels() const noexcept
{
    switch (pixelFormat_)
    {
    case PixelFormat::GRAY8:
        return 1;
    case PixelFormat::BGR8:
        return 3;
    default:
        return 0;
    }
}

bool Image::isValid() const noexcept
{
    if (empty())
    {
        return false;
    }

    const int channelCount = channels();
    if (channelCount == 0)
    {
        return false;
    }

    const auto minimumStride = static_cast<std::size_t>(width_) *
                               static_cast<std::size_t>(channelCount);
    if (rowStrideBytes_ < minimumStride)
    {
        return false;
    }

    const auto imageHeight = static_cast<std::size_t>(height_);
    if (rowStrideBytes_ > std::numeric_limits<std::size_t>::max() / imageHeight)
    {
        return false;
    }

    return pixels_.size() >= rowStrideBytes_ * imageHeight;
}

} // namespace sfm::scene
