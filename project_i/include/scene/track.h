#pragma once

#include "scene/observation.h"
#include <vector>

namespace sfm::scene
{
struct Track
{
    TrackId id_ = 0;
    std::vector<Observation> observations_;
    PointId pointId_ = INVALID_POINT_ID;
};
}
