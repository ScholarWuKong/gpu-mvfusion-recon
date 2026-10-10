#include "io/image_io.h"

#include <opencv2/imgcodecs.hpp>

#include <cstring>
#include <stdexcept>

namespace sfm::io
{

scene::Image loadImage(const std::string& filePath, const std::uint32_t imageId)
{
    const cv::Mat grayImage = cv::imread(filePath, cv::IMREAD_GRAYSCALE);
    if (grayImage.empty())
    {
        throw std::runtime_error("Failed to load image: " + filePath);
    }

    scene::Image image;
    image.id_ = imageId;
    image.width_ = grayImage.cols;
    image.height_ = grayImage.rows;
    image.rowStrideBytes_ = static_cast<std::size_t>(grayImage.cols);
    image.pixelFormat_ = scene::PixelFormat::GRAY8;
    image.pixels_.resize(image.rowStrideBytes_ * static_cast<std::size_t>(image.height_));

    for (int rowIndex = 0; rowIndex < grayImage.rows; ++rowIndex)
    {
        const auto* sourceRow = grayImage.ptr<std::uint8_t>(rowIndex);
        auto* destinationRow = image.pixels_.data() +
                               static_cast<std::size_t>(rowIndex) * image.rowStrideBytes_;
        std::memcpy(destinationRow, sourceRow, image.rowStrideBytes_);
    }

    return image;
}

} // namespace sfm::io
