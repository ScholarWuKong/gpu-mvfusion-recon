#pragma once

#include "util/types.h"

namespace sfm::scene
{
struct Observation
{
    ImageId imageId_ = 0;
    std::size_t featureIndex_ = 0;
    double x_ = 0.0;
    double y_ = 0.0;
    bool enabled_ = true;
};
}
