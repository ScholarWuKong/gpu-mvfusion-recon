#pragma once

#include "scene/image.h"

#include <cstdint>
#include <string>

namespace sfm::io
{

scene::Image loadImage(const std::string& filePath, std::uint32_t imageId = 0);

} // namespace sfm::io
