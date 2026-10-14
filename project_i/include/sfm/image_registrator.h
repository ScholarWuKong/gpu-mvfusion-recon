#pragma once

#include "config.h"
#include "scene/reconstruction.h"

namespace sfm
{
class ImageRegistrator
{
public:
    bool registerImage(ImageId imageId, scene::Reconstruction& reconstruction,
                        const MapperConfig& config) const;
};
}
